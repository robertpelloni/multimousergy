#include "NetMuxFramework.hpp"
#include <iostream>
#include <algorithm>
#include <thread>
#include <cstring>
#include <chrono>

#ifdef _WIN32
#include <windows.h>
#endif

NetMuxFramework::NetMuxFramework()
    : m_running(false), m_lastSyncTime(0), m_lastPerfLog(0), m_overlayDirty(false) {}

NetMuxFramework::~NetMuxFramework() {
    Shutdown();
}

bool NetMuxFramework::Initialize(const AppSettings& settings) {
    m_settings = settings;
    m_network.SetIsServer(m_settings.isServer);

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
    Timer frameTimer;
    while (m_running) {
        double dt = frameTimer.ElapsedMilliseconds();
        frameTimer.Reset();

        m_input.Update();

        PerformLatencySync();
        PerformDiscoveryBroadcast();
        PerformClipboardSync();
        ProcessOutgoingPackets();
        ProcessIncomingPackets();

        m_sync.Step(dt);

        if (m_loopTimer.ElapsedMilliseconds() - m_lastPerfLog > 5000.0) {
            std::cout << "[Perf] Frame Delta: " << dt << " ms" << std::endl;
            m_lastPerfLog = m_loopTimer.ElapsedMilliseconds();
        }

        // Ultra-Smooth Rendering: Match monitor refresh or higher
        // For alpha, we maintain ~144Hz (7ms) but ensure interpolation points are fresh.
        if (m_renderTimer.ElapsedMilliseconds() > 7.0) {
            std::map<unsigned long long, RemoteCursorState> overlayPeers;
            auto peers = m_sync.GetAllPeers();
            for (auto const& [id, peer] : peers) {
                overlayPeers[id] = { peer.x, peer.y, peer.colorR, peer.colorG, peer.colorB };
            }
            m_overlay.RenderPeers(overlayPeers);
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
            outPkt.senderId = 0; // Local sender ID is 0 for alpha
            m_network.SendPacket(outPkt);
        }
    }
}

void NetMuxFramework::ProcessIncomingPackets() {
    Packet inPkt;
    while (m_network.ReceivePacket(inPkt)) {
        unsigned long long peerId = inPkt.senderId;

        if (inPkt.type == PacketType::Movement) {
            // Deprecated: Movement packets are handled by SyncModule if converted to absolute
            m_driver.SendMouseMovement(inPkt.x, inPkt.y);
        } else if (inPkt.type == PacketType::AbsoluteMovement) {
            // HIGH-PRIORITY REAL-TIME SYNC:
            // Update SyncModule state and trigger hardware movement immediately.
            PeerState oldPeer;
            m_sync.GetPeerState(peerId, oldPeer);

            m_sync.UpdatePeer(peerId, inPkt.x, inPkt.y);

            PeerState newPeer;
            m_sync.GetPeerState(peerId, newPeer);

            long dx = newPeer.x - oldPeer.x;
            long dy = newPeer.y - oldPeer.y;

            m_overlayDirty = true;
            m_driver.SendMouseMovement(dx, dy);

            // OPTIMIZED REBROADCAST:
            // In server mode, immediately propagate the absolute position to all other peers.
            if (m_settings.isServer) {
                m_network.SendPacket(inPkt);
            }
        } else if (inPkt.type == PacketType::Click) {
            PeerState peer;
            if (m_sync.GetPeerState(peerId, peer)) {
                // Ensure peer is allowed to click (e.g. they are the active peer or everyone is allowed)
                // For Alpha, the first one to click becomes 'active' if no one else is active.
                if (m_sync.GetActivePeer() == 0) m_sync.SetActivePeer(peerId);

                if (m_sync.GetActivePeer() == peerId) {
                    if (!m_driver.SendMouseButton(inPkt.button, inPkt.down)) {
                        m_input.PerformWarpClickRestore(peer.x, peer.y, inPkt.button, inPkt.down);
                    }
                }

                if (!inPkt.down) {
                    // Release active lock if no buttons are down (optional/configurable)
                    // m_sync.SetActivePeer(0);
                }
            }
        } else if (inPkt.type == PacketType::Sync) {
            if (m_settings.isServer) {
                m_network.SendPacket(inPkt);
            } else {
                double rtt = m_syncTimer.ElapsedMilliseconds();
                m_sync.UpdateLatency(peerId, rtt);
                std::cout << "[Latency] RTT: " << rtt << " ms" << std::endl;
            }
        } else if (inPkt.type == PacketType::Heartbeat) {
            if (m_settings.isServer) {
                // Rebroadcast heartbeat to all clients for clock sync
                m_network.SendPacket(inPkt);
            }
        } else if (inPkt.type == PacketType::ClipboardSync) {
            std::string text(inPkt.payload, inPkt.payloadSize);
            m_clipboard.SetText(text);

            if (m_settings.isServer) {
                m_network.SendPacket(inPkt);
            }
        }
    }
}

void NetMuxFramework::PerformLatencySync() {
    // Regular RTT sync
    if (m_loopTimer.ElapsedMilliseconds() - m_lastSyncTime > 1000.0) {
        Packet syncPkt = { 0, PacketType::Sync, 0, 0, 0, false };
        m_network.SendPacket(syncPkt);
        m_syncTimer.Reset();
        m_lastSyncTime = m_loopTimer.ElapsedMilliseconds();
    }

    // High-precision Heartbeat (Clock Sync)
    static double lastHeartbeat = 0;
    if (m_loopTimer.ElapsedMilliseconds() - lastHeartbeat > 100.0) {
        Packet hbPkt = { 0, PacketType::Heartbeat, 0, 0, 0, false };
        // We pack current local timestamp into coordinates for precision
        unsigned int now = (unsigned int)m_loopTimer.ElapsedMilliseconds();
        hbPkt.x = (int)(now & 0xFFFF);
        hbPkt.y = (int)((now >> 16) & 0xFFFF);
        m_network.SendPacket(hbPkt);
        lastHeartbeat = m_loopTimer.ElapsedMilliseconds();
    }
}

void NetMuxFramework::PerformDiscoveryBroadcast() {
    static double lastBroadcast = 0;
    if (m_settings.isServer && m_loopTimer.ElapsedMilliseconds() - lastBroadcast > 3000.0) {
        m_network.BroadcastDiscovery(5556); // Use dedicated discovery port
        lastBroadcast = m_loopTimer.ElapsedMilliseconds();
    }
}

void NetMuxFramework::PerformClipboardSync() {
    if (m_clipboard.HasChanged()) {
        std::string text = m_clipboard.GetText();
        if (text.size() < 1024) {
            Packet pkt = { 0, PacketType::ClipboardSync, 0, 0, 0, false };
            memcpy(pkt.payload, text.c_str(), text.size());
            pkt.payloadSize = (int)text.size();
            m_network.SendPacket(pkt);
        }
    }
}
