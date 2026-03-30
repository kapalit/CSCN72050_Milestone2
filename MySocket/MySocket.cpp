#include "MySocket.h"
#include <iostream>

// -------------------------------------------------------
// MySocket() -- Constructor
//
// Configures the socket based on the provided parameters:
//   - Stores all configuration (type, IP, port, protocol)
//   - Validates and allocates the communication buffer
//   - Initializes Winsock (WSAStartup)
//   - Creates and configures the appropriate socket:
//       TCP Server  -> creates WelcomeSocket, binds, listens
//       TCP Client  -> creates ConnectionSocket (connect handled by ConnectTCP)
//       UDP Server  -> creates ConnectionSocket, binds (ready to receive)
//       UDP Client  -> creates ConnectionSocket (ready to send)
//
// Parameters:
//   socketType  - CLIENT or SERVER
//   ipAddr      - IPv4 address string (e.g. "192.168.1.1")
//   port        - port number to use
//   connType    - TCP or UDP
//   bufferSize  - size of the communication buffer (uses DEFAULT_SIZE if 0 or invalid)
// -------------------------------------------------------
MySocket::MySocket(SocketType socketType, std::string ipAddr, unsigned int port,
                   ConnectionType connType, unsigned int bufferSize)
{
    // Store configuration
    mySocket       = socketType;
    IPAddr         = ipAddr;
    Port           = (int)port;
    connectionType = connType;
    bTCPConnect    = false;

    // Initialize socket handles to invalid state
    WelcomeSocket    = INVALID_SOCKET;
    ConnectionSocket = INVALID_SOCKET;

    // Validate buffer size — fall back to DEFAULT_SIZE if zero or invalid
    MaxSize = (bufferSize > 0) ? (int)bufferSize : DEFAULT_SIZE;

    // Allocate and zero-initialize the communication buffer
    Buffer = new char[MaxSize];
    memset(Buffer, 0, MaxSize);

    // Initialize Winsock 2.2
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        std::cerr << "WSAStartup failed." << std::endl;
        return;
    }

    // Build the server address structure
    memset(&SvrAddr, 0, sizeof(SvrAddr));
    SvrAddr.sin_family      = AF_INET;
    inet_pton(AF_INET, IPAddr.c_str(), &SvrAddr.sin_addr);
    SvrAddr.sin_port        = htons((u_short)Port);

    if (connectionType == TCP)
    {
        if (mySocket == SERVER)
        {
            // Create the welcome (listening) socket for incoming TCP connections
            WelcomeSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (WelcomeSocket == INVALID_SOCKET)
            {
                std::cerr << "Failed to create WelcomeSocket." << std::endl;
                return;
            }

            // Bind the welcome socket to the configured address and port
            if (bind(WelcomeSocket, (struct sockaddr*)&SvrAddr, sizeof(SvrAddr)) == SOCKET_ERROR)
            {
                std::cerr << "TCP Server bind failed." << std::endl;
                return;
            }

            // Put the socket into listening state — ready to accept connections
            if (listen(WelcomeSocket, SOMAXCONN) == SOCKET_ERROR)
            {
                std::cerr << "TCP Server listen failed." << std::endl;
                return;
            }
        }
        else // TCP CLIENT
        {
            // Create the connection socket — actual connect() happens in ConnectTCP()
            ConnectionSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (ConnectionSocket == INVALID_SOCKET)
            {
                std::cerr << "Failed to create TCP ConnectionSocket." << std::endl;
                return;
            }
        }
    }
    else // UDP
    {
        // Create a single UDP socket for both client and server
        ConnectionSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (ConnectionSocket == INVALID_SOCKET)
        {
            std::cerr << "Failed to create UDP socket." << std::endl;
            return;
        }

        if (mySocket == SERVER)
        {
            // UDP Server binds to port so it can receive incoming datagrams
            if (bind(ConnectionSocket, (struct sockaddr*)&SvrAddr, sizeof(SvrAddr)) == SOCKET_ERROR)
            {
                std::cerr << "UDP Server bind failed." << std::endl;
                return;
            }
        }
        // UDP Client: no bind needed — SendData will use SvrAddr with sendto()
    }
}

// -------------------------------------------------------
// ~MySocket() -- Destructor
//
// Cleans up all dynamically allocated resources:
//   - Frees the communication buffer
//   - Closes the ConnectionSocket (if open)
//   - Closes the WelcomeSocket (if open, TCP server only)
//   - Calls WSACleanup to release Winsock resources
// -------------------------------------------------------
MySocket::~MySocket()
{
    // Free the communication buffer
    if (Buffer != nullptr)
    {
        delete[] Buffer;
        Buffer = nullptr;
    }

    // Close the active connection socket
    if (ConnectionSocket != INVALID_SOCKET)
    {
        closesocket(ConnectionSocket);
        ConnectionSocket = INVALID_SOCKET;
    }

    // Close the TCP server welcome socket (if applicable)
    if (WelcomeSocket != INVALID_SOCKET)
    {
        closesocket(WelcomeSocket);
        WelcomeSocket = INVALID_SOCKET;
    }

    // Release Winsock resources
    WSACleanup();
}


// -------------------------------------------------------
// Remaining member function stubs
// (To be implemented by teammates)
// -------------------------------------------------------

void MySocket::ConnectTCP()
{
    // TODO: Person 2
    // Guard: do nothing if connectionType == UDP
    // CLIENT: call connect() on ConnectionSocket using SvrAddr, set bTCPConnect = true
    // SERVER: call accept() on WelcomeSocket, store result in ConnectionSocket, set bTCPConnect = true
}

void MySocket::DisconnectTCP()
{
    // TODO: Person 2
    // Guard: do nothing if connectionType == UDP or bTCPConnect == false
    // Call shutdown() then closesocket() on ConnectionSocket
    // For SERVER: re-open a fresh ConnectionSocket ready for the next accept()
    // Set bTCPConnect = false
}

void MySocket::SendData(const char* data, int size)
{
    // TODO: Person 2
    // TCP: call send() on ConnectionSocket
    // UDP Client: call sendto() using SvrAddr
    // UDP Server: call sendto() using the client address captured during GetData()
}

int MySocket::GetData(char* dest)
{
    // TODO: Person 2
    // TCP: call recv() into Buffer, copy to dest, return byte count
    // UDP: call recvfrom() into Buffer, copy to dest, return byte count
    return 0;
}

std::string MySocket::GetIPAddr()
{
    // TODO: Person 3
    return "";
}

void MySocket::SetIPAddr(std::string ip)
{
    // TODO: Person 3
    // Print error and return if bTCPConnect == true
}

void MySocket::SetPort(int port)
{
    // TODO: Person 3
    // Print error and return if bTCPConnect == true
}

int MySocket::GetPort()
{
    // TODO: Person 3
    return 0;
}

SocketType MySocket::GetType()
{
    // TODO: Person 3
    return CLIENT;
}

void MySocket::SetType(SocketType type)
{
    // TODO: Person 3
    // Guard: prevent change if bTCPConnect == true or WelcomeSocket is open
}
