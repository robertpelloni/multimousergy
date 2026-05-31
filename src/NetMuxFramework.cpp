#include "NetMuxFramework.hpp"
#include "ConfigGUI.hpp"
#include <iostream>
#include <cmath>
#include <algorithm>
#include <thread>
#include <cstring>
#include <fstream>
#include <chrono>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <bcrypt.h>
#else
#include <unistd.h>
#include <openssl/sha.h>
#endif

// Helper to compute simple SHA256-like hash for auth
static void ComputeHash(int nonce, const std::string& key, unsigned char* outHash) {
    std::string data = std::to_string(nonce) + key;
#ifdef _WIN32
    BCRYPT_ALG_HANDLE hAlg = NULL;
    BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM, NULL, 0);
    // BCryptHash stubbed
    BCryptCloseAlgorithmProvider(hAlg, 0);
#else
    SHA256((const unsigned char*)data.c_str(), data.length(), outHash);
#endif
}

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

        // Handshake: Initial connection. Security key will be challenged by server.
        Packet handshake = { m_localId, m_settings.groupId, 0.0, NetMuxPacketType::Handshake, 0, 0, 0, false, false, 0, 0, "", 0 };
        std::string meta = m_settings.sessionName + "|" + m_settings.groupName;
        strncpy(handshake.payload, meta.c_str(), sizeof(handshake.payload) - 1);
        handshake.payloadSize = (int)meta.size();
        m_network.SendPacket(handshake);
    }

    if (!m_overlay.Initialize()) {
        std::cerr << "Failed to initialize overlay components." << std::endl;
        return false;
    }

    if (!m_driver.Initialize(m_settings.driverType)) {
        std::cout << "[Warning] Virtual HID Driver not available. Using software fallback." << std::endl;
    }

    // Broadcast initial resolution
    int sw = 1920;
    int sh = 1080;
#ifdef _WIN32
    sw = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    sh = GetSystemMetrics(SM_CYVIRTUALSCREEN);
#endif
    Packet resPkt = { m_localId, m_settings.groupId, 0.0, NetMuxPacketType::ResolutionUpdate, sw, sh, 0, false, false, 0, 0, "", 0 };
    m_network.SendPacket(resPkt);

    // Default color
    m_overlay.SetColor(255, 0, 0);

    m_running = true;
    return true;
}

void NetMuxFramework::Run() {
    Timer frameTimer;
    const int MAX_PEER_COUNT = 20;
    bool warningLogged = false;

    while (m_running) {
        double dt = frameTimer.ElapsedMilliseconds();
        frameTimer.Reset();

        PerformLatencySync();
        PerformDiscoveryBroadcast();
        PerformClipboardSync();
        PerformMasterStateSync();
    PerformSyncCheck();
        ProcessOutgoingPackets();
        ProcessIncomingPackets();
        ProcessInteractionQueue();

        m_sync.Step(dt);

        // Update input capture permission based on connectivity
        if (m_settings.isServer) {
            m_input.SetPeerConnected(m_network.GetClientCount() > 0);
        } else {
            // Client is "connected" if we have any peers being tracked by sync (the server)
            m_input.SetPeerConnected(!m_sync.GetAllPeers().empty());
        }

        // UI TICK: Keep the ConfigGUI updated during active sessions
        static double lastUIUpdate = 0;
        if (m_loopTimer.ElapsedMilliseconds() - lastUIUpdate > 100.0) {
            std::map<unsigned long long, PeerState> peers = m_sync.GetAllPeers();
            ConfigGUI::UpdateCursorMonitor(peers);
            lastUIUpdate = m_loopTimer.ElapsedMilliseconds();
        }

        // Load Balancing / Connection Management
        if (m_settings.isServer) {
            int currentClients = m_network.GetClientCount();
            if (currentClients > MAX_PEER_COUNT && !warningLogged) {
                std::cerr << "[Warning] Connection threshold exceeded (" << currentClients << "/" << MAX_PEER_COUNT << "). Performance may degrade." << std::endl;
                warningLogged = true;
            } else if (currentClients <= MAX_PEER_COUNT) {
                warningLogged = false;
            }
        }

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
                std::ofstream benchFile("BENCHMARK_RESULTS.csv", std::ios::app);
                if (benchFile.is_open()) {
                    benchFile << m_loopTimer.ElapsedMilliseconds() << "," << dt << "," << avgLatency << "," << avgE2ELatency << "," << peers.size() << "\n";
                }
            }

            m_lastPerfLog = m_loopTimer.ElapsedMilliseconds();
        }

        // Ultra-Smooth Rendering: Match monitor refresh or higher
        if (m_renderTimer.ElapsedMilliseconds() > 7.0) {
            std::map<unsigned long long, RemoteCursorState> overlayPeers;
            auto peers = m_sync.GetAllPeers();
            unsigned long long activeId = m_sync.GetActivePeer();

            for (auto const& [id, peer] : peers) {
                overlayPeers[id] = { (int)peer.x, (int)peer.y, peer.colorR, peer.colorG, peer.colorB, peer.groupId, peer.isSelecting, peer.isConflictBlocked, (int)peer.selStartX, (int)peer.selStartY };

                // Active cursor visual indicator (White for owner, original for others)
                if (id == activeId) {
                    overlayPeers[id].r = 255; overlayPeers[id].g = 255; overlayPeers[id].b = 255;
                } else if (peer.drift > 327) {
                    // Visual indicator for "Out of Sync" or "Conflict" potential
                    overlayPeers[id].r = 255; overlayPeers[id].g = 165; overlayPeers[id].b = 0; // Orange
                }
            }
            m_overlay.SetActivePeer(activeId);
            m_overlay.RenderPeers(overlayPeers);
            ConfigGUI::UpdateCursorMonitor(peers);
            m_renderTimer.Reset();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void NetMuxFramework::UpdateSessionMetadata(const std::string& name, unsigned int groupId) {
    m_settings.sessionName = name;
    m_settings.groupId = groupId;

    Packet update = { m_localId, m_settings.groupId, 0.0, NetMuxPacketType::SessionUpdate, 0, 0, 0, false, false, 0, 0, "", 0 };
    std::string meta = m_settings.sessionName + "|" + m_settings.groupName;
    strncpy(update.payload, meta.c_str(), sizeof(update.payload) - 1);
    update.payloadSize = (int)meta.size();
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

            if (m_sync.ResolveInteraction(event.peerId, event.timestamp, !event.down)) {
                if (!m_driver.SendMouseButton(event.button, event.down)) {
                    m_input.PerformWarpClickRestore((int)peer.x, (int)peer.y, event.button, event.down);
                }

                // Rebroadcast focus update if owner changed
                if (m_settings.isServer && previousOwner != event.peerId) {
                    Packet focusPkt = { m_localId, m_settings.groupId, 0.0, NetMuxPacketType::FocusUpdate, 0, 0, 0, false, false, 0, 0, "", 0 };
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
            
            // Sync local state for rendering on our own overlay
            if (outPkt.type == NetMuxPacketType::AbsoluteMovement || outPkt.type == NetMuxPacketType::SelectionUpdate) {
                m_sync.UpdateLocalState(m_localId, m_settings.groupId, outPkt.x, outPkt.y, outPkt.isSelecting, outPkt.selectionStartX, outPkt.selectionStartY);
            }

            m_network.SendPacket(outPkt);
        }
    }
}

void NetMuxFramework::ProcessIncomingPackets() {
    Packet inPkt;
    while (m_network.ReceivePacket(inPkt)) {
        unsigned long long peerId = inPkt.senderId;

        if (!m_settings.isServer && inPkt.groupId != m_settings.groupId) {
            continue;
        }

        if (inPkt.type == NetMuxPacketType::Movement) {
            PeerState peer;
            if (m_sync.GetPeerState(peerId, peer) && (peer.isAuthenticated || m_settings.securityKey.empty())) {
                m_driver.SendMouseMovement(inPkt.x, inPkt.y);
            }
        } else if (inPkt.type == NetMuxPacketType::AbsoluteMovement) {
            PeerState peer;
            bool auth = false;
            if (m_sync.GetPeerState(peerId, peer)) auth = peer.isAuthenticated;
            if (!auth && !m_settings.securityKey.empty()) continue;

            PeerState oldPeer;
            m_sync.GetPeerState(peerId, oldPeer);

            int localScreenWidth = 1920;
            int localScreenHeight = 1080;
#ifdef _WIN32
            localScreenWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
            localScreenHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);
#endif
            float newX = (float)SyncModule::Denormalize(inPkt.x, localScreenWidth);
            float newY = (float)SyncModule::Denormalize(inPkt.y, localScreenHeight);

            long dx = (long)(newX - oldPeer.x);
            long dy = (long)(newY - oldPeer.y);

            m_sync.UpdatePeer(peerId, inPkt.groupId, inPkt.x, inPkt.y, inPkt.localTimestamp);
            m_overlayDirty = true;
            m_driver.SendMouseMovement(dx, dy);

            if (m_settings.isServer) {
                m_network.SendPacketToGroup(inPkt, inPkt.groupId);
            }
        } else if (inPkt.type == NetMuxPacketType::Click) {
            PeerState peer;
            if (m_sync.GetPeerState(peerId, peer) && (peer.isAuthenticated || m_settings.securityKey.empty())) {
                m_sync.UpdatePeerButtons(peerId, inPkt.button, inPkt.down);
                std::lock_guard<std::mutex> lock(m_interactionMutex);
                m_interactionQueue.push({peerId, inPkt.button, inPkt.down, inPkt.localTimestamp, inPkt.groupId});
            }
        } else if (inPkt.type == NetMuxPacketType::Sync) {
            if (m_settings.isServer) {
                m_network.SendPacket(inPkt);
            } else {
                double rtt = m_syncTimer.ElapsedMilliseconds();
                m_sync.UpdateLatency(peerId, rtt);
            }
        } else if (inPkt.type == NetMuxPacketType::Heartbeat) {
            if (m_settings.isServer) {
                m_network.SendPacket(inPkt);
            } else {
                unsigned int remoteLow = (unsigned int)inPkt.x;
                unsigned int remoteHigh = (unsigned int)inPkt.y;
                double remoteTime = (double)(remoteLow | (remoteHigh << 16));
                m_sync.UpdateClockOffset(peerId, remoteTime, m_loopTimer.ElapsedMilliseconds());
            }
        } else if (inPkt.type == NetMuxPacketType::ClipboardSync) {
            PeerState peer;
            if (m_sync.GetPeerState(peerId, peer) && (peer.isAuthenticated || m_settings.securityKey.empty())) {
                int size = std::min(inPkt.payloadSize, (int)sizeof(inPkt.payload));
                std::string text(inPkt.payload, size);
                m_clipboard.SetText(text);

                if (m_settings.isServer) {
                    m_network.SendPacket(inPkt);
                }
            }
        } else if (inPkt.type == NetMuxPacketType::Handshake) {
            int size = std::min(inPkt.payloadSize, (int)sizeof(inPkt.payload));
            std::string meta(inPkt.payload, size);
            size_t sep = meta.find('|');
            std::string remoteName = (sep != std::string::npos) ? meta.substr(0, sep) : meta;
            std::string remoteGroupName = (sep != std::string::npos) ? meta.substr(sep + 1) : "";

            std::cout << "[Network] Handshake from peer: " << remoteName << " (Group: " << remoteGroupName << ")" << std::endl;

            m_sync.UpdatePeer(peerId, inPkt.groupId, 0, 0, 0, remoteName.c_str(), remoteGroupName.c_str());

            if (m_settings.isServer) {
                Packet challenge = { m_localId, m_settings.groupId, 0.0, NetMuxPacketType::AuthChallenge, 0, 0, 0, false, false, 0, 0, "", 0 };
                int nonce = rand();
                challenge.x = nonce;
                m_pendingNonces[peerId] = nonce;
                m_network.SendPacket(challenge);

                Packet reply = { m_localId, m_settings.groupId, 0.0, NetMuxPacketType::Handshake, 0, 0, 0, false, false, 0, 0, "", 0 };
                std::string sMeta = m_settings.sessionName + "|" + m_settings.groupName;
                strncpy(reply.payload, sMeta.c_str(), sizeof(reply.payload) - 1);
                reply.payloadSize = (int)sMeta.size();
                m_network.SendPacket(reply);
            }
        } else if (inPkt.type == NetMuxPacketType::AuthChallenge) {
            int nonce = inPkt.x;
            unsigned char hash[32];
            ComputeHash(nonce, m_settings.securityKey, hash);

            Packet authPkt = { m_localId, m_settings.groupId, 0.0, NetMuxPacketType::AuthResponse, 0, 0, 0, false, false, 0, 0, "", 0 };
            memcpy(authPkt.payload, hash, 32);
            authPkt.payloadSize = 32;
            m_network.SendPacket(authPkt);
            std::cout << "[Security] Auth response (Hash) sent." << std::endl;

        } else if (inPkt.type == NetMuxPacketType::AuthResponse) {
            if (m_settings.isServer) {
                std::cout << "[Security] Auth response received from peer " << peerId << std::endl;

                bool authenticated = false;
                if (m_settings.securityKey.empty()) {
                    authenticated = true;
                } else if (m_pendingNonces.count(peerId)) {
                    int nonce = m_pendingNonces[peerId];
                    unsigned char expectedHash[32];
                    ComputeHash(nonce, m_settings.securityKey, expectedHash);

                    if (inPkt.payloadSize == 32 && memcmp(inPkt.payload, expectedHash, 32) == 0) {
                        authenticated = true;
                    } else {
                        std::cerr << "[Security] Auth FAILED for peer " << peerId << " (Hash mismatch)" << std::endl;
                    }
                    m_pendingNonces.erase(peerId);
                }

                if (authenticated) {
                    m_sync.SetAuthenticated(peerId, true);
                    std::cout << "[Security] Peer " << peerId << " authenticated successfully." << std::endl;
                }
            }
        } else if (inPkt.type == NetMuxPacketType::FocusUpdate) {
            unsigned long long newOwner = (unsigned long long)inPkt.button;
            m_sync.SetActivePeer(newOwner);
        } else if (inPkt.type == NetMuxPacketType::SessionUpdate) {
            int size = std::min(inPkt.payloadSize, (int)sizeof(inPkt.payload));
            std::string meta(inPkt.payload, size);
            size_t sep = meta.find('|');
            std::string sessionName = (sep != std::string::npos) ? meta.substr(0, sep) : meta;
            std::string groupName = (sep != std::string::npos) ? meta.substr(sep + 1) : "";

            m_sync.UpdatePeer(peerId, inPkt.groupId, 0, 0, 0, sessionName.c_str(), groupName.c_str());

            if (m_settings.isServer) {
                m_network.SendPacket(inPkt);
            }
        } else if (inPkt.type == NetMuxPacketType::ResolutionUpdate) {
            m_sync.UpdatePeerResolution(peerId, inPkt.x, inPkt.y);
            if (m_settings.isServer) m_network.SendPacket(inPkt);
        } else if (inPkt.type == NetMuxPacketType::SyncCheck) {
            if (m_settings.isServer) {
                unsigned long long subjectPeerId = (unsigned long long)inPkt.button;
                PeerState authoritative;
                if (m_sync.GetPeerState(subjectPeerId, authoritative)) {
                    float dx = (float)(inPkt.x - authoritative.normalizedX);
                    float dy = (float)(inPkt.y - authoritative.normalizedY);
                    float drift = std::sqrt(dx*dx + dy*dy);

                    m_sync.UpdateDrift(peerId, drift); // Log drift of the reporting client

                    if (drift > 327) { // ~5 pixels in 65535 space
                        // Trigger immediate corrective sync for this client
                        Packet masterPkt = { subjectPeerId, authoritative.groupId, 0.0, NetMuxPacketType::MasterStateSync, authoritative.normalizedX, authoritative.normalizedY, 0, false, false, 0, 0, "", 0 };
                        m_network.SendPacket(masterPkt);
                        std::cout << "[Sync] Corrective MasterSync issued to client " << peerId << " for peer " << subjectPeerId << " (Drift: " << drift << ")" << std::endl;
                    }
                }
            }
        } else if (inPkt.type == NetMuxPacketType::SelectionUpdate) {
            m_sync.UpdatePeerSelection(peerId, inPkt.isSelecting, inPkt.selectionStartX, inPkt.selectionStartY);
            if (m_settings.isServer) {
                m_network.SendPacketToGroup(inPkt, inPkt.groupId);
            }
        } else if (inPkt.type == NetMuxPacketType::Ping) {
            Packet pong = { m_localId, m_settings.groupId, 0.0, NetMuxPacketType::Heartbeat, 0, 0, 0, false, false, 0, 0, "", 0 };
            unsigned int now = (unsigned int)m_loopTimer.ElapsedMilliseconds();
            pong.x = (int)(now & 0xFFFF);
            pong.y = (int)((now >> 16) & 0xFFFF);
            m_network.SendPacket(pong);
        }
    }
}

void NetMuxFramework::PerformSyncCheck() {
    // Clients send their current view of peers to the server for validation
    if (m_settings.isServer) return;

    static double lastSyncCheck = 0;
    if (m_loopTimer.ElapsedMilliseconds() - lastSyncCheck > 500.0) {
        auto peers = m_sync.GetAllPeers();
        for (auto const& [id, peer] : peers) {
            Packet checkPkt = { m_localId, m_settings.groupId, 0.0, NetMuxPacketType::SyncCheck, peer.normalizedX, peer.normalizedY, 0, false, false, 0, 0, "", 0 };
            checkPkt.button = (int)id; // Peer ID we are reporting on
            m_network.SendPacket(checkPkt);
        }
        lastSyncCheck = m_loopTimer.ElapsedMilliseconds();
    }
}

void NetMuxFramework::PerformMasterStateSync() {
    if (!m_settings.isServer) return;

    static double lastMasterSync = 0;
    if (m_loopTimer.ElapsedMilliseconds() - lastMasterSync > 100.0) {
        auto peers = m_sync.GetAllPeers();
        for (auto const& [id, peer] : peers) {
            Packet masterPkt = { id, peer.groupId, 0.0, NetMuxPacketType::MasterStateSync, peer.normalizedX, peer.normalizedY, 0, false, false, 0, 0, "", 0 };
            masterPkt.localTimestamp = m_loopTimer.ElapsedMilliseconds();
            m_network.SendPacket(masterPkt);
        }
        lastMasterSync = m_loopTimer.ElapsedMilliseconds();
    }
}

void NetMuxFramework::PerformLatencySync() {
    if (m_loopTimer.ElapsedMilliseconds() - m_lastSyncTime > 1000.0) {
        Packet syncPkt = { m_localId, m_settings.groupId, 0.0, NetMuxPacketType::Sync, 0, 0, 0, false, false, 0, 0, "", 0 };
        m_network.SendPacket(syncPkt);
        m_syncTimer.Reset();
        m_lastSyncTime = m_loopTimer.ElapsedMilliseconds();
    }

    static double lastPing = 0;
    if (m_loopTimer.ElapsedMilliseconds() - lastPing > 2000.0) {
        Packet pingPkt = { m_localId, m_settings.groupId, 0.0, NetMuxPacketType::Ping, 0, 0, 0, false, false, 0, 0, "", 0 };
        m_network.SendPacket(pingPkt);
        lastPing = m_loopTimer.ElapsedMilliseconds();
    }

    static double lastHeartbeat = 0;
    if (m_loopTimer.ElapsedMilliseconds() - lastHeartbeat > 100.0) {
        Packet hbPkt = { m_localId, m_settings.groupId, 0.0, NetMuxPacketType::Heartbeat, 0, 0, 0, false, false, 0, 0, "", 0 };
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
        m_network.BroadcastDiscovery(5556);
        lastBroadcast = m_loopTimer.ElapsedMilliseconds();
    }
}

void NetMuxFramework::PerformClipboardSync() {
    if (m_clipboard.HasChanged()) {
        std::string text = m_clipboard.GetText();
        if (text.size() < 1024) {
            Packet pkt = { m_localId, m_settings.groupId, 0.0, NetMuxPacketType::ClipboardSync, 0, 0, 0, false, false, 0, 0, "", 0 };
            memcpy(pkt.payload, text.c_str(), text.size());
            pkt.payloadSize = (int)text.size();
            m_network.SendPacket(pkt);
        }
    }
}
