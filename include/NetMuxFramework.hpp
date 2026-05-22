#pragma once
#include "NetworkManager.hpp"
#include "InputEngine.hpp"
#include "DriverInterface.hpp"
#include "OverlayEngine.hpp"
#include "ConfigManager.hpp"
#include "Timer.hpp"
#include <map>

struct PeerCursor {
    int x;
    int y;
    unsigned char colorR;
    unsigned char colorG;
    unsigned char colorB;
};

class NetMuxFramework {
public:
    NetMuxFramework();
    ~NetMuxFramework();

    bool Initialize(const AppSettings& settings);
    void Run();
    void Shutdown();

private:
    void ProcessOutgoingPackets();
    void ProcessIncomingPackets();
    void PerformLatencySync();
    void PerformDiscoveryBroadcast();

    NetworkManager m_network;
    InputEngine m_input;
    DriverInterface m_driver;
    OverlayEngine m_overlay;

    AppSettings m_settings;
    bool m_running;

    std::map<unsigned long long, PeerCursor> m_peers;

    Timer m_loopTimer;
    Timer m_syncTimer;
    Timer m_renderTimer;
    double m_lastSyncTime;
    bool m_overlayDirty;
};
