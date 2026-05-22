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
};
