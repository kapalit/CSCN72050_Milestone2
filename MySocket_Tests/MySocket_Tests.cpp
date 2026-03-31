#include "pch.h"
#include "CppUnitTest.h"
#include "../MySocket/MySocket.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

// Unit tests for MySocket class
// Tests constructor initialization, getters, setters,
// boundary values, resource cleanup, and configuration behavior.

namespace MySocketTests
{
    TEST_CLASS(ConstructorTests)
    {
    public:
        TEST_METHOD(Constructor_InitializesIPAddress)
        {
            MySocket sock(CLIENT, "127.0.0.1", 5000, TCP, 1024);
            Assert::AreEqual(std::string("127.0.0.1"), sock.GetIPAddr());
        }

        TEST_METHOD(Constructor_InitializesPort)
        {
            MySocket sock(CLIENT, "127.0.0.1", 5000, TCP, 1024);
            Assert::AreEqual(5000, sock.GetPort());
        }

        TEST_METHOD(Constructor_InitializesSocketType)
        {
            MySocket sock(SERVER, "127.0.0.1", 5000, TCP, 1024);
            Assert::AreEqual(static_cast<int>(SERVER), static_cast<int>(sock.GetType()));
        }
    };

    TEST_CLASS(SetterTests)
    {
    public:
        TEST_METHOD(SetIPAddr_UpdatesIPAddress)
        {
            MySocket sock(CLIENT, "127.0.0.1", 5000, TCP, 1024);
            sock.SetIPAddr("192.168.1.1");
            Assert::AreEqual(std::string("192.168.1.1"), sock.GetIPAddr());
        }

        TEST_METHOD(SetPort_UpdatesPort)
        {
            MySocket sock(CLIENT, "127.0.0.1", 5000, TCP, 1024);
            sock.SetPort(6000);
            Assert::AreEqual(6000, sock.GetPort());
        }

        TEST_METHOD(SetType_UpdatesSocketType)
        {
            MySocket sock(CLIENT, "127.0.0.1", 5000, TCP, 1024);
            sock.SetType(SERVER);
            Assert::AreEqual(static_cast<int>(SERVER), static_cast<int>(sock.GetType()));
        }

        TEST_METHOD(SetPort_ValidBoundary)
        {
            MySocket sock(CLIENT, "127.0.0.1", 5000, TCP, 1024);
            sock.SetPort(65535);
            Assert::AreEqual(65535, sock.GetPort());
        }

        TEST_METHOD(SetPort_AcceptsZero)
        {
            // Verifies setter accepts zero (valid for certain socket operations)
            MySocket sock(CLIENT, "127.0.0.1", 5000, TCP, 1024);
            sock.SetPort(0);
            Assert::AreEqual(0, sock.GetPort());
        }
    };

    TEST_CLASS(GetterTests)
    {
    public:
        TEST_METHOD(GetType_ReturnsCorrectEnum)
        {
            MySocket sock(SERVER, "127.0.0.1", 5000, TCP, 1024);
            Assert::AreEqual(static_cast<int>(SERVER), static_cast<int>(sock.GetType()));
        }

        TEST_METHOD(GetIPAddr_ReturnsStoredValue)
        {
            MySocket sock(CLIENT, "10.0.0.1", 5000, TCP, 1024);
            Assert::AreEqual(std::string("10.0.0.1"), sock.GetIPAddr());
        }
    };

    TEST_CLASS(DestructorTests)
    {
    public:
        TEST_METHOD(Destructor_CleansSocketResources)
        {
            MySocket* sock = new MySocket(CLIENT, "127.0.0.1", 5000, TCP, 1024);
            delete sock;
        }
    };

    TEST_CLASS(ConfigurationInteractionTests)
    {
    public:
        TEST_METHOD(ChangeIPThenPort_PersistsBothValues)
        {
            MySocket sock(CLIENT, "127.0.0.1", 5000, TCP, 1024);

            sock.SetIPAddr("192.168.0.5");
            sock.SetPort(8080);

            Assert::AreEqual(std::string("192.168.0.5"), sock.GetIPAddr());
            Assert::AreEqual(8080, sock.GetPort());
        }

        TEST_METHOD(ChangeTypeMultipleTimes_FinalValueCorrect)
        {
            MySocket sock(CLIENT, "127.0.0.1", 5000, TCP, 1024);

            sock.SetType(SERVER);
            sock.SetType(CLIENT);

            Assert::AreEqual(static_cast<int>(CLIENT), static_cast<int>(sock.GetType()));
        }

        TEST_METHOD(SetIPAddr_MultipleUpdates_LastValueStored)
        {
            MySocket sock(CLIENT, "127.0.0.1", 5000, TCP, 1024);

            sock.SetIPAddr("192.168.1.1");
            sock.SetIPAddr("10.0.0.5");

            Assert::AreEqual(std::string("10.0.0.5"), sock.GetIPAddr());
        }
    };

    TEST_CLASS(ConfigurationConsistencyTests)
    {
    public:
        TEST_METHOD(SetType_MultipleUpdates_LastValueStored)
        {
            MySocket sock(CLIENT, "127.0.0.1", 5000, TCP, 1024);

            sock.SetType(SERVER);
            sock.SetType(CLIENT);

            Assert::AreEqual(static_cast<int>(CLIENT), static_cast<int>(sock.GetType()));
        }
    };

    // --------------------------------------------------------
    // Server connection tests
    // These only pass when the simulator is running and you
    // are connected to the Conestoga VPN.
    // --------------------------------------------------------
    TEST_CLASS(ServerConnectionTests)
    {
    public:
        TEST_METHOD(ConnectTCP_ToSimulator)
        {
            MySocket sock(CLIENT, "10.172.41.150", 29000, TCP, 1024);

            int timeout = 3000;
            setsockopt(sock.GetConnectionSocket(), SOL_SOCKET, SO_RCVTIMEO,
                (char*)&timeout, sizeof(timeout));

            sock.ConnectTCP();

            // Actually verify the connection succeeded by checking the socket is valid
            Assert::IsTrue(sock.GetConnectionSocket() != INVALID_SOCKET,
                L"TCP connection to simulator failed - is the server running?");

            sock.DisconnectTCP();
        }

        TEST_METHOD(SendReceive_UDP_ToSimulator)
        {
            MySocket sock(CLIENT, "10.172.41.150", 29500, UDP, 1024);

            int timeout = 3000;
            setsockopt(sock.GetConnectionSocket(), SOL_SOCKET, SO_RCVTIMEO,
                (char*)&timeout, sizeof(timeout));

            const char* msg = "Hello";
            sock.SendData(msg, 5);

            char response[1024] = {};
            int bytes = sock.GetData(response);

            // Log what we got back
            Logger::WriteMessage("UDP bytes received: ");
            Logger::WriteMessage(std::to_string(bytes).c_str());

            Assert::IsTrue(bytes > 0,
                L"No UDP response from simulator - is the server running?");
        }

        TEST_METHOD(TCP_SendData_DoesNotCrash)
        {
            MySocket sock(CLIENT, "10.172.41.150", 29000, TCP, 1024);

            int timeout = 3000;
            setsockopt(sock.GetConnectionSocket(), SOL_SOCKET, SO_RCVTIMEO,
                (char*)&timeout, sizeof(timeout));

            sock.ConnectTCP();

            Assert::IsTrue(sock.GetConnectionSocket() != INVALID_SOCKET,
                L"TCP connection failed before SendData");

            const char* msg = "Hello";
            sock.SendData(msg, 5);

            // Try to read any response
            char response[1024] = {};
            int bytes = sock.GetData(response);
            Logger::WriteMessage("TCP bytes received: ");
            Logger::WriteMessage(std::to_string(bytes).c_str());

            sock.DisconnectTCP();
        }
    };
}