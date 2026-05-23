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
    #include <algorithm>
    typedef int SOCKET;
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    #define closesocket close
#endif

NetworkManager::NetworkManager() : m_running(false), m_udpSocket(INVALID_SOCKET), m_tcpSocket(INVALID_SOCKET), m_hasRemoteAddr(false) {
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

    if (packet.type == PacketType::Movement || packet.type == PacketType::AbsoluteMovement) {
        // UDP for movement (Low Latency)
        if (m_hasRemoteAddr) {
            sendto(m_udpSocket, (const char*)&packet, sizeof(packet), 0, (struct sockaddr*)&m_remoteAddr, sizeof(sockaddr_in));
        }
    } else {
        // TCP for clicks/sync
        if (!m_clientTcpSockets.empty()) {
            for (auto client : m_clientTcpSockets) {
                send(client, (const char*)&packet, sizeof(packet), 0);
            }
        } else if (m_tcpSocket != INVALID_SOCKET) {
            send(m_tcpSocket, (const char*)&packet, sizeof(packet), 0);
        }
    }
}

bool NetworkManager::BroadcastDiscovery(int port) {
    if (m_udpSocket == INVALID_SOCKET) {
        m_udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (m_udpSocket == INVALID_SOCKET) return false;
    }

#ifdef _WIN32
    BOOL broadcast = TRUE;
    setsockopt(m_udpSocket, SOL_SOCKET, SO_BROADCAST, (const char*)&broadcast, sizeof(broadcast));
#else
    int broadcast = 1;
    setsockopt(m_udpSocket, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
#endif

    sockaddr_in broadcastAddr;
    broadcastAddr.sin_family = AF_INET;
    broadcastAddr.sin_port = htons(port);
    broadcastAddr.sin_addr.s_addr = INADDR_BROADCAST;

    DiscoveryPacket pkt = {0};
    gethostname(pkt.hostname, sizeof(pkt.hostname) - 1);
    pkt.hostname[sizeof(pkt.hostname) - 1] = '\0';
    pkt.port = port;

    sendto(m_udpSocket, (const char*)&pkt, sizeof(pkt), 0, (struct sockaddr*)&broadcastAddr, sizeof(broadcastAddr));
    return true;
}

bool NetworkManager::ListenForPeers(DiscoveryPacket& pkt) {
    if (m_udpSocket == INVALID_SOCKET) {
        m_udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (m_udpSocket == INVALID_SOCKET) return false;

        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(5556); // Discovery port
        bind(m_udpSocket, (struct sockaddr*)&addr, sizeof(addr));

#ifdef _WIN32
        unsigned long mode = 1;
        ioctlsocket(m_udpSocket, FIONBIO, &mode);
#else
        fcntl(m_udpSocket, F_SETFL, O_NONBLOCK);
#endif
    }

    sockaddr_in fromAddr;
    socklen_t fromLen = sizeof(fromAddr);
    int result = recvfrom(m_udpSocket, (char*)&pkt, sizeof(pkt), 0, (struct sockaddr*)&fromAddr, &fromLen);

    if (result > 0) {
        if (!m_hasRemoteAddr) {
            m_remoteAddr = fromAddr;
            m_hasRemoteAddr = true;
        }
        return true;
    }
    return false;
}

bool NetworkManager::PollDiscovery(DiscoveryPacket& pkt) {
    if (m_udpSocket == INVALID_SOCKET) {
        m_udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (m_udpSocket == INVALID_SOCKET) return false;

        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(5556); // Discovery port
        bind(m_udpSocket, (struct sockaddr*)&addr, sizeof(addr));

#ifdef _WIN32
        unsigned long mode = 1;
        ioctlsocket(m_udpSocket, FIONBIO, &mode);
#else
        fcntl(m_udpSocket, F_SETFL, O_NONBLOCK);
#endif
    }

    sockaddr_in fromAddr;
    socklen_t fromLen = sizeof(fromAddr);
    int result = recvfrom(m_udpSocket, (char*)&pkt, sizeof(pkt), 0, (struct sockaddr*)&fromAddr, &fromLen);

    if (result >= (int)sizeof(pkt)) {
        pkt.hostname[sizeof(pkt.hostname) - 1] = '\0'; // Safety null-termination
        return true;
    }
    return false;
}

bool NetworkManager::ReceivePacket(Packet& packet) {
    if (!m_running) return false;

    // Drain entire socket buffer into m_tcpBuffer for minimal input lag

    // Handle incoming TCP connections if we are a server
    if (m_tcpSocket != INVALID_SOCKET) {
        sockaddr_in clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);
        SOCKET newClient = accept(m_tcpSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);
        if (newClient != INVALID_SOCKET) {
            std::cout << "[Network] Accepted new TCP connection." << std::endl;
#ifdef _WIN32
            unsigned long mode = 1;
            ioctlsocket(newClient, FIONBIO, &mode);
#else
            fcntl(newClient, F_SETFL, O_NONBLOCK);
#endif
            m_clientTcpSockets.push_back(newClient);
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

    // Then check TCP (accepted client sockets if we are server, or m_tcpSocket if we are client)
    std::vector<unsigned long long> targets;
    if (!m_clientTcpSockets.empty()) {
        targets = m_clientTcpSockets;
    } else if (m_tcpSocket != INVALID_SOCKET && m_remoteAddr.sin_family != 0 && !m_isServer) {
        // Only call recv on m_tcpSocket if we are NOT a server (i.e. we are the client connection)
        targets.push_back(m_tcpSocket);
    }

    for (auto it = targets.begin(); it != targets.end(); ) {
        SOCKET targetTcp = *it;
        char tempBuf[sizeof(Packet) * 10];

        // Loop to drain the specific socket
        while(true) {
            result = recv(targetTcp, tempBuf, sizeof(tempBuf), 0);
            if (result > 0) {
                m_tcpBuffer.insert(m_tcpBuffer.end(), tempBuf, tempBuf + result);
            } else {
                break;
            }
        }

        if (result == 0) {
            std::cout << "[Network] Peer closed TCP connection." << std::endl;
            closesocket(targetTcp);
            if (!m_clientTcpSockets.empty()) {
                auto clientIt = std::find(m_clientTcpSockets.begin(), m_clientTcpSockets.end(), targetTcp);
                if (clientIt != m_clientTcpSockets.end()) m_clientTcpSockets.erase(clientIt);
            } else {
                m_tcpSocket = INVALID_SOCKET;
            }
            it = targets.erase(it);
            continue;
        }
        ++it;
    }

    if (m_tcpBuffer.size() >= sizeof(Packet)) {
        std::memcpy(&packet, m_tcpBuffer.data(), sizeof(Packet));
        m_tcpBuffer.erase(m_tcpBuffer.begin(), m_tcpBuffer.begin() + sizeof(Packet));
        return true;
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
        for (auto client : m_clientTcpSockets) {
            closesocket(client);
        }
        m_clientTcpSockets.clear();
        m_hasRemoteAddr = false;
        m_running = false;
    }
}
