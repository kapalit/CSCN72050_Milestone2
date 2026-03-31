#include "pch.h"
#include "CppUnitTest.h"
#include "../MySocket/MySocket.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace MySocketTests
{

    TEST_CLASS(ConstructorTests)
    {
    public:

        TEST_METHOD(Constructor_SetsIPAddress)
        {
            MySocket sock(CLIENT, "127.0.0.1", 5000, TCP, 1024);

            Assert::AreEqual(std::string("127.0.0.1"), sock.GetIPAddr());
        }

        TEST_METHOD(Constructor_SetsPort)
        {
            MySocket sock(CLIENT, "127.0.0.1", 5000, TCP, 1024);

            Assert::AreEqual(5000, sock.GetPort());
        }

        TEST_METHOD(Constructor_SetsSocketType)
        {
            MySocket sock(SERVER, "127.0.0.1", 5000, TCP, 1024);

            Assert::AreEqual((int)SERVER, (int)sock.GetType());
        }

    };

    TEST_CLASS(SetIPAddrTests)
    {
    public:

        TEST_METHOD(SetIPAddr_ChangesIP)
        {
            MySocket sock(CLIENT, "127.0.0.1", 5000, TCP, 1024);

            sock.SetIPAddr("192.168.1.1");

            Assert::AreEqual(std::string("192.168.1.1"), sock.GetIPAddr());
        }

    };

    TEST_CLASS(SetPortTests)
    {
    public:

        TEST_METHOD(SetPort_ChangesPort)
        {
            MySocket sock(CLIENT, "127.0.0.1", 5000, TCP, 1024);

            sock.SetPort(6000);

            Assert::AreEqual(6000, sock.GetPort());
        }

    };

    TEST_CLASS(SetTypeTests)
    {
    public:

        TEST_METHOD(SetType_ChangesSocketType)
        {
            MySocket sock(CLIENT, "127.0.0.1", 5000, TCP, 1024);

            sock.SetType(SERVER);

            Assert::AreEqual((int)SERVER, (int)sock.GetType());
        }

    };

}