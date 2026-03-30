#pragma once
#include <cstring>

// -------------------------------------------------------
// Direction Constants
// -------------------------------------------------------
const int FORWARD  = 1;
const int BACKWARD = 2;
const int RIGHT    = 3;
const int LEFT     = 4;

// -------------------------------------------------------
// HEADERSIZE: calculated by hand
//   PktCount  = 2 bytes (unsigned short)
//   Flags     = 1 byte  (Drive:1, Status:1, Sleep:1, Ack:1, Padding:4)
//   Length    = 1 byte  (unsigned char)
//   TOTAL     = 4 bytes
// -------------------------------------------------------
const int HEADERSIZE = 4;

// -------------------------------------------------------
// Command Type Enumeration
// -------------------------------------------------------
enum CmdType { DRIVE, SLEEP, RESPONSE };

// -------------------------------------------------------
// Header Structure
// The flags byte uses bit-fields packed into a single byte:
//   Drive   [bit 0] - set to 1 for DRIVE command
//   Status  [bit 1] - set to 1 to request telemetry response
//   Sleep   [bit 2] - set to 1 for SLEEP command
//   Ack     [bit 3] - set to 1 for acknowledgement packets
//   Padding [bits 4-7] - unused, always 0
// -------------------------------------------------------
struct Header {
    unsigned short  PktCount;   // 2 bytes - incrementing packet counter
    unsigned char   Drive   : 1;
    unsigned char   Status  : 1;
    unsigned char   Sleep   : 1;
    unsigned char   Ack     : 1;
    unsigned char   Padding : 4;
    unsigned char   Length;     // 1 byte - total packet size in bytes
};

// -------------------------------------------------------
// DriveBody Structure - used for FORWARD / BACKWARD commands
// -------------------------------------------------------
struct DriveBody {
    unsigned char Direction;    // 1 byte - FORWARD(1) or BACKWARD(2)
    unsigned char Duration;     // 1 byte - seconds to execute command
    unsigned char Power;        // 1 byte - motor duty cycle (80-100%)
};

// -------------------------------------------------------
// TurnBody Structure - used for LEFT / RIGHT commands
// NOTE: Turns always use 100% power (no Power field)
// -------------------------------------------------------
struct TurnBody {
    unsigned char  Direction;   // 1 byte  - RIGHT(3) or LEFT(4)
    unsigned short Duration;    // 2 bytes - seconds to execute command
};

// -------------------------------------------------------
// PktDef Class Declaration
// -------------------------------------------------------
class PktDef {
private:
    // CmdPacket: internal representation of a full packet
    struct CmdPacket {
        Header  header;     // Packet header (4 bytes)
        char*   Data;       // Pointer to dynamic body data
        char    CRC;        // 1-byte CRC trailer
    };

    CmdPacket   cmdPacket;  // The structured packet object
    char*       RawBuffer;  // Serialized packet buffer for transmission

public:
    // Constructors
    PktDef();           // Default constructor - safe state (all zeros)
    PktDef(char*);      // Overloaded constructor - parses a raw buffer

    // Setters
    void    SetCmd(CmdType);
    void    SetBodyData(char*, int);
    void    SetPktCount(int);

    // Getters
    CmdType GetCmd();
    bool    GetAck();
    int     GetLength();
    char*   GetBodyData();
    int     GetPktCount();

    // CRC functions
    bool    CheckCRC(char*, int);
    void    CalcCRC();

    // Packet generation
    char*   GenPacket();
};
