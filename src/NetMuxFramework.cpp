#include "NetMuxFramework.hpp"
#include <iostream>
#include <algorithm>
#include <thread>
#include <cstring>
#include <fstream>
#include <chrono>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

NetMuxFramework::NetMuxFramework()
    : m_running(false), m_lastSyncTime(0), m_lastPerfLog(0), m_overlayDirty(false) {
    m_localId = (unsigned long long)std::chrono::system_clock::now().time_since_epoch().count();
}

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

        // Handshake: Send initial metadata
        Packet handshake = { m_localId, m_settings.groupId, 0.0, PacketType::Handshake, 0, 0, 0, false, "", 0 };
        strncpy(handshake.payload, m_settings.sessionName.c_str(), sizeof(handshake.payload) - 1);
        handshake.payloadSize = (int)strlen(handshake.payload);
        m_network.SendPacket(handshake);
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
        ProcessInteractionQueue();

        m_sync.Step(dt);

        if (m_loopTimer.ElapsedMilliseconds() - m_lastPerfLog > (m_benchmarking ? 1000.0 : 5000.0)) {
            std::map<unsigned long long, PeerState> peers = m_sync.GetAllPeers();
            double avgLatency = 0;
            double avgE2ELatency = 0;
            if (!peers.empty()) {
                for (auto const& [id, s] : peers) {
                    avgLatency += s.latency;
                    avgE2ELatency += s.e2eLatency;
                }
                avgLatency /= peers.size();
                avgE2ELatency /= peers.size();
            }

            std::cout << "[Perf] Frame: " << dt << "ms | RTT: " << avgLatency << "ms | E2E: " << avgE2ELatency << "ms | Peers: " << peers.size() << std::endl;

            if (m_benchmarking) {
                bool exists = std::ifstream("BENCHMARK_RESULTS.csv").good();
                std::ofstream benchFile("BENCHMARK_RESULTS.csv", std::ios::app);
                if (benchFile.is_open()) {
                    if (!exists) benchFile << "Timestamp(ms),FrameDelta(ms),AvgRTT(ms),AvgE2ELatency(ms),PeerCount\n";
                    benchFile << m_loopTimer.ElapsedMilliseconds() << "," << dt << "," << avgLatency << "," << avgE2ELatency << "," << peers.size() << "\n";
                }
            }

            m_lastPerfLog = m_loopTimer.ElapsedMilliseconds();
        }

        // Ultra-Smooth Rendering: Match monitor refresh or higher
        // For alpha, we maintain ~144Hz (7ms) but ensure interpolation points are fresh.
        if (m_renderTimer.ElapsedMilliseconds() > 7.0) {
            std::map<unsigned long long, RemoteCursorState> overlayPeers;
            auto peers = m_sync.GetAllPeers();
            unsigned long long activeId = m_sync.GetActivePeer();

            for (auto const& [id, peer] : peers) {
                overlayPeers[id] = { (int)peer.x, (int)peer.y, peer.colorR, peer.colorG, peer.colorB, peer.groupId };

                // Active cursor visual indicator (White for owner, original for others)
                if (id == activeId) {
                    overlayPeers[id].r = 255; overlayPeers[id].g = 255; overlayPeers[id].b = 255;
                }
            }
            m_overlay.RenderPeers(overlayPeers);
            m_renderTimer.Reset();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void NetMuxFramework::UpdateSessionMetadata(const std::string& name, unsigned int groupId) {
    m_settings.sessionName = name;
    m_settings.groupId = groupId;

    Packet update = { m_localId, m_settings.groupId, 0.0, PacketType::SessionUpdate, 0, 0, 0, false, "", 0 };
    strncpy(update.payload, m_settings.sessionName.c_str(), sizeof(update.payload) - 1);
    update.payloadSize = (int)m_settings.sessionName.size();
    m_network.SendPacket(update);

    std::cout << "[Framework] Local session updated: " << name << " (Group: " << groupId << ")" << std::endl;
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

void NetMuxFramework::ProcessInteractionQueue() {
    std::lock_guard<std::mutex> lock(m_interactionMutex);
    while (!m_interactionQueue.empty()) {
        auto event = m_interactionQueue.front();
        m_interactionQueue.pop();

        PeerState peer;
        if (m_sync.GetPeerState(event.peerId, peer)) {
            unsigned long long previousOwner = m_sync.GetActivePeer();

            if (m_sync.ResolveConflict(event.peerId, m_loopTimer.ElapsedMilliseconds())) {
                if (!m_driver.SendMouseButton(event.button, event.down)) {
                    m_input.PerformWarpClickRestore((int)peer.x, (int)peer.y, event.button, event.down);
                }

                // Rebroadcast focus update if owner changed
                if (m_settings.isServer && previousOwner != event.peerId) {
                    Packet focusPkt = { m_localId, m_settings.groupId, 0.0, PacketType::FocusUpdate, 0, 0, 0, false, "", 0 };
                    focusPkt.button = (int)event.peerId; // Reuse button field for peer ID
                    m_network.SendPacket(focusPkt);
                }
            } else {
                std::cout << "[Conflict] Peer " << event.peerId << " denied focus ownership." << std::endl;
            }
        }
    }
}

void NetMuxFramework::ProcessOutgoingPackets() {
    Packet outPkt;
    while (m_input.GetPendingPacket(outPkt)) {
        if (m_input.IsCaptured()) {
            outPkt.senderId = m_localId;
            outPkt.groupId = m_settings.groupId;
            outPkt.localTimestamp = m_loopTimer.ElapsedMilliseconds();
            m_network.SendPacket(outPkt);
        }
    }
}

void NetMuxFramework::ProcessIncomingPackets() {
    Packet inPkt;
    while (m_network.ReceivePacket(inPkt)) {
        unsigned long long peerId = inPkt.senderId;

        // Group Filtering: Ignore packets from other groups unless we are the server
        // (Server rebroadcasts everything for global orchestration)
        if (!m_settings.isServer && inPkt.groupId != m_settings.groupId) {
            continue;
        }

        if (inPkt.type == PacketType::Movement) {
            // Deprecated: Movement packets are handled by SyncModule if converted to absolute
            m_driver.SendMouseMovement(inPkt.x, inPkt.y);
        } else if (inPkt.type == PacketType::AbsoluteMovement) {
            // HIGH-PRIORITY REAL-TIME SYNC:
            // Update SyncModule state and trigger hardware movement immediately.
            PeerState oldPeer;
            m_sync.GetPeerState(peerId, oldPeer);

            m_sync.UpdatePeer(peerId, inPkt.groupId, inPkt.x, inPkt.y, inPkt.localTimestamp);

            PeerState newPeer;
            m_sync.GetPeerState(peerId, newPeer);

            long dx = (long)(newPeer.x - oldPeer.x);
            long dy = (long)(newPeer.y - oldPeer.y);

            m_overlayDirty = true;
            m_driver.SendMouseMovement(dx, dy);

            // OPTIMIZED REBROADCAST:
            // In server mode, immediately propagate the absolute position to all peers in the SAME group.
            if (m_settings.isServer) {
                m_network.SendPacketToGroup(inPkt, inPkt.groupId);
            }
        } else if (inPkt.type == PacketType::Click) {
            std::lock_guard<std::mutex> lock(m_interactionMutex);
            m_interactionQueue.push({peerId, inPkt.button, inPkt.down, inPkt.groupId});
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
        } else if (inPkt.type == PacketType::Handshake) {
            std::string remoteHost(inPkt.payload, inPkt.payloadSize);
            std::cout << "[Network] Handshake completed with peer: " << remoteHost << " (ID: " << peerId << ")" << std::endl;

            m_sync.UpdatePeer(peerId, inPkt.groupId, 0, 0, 0, remoteHost.c_str());

            if (m_settings.isServer) {
                // Server replies with its own handshake
                Packet reply = { m_localId, m_settings.groupId, 0.0, PacketType::Handshake, 0, 0, 0, false, "", 0 };
                strncpy(reply.payload, m_settings.sessionName.c_str(), sizeof(reply.payload) - 1);
                reply.payloadSize = (int)strlen(reply.payload);
                m_network.SendPacket(reply);
            }
        } else if (inPkt.type == PacketType::FocusUpdate) {
            unsigned long long newOwner = (unsigned long long)inPkt.button;
            m_sync.SetActivePeer(newOwner);
            std::cout << "[Sync] Focus ownership transferred to peer " << newOwner << std::endl;
        } else if (inPkt.type == PacketType::SessionUpdate) {
            std::string sessionName(inPkt.payload, inPkt.payloadSize);
            m_sync.UpdatePeer(peerId, inPkt.groupId, 0, 0, 0, sessionName.c_str());
            std::cout << "[Sync] Session update from peer " << peerId << ": " << sessionName << " (Group: " << inPkt.groupId << ")" << std::endl;

            if (m_settings.isServer) {
                m_network.SendPacket(inPkt); // Rebroadcast
            }
        }
    }
}

void NetMuxFramework::PerformLatencySync() {
    // Regular RTT sync
    if (m_loopTimer.ElapsedMilliseconds() - m_lastSyncTime > 1000.0) {
        Packet syncPkt = { m_localId, m_settings.groupId, 0.0, PacketType::Sync, 0, 0, 0, false, "", 0 };
        m_network.SendPacket(syncPkt);
        m_syncTimer.Reset();
        m_lastSyncTime = m_loopTimer.ElapsedMilliseconds();
    }

    // High-precision Heartbeat (Clock Sync)
    static double lastHeartbeat = 0;
    if (m_loopTimer.ElapsedMilliseconds() - lastHeartbeat > 100.0) {
        Packet hbPkt = { m_localId, m_settings.groupId, 0.0, PacketType::Heartbeat, 0, 0, 0, false, "", 0 };
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
            Packet pkt = { m_localId, m_settings.groupId, 0.0, PacketType::ClipboardSync, 0, 0, 0, false, "", 0 };
            memcpy(pkt.payload, text.c_str(), text.size());
            pkt.payloadSize = (int)text.size();
            m_network.SendPacket(pkt);
        }
    }
}
