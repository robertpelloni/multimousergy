#pragma once
#include <vector>
#include <queue>
#include "NetworkManager.hpp"

struct Config {
    int boundaryX; // Screen coordinate where crossing happens
    int boundaryY;
    bool isLeft;   // Crossing to the left or right
};

class InputEngine {
public:
    InputEngine();
    ~InputEngine();

    bool Initialize(const Config& config);
    void Update();
    void Shutdown();

    bool IsAtBoundary(int x, int y);
    bool GetPendingPacket(Packet& pkt);
    void PerformWarpClickRestore(int targetX, int targetY, int button, bool down);

private:
    bool m_active;
    Config m_config;
    std::queue<Packet> m_pendingPackets;
#ifdef _WIN32
    void* m_mouseHook;
    void* m_hwnd;
#endif
};
