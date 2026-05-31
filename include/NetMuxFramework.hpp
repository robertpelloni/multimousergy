#pragma once
#include "NetworkManager.hpp"
#include "InputEngine.hpp"
#include "DriverInterface.hpp"
#include "OverlayEngine.hpp"
#include "SyncModule.hpp"
#include "ClipboardModule.hpp"
#include "ConfigManager.hpp"
#include "AuthService.hpp"
#include "Timer.hpp"
#include <map>
#include <queue>
#include <mutex>

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

    void SetUseD3D11(bool enable) {
        m_overlay.SetBackend(enable ? OverlayBackend::D3D11 : OverlayBackend::GDI);
    }

    void UpdateSessionMetadata(const std::string& name, unsigned int groupId);

    void EnableBenchmarking(bool enable) { m_benchmarking = enable; }
    SyncModule& GetSyncModule() { return m_sync; }
    InputEngine& GetInputEngine() { return m_input; }
    bool IsRunning() const { return m_running; }

private:
    void ProcessOutgoingPackets();
    void ProcessInteractionQueue();
    void ProcessIncomingPackets();
    void PerformMasterStateSync();
    void PerformSyncCheck();
    void PerformLatencySync();
    void PerformDiscoveryBroadcast();
    void PerformClipboardSync();
    void PerformPeerCleanup();

    bool IsPeerTrusted(unsigned long long peerId, NetMuxPacketType type);

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
    unsigned int m_sequenceCounter = 0;

    Timer m_loopTimer;
    Timer m_syncTimer;
    Timer m_renderTimer;
    Timer m_perfTimer;
    double m_lastSyncTime;
    double m_lastPerfLog;
    bool m_overlayDirty;

    struct InteractionEvent {
        unsigned long long peerId;
        int button;
        bool down;
        double timestamp;
        unsigned int groupId;
    };
    std::queue<InteractionEvent> m_interactionQueue;
    std::mutex m_interactionMutex;
    AuthService m_authService;
    std::map<unsigned long long, unsigned int> m_lastSequence;
};
