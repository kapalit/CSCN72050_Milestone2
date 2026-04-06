#include "PktDef.h"

// -------------------------------------------------------
// PktDef() -- Default Constructor
//
// Places the PktDef object in a safe initial state:
//   - All Header fields set to zero
//   - Data pointer set to nullptr
//   - CRC set to zero
//   - RawBuffer set to nullptr
// -------------------------------------------------------
PktDef::PktDef()
{
    // Zero out all header fields
    cmdPacket.header.PktCount = 0;
    cmdPacket.header.Drive = 0;
    cmdPacket.header.Status = 0;
    cmdPacket.header.Sleep = 0;
    cmdPacket.header.Ack = 0;
    cmdPacket.header.Padding = 0;
    cmdPacket.header.Length = 0;

    // Safe-state the body and trailer
    cmdPacket.Data = nullptr;
    cmdPacket.CRC = 0;

    // No serialized buffer yet
    RawBuffer = nullptr;
}

// -------------------------------------------------------
// Remaining member function stubs
// (To be implemented by team members 2, 3, and 4)
// -------------------------------------------------------

PktDef::PktDef(char *raw)
{
    // Initialize to safe defaults first
    cmdPacket.header.PktCount = 0;
    cmdPacket.header.Drive = 0;
    cmdPacket.header.Status = 0;
    cmdPacket.header.Sleep = 0;
    cmdPacket.header.Ack = 0;
    cmdPacket.header.Padding = 0;
    cmdPacket.header.Length = 0;
    cmdPacket.Data = nullptr;
    cmdPacket.CRC = 0;
    RawBuffer = nullptr;

    if (raw == nullptr)
    {
        return;
    }

    // Copy header bytes into our packed header struct
    std::memcpy(&cmdPacket.header, raw, HEADERSIZE);

    // Validate length as best as we can (Length is total packet size in bytes)
    // Minimal packet is header + CRC (no body)
    const int minLen = HEADERSIZE + 1;
    if (cmdPacket.header.Length < minLen)
    {
        // Treat malformed packet as empty
        cmdPacket.header.Length = 0;
        return;
    }

    // Body length is total length minus header and CRC
    const int bodyLen = static_cast<int>(cmdPacket.header.Length) - HEADERSIZE - 1;

    if (bodyLen > 0)
    {
        cmdPacket.Data = new char[bodyLen];
        std::memcpy(cmdPacket.Data, raw + HEADERSIZE, bodyLen);
    }
    else
    {
        cmdPacket.Data = nullptr;
    }

    // CRC trailer is last byte
    cmdPacket.CRC = *(raw + HEADERSIZE + bodyLen);
}

void PktDef::SetCmd(CmdType cmd)
{
    // Clear command-related flags first
    cmdPacket.header.Drive = 0;
    cmdPacket.header.Sleep = 0;
    cmdPacket.header.Status = 0;

    switch (cmd)
    {
    case DRIVE:
        cmdPacket.header.Drive = 1;
        break;
    case SLEEP:
        cmdPacket.header.Sleep = 1;
        break;
    case RESPONSE:
        // RESPONSE packets are expected to use Status flag (telemetry)
        cmdPacket.header.Status = 1;
        break;
    default:
        break;
    }
}

void PktDef::SetBodyData(char *data, int len)
{
    // Clean up any existing body
    if (cmdPacket.Data != nullptr)
    {
        delete[] cmdPacket.Data;
        cmdPacket.Data = nullptr;
    }

    if (data == nullptr || len <= 0)
    {
        // Body cleared; set length to header + CRC
        cmdPacket.header.Length = static_cast<unsigned char>(HEADERSIZE + 1);
        return;
    }

    cmdPacket.Data = new char[len];
    std::memcpy(cmdPacket.Data, data, len);

    // Total packet length is header + body + CRC
    cmdPacket.header.Length = static_cast<unsigned char>(HEADERSIZE + len + 1);
}

void PktDef::SetPktCount(int count)
{
    if (count < 0)
    {
        count = 0;
    }
    cmdPacket.header.PktCount = static_cast<unsigned short>(count);
}
CmdType PktDef::GetCmd()
{
    // The function checks which command flag is set and returns the corresponding CmdType enumeration.
    if (cmdPacket.header.Drive == 1)
        return DRIVE;

    if (cmdPacket.header.Sleep == 1)
        return SLEEP;

    if (cmdPacket.header.Status == 1)
        return RESPONSE;
    return DRIVE;
}

bool PktDef::GetAck()
{

    // Returns true if the ACK flag is set in the header
    return (cmdPacket.header.Ack == 1);
}

int PktDef::GetLength()
{

    return cmdPacket.header.Length;
}

char *PktDef::GetBodyData()
{

    return cmdPacket.Data;
}

int PktDef::GetPktCount()
{

    return cmdPacket.header.PktCount;
}

char *PktDef::GenPacket()
{

    // generate a serialized packet buffer

    int totalLength = cmdPacket.header.Length;

    if (totalLength < HEADERSIZE + 1)
        return nullptr;

    if (RawBuffer != nullptr)
    {
        delete[] RawBuffer;
        RawBuffer = nullptr;
    }

    RawBuffer = new char[totalLength];

    // Copy header into buffer
    std::memcpy(RawBuffer, &cmdPacket.header, HEADERSIZE);

    int bodyLength = totalLength - HEADERSIZE - 1;

    // Copy body if present
    if (bodyLength > 0 && cmdPacket.Data != nullptr)
    {
        std::memcpy(RawBuffer + HEADERSIZE, cmdPacket.Data, bodyLength);
    }

    // Place CRC after the body
    RawBuffer[HEADERSIZE + bodyLength] = cmdPacket.CRC;

    return RawBuffer;
}

/*
* CheckCRC char* buffer, int size)
* Description: Validates the CRC of a raw packet buffer
* CRC os a simple count of all bits set to '1' in the packet (excluding the CRC byte)
* Parameters:
    buffer - pointer to the raw packet data (header + body + CRC)
    size   - total size of the buffer in bytes (including CRC byte)

* Returns:
    true - if calculated bit count matches the CRC byte
    false - if CRC mismatch (packet courrupted)
*
 */
bool PktDef::CheckCRC(char *buffer, int size)
{
    if (buffer == nullptr || size < 1)
    {
        return false;
    }

    // The CRC byte is the last byte in the buffer
    unsigned char storedCRC = static_cast<unsigned char>(buffer[size - 1]);

    // Count all bits set to '1' in the packet (excluding CRC byte)
    int bitCount = 0;
    for (int i = 0; i < size - 1; i++)
    {
        unsigned char byte = static_cast<unsigned char>(buffer[i]);
        // Count bits in this byte
        while (byte)
        {
            bitCount += (byte & 1);
            byte >>= 1;
        }
    }

    // Compare calculated bit count to stored CRC
    return (bitCount == storedCRC);
}

/*
* CalcCRC()
* Description: Calculates the CRC for the current packet and stories it in cmdPacket.CRC.
* The CRC is a count for all bits set to '1' in the Header and Body
* Packet stucture being counted:
    - Header PKTCount (2 bytes) + Flags (1 byte) + Length (1 byte)
    - Body Data (cmdPacket.header.Length - HEADERSIZE - 1 bytes)
* Returns: None
*/
void PktDef::CalcCRC()
{
    int bitCount = 0;

    // Count bits in PktCount (2 bytes)
    unsigned short pktCount = cmdPacket.header.PktCount;
    while (pktCount)
    {
        bitCount += (pktCount & 1);
        pktCount >>= 1;
    }

    // Count bits in the flags byte (Drive, Status, Sleep, Ack, Padding)
    // These are packed into a single byte in memory
    unsigned char flagsByte = 0;
    flagsByte |= (cmdPacket.header.Drive & 0x01);        // bit 0
    flagsByte |= (cmdPacket.header.Status & 0x01) << 1;  // bit 1
    flagsByte |= (cmdPacket.header.Sleep & 0x01) << 2;   // bit 2
    flagsByte |= (cmdPacket.header.Ack & 0x01) << 3;     // bit 3
    flagsByte |= (cmdPacket.header.Padding & 0x0F) << 4; // bits 4-7

    while (flagsByte)
    {
        bitCount += (flagsByte & 1);
        flagsByte >>= 1;
    }

    // Count bits in Length (1 byte)
    unsigned char length = cmdPacket.header.Length;
    while (length)
    {
        bitCount += (length & 1);
        length >>= 1;
    }

    // Count bits in Body data (if present)
    if (cmdPacket.Data != nullptr && cmdPacket.header.Length > HEADERSIZE + 1)
    {
        int bodySize = cmdPacket.header.Length - HEADERSIZE - 1; // -1 for CRC byte
        for (int i = 0; i < bodySize; i++)
        {
            unsigned char byte = static_cast<unsigned char>(cmdPacket.Data[i]);
            while (byte)
            {
                bitCount += (byte & 1);
                byte >>= 1;
            }
        }
    }

    // Store the calculated CRC
    cmdPacket.CRC = static_cast<char>(bitCount);
}