#pragma once
#include <string>
#include <vector>
#include <map>

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
    Discovery,
    Heartbeat,
    ClipboardSync,
    Handshake
};

struct Packet {
    unsigned long long senderId;
    PacketType type;
    int x;
    int y;
    int button;
    bool down;
    char payload[1024]; // Dynamic data payload (e.g. clipboard text)
    int payloadSize;
};

struct DiscoveryPacket {
    char hostname[64];
    int port;
};

struct AppSettings;

struct ClientConnection {
    unsigned long long socket;
    std::vector<char> buffer;
};

class NetworkManager {
public:
    NetworkManager();
    ~NetworkManager();

    bool StartServer(int port);
    bool Connect(const std::string& address, int port);

    void SetIsServer(bool isServer) { m_isServer = isServer; }
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
    std::vector<ClientConnection> m_clients; // Per-client state to prevent interleaving
    std::map<unsigned long long, sockaddr_in> m_udpPeerMap; // Multi-client UDP routing
    sockaddr_in m_remoteAddr;
    bool m_isServer;
    bool m_hasRemoteAddr;

    std::vector<char> m_connectorBuffer; // Buffer for the client-side connector socket
};
