#pragma once
#include "NetworkManager.hpp"
#include "InputEngine.hpp"
#include "DriverInterface.hpp"
#include "OverlayEngine.hpp"
#include "SyncModule.hpp"
#include "ConfigManager.hpp"
#include "Timer.hpp"
#include <map>

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
    SyncModule m_sync;

    AppSettings m_settings;
    bool m_running;

    Timer m_loopTimer;
    Timer m_syncTimer;
    Timer m_renderTimer;
    Timer m_perfTimer;
    double m_lastSyncTime;
    double m_lastPerfLog;
    bool m_overlayDirty;
};
