#pragma once
#include <vector>
#include <queue>
#include "NetworkManager.hpp"

class InputEngine {
public:
    InputEngine();
    ~InputEngine();

    bool Initialize();
    void Update();
    void Shutdown();

    bool IsAtBoundary(int x, int y);
    bool GetPendingPacket(Packet& pkt);

private:
    bool m_active;
    std::queue<Packet> m_pendingPackets;
#ifdef _WIN32
    void* m_mouseHook;
#endif
};
