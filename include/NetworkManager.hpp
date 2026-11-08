#pragma once
#include <string>
#include <vector>
#include <map>
#include "CommonTypes.hpp"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET Socket;
#define INVALID_SOCKET_HANDLE INVALID_SOCKET
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
typedef int Socket;
#define INVALID_SOCKET_HANDLE -1
#define SOCKET_ERROR -1
#endif

struct DiscoveryPacket {
    char hostname[64];
    char ip[64];
    int port;
};

enum class ConnectionState {
    Disconnected,
    Connecting,
    Connected,
    Error
};

struct ClientConnection {
    Socket socket;
    std::vector<char> buffer;
};

struct TokenBucketLimiter {
    double tokens = 200.0;
    double maxTokens = 200.0;
    double refillRate = 100.0; // Tokens added per second
    double lastRefill = 0.0;
};

class NetworkManager {
public:
    NetworkManager();
    ~NetworkManager();

    bool StartServer(int port);
    bool Connect(const std::string& address, int port);

    ConnectionState GetTcpState() const { return m_tcpState; }

    void SetIsServer(bool isServer) { m_isServer = isServer; }
    void SendPacket(const Packet& packet);
    void SendPacketToGroup(const Packet& packet, unsigned int groupId);
    bool ReceivePacket(Packet& packet);

    bool BroadcastDiscovery(int port);
    bool ListenForPeers(DiscoveryPacket& pkt);
    bool PollDiscovery(DiscoveryPacket& pkt);

    int GetClientCount() const { return (int)m_clients.size(); }

    // Token-Bucket Rate Limiting
    void ConfigureRateLimiter(double maxTokens, double refillRate);
    bool CheckRateLimit(unsigned long long peerId, double cost, double currentTime);

    void Shutdown();

private:
    bool m_running;
    Socket m_udpSocket;
    Socket m_tcpSocket;
    std::vector<ClientConnection> m_clients; // Per-client state to prevent interleaving
    struct PeerInfo {
        sockaddr_in addr;
        unsigned int groupId;
    };
    std::map<unsigned long long, PeerInfo> m_udpPeerMap; // Multi-client UDP routing
    sockaddr_in m_remoteAddr;
    bool m_isServer;
    bool m_hasRemoteAddr;
    ConnectionState m_tcpState;

    std::vector<char> m_connectorBuffer; // Buffer for the client-side connector socket

    std::map<unsigned long long, TokenBucketLimiter> m_rateLimiters;
    double m_defaultMaxTokens = 200.0;
    double m_defaultRefillRate = 100.0;
};
