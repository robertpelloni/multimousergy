#pragma once
#include <string>
#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <netinet/in.h>
#endif

enum class PacketType {
    Movement,
    AbsoluteMovement,
    Click,
    Sync,
    Discovery
};

struct Packet {
    unsigned long long senderId;
    PacketType type;
    int x;
    int y;
    int button;
    bool down;
};

struct DiscoveryPacket {
    char hostname[64];
    int port;
};

class NetworkManager {
public:
    NetworkManager();
    ~NetworkManager();

    bool StartServer(int port);
    bool Connect(const std::string& address, int port);

    void SendPacket(const Packet& packet);
    bool ReceivePacket(Packet& packet);

    bool BroadcastDiscovery(int port);
    bool ListenForPeers(DiscoveryPacket& pkt);
    bool PollDiscovery(DiscoveryPacket& pkt);

    void Shutdown();

private:
    bool m_running;
    unsigned long long m_udpSocket; // Using unsigned long long to accommodate SOCKET on 64-bit Windows
    unsigned long long m_tcpSocket;
    std::vector<unsigned long long> m_clientTcpSockets;
    sockaddr_in m_remoteAddr;
    bool m_hasRemoteAddr;

    std::vector<char> m_tcpBuffer;
};
