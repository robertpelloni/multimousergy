#include "NetworkManager.hpp"
#include <iostream>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <cstring>
    typedef int SOCKET;
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    #define closesocket close
#endif

NetworkManager::NetworkManager() : m_running(false), m_udpSocket(INVALID_SOCKET), m_tcpSocket(INVALID_SOCKET), m_clientTcpSocket(INVALID_SOCKET), m_hasRemoteAddr(false) {
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
}

NetworkManager::~NetworkManager() {
    Shutdown();
#ifdef _WIN32
    WSACleanup();
#endif
}

bool NetworkManager::StartServer(int port) {
    std::cout << "[Network] Starting server on port " << port << "..." << std::endl;

    m_udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_udpSocket == INVALID_SOCKET) return false;

#ifdef _WIN32
    unsigned long mode = 1;
    ioctlsocket(m_udpSocket, FIONBIO, &mode);
#else
    fcntl(m_udpSocket, F_SETFL, O_NONBLOCK);
#endif

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (bind(m_udpSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        return false;
    }

    m_tcpSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_tcpSocket == INVALID_SOCKET) return false;

#ifdef _WIN32
    ioctlsocket(m_tcpSocket, FIONBIO, &mode);
#else
    fcntl(m_tcpSocket, F_SETFL, O_NONBLOCK);
#endif

    if (bind(m_tcpSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        return false;
    }

    if (listen(m_tcpSocket, SOMAXCONN) == SOCKET_ERROR) {
        return false;
    }

    m_running = true;
    return true;
}

bool NetworkManager::Connect(const std::string& address, int port) {
    std::cout << "[Network] Connecting to " << address << ":" << port << "..." << std::endl;

    m_remoteAddr.sin_family = AF_INET;
    m_remoteAddr.sin_port = htons(port);
    inet_pton(AF_INET, address.c_str(), &m_remoteAddr.sin_addr);
    m_hasRemoteAddr = true;

    m_udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_udpSocket == INVALID_SOCKET) return false;

#ifdef _WIN32
    unsigned long mode = 1;
    ioctlsocket(m_udpSocket, FIONBIO, &mode);
#else
    fcntl(m_udpSocket, F_SETFL, O_NONBLOCK);
#endif

    m_tcpSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_tcpSocket == INVALID_SOCKET) return false;

    sockaddr_in clientAddr;
    clientAddr.sin_family = AF_INET;
    clientAddr.sin_port = htons(port);
    inet_pton(AF_INET, address.c_str(), &clientAddr.sin_addr);

    // For TCP, connect is blocking by default, but we can make it non-blocking later if needed.
    // However, the prompt review mentioned blocking recv as the main issue.
    if (connect(m_tcpSocket, (struct sockaddr*)&clientAddr, sizeof(clientAddr)) == SOCKET_ERROR) {
#ifdef _WIN32
        if (WSAGetLastError() != WSAEWOULDBLOCK) return false;
#else
        if (errno != EINPROGRESS) return false;
#endif
    }

#ifdef _WIN32
    ioctlsocket(m_tcpSocket, FIONBIO, &mode);
#else
    fcntl(m_tcpSocket, F_SETFL, O_NONBLOCK);
#endif

    m_running = true;
    return true;
}

void NetworkManager::SendPacket(const Packet& packet) {
    if (!m_running || m_udpSocket == INVALID_SOCKET) return;

    if (packet.type == PacketType::Movement) {
        // UDP for movement
        if (m_hasRemoteAddr) {
            sendto(m_udpSocket, (const char*)&packet, sizeof(packet), 0, (struct sockaddr*)&m_remoteAddr, sizeof(sockaddr_in));
        }
    } else {
        // TCP for clicks/sync
        SOCKET targetTcp = (m_clientTcpSocket != INVALID_SOCKET) ? m_clientTcpSocket : m_tcpSocket;
        if (targetTcp != INVALID_SOCKET) {
            send(targetTcp, (const char*)&packet, sizeof(packet), 0);
        }
    }
}

bool NetworkManager::ReceivePacket(Packet& packet) {
    if (!m_running) return false;

    // Handle incoming TCP connections if we are a server
    if (m_tcpSocket != INVALID_SOCKET && m_clientTcpSocket == INVALID_SOCKET) {
        sockaddr_in clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);
        m_clientTcpSocket = accept(m_tcpSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);
        if (m_clientTcpSocket != INVALID_SOCKET) {
            std::cout << "[Network] Accepted new TCP connection." << std::endl;
#ifdef _WIN32
            unsigned long mode = 1;
            ioctlsocket(m_clientTcpSocket, FIONBIO, &mode);
#else
            fcntl(m_clientTcpSocket, F_SETFL, O_NONBLOCK);
#endif
        }
    }

    // Check UDP first for movement
    sockaddr_in fromAddr;
    socklen_t fromLen = sizeof(fromAddr);
    int result = recvfrom(m_udpSocket, (char*)&packet, sizeof(packet), 0, (struct sockaddr*)&fromAddr, &fromLen);
    if (result > 0) {
        // If we don't have a remote address yet, learn it from this UDP packet
        if (!m_hasRemoteAddr) {
            m_remoteAddr = fromAddr;
            m_hasRemoteAddr = true;
        }
        return true;
    }

#ifdef _WIN32
    if (result == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK) {
        // Handle error
    }
#else
    if (result == SOCKET_ERROR && errno != EWOULDBLOCK && errno != EAGAIN) {
        // Handle error
    }
#endif

    // Then check TCP (the accepted client socket if we are server, or m_tcpSocket if we are client)
    SOCKET targetTcp = (m_clientTcpSocket != INVALID_SOCKET) ? m_clientTcpSocket : m_tcpSocket;

    if (targetTcp != INVALID_SOCKET) {
        result = recv(targetTcp, (char*)&packet, sizeof(packet), 0);
        if (result > 0) return true;
    }

    return false;
}

void NetworkManager::Shutdown() {
    if (m_running) {
        std::cout << "[Network] Shutting down connections..." << std::endl;
        if (m_udpSocket != INVALID_SOCKET) {
            closesocket(m_udpSocket);
            m_udpSocket = INVALID_SOCKET;
        }
        if (m_tcpSocket != INVALID_SOCKET) {
            closesocket(m_tcpSocket);
            m_tcpSocket = INVALID_SOCKET;
        }
        if (m_clientTcpSocket != INVALID_SOCKET) {
            closesocket(m_clientTcpSocket);
            m_clientTcpSocket = INVALID_SOCKET;
        }
        m_hasRemoteAddr = false;
        m_running = false;
    }
}
