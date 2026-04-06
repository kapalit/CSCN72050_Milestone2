
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
    // Guard: only valid for TCP
    if (connectionType != TCP)
    {
        std::cerr << "ConnectTCP: not a TCP socket." << std::endl;
        return;
    }

    if (mySocket == CLIENT)
    {
        // Connect the client socket to the server address
        if (connect(ConnectionSocket, (struct sockaddr*)&SvrAddr, sizeof(SvrAddr)) == SOCKET_ERROR)
        {
            std::cerr << "ConnectTCP: connect() failed. Error: " << WSAGetLastError() << std::endl;
            return;
        }
        bTCPConnect = true;
    }
    else // SERVER
    {
        // Block until a client connects; store the resulting socket
        ConnectionSocket = accept(WelcomeSocket, nullptr, nullptr);
        if (ConnectionSocket == INVALID_SOCKET)
        {
            std::cerr << "ConnectTCP: accept() failed. Error: " << WSAGetLastError() << std::endl;
            return;
        }
        bTCPConnect = true;
    }
}

void MySocket::DisconnectTCP()
{
    // Guard: only valid for an active TCP connection
    if (connectionType != TCP)
    {
        std::cerr << "DisconnectTCP: not a TCP socket." << std::endl;
        return;
    }
    if (!bTCPConnect)
    {
        std::cerr << "DisconnectTCP: no active TCP connection." << std::endl;
        return;
    }

    // Gracefully shut down both directions, then close
    shutdown(ConnectionSocket, SD_BOTH);
    closesocket(ConnectionSocket);
    ConnectionSocket = INVALID_SOCKET;
    bTCPConnect = false;

    if (mySocket == SERVER)
    {
        // Re-open a fresh socket so the server can accept the next client
        ConnectionSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (ConnectionSocket == INVALID_SOCKET)
        {
            std::cerr << "DisconnectTCP: failed to re-create server ConnectionSocket. Error: "
                << WSAGetLastError() << std::endl;
        }
    }
}

void MySocket::SendData(const char* data, int size)
{
    if (data == nullptr || size <= 0)
    {
        std::cerr << "SendData: invalid data or size." << std::endl;
        return;
    }

    if (connectionType == TCP)
    {
        // TCP: send over the established connection
        int bytesSent = send(ConnectionSocket, data, size, 0);
        if (bytesSent == SOCKET_ERROR)
        {
            std::cerr << "SendData (TCP): send() failed. Error: " << WSAGetLastError() << std::endl;
        }
    }
    else // UDP
    {
        if (mySocket == CLIENT)
        {
            // UDP Client: send to the server address stored in SvrAddr
            int bytesSent = sendto(ConnectionSocket, data, size, 0,
                (struct sockaddr*)&SvrAddr, sizeof(SvrAddr));
            if (bytesSent == SOCKET_ERROR)
            {
                std::cerr << "SendData (UDP Client): sendto() failed. Error: "
                    << WSAGetLastError() << std::endl;
            }
        }
        else // UDP SERVER
        {
            // UDP Server: reply to the client address captured in GetData()
            int bytesSent = sendto(ConnectionSocket, data, size, 0,
                (struct sockaddr*)&ClientAddr, sizeof(ClientAddr));
            if (bytesSent == SOCKET_ERROR)
            {
                std::cerr << "SendData (UDP Server): sendto() failed. Error: "
                    << WSAGetLastError() << std::endl;
            }
        }
    }
}

int MySocket::GetData(char* dest)
{
    if (dest == nullptr)
    {
        std::cerr << "GetData: null destination pointer." << std::endl;
        return 0;
    }

    int bytesReceived = 0;

    if (connectionType == TCP)
    {
        // TCP: receive into the internal buffer
        bytesReceived = recv(ConnectionSocket, Buffer, MaxSize, 0);
        if (bytesReceived == SOCKET_ERROR)
        {
            std::cerr << "GetData (TCP): recv() failed. Error: " << WSAGetLastError() << std::endl;
            return 0;
        }
    }
    else // UDP
    {
        // UDP: receive and capture the sender's address for later SendData replies
        int addrLen = sizeof(ClientAddr);
        bytesReceived = recvfrom(ConnectionSocket, Buffer, MaxSize, 0,
            (struct sockaddr*)&ClientAddr, &addrLen);
        if (bytesReceived == SOCKET_ERROR)
        {
            std::cerr << "GetData (UDP): recvfrom() failed. Error: " << WSAGetLastError() << std::endl;
            return 0;
        }
    }

    // Copy received bytes into the caller's destination buffer
    memcpy(dest, Buffer, bytesReceived);
    return bytesReceived;
}

std::string MySocket::GetIPAddr()
{
    
    return IPAddr;
}

void MySocket::SetIPAddr(std::string ip)
{
    // Prevent change if TCP connection is active
    if (bTCPConnect)
    {
        std::cerr << "SetIPAddr error: Cannot change IP address while TCP connection is active." << std::endl;
        return;
    }

    // Update stored IP address
    IPAddr = ip;

    // Update socket address structure
    inet_pton(AF_INET, IPAddr.c_str(), &SvrAddr.sin_addr);
}

void MySocket::SetPort(int port)
{
    // Prevent modification if a TCP connection already exists
    if (bTCPConnect)
    {
        std::cerr << "SetPort error: Cannot change port while TCP connection is active." << std::endl;
        return;
    }

    // Update the stored port number
    Port = port;

    // Update the server address structure with the new port
    SvrAddr.sin_port = htons((u_short)Port);
}

int MySocket::GetPort()
{
    // GetPort()
    // Returns the configured port number used by the socket
    return Port;
}

SocketType MySocket::GetType()
{
    // GetType()
    // Returns whether this socket is configured as client or server
    return mySocket;
}

void MySocket::SetType(SocketType type)
{

    // Guard: prevent change if bTCPConnect == true or WelcomeSocket is open
    // Prevent modification if a connection already exists
    if (bTCPConnect || WelcomeSocket != INVALID_SOCKET)
    {
        std::cerr << "SetType error: Cannot change socket type while connection is active." << std::endl;
        return;
    }

    // Update socket type
    mySocket = type;
}

