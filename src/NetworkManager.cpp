#include "NetworkManager.hpp"
#include <iostream>
#include <cstring>
#include <algorithm>

#ifdef _WIN32
    #pragma comment(lib, "ws2_32.lib")
#else
    #define closesocket close
#endif

static void SetNonBlocking(Socket s) {
#ifdef _WIN32
    unsigned long mode = 1;
    ioctlsocket(s, FIONBIO, &mode);
#else
    fcntl(s, F_SETFL, O_NONBLOCK);
#endif
}

NetworkManager::NetworkManager() : m_running(false), m_udpSocket(INVALID_SOCKET_HANDLE), m_tcpSocket(INVALID_SOCKET_HANDLE), m_hasRemoteAddr(false), m_isServer(false) {
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
    SetNonBlocking(m_udpSocket);

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (bind(m_udpSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "[Network] UDP Bind failed on port " << port << " Error: " << WSAGetLastError() << std::endl;
        return false;
    }

    m_tcpSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_tcpSocket == INVALID_SOCKET_HANDLE) return false;
    SetNonBlocking(m_tcpSocket);

    int reuse = 1;
    setsockopt(m_tcpSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse));

    if (bind(m_tcpSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "[Network] TCP Bind failed on port " << port << " Error: " << WSAGetLastError() << std::endl;
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
    SetNonBlocking(m_udpSocket);

    m_tcpSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_tcpSocket == INVALID_SOCKET_HANDLE) return false;

    if (connect(m_tcpSocket, (struct sockaddr*)&m_remoteAddr, sizeof(m_remoteAddr)) == SOCKET_ERROR) {
#ifdef _WIN32
        if (WSAGetLastError() != WSAEWOULDBLOCK) return false;
#else
        if (errno != EINPROGRESS) return false;
#endif
    }
    SetNonBlocking(m_tcpSocket);

    m_running = true;
    return true;
}

static int SafeSend(Socket s, const char* data, int len) {
    int sent = 0;
    while (sent < len) {
        int result = send(s, data + sent, len - sent, 0);
        if (result == SOCKET_ERROR) {
#ifdef _WIN32
            if (WSAGetLastError() == WSAEWOULDBLOCK) continue;
#else
            if (errno == EAGAIN || errno == EWOULDBLOCK) continue;
#endif
            return SOCKET_ERROR;
        }
        sent += result;
    }
    return sent;
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

    if (packet.type == NetMuxPacketType::Movement || packet.type == NetMuxPacketType::AbsoluteMovement) {
        if (m_isServer) {
            for (auto const& [id, peer] : m_udpPeerMap) {
                sendto(m_udpSocket, (const char*)&packet, sizeof(packet), 0, (struct sockaddr*)&peer.addr, sizeof(sockaddr_in));
            }
        } else if (m_hasRemoteAddr) {
            sendto(m_udpSocket, (const char*)&packet, sizeof(packet), 0, (struct sockaddr*)&m_remoteAddr, sizeof(sockaddr_in));
        }
    } else {
        if (m_isServer) {
            for (auto& client : m_clients) {
                SafeSend(client.socket, (const char*)&packet, sizeof(packet));
            }
        } else if (m_tcpSocket != INVALID_SOCKET_HANDLE) {
            SafeSend(m_tcpSocket, (const char*)&packet, sizeof(packet));
        }
    }
}

bool NetworkManager::BroadcastDiscovery(int port) {
    Socket s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s == INVALID_SOCKET_HANDLE) return false;

#ifdef _WIN32
    BOOL broadcast = TRUE;
    setsockopt(s, SOL_SOCKET, SO_BROADCAST, (const char*)&broadcast, sizeof(broadcast));
#else
    int broadcast = 1;
    setsockopt(s, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
#endif

    sockaddr_in broadcastAddr;
    broadcastAddr.sin_family = AF_INET;
    broadcastAddr.sin_port = htons(port);
    broadcastAddr.sin_addr.s_addr = INADDR_BROADCAST;

    DiscoveryPacket pkt = {0};
    gethostname(pkt.hostname, sizeof(pkt.hostname) - 1);
    pkt.port = 5555; // Core port

    sendto(s, (const char*)&pkt, sizeof(pkt), 0, (struct sockaddr*)&broadcastAddr, sizeof(broadcastAddr));
    closesocket(s);
    return true;
}

bool NetworkManager::ListenForPeers(DiscoveryPacket& pkt) {
    return PollDiscovery(pkt);
}

bool NetworkManager::PollDiscovery(DiscoveryPacket& pkt) {
    static Socket s_disc = INVALID_SOCKET_HANDLE;
    if (s_disc == INVALID_SOCKET_HANDLE) {
        s_disc = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(5556);
        bind(s_disc, (struct sockaddr*)&addr, sizeof(addr));
        SetNonBlocking(s_disc);
    }

    sockaddr_in fromAddr;
    socklen_t fromLen = sizeof(fromAddr);
    int result = recvfrom(s_disc, (char*)&pkt, sizeof(pkt), 0, (struct sockaddr*)&fromAddr, &fromLen);

    if (result >= (int)sizeof(pkt)) {
        pkt.hostname[sizeof(pkt.hostname) - 1] = '\0';
        return true;
    }
    return false;
}

bool NetworkManager::ReceivePacket(Packet& packet) {
    if (!m_running) return false;

    if (m_isServer && m_tcpSocket != INVALID_SOCKET_HANDLE) {
        sockaddr_in clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);
        Socket newClient = accept(m_tcpSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);
        if (newClient != INVALID_SOCKET_HANDLE) {
            SetNonBlocking(newClient);
            m_clients.push_back({newClient, {}});
            std::cout << "[Network] Client connected from " << inet_ntoa(clientAddr.sin_addr) << std::endl;
        }
    }

    sockaddr_in fromAddr;
    socklen_t fromLen = sizeof(fromAddr);
    int result = recvfrom(m_udpSocket, (char*)&packet, sizeof(packet), 0, (struct sockaddr*)&fromAddr, &fromLen);
    if (result >= (int)sizeof(Packet)) {
        if (m_isServer) m_udpPeerMap[packet.senderId] = { fromAddr, packet.groupId };
        if (!m_hasRemoteAddr) { m_remoteAddr = fromAddr; m_hasRemoteAddr = true; }
        return true;
    }

    for (auto it = m_clients.begin(); it != m_clients.end(); ) {
        char tempBuf[sizeof(Packet) * 4];
        result = recv(it->socket, tempBuf, sizeof(tempBuf), 0);
        if (result > 0) it->buffer.insert(it->buffer.end(), tempBuf, tempBuf + result);

        if (it->buffer.size() >= sizeof(Packet)) {
            std::memcpy(&packet, it->buffer.data(), sizeof(Packet));
            it->buffer.erase(it->buffer.begin(), it->buffer.begin() + sizeof(Packet));
            return true;
        }

        if (result == 0) {
            std::cout << "[Network] Peer disconnected." << std::endl;
            closesocket(it->socket);
            it = m_clients.erase(it);
        } else ++it;
    }

    if (!m_isServer && m_tcpSocket != INVALID_SOCKET_HANDLE) {
        char tempBuf[sizeof(Packet) * 4];
        result = recv(m_tcpSocket, tempBuf, sizeof(tempBuf), 0);
        if (result > 0) m_connectorBuffer.insert(m_connectorBuffer.end(), tempBuf, tempBuf + result);

        if (m_connectorBuffer.size() >= sizeof(Packet)) {
            std::memcpy(&packet, m_connectorBuffer.data(), sizeof(Packet));
            m_connectorBuffer.erase(m_connectorBuffer.begin(), m_connectorBuffer.begin() + sizeof(Packet));
            return true;
        }

        if (result == 0) {
            std::cout << "[Network] Connection lost." << std::endl;
            closesocket(m_tcpSocket);
            m_tcpSocket = INVALID_SOCKET_HANDLE;
        }
    }

    return false;
}

void NetworkManager::Shutdown() {
    if (m_running) {
        m_running = false;
        if (m_udpSocket != INVALID_SOCKET_HANDLE) closesocket(m_udpSocket);
        if (m_tcpSocket != INVALID_SOCKET_HANDLE) closesocket(m_tcpSocket);
        for (auto& client : m_clients) closesocket(client.socket);
        m_clients.clear();
        m_udpSocket = INVALID_SOCKET_HANDLE;
        m_tcpSocket = INVALID_SOCKET_HANDLE;
    }
}
