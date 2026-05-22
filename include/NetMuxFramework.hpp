#pragma once
#include "NetworkManager.hpp"
#include "InputEngine.hpp"
#include "DriverInterface.hpp"
#include "OverlayEngine.hpp"
#include "ConfigManager.hpp"
#include "Timer.hpp"

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

    NetworkManager m_network;
    InputEngine m_input;
    DriverInterface m_driver;
    OverlayEngine m_overlay;

    AppSettings m_settings;
    bool m_running;
    int m_remoteX;
    int m_remoteY;

    Timer m_loopTimer;
    Timer m_syncTimer;
    double m_lastSyncTime;
};
