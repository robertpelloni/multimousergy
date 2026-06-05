#pragma once
#include <vector>
#include <queue>
#include "CommonTypes.hpp"

class InputEngine {
public:
    InputEngine();
    ~InputEngine();

#ifdef __linux__
    bool Initialize(const Config& config, void* xDisplay = nullptr);
#else
    bool Initialize(const Config& config);
#endif
    void Update();
    void Shutdown();

    // Cross-Platform Input Capture Interface
    bool IsAtBoundary(int x, int y);
    bool GetPendingPacket(Packet& pkt);
    void PerformWarpClickRestore(int targetX, int targetY, int button, bool down);
    bool IsLocalUserActive() const;
    double GetLastLocalActivity() const;

    bool IsCaptured() const;
    void SetPeerConnected(bool connected);

    bool m_active;
    bool m_isCaptured;
    bool m_isSelecting;
    bool m_peerConnected = false;
    int m_selStartX;
    int m_selStartY;
    long m_accumulatedX;
    int m_virtualX;
    int m_virtualY;
    double m_lastLocalActivity;
    Config m_config;
    std::queue<Packet> m_pendingPackets;
#ifdef _WIN32
    void* m_mouseHook;
    void* m_keyboardHook;
    void* m_hwnd;
#endif
#ifdef __linux__
    std::vector<int> m_fds;
    void* m_xDisplay = nullptr;
#endif

private:
};
