#pragma once
#include <string>

enum class PacketType {
    Movement,
    Click,
    Sync
};

struct Packet {
    PacketType type;
    int x;
    int y;
    int button;
    bool down;
};

class NetworkManager {
public:
    NetworkManager();
    ~NetworkManager();

    bool StartServer(int port);
    bool Connect(const std::string& address, int port);

    void SendPacket(const Packet& packet);
    bool ReceivePacket(Packet& packet);

    void Shutdown();

private:
    bool m_running;
    unsigned long long m_udpSocket; // Using unsigned long long to accommodate SOCKET on 64-bit Windows
    unsigned long long m_tcpSocket;
    unsigned long long m_clientTcpSocket;
    void* m_remoteAddr; // sockaddr_in storage
};
