#include "NetMuxFramework.hpp"
#include <iostream>
#include <algorithm>
#include <thread>
#include <chrono>

#ifdef _WIN32
#include <windows.h>
#endif

NetMuxFramework::NetMuxFramework()
    : m_running(false), m_lastSyncTime(0), m_overlayDirty(false) {}

NetMuxFramework::~NetMuxFramework() {
    Shutdown();
}

bool NetMuxFramework::Initialize(const AppSettings& settings) {
    m_settings = settings;

    if (m_settings.isServer) {
        if (!m_network.StartServer(m_settings.port)) {
            std::cerr << "Failed to start server on port " << m_settings.port << std::endl;
            return false;
        }
    } else {
        if (!m_network.Connect(m_settings.remoteIp, m_settings.port)) {
            std::cerr << "Failed to connect to " << m_settings.remoteIp << ":" << m_settings.port << std::endl;
            return false;
        }
    }

    if (!m_input.Initialize(m_settings.inputConfig) || !m_overlay.Initialize()) {
        std::cerr << "Failed to initialize input or overlay components." << std::endl;
        return false;
    }

    if (!m_driver.Initialize()) {
        std::cout << "[Warning] Virtual HID Driver not available. Using software fallback." << std::endl;
    }

    // Default color
    m_overlay.SetColor(255, 0, 0);

    m_running = true;
    return true;
}

void NetMuxFramework::Run() {
    while (m_running) {
        m_input.Update();

        PerformLatencySync();
        PerformDiscoveryBroadcast();
        ProcessOutgoingPackets();
        ProcessIncomingPackets();

        // Throttle Overlay Rendering to ~144Hz (approx 7ms)
        if (m_overlayDirty && m_renderTimer.ElapsedMilliseconds() > 7.0) {
            std::map<unsigned long long, RemoteCursorState> overlayPeers;
            for (auto const& [id, peer] : m_peers) {
                overlayPeers[id] = { peer.x, peer.y, peer.colorR, peer.colorG, peer.colorB };
            }
            m_overlay.RenderPeers(overlayPeers);
            m_overlayDirty = false;
            m_renderTimer.Reset();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void NetMuxFramework::Shutdown() {
    if (m_running) {
        m_running = false;
        m_input.Shutdown();
        m_driver.Shutdown();
        m_overlay.Shutdown();
        m_network.Shutdown();
    }
}

void NetMuxFramework::ProcessOutgoingPackets() {
    Packet outPkt;
    while (m_input.GetPendingPacket(outPkt)) {
        if (m_input.IsCaptured()) {
            m_network.SendPacket(outPkt);
        }
    }
}

void NetMuxFramework::ProcessIncomingPackets() {
    Packet inPkt;
    while (m_network.ReceivePacket(inPkt)) {
        // For now, assuming DeviceID is passed in button field or similar for multiple cursors
        // In a real implementation, Packet structure should include SenderID.
        // For alpha multi-cursor demonstration, we treat each session as one peer.
        unsigned long long peerId = 0;

        if (inPkt.type == PacketType::Movement) {
            PeerCursor& peer = m_peers[peerId];
            peer.x += inPkt.x;
            peer.y += inPkt.y;

            // Clamp to screen bounds
#ifdef _WIN32
            peer.x = std::max(0, std::min(peer.x, (int)GetSystemMetrics(SM_CXSCREEN)));
            peer.y = std::max(0, std::min(peer.y, (int)GetSystemMetrics(SM_CYSCREEN)));
#endif

            m_overlayDirty = true;

            // ARCHITECTURE ALIGNMENT:
            // We must use the hardware-level DriverInterface for injection to avoid
            // hijacking the native system cursor.
            if (!m_driver.SendMouseMovement(inPkt.x, inPkt.y)) {
                // Future fallback logic could go here if the driver is unavailable.
            }
        } else if (inPkt.type == PacketType::Click) {
            PeerCursor& peer = m_peers[peerId];

            // Use driver-level hardware injection for clicks first.
            if (!m_driver.SendMouseButton(inPkt.button, inPkt.down)) {
                // FALLBACK: If driver injection is unavailable, we use the "Warp-Click-Restore"
                // cycle as a software-level fallback to ensure basic functionality.
                m_input.PerformWarpClickRestore(peer.x, peer.y, inPkt.button, inPkt.down);
            }
        } else if (inPkt.type == PacketType::Sync) {
            if (m_settings.isServer) {
                m_network.SendPacket(inPkt);
            } else {
                double rtt = m_syncTimer.ElapsedMilliseconds();
                std::cout << "[Latency] RTT: " << rtt << " ms" << std::endl;
            }
        }
    }
}

void NetMuxFramework::PerformLatencySync() {
    if (m_loopTimer.ElapsedMilliseconds() - m_lastSyncTime > 1000.0) {
        Packet syncPkt = { PacketType::Sync, 0, 0, 0, false };
        m_network.SendPacket(syncPkt);
        m_syncTimer.Reset();
        m_lastSyncTime = m_loopTimer.ElapsedMilliseconds();
    }
}

void NetMuxFramework::PerformDiscoveryBroadcast() {
    static double lastBroadcast = 0;
    if (m_settings.isServer && m_loopTimer.ElapsedMilliseconds() - lastBroadcast > 3000.0) {
        m_network.BroadcastDiscovery(5556); // Use dedicated discovery port
        lastBroadcast = m_loopTimer.ElapsedMilliseconds();
    }
}
