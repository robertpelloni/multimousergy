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
    typedef int SOCKET;
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    #define closesocket close
#endif

NetworkManager::NetworkManager() : m_running(false), m_udpSocket(INVALID_SOCKET), m_tcpSocket(INVALID_SOCKET), m_remoteAddr(nullptr) {
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

    m_remoteAddr = malloc(sizeof(sockaddr_in));
    sockaddr_in* remoteAddr = (sockaddr_in*)m_remoteAddr;
    remoteAddr->sin_family = AF_INET;
    remoteAddr->sin_port = htons(port);
    inet_pton(AF_INET, address.c_str(), &remoteAddr->sin_addr);

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
        if (m_remoteAddr) {
            sendto(m_udpSocket, (const char*)&packet, sizeof(packet), 0, (struct sockaddr*)m_remoteAddr, sizeof(sockaddr_in));
        }
    } else {
        // TCP for clicks/sync
        if (m_tcpSocket != INVALID_SOCKET) {
            send(m_tcpSocket, (const char*)&packet, sizeof(packet), 0);
        }
    }
}

bool NetworkManager::ReceivePacket(Packet& packet) {
    if (!m_running) return false;

    // Check UDP first for movement
    int result = recv(m_udpSocket, (char*)&packet, sizeof(packet), 0);
    if (result > 0) return true;

#ifdef _WIN32
    if (result == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK) {
        // Handle error
    }
#else
    if (result == SOCKET_ERROR && errno != EWOULDBLOCK && errno != EAGAIN) {
        // Handle error
    }
#endif

    // Then check TCP
    if (m_tcpSocket != INVALID_SOCKET) {
        result = recv(m_tcpSocket, (char*)&packet, sizeof(packet), 0);
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
        if (m_remoteAddr) {
            free(m_remoteAddr);
            m_remoteAddr = nullptr;
        }
        m_running = false;
    }
}
