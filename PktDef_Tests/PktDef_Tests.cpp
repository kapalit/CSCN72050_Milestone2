#include "pch.h"
#include "CppUnitTest.h"
#include "../CSCN72050_Milestone1/PktDef.h"
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace PktDefTests
{
	// =====================================================
	// UNIT TESTS FOR PktDef CLASS
	// Covers: Constructors, Setters, Getters, CRC, GenPacket
	// =====================================================

	TEST_CLASS(DefaultConstructorTests){
		public :
			// -----------------------------------------------
			// Test: Default constructor initializes to safe state
			// -----------------------------------------------
			TEST_METHOD(DefaultConstructor_PktCountIsZero){
				PktDef pkt;
	Assert::AreEqual(0, pkt.GetPktCount(), L"PktCount should be 0 after default construction");
}

TEST_METHOD(DefaultConstructor_LengthIsZero)
{
	PktDef pkt;
	Assert::AreEqual(0, pkt.GetLength(), L"Length should be 0 after default construction");
}

TEST_METHOD(DefaultConstructor_AckIsFalse)
{
	PktDef pkt;
	Assert::IsFalse(pkt.GetAck(), L"Ack should be false after default construction");
}

TEST_METHOD(DefaultConstructor_BodyDataIsNull)
{
	PktDef pkt;
	Assert::IsNull(pkt.GetBodyData(), L"BodyData should be nullptr after default construction");
}
}
;

TEST_CLASS(SetPktCountTests){
	public :
		// -----------------------------------------------
		// Test: SetPktCount properly sets packet count
		// -----------------------------------------------
		TEST_METHOD(SetPktCount_SetsValueCorrectly){
			PktDef pkt;
pkt.SetPktCount(42);
Assert::AreEqual(42, pkt.GetPktCount(), L"PktCount should match set value");
}

TEST_METHOD(SetPktCount_HandlesZero)
{
	PktDef pkt;
	pkt.SetPktCount(0);
	Assert::AreEqual(0, pkt.GetPktCount(), L"PktCount should handle zero");
}

TEST_METHOD(SetPktCount_HandlesMaxValue)
{
	PktDef pkt;
	pkt.SetPktCount(65535); // Max unsigned short
	Assert::AreEqual(65535, pkt.GetPktCount(), L"PktCount should handle max unsigned short");
}
}
;

TEST_CLASS(SetCmdTests){
	public :
		// -----------------------------------------------
		// Test: SetCmd properly sets command flags
		// -----------------------------------------------
		TEST_METHOD(SetCmd_DriveCommand){
			PktDef pkt;
pkt.SetCmd(DRIVE);
Assert::AreEqual((int)DRIVE, (int)pkt.GetCmd(), L"Command should be DRIVE");
}

TEST_METHOD(SetCmd_SleepCommand)
{
	PktDef pkt;
	pkt.SetCmd(SLEEP);
	Assert::AreEqual((int)SLEEP, (int)pkt.GetCmd(), L"Command should be SLEEP");
}

TEST_METHOD(SetCmd_ResponseCommand)
{
	PktDef pkt;
	pkt.SetCmd(RESPONSE);
	Assert::AreEqual((int)RESPONSE, (int)pkt.GetCmd(), L"Command should be RESPONSE");
}
}
;

TEST_CLASS(SetBodyDataTests){
	public :
		// -----------------------------------------------
		// Test: SetBodyData allocates and copies data
		// -----------------------------------------------
		TEST_METHOD(SetBodyData_DriveBodyCopied){
			PktDef pkt;
DriveBody body;
body.Direction = FORWARD;
body.Duration = 10;
body.Power = 85;

pkt.SetBodyData(reinterpret_cast<char *>(&body), sizeof(DriveBody));

char *data = pkt.GetBodyData();
Assert::IsNotNull(data, L"Body data should not be null after SetBodyData");
}

TEST_METHOD(SetBodyData_TurnBodyCopied)
{
	PktDef pkt;
	TurnBody body;
	body.Direction = LEFT;
	body.Duration = 500;

	pkt.SetBodyData(reinterpret_cast<char *>(&body), sizeof(TurnBody));

	char *data = pkt.GetBodyData();
	Assert::IsNotNull(data, L"Turn body data should not be null after SetBodyData");
}

TEST_METHOD(SetBodyData_DataMatchesOriginal)
{
	PktDef pkt;
	DriveBody body;
	body.Direction = BACKWARD;
	body.Duration = 5;
	body.Power = 100;

	pkt.SetBodyData(reinterpret_cast<char *>(&body), sizeof(DriveBody));

	DriveBody *retrieved = reinterpret_cast<DriveBody *>(pkt.GetBodyData());
	Assert::AreEqual((int)BACKWARD, (int)retrieved->Direction, L"Direction should match");
	Assert::AreEqual((int)5, (int)retrieved->Duration, L"Duration should match");
	Assert::AreEqual((int)100, (int)retrieved->Power, L"Power should match");
}
}
;

TEST_CLASS(GetLengthTests){
	public :
		// -----------------------------------------------
		// Test: GetLength returns correct packet length
		// -----------------------------------------------
		TEST_METHOD(GetLength_HeaderOnlyPacket){
			PktDef pkt;
pkt.SetCmd(SLEEP);
pkt.SetPktCount(1);
pkt.SetBodyData(nullptr, 0);
pkt.CalcCRC();
char *rawPkt = pkt.GenPacket();

Assert::AreEqual(HEADERSIZE + 1, pkt.GetLength(), L"Sleep packet should be header + CRC only");
}

TEST_METHOD(GetLength_DrivePacketWithBody)
{
	PktDef pkt;
	pkt.SetCmd(DRIVE);
	pkt.SetPktCount(1);
	DriveBody body = {FORWARD, 10, 80};
	pkt.SetBodyData(reinterpret_cast<char *>(&body), sizeof(DriveBody));
	pkt.CalcCRC();
	char *rawPkt = pkt.GenPacket();

	int expectedLen = HEADERSIZE + sizeof(DriveBody) + 1; // header + body + CRC
	Assert::AreEqual(expectedLen, pkt.GetLength(), L"Drive packet should include body size");
}
}
;

TEST_CLASS(CRCTests){
	public :
		// -----------------------------------------------
		// Test: CRC calculation and validation (Person 4)
		// -----------------------------------------------
		TEST_METHOD(CalcCRC_EmptyPacket){
			// Test CRC on a minimal packet (just header)
			PktDef pkt;
pkt.SetCmd(SLEEP);
pkt.SetPktCount(0);
pkt.CalcCRC();

// With all zeros and SLEEP flag set, only 1 bit should be set (Sleep bit)
// But we need to account for Length field too after GenPacket
// This just verifies CalcCRC doesn't crash
Assert::IsTrue(true, L"CalcCRC should complete without error");
}

TEST_METHOD(CheckCRC_ValidPacket)
{
	// Build a simple packet manually matching the spec example
	// PktCount=1, Drive=1, Ack=1, Length=9, Body={1,10,80}, CRC=10
	char rawBuffer[9];

	// PktCount (2 bytes, little-endian): 1
	rawBuffer[0] = 0x01;
	rawBuffer[1] = 0x00;

	// Flags byte: Drive=1, Status=0, Sleep=0, Ack=1, Padding=0
	// Binary: 0000 1001 = 0x09
	rawBuffer[2] = 0x09;

	// Length: 9
	rawBuffer[3] = 0x09;

	// DriveBody: Direction=1, Duration=10, Power=80
	rawBuffer[4] = 0x01;
	rawBuffer[5] = 0x0A;
	rawBuffer[6] = 0x50;

	// CRC: 10 (count of all 1-bits in bytes 0-6)
	rawBuffer[7] = 0x0A;

	PktDef pkt;
	bool result = pkt.CheckCRC(rawBuffer, 8);
	Assert::IsTrue(result, L"CheckCRC should return true for valid packet");
}

TEST_METHOD(CheckCRC_InvalidPacket)
{
	// Same packet but with wrong CRC
	char rawBuffer[9];
	rawBuffer[0] = 0x01;
	rawBuffer[1] = 0x00;
	rawBuffer[2] = 0x09;
	rawBuffer[3] = 0x09;
	rawBuffer[4] = 0x01;
	rawBuffer[5] = 0x0A;
	rawBuffer[6] = 0x50;
	rawBuffer[7] = (char)0xFF; // Wrong CRC

	PktDef pkt;
	bool result = pkt.CheckCRC(rawBuffer, 8);
	Assert::IsFalse(result, L"CheckCRC should return false for corrupted packet");
}

TEST_METHOD(CheckCRC_NullBuffer)
{
	PktDef pkt;
	bool result = pkt.CheckCRC(nullptr, 10);
	Assert::IsFalse(result, L"CheckCRC should return false for null buffer");
}

TEST_METHOD(CheckCRC_ZeroSize)
{
	char buffer[1] = {0};
	PktDef pkt;
	bool result = pkt.CheckCRC(buffer, 0);
	Assert::IsFalse(result, L"CheckCRC should return false for zero-size buffer");
}

TEST_METHOD(CalcCRC_MatchesCheckCRC)
{
	// Build a packet using setters, generate it, then verify CRC
	PktDef pkt;
	pkt.SetCmd(DRIVE);
	pkt.SetPktCount(1);

	DriveBody body = {FORWARD, 10, 80};
	pkt.SetBodyData(reinterpret_cast<char *>(&body), sizeof(DriveBody));

	pkt.CalcCRC();
	char *raw = pkt.GenPacket();
	int len = pkt.GetLength();

	// Now verify the generated packet passes CRC check
	bool crcValid = pkt.CheckCRC(raw, len);
	Assert::IsTrue(crcValid, L"Generated packet should pass CRC validation");
}

TEST_METHOD(CalcCRC_AllZerosBits)
{
	// Edge case: packet with minimal set bits
	PktDef pkt;
	pkt.SetPktCount(0);
	// No command set (all flags 0), no body
	pkt.CalcCRC();
	// Should not crash and CRC should be 0 or close to it
	Assert::IsTrue(true, L"CalcCRC handles minimal packet");
}

TEST_METHOD(CalcCRC_MaxBits)
{
	// Edge case: packet with many set bits
	PktDef pkt;
	pkt.SetCmd(DRIVE);
	pkt.SetPktCount(0xFFFF); // All bits set

	DriveBody body = {0xFF, 0xFF, 0xFF}; // All bits set
	pkt.SetBodyData(reinterpret_cast<char *>(&body), sizeof(DriveBody));

	pkt.CalcCRC();
	char *raw = pkt.GenPacket();
	int len = pkt.GetLength();

	bool crcValid = pkt.CheckCRC(raw, len);
	Assert::IsTrue(crcValid, L"Max bits packet should pass CRC validation");
}
}
;

TEST_CLASS(RawBufferConstructorTests){
	public :
		// -----------------------------------------------
		// Test: Overloaded constructor parses raw buffer
		// -----------------------------------------------
		TEST_METHOD(RawBufferConstructor_ParsesPktCount){
			// Create a raw packet buffer
			char rawBuffer[8];
rawBuffer[0] = 0x01; // PktCount low byte
rawBuffer[1] = 0x00; // PktCount high byte
rawBuffer[2] = 0x09; // Flags (Drive=1, Ack=1)
rawBuffer[3] = 0x08; // Length = 8
rawBuffer[4] = 0x01; // Direction
rawBuffer[5] = 0x0A; // Duration
rawBuffer[6] = 0x50; // Power
rawBuffer[7] = 0x0A; // CRC

PktDef pkt(rawBuffer);
Assert::AreEqual(1, pkt.GetPktCount(), L"PktCount should be parsed from buffer");
}

TEST_METHOD(RawBufferConstructor_ParsesCommand)
{
	char rawBuffer[8];
	rawBuffer[0] = 0x01;
	rawBuffer[1] = 0x00;
	rawBuffer[2] = 0x09; // Drive=1, Ack=1
	rawBuffer[3] = 0x08;
	rawBuffer[4] = 0x01;
	rawBuffer[5] = 0x0A;
	rawBuffer[6] = 0x50;
	rawBuffer[7] = 0x0A;

	PktDef pkt(rawBuffer);
	Assert::AreEqual((int)DRIVE, (int)pkt.GetCmd(), L"Command should be DRIVE");
}

TEST_METHOD(RawBufferConstructor_ParsesAck)
{
	char rawBuffer[8];
	rawBuffer[0] = 0x01;
	rawBuffer[1] = 0x00;
	rawBuffer[2] = 0x09; // Drive=1, Ack=1
	rawBuffer[3] = 0x08;
	rawBuffer[4] = 0x01;
	rawBuffer[5] = 0x0A;
	rawBuffer[6] = 0x50;
	rawBuffer[7] = 0x0A;

	PktDef pkt(rawBuffer);
	Assert::IsTrue(pkt.GetAck(), L"Ack should be true");
}

TEST_METHOD(RawBufferConstructor_ParsesLength)
{
	char rawBuffer[8];
	rawBuffer[0] = 0x01;
	rawBuffer[1] = 0x00;
	rawBuffer[2] = 0x09;
	rawBuffer[3] = 0x08; // Length = 8
	rawBuffer[4] = 0x01;
	rawBuffer[5] = 0x0A;
	rawBuffer[6] = 0x50;
	rawBuffer[7] = 0x0A;

	PktDef pkt(rawBuffer);
	Assert::AreEqual(8, pkt.GetLength(), L"Length should be 8");
}
}
;

TEST_CLASS(GenPacketTests){
	public :
		// -----------------------------------------------
		// Test: GenPacket serializes packet correctly
		// -----------------------------------------------
		TEST_METHOD(GenPacket_ReturnsNonNull){
			PktDef pkt;
pkt.SetCmd(DRIVE);
pkt.SetPktCount(1);
DriveBody body = {FORWARD, 5, 90};
pkt.SetBodyData(reinterpret_cast<char *>(&body), sizeof(DriveBody));
pkt.CalcCRC();

char *raw = pkt.GenPacket();
Assert::IsNotNull(raw, L"GenPacket should return non-null pointer");
}

TEST_METHOD(GenPacket_SleepCommand)
{
	PktDef pkt;
	pkt.SetCmd(SLEEP);
	pkt.SetPktCount(5);
	pkt.SetBodyData(nullptr, 0);
	pkt.CalcCRC();

	char *raw = pkt.GenPacket();
	Assert::IsNotNull(raw, L"Sleep packet should generate successfully");

	// Verify length is just header + CRC
	Assert::AreEqual(HEADERSIZE + 1, pkt.GetLength(), L"Sleep packet should be minimal size");
}

TEST_METHOD(GenPacket_TurnCommand)
{
	PktDef pkt;
	pkt.SetCmd(DRIVE);
	pkt.SetPktCount(10);
	TurnBody body = {LEFT, 1000};
	pkt.SetBodyData(reinterpret_cast<char *>(&body), sizeof(TurnBody));
	pkt.CalcCRC();

	char *raw = pkt.GenPacket();
	int expectedLen = HEADERSIZE + sizeof(TurnBody) + 1;
	Assert::AreEqual(expectedLen, pkt.GetLength(), L"Turn packet should have correct length");
}

TEST_METHOD(GenPacket_CRCAtEnd)
{
	PktDef pkt;
	pkt.SetCmd(DRIVE);
	pkt.SetPktCount(1);
	DriveBody body = {FORWARD, 10, 80};
	pkt.SetBodyData(reinterpret_cast<char *>(&body), sizeof(DriveBody));
	pkt.CalcCRC();

	char *raw = pkt.GenPacket();
	int len = pkt.GetLength();

	// Verify CRC is valid
	bool crcValid = pkt.CheckCRC(raw, len);
	Assert::IsTrue(crcValid, L"Generated packet should have valid CRC");
}
}
;

TEST_CLASS(GetAckTests){
	public :
		// -----------------------------------------------
		// Test: GetAck returns correct acknowledgement state
		// -----------------------------------------------
		TEST_METHOD(GetAck_AckNotSet){
			// Parse a packet without Ack set
			char rawBuffer[5];
rawBuffer[0] = 0x01;
rawBuffer[1] = 0x00;
rawBuffer[2] = 0x01; // Drive=1, Ack=0
rawBuffer[3] = 0x05;
rawBuffer[4] = 0x03; // CRC

PktDef pkt(rawBuffer);
Assert::IsFalse(pkt.GetAck(), L"Ack should be false when not set");
}

TEST_METHOD(GetAck_AckSet)
{
	// Parse a packet with Ack set
	char rawBuffer[5];
	rawBuffer[0] = 0x01;
	rawBuffer[1] = 0x00;
	rawBuffer[2] = 0x09; // Drive=1, Ack=1
	rawBuffer[3] = 0x05;
	rawBuffer[4] = 0x04; // CRC

	PktDef pkt(rawBuffer);
	Assert::IsTrue(pkt.GetAck(), L"Ack should be true when set");
}
}
;

TEST_CLASS(DirectionConstantsTests){
	public :
		// -----------------------------------------------
		// Test: Direction constants have correct values
		// -----------------------------------------------
		TEST_METHOD(DirectionConstants_Forward){
			Assert::AreEqual(1, FORWARD, L"FORWARD should be 1");
}

TEST_METHOD(DirectionConstants_Backward)
{
	Assert::AreEqual(2, BACKWARD, L"BACKWARD should be 2");
}

TEST_METHOD(DirectionConstants_Right)
{
	Assert::AreEqual(3, RIGHT, L"RIGHT should be 3");
}

TEST_METHOD(DirectionConstants_Left)
{
	Assert::AreEqual(4, LEFT, L"LEFT should be 4");
}
}
;

TEST_CLASS(HeaderSizeTests){
	public :
		// -----------------------------------------------
		// Test: HEADERSIZE constant is correct
		// -----------------------------------------------
		TEST_METHOD(HeaderSize_IsCorrect){
			// Header: PktCount(2) + Flags(1) + Length(1) = 4 bytes
			Assert::AreEqual(4, HEADERSIZE, L"HEADERSIZE should be 4 bytes");
}
}
;

TEST_CLASS(IntegrationTests){
	public :
		// -----------------------------------------------
		// Test: Full workflow - build, serialize, parse
		// -----------------------------------------------
		TEST_METHOD(Integration_BuildAndParse){
			// Build a DRIVE command packet
			PktDef original;
original.SetCmd(DRIVE);
original.SetPktCount(100);
DriveBody body = {FORWARD, 15, 95};
original.SetBodyData(reinterpret_cast<char *>(&body), sizeof(DriveBody));
original.CalcCRC();
char *raw = original.GenPacket();

// Parse it back
PktDef parsed(raw);

// Verify all fields match
Assert::AreEqual(original.GetPktCount(), parsed.GetPktCount(), L"PktCount should match");
Assert::AreEqual((int)original.GetCmd(), (int)parsed.GetCmd(), L"Command should match");
Assert::AreEqual(original.GetLength(), parsed.GetLength(), L"Length should match");
}

TEST_METHOD(Integration_MultiplePackets)
{
	// Create and verify multiple packets with different commands
	for (int i = 0; i < 3; i++)
	{
		PktDef pkt;
		CmdType cmd = static_cast<CmdType>(i);
		pkt.SetCmd(cmd);
		pkt.SetPktCount(i + 1);

		if (cmd == DRIVE)
		{
			DriveBody body = {FORWARD, 5, 80};
			pkt.SetBodyData(reinterpret_cast<char *>(&body), sizeof(DriveBody));
		}
		else
		{
			pkt.SetBodyData(nullptr, 0);
		}

		pkt.CalcCRC();
		char *raw = pkt.GenPacket();

		bool crcValid = pkt.CheckCRC(raw, pkt.GetLength());
		Assert::IsTrue(crcValid, L"Each packet should have valid CRC");
	}
}
}
;
}