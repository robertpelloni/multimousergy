#pragma once
#include "NetworkManager.hpp"
#include "InputEngine.hpp"
#include "DriverInterface.hpp"
#include "OverlayEngine.hpp"
#include "SyncModule.hpp"
#include "ClipboardModule.hpp"
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

    void SetCursorColor(unsigned char r, unsigned char g, unsigned char b) {
        m_overlay.SetColor(r, g, b);
    }

    void SetCursorScale(float scale) {
        m_overlay.SetScale(scale);
    }

    void EnableBenchmarking(bool enable) { m_benchmarking = enable; }

private:
    void ProcessOutgoingPackets();
    void ProcessIncomingPackets();
    void PerformLatencySync();
    void PerformDiscoveryBroadcast();
    void PerformClipboardSync();

    NetworkManager m_network;
    InputEngine m_input;
    DriverInterface m_driver;
    OverlayEngine m_overlay;
    SyncModule m_sync;
    ClipboardModule m_clipboard;

    AppSettings m_settings;
    bool m_running;
    bool m_benchmarking = false;
    unsigned long long m_localId;

    Timer m_loopTimer;
    Timer m_syncTimer;
    Timer m_renderTimer;
    Timer m_perfTimer;
    double m_lastSyncTime;
    double m_lastPerfLog;
    bool m_overlayDirty;
};
