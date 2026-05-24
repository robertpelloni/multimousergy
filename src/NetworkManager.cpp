#include "NetworkManager.hpp"
#include <iostream>

#include <cstring>
#include <algorithm>

#ifdef _WIN32
    #pragma comment(lib, "ws2_32.lib")
#else
    #define closesocket close
#endif

NetworkManager::NetworkManager() : m_running(false), m_udpSocket(INVALID_SOCKET_HANDLE), m_tcpSocket(INVALID_SOCKET_HANDLE), m_hasRemoteAddr(false) {
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
    if (m_udpSocket == INVALID_SOCKET_HANDLE) return false;

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
    if (m_tcpSocket == INVALID_SOCKET_HANDLE) return false;

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
    if (m_udpSocket == INVALID_SOCKET_HANDLE) return false;

#ifdef _WIN32
    unsigned long mode = 1;
    ioctlsocket(m_udpSocket, FIONBIO, &mode);
#else
    fcntl(m_udpSocket, F_SETFL, O_NONBLOCK);
#endif

    m_tcpSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_tcpSocket == INVALID_SOCKET_HANDLE) return false;

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

void NetworkManager::SendPacketToGroup(const Packet& packet, unsigned int groupId) {
    if (!m_running || m_udpSocket == INVALID_SOCKET_HANDLE || !m_isServer) return;

    for (auto const& [id, peer] : m_udpPeerMap) {
        if (peer.groupId == groupId) {
            sendto(m_udpSocket, (const char*)&packet, sizeof(packet), 0, (struct sockaddr*)&peer.addr, sizeof(sockaddr_in));
        }
    }
}

void NetworkManager::SendPacket(const Packet& packet) {
    if (!m_running || m_udpSocket == INVALID_SOCKET_HANDLE) return;

    if (packet.type == PacketType::Movement || packet.type == PacketType::AbsoluteMovement) {
        // UDP for movement (Low Latency)
        if (m_isServer) {
            // Server broadcasts movement to all known UDP peers
            for (auto const& [id, peer] : m_udpPeerMap) {
                sendto(m_udpSocket, (const char*)&packet, sizeof(packet), 0, (struct sockaddr*)&peer.addr, sizeof(sockaddr_in));
            }
        } else if (m_hasRemoteAddr) {
            sendto(m_udpSocket, (const char*)&packet, sizeof(packet), 0, (struct sockaddr*)&m_remoteAddr, sizeof(sockaddr_in));
        }
    } else {
        // TCP for clicks/sync
        if (!m_clients.empty()) {
            for (auto& client : m_clients) {
                send(client.socket, (const char*)&packet, sizeof(packet), 0);
            }
        } else if (m_tcpSocket != INVALID_SOCKET_HANDLE && !m_isServer) {
            send(m_tcpSocket, (const char*)&packet, sizeof(packet), 0);
        }
    }
}

bool NetworkManager::BroadcastDiscovery(int port) {
    if (m_udpSocket == INVALID_SOCKET_HANDLE) {
        m_udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (m_udpSocket == INVALID_SOCKET_HANDLE) return false;
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
    if (m_udpSocket == INVALID_SOCKET_HANDLE) {
        m_udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (m_udpSocket == INVALID_SOCKET_HANDLE) return false;

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
    if (m_udpSocket == INVALID_SOCKET_HANDLE) {
        m_udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (m_udpSocket == INVALID_SOCKET_HANDLE) return false;

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

    // 1. Accept new TCP connections
    if (m_isServer && m_tcpSocket != INVALID_SOCKET_HANDLE) {
        sockaddr_in clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);
        Socket newClient = accept(m_tcpSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);
        if (newClient != INVALID_SOCKET_HANDLE) {
            std::cout << "[Network] Accepted new TCP connection." << std::endl;
#ifdef _WIN32
            unsigned long mode = 1;
            ioctlsocket(newClient, FIONBIO, &mode);
#else
            fcntl(newClient, F_SETFL, O_NONBLOCK);
#endif
            m_clients.push_back({newClient, {}});
        }
    }

    // 2. Check UDP movement (Low Latency)
    sockaddr_in fromAddr;
    socklen_t fromLen = sizeof(fromAddr);
    int result = recvfrom(m_udpSocket, (char*)&packet, sizeof(packet), 0, (struct sockaddr*)&fromAddr, &fromLen);
    if (result >= (int)sizeof(Packet)) {
        if (m_isServer) {
            // Update/Add peer to UDP routing map
            m_udpPeerMap[packet.senderId] = { fromAddr, packet.groupId };
        }

        if (!m_hasRemoteAddr) {
            m_remoteAddr = fromAddr;
            m_hasRemoteAddr = true;
        }
        return true;
    }

    // 3. Process Per-Client TCP buffers to prevent interleaving
    for (auto it = m_clients.begin(); it != m_clients.end(); ) {
        char tempBuf[sizeof(Packet) * 10];
        while(true) {
            result = recv(it->socket, tempBuf, sizeof(tempBuf), 0);
            if (result > 0) {
                it->buffer.insert(it->buffer.end(), tempBuf, tempBuf + result);
            } else break;
        }

        if (it->buffer.size() >= sizeof(Packet)) {
            std::memcpy(&packet, it->buffer.data(), sizeof(Packet));
            it->buffer.erase(it->buffer.begin(), it->buffer.begin() + sizeof(Packet));
            return true;
        }

        if (result == 0) {
            std::cout << "[Network] Peer closed connection." << std::endl;
            closesocket(it->socket);
            it = m_clients.erase(it);
        } else ++it;
    }

    // 4. Process Connector TCP buffer (Client-side)
    if (!m_isServer && m_tcpSocket != INVALID_SOCKET_HANDLE && m_hasRemoteAddr) {
        char tempBuf[sizeof(Packet) * 10];
        while(true) {
            result = recv(m_tcpSocket, tempBuf, sizeof(tempBuf), 0);
            if (result > 0) {
                m_connectorBuffer.insert(m_connectorBuffer.end(), tempBuf, tempBuf + result);
            } else break;
        }

        if (m_connectorBuffer.size() >= sizeof(Packet)) {
            std::memcpy(&packet, m_connectorBuffer.data(), sizeof(Packet));
            m_connectorBuffer.erase(m_connectorBuffer.begin(), m_connectorBuffer.begin() + sizeof(Packet));
            return true;
        }

        if (result == 0) {
            std::cout << "[Network] Server closed connection." << std::endl;
            closesocket(m_tcpSocket);
            m_tcpSocket = INVALID_SOCKET_HANDLE;
        }
    }

    return false;
}

void NetworkManager::Shutdown() {
    if (m_running) {
        std::cout << "[Network] Shutting down connections..." << std::endl;
        if (m_udpSocket != INVALID_SOCKET_HANDLE) {
            closesocket(m_udpSocket);
            m_udpSocket = INVALID_SOCKET_HANDLE;
        }
        if (m_tcpSocket != INVALID_SOCKET_HANDLE) {
            closesocket(m_tcpSocket);
            m_tcpSocket = INVALID_SOCKET_HANDLE;
        }
        for (auto& client : m_clients) {
            closesocket(client.socket);
        }
        m_clients.clear();
        m_hasRemoteAddr = false;
        m_running = false;
    }
}
