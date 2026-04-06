#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>

#pragma comment(lib, "ws2_32.lib")

// -------------------------------------------------------
// Global Enumerations
// -------------------------------------------------------
enum SocketType { CLIENT, SERVER };
enum ConnectionType { TCP, UDP };

// -------------------------------------------------------
// Default buffer size used if an invalid size is provided
// -------------------------------------------------------
const int DEFAULT_SIZE = 1024;

// -------------------------------------------------------
// MySocket Class Declaration
//
// Manages a single TCP or UDP socket connection.
// Can operate as either a Client or Server.
// -------------------------------------------------------
class MySocket
{
private:
    char* Buffer;           // Dynamically allocated RAW communication buffer
    SOCKET             WelcomeSocket;    // TCP server listening socket (accepts incoming connections)
    SOCKET             ConnectionSocket; // Active communication socket (TCP & UDP)
    struct sockaddr_in SvrAddr;          // Stores server address and port information
    struct sockaddr_in ClientAddr;
    SocketType         mySocket;         // CLIENT or SERVER
    std::string        IPAddr;           // IPv4 address string
    int                Port;             // Port number
    ConnectionType     connectionType;   // TCP or UDP
    bool               bTCPConnect;      // True if a TCP connection is currently established
    int                MaxSize;          // Maximum buffer capacity in bytes

public:
    // Constructor & Destructor
    MySocket(SocketType, std::string, unsigned int, ConnectionType, unsigned int);
    ~MySocket();

    // TCP Connection Management
    void ConnectTCP();
    void DisconnectTCP();

    // Data Transmission (works for both TCP and UDP)
    void SendData(const char*, int);
    int  GetData(char*);

    // Getters
    std::string GetIPAddr();
    int         GetPort();
    SocketType  GetType();
    SOCKET      GetConnectionSocket() { return ConnectionSocket; }

    // Setters (blocked if a TCP connection is already established)
    void SetIPAddr(std::string);
    void SetPort(int);
    void SetType(SocketType);
};