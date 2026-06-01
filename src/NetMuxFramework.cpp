#include "NetMuxFramework.hpp"
#include "ConfigGUI.hpp"
#include "AuthModule.hpp"

#ifdef __linux__
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#endif
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
#else
#include <unistd.h>
#endif

NetMuxFramework::NetMuxFramework()
    : m_running(false), m_lastSyncTime(0), m_lastPerfLog(0), m_overlayDirty(false) {
    m_localId = (unsigned long long)std::chrono::system_clock::now().time_since_epoch().count();
#ifdef __linux__
    m_xDisplay = XOpenDisplay(NULL);
    if (m_xDisplay) {
        m_xWindow = XCreateSimpleWindow((Display*)m_xDisplay, DefaultRootWindow((Display*)m_xDisplay), 0, 0, 1, 1, 0, 0, 0);
    }
#endif
}

NetMuxFramework::~NetMuxFramework() {
    Shutdown();
#ifdef __linux__
    if (m_xDisplay) {
        XDestroyWindow((Display*)m_xDisplay, (Window)m_xWindow);
        XCloseDisplay((Display*)m_xDisplay);
        m_xDisplay = nullptr;
    }
#endif
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
        Packet handshake = { m_localId, m_settings.groupId, m_sequenceCounter++, 0.0, NetMuxPacketType::Handshake, 0, 0, 0, false, false, 0, 0, 0, false, 0, 0, "", 0 };
        std::string meta = m_settings.sessionName + "|" + m_settings.groupName;
        strncpy(handshake.payload, meta.c_str(), sizeof(handshake.payload) - 1);
        handshake.payloadSize = (int)meta.size();
        m_network.SendPacket(handshake);
    }

    if (!m_overlay.Initialize()) {
        std::cerr << "Failed to initialize overlay components." << std::endl;
        return false;
    }

    if (!m_settings.cursorThemePath.empty()) {
        m_overlay.LoadCursorTheme(m_settings.cursorThemePath);
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
    Packet resPkt = { m_localId, m_settings.groupId, m_sequenceCounter++, 0.0, NetMuxPacketType::ResolutionUpdate, sw, sh, 0, false, false, 0, 0, 0, false, 0, 0, "", 0 };
    m_network.SendPacket(resPkt);

    // Default color
    m_overlay.SetColor(255, 0, 0);
    m_overlay.SetSelectionColor(m_settings.selectionColorR, m_settings.selectionColorG, m_settings.selectionColorB);

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
        PerformPeerCleanup();
#ifdef __linux__
        ProcessX11Events();
#endif
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
                // Only render the peer if it is NOT us, OR if it's us and we are captured 
                // (though usually we only want to see OTHER people's cursors on our screen).
                // If we are the server, we want to see the client's cursor.
                // If we are the client, we want to see the server's cursor.
                if (id == m_localId) continue; 

                overlayPeers[id] = { (int)peer.x, (int)peer.y, peer.colorR, peer.colorG, peer.colorB, peer.groupId, peer.isSelecting, peer.isConflictBlocked, (int)peer.selStartX, (int)peer.selStartY };

                // Active cursor visual indicator (White for owner, original for others)
                if (id == activeId) {
                    overlayPeers[id].r = 255; overlayPeers[id].g = 255; overlayPeers[id].b = 255;
                }
            }
            m_overlay.SetActivePeer(activeId);
            m_overlay.RenderPeers(overlayPeers);
            
            // Minimap logic: Only show peers that are "active" on our screen
            ConfigGUI::UpdateCursorMonitor(peers); 
            m_renderTimer.Reset();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void NetMuxFramework::UpdateSessionMetadata(const std::string& name, unsigned int groupId) {
    m_settings.sessionName = name;
    m_settings.groupId = groupId;

    Packet update = { m_localId, m_settings.groupId, m_sequenceCounter++, 0.0, NetMuxPacketType::SessionUpdate, 0, 0, 0, false, false, 0, 0, 0, false, 0, 0, "", 0 };
    std::string meta = m_settings.sessionName + "|" + m_settings.groupName;
    strncpy(update.payload, meta.c_str(), sizeof(update.payload) - 1);
    update.payloadSize = (int)meta.size();
    m_network.SendPacket(update);

    std::cout << "[Framework] Local session updated: " << name << " (Group: " << groupId << ")" << std::endl;
}

void NetMuxFramework::Shutdown() {
    if (m_running) {
        m_running = false;

        // Broadcast disconnect to peers
        Packet discPkt = { m_localId, m_settings.groupId, m_sequenceCounter++, 0.0, NetMuxPacketType::Disconnect, 0, 0, 0, false, false, 0, 0, 0, false, 0, 0, "", 0 };
        m_network.SendPacket(discPkt);

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
                    Packet focusPkt = { m_localId, m_settings.groupId, m_sequenceCounter++, 0.0, NetMuxPacketType::FocusUpdate, 0, 0, 0, false, false, 0, 0, 0, false, 0, 0, "", 0 };
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
            outPkt.sequenceNumber = m_sequenceCounter++;
            outPkt.localTimestamp = m_loopTimer.ElapsedMilliseconds();
            
            // Sync local state for rendering on our own overlay
            if (outPkt.type == NetMuxPacketType::AbsoluteMovement || outPkt.type == NetMuxPacketType::SelectionUpdate) {
                m_sync.UpdateLocalState(m_localId, m_settings.groupId, outPkt.x, outPkt.y, outPkt.isSelecting, outPkt.selectionStartX, outPkt.selectionStartY);
            }

            m_network.SendPacket(outPkt);
        }
    }
}

bool NetMuxFramework::IsPeerTrusted(unsigned long long peerId, NetMuxPacketType type) {
    if (m_settings.securityKey.empty()) return true;

    PeerState peer;
    if (m_sync.GetPeerState(peerId, peer)) {
        if (peer.isAuthenticated) return true;
    }

    // Auto-Challenge: If we get a sensitive packet from an unauthenticated peer,
    // they might have lost state or we might have restarted. Give them a chance to re-auth.
    static std::map<unsigned long long, double> lastChallengeTime;
    double now = m_loopTimer.ElapsedMilliseconds();

    if (now - lastChallengeTime[peerId] > 2000.0) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Auto-Challenge peer %llu (Type %d)", peerId, (int)type);
        ConfigGUI::LogSecurityEvent(msg);
        Packet challenge = { m_localId, m_settings.groupId, m_sequenceCounter++, 0.0, NetMuxPacketType::AuthChallenge, 0, 0, 0, false, false, 0, 0, 0, false, 0, 0, "", 0 };
        challenge.x = m_authService.CreateChallenge(peerId);
        m_network.SendPacket(challenge);
        lastChallengeTime[peerId] = now;
    }

    return false;
}

void NetMuxFramework::ProcessIncomingPackets() {
    Packet inPkt;
    while (m_network.ReceivePacket(inPkt)) {
        unsigned long long peerId = inPkt.senderId;

        if (!m_settings.isServer && inPkt.groupId != m_settings.groupId) {
            continue;
        }

        // Replay Protection: Monotonic sequence only for movement/updates.
        // Clicks and other sensitive actions via TCP are already ordered and guaranteed.
        bool isUdpPacket = (inPkt.type == NetMuxPacketType::Movement || inPkt.type == NetMuxPacketType::AbsoluteMovement ||
                            inPkt.type == NetMuxPacketType::Heartbeat || inPkt.type == NetMuxPacketType::SyncCheck);

        if (isUdpPacket) {
            if (m_lastSequence.count(peerId) && inPkt.sequenceNumber <= m_lastSequence[peerId]) {
                continue;
            }
            m_lastSequence[peerId] = inPkt.sequenceNumber;
        }

        if (inPkt.type == NetMuxPacketType::Movement) {
            if (IsPeerTrusted(peerId, inPkt.type)) {
                m_driver.SendMouseMovement(inPkt.x, inPkt.y);
            }
        } else if (inPkt.type == NetMuxPacketType::AbsoluteMovement) {
            if (!IsPeerTrusted(peerId, inPkt.type)) continue;

            PeerState oldPeer;
            m_sync.GetPeerState(peerId, oldPeer);

            int localScreenWidth = 1920, localScreenHeight = 1080;
            int screenLeft = 0, screenTop = 0;
#ifdef _WIN32
            screenLeft = GetSystemMetrics(SM_XVIRTUALSCREEN);
            screenTop = GetSystemMetrics(SM_YVIRTUALSCREEN);
            localScreenWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
            localScreenHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);
#endif
            float newX = (float)(screenLeft + SyncModule::Denormalize(inPkt.x, localScreenWidth));
            float newY = (float)(screenTop + SyncModule::Denormalize(inPkt.y, localScreenHeight));

            long dx = (long)(newX - oldPeer.x);
            long dy = (long)(newY - oldPeer.y);

            m_sync.UpdatePeer(peerId, inPkt.groupId, inPkt.x, inPkt.y, inPkt.localTimestamp);
            m_overlayDirty = true;
            m_driver.SendMouseMovement(dx, dy);

            if (m_settings.isServer) {
                m_network.SendPacketToGroup(inPkt, inPkt.groupId);
            }
        } else if (inPkt.type == NetMuxPacketType::Click) {
            if (IsPeerTrusted(peerId, inPkt.type)) {
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
            if (!IsPeerTrusted(peerId, inPkt.type)) continue;

            if (m_settings.isServer) {
                m_network.SendPacket(inPkt);
            } else {
                unsigned int remoteLow = (unsigned int)inPkt.x;
                unsigned int remoteHigh = (unsigned int)inPkt.y;
                double remoteTime = (double)(remoteLow | (remoteHigh << 16));
                m_sync.UpdateClockOffset(peerId, remoteTime, m_loopTimer.ElapsedMilliseconds());
            }
        } else if (inPkt.type == NetMuxPacketType::ClipboardSync) {
            if (IsPeerTrusted(peerId, inPkt.type)) {
                double remoteTimestamp = m_sync.GetAdjustedTimestamp(peerId, inPkt.localTimestamp);
                if (remoteTimestamp > m_lastClipboardTimestamp) {
                    auto& buffer = m_clipboardReassembly[peerId];
                    if (inPkt.chunkIndex == 0) buffer.clear();

                    int pSize = std::min(inPkt.payloadSize, (int)sizeof(inPkt.payload));
                    buffer.insert(buffer.end(), inPkt.payload, inPkt.payload + pSize);

                    if (inPkt.chunkIndex == inPkt.totalChunks - 1) {
                        std::string fullText(buffer.begin(), buffer.end());
                        m_clipboard.SetText(fullText);
                        m_lastClipboardTimestamp = remoteTimestamp;
                        buffer.clear();
                    }

                    if (m_settings.isServer) {
                        m_network.SendPacket(inPkt);
                    }
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
            m_lastSequence[peerId] = inPkt.sequenceNumber; // Reset sequence for new handshake

            // Mutual Authentication: Both sides challenge each other
            Packet challenge = { m_localId, m_settings.groupId, m_sequenceCounter++, 0.0, NetMuxPacketType::AuthChallenge, 0, 0, 0, false, false, 0, 0, 0, false, 0, 0, "", 0 };
            challenge.x = m_authService.CreateChallenge(peerId);
            m_network.SendPacket(challenge);

            if (m_settings.isServer) {
                Packet reply = { m_localId, m_settings.groupId, m_sequenceCounter++, 0.0, NetMuxPacketType::Handshake, 0, 0, 0, false, false, 0, 0, 0, false, 0, 0, "", 0 };
                std::string sMeta = m_settings.sessionName + "|" + m_settings.groupName;
                strncpy(reply.payload, sMeta.c_str(), sizeof(reply.payload) - 1);
                reply.payloadSize = (int)sMeta.size();
                m_network.SendPacket(reply);
            }
        } else if (inPkt.type == NetMuxPacketType::AuthChallenge) {
            int nonce = inPkt.x;
            unsigned char hash[32];
            AuthModule::GenerateResponse(nonce, m_settings.securityKey, hash);

            Packet authPkt = { m_localId, m_settings.groupId, m_sequenceCounter++, 0.0, NetMuxPacketType::AuthResponse, 0, 0, 0, false, false, 0, 0, 0, false, 0, 0, "", 0 };
            memcpy(authPkt.payload, hash, 32);
            authPkt.payloadSize = 32;
            m_network.SendPacket(authPkt);
            std::cout << "[Security] Auth response (Hash) sent." << std::endl;

        } else if (inPkt.type == NetMuxPacketType::AuthResponse) {
            std::cout << "[Security] Auth response received from peer " << peerId << std::endl;

            bool authenticated = false;
            if (m_settings.securityKey.empty()) {
                authenticated = true;
            } else {
                if (inPkt.payloadSize == 32 && m_authService.VerifyResponse(peerId, m_settings.securityKey, (const unsigned char*)inPkt.payload)) {
                    authenticated = true;
                } else {
                    char msg[256];
                    snprintf(msg, sizeof(msg), "Auth FAILED for peer %llu (Hash mismatch)", peerId);
                    ConfigGUI::LogSecurityEvent(msg);
                }
            }

            if (authenticated) {
                m_sync.SetAuthenticated(peerId, true);
                char msg[256];
                snprintf(msg, sizeof(msg), "Peer %llu authenticated successfully.", peerId);
                ConfigGUI::LogSecurityEvent(msg);
            }
        } else if (inPkt.type == NetMuxPacketType::FocusUpdate) {
            unsigned long long newOwner = (unsigned long long)inPkt.button;
            m_sync.SetActivePeer(newOwner);
        } else if (inPkt.type == NetMuxPacketType::SessionUpdate) {
            if (!IsPeerTrusted(peerId, inPkt.type)) continue;

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
            if (!IsPeerTrusted(peerId, inPkt.type)) continue;

            m_sync.UpdatePeerResolution(peerId, inPkt.x, inPkt.y);
            if (m_settings.isServer) m_network.SendPacket(inPkt);
        } else if (inPkt.type == NetMuxPacketType::SyncCheck) {
            if (!IsPeerTrusted(peerId, inPkt.type)) continue;

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
                        Packet masterPkt = { subjectPeerId, authoritative.groupId, m_sequenceCounter++, 0.0, NetMuxPacketType::MasterStateSync, authoritative.normalizedX, authoritative.normalizedY, 0, false, false, 0, 0, 0, false, 0, 0, "", 0 };
                        m_network.SendPacket(masterPkt);
                        std::cout << "[Sync] Corrective MasterSync issued to client " << peerId << " for peer " << subjectPeerId << " (Drift: " << drift << ")" << std::endl;
                    }
                }
            }
        } else if (inPkt.type == NetMuxPacketType::SelectionUpdate) {
            if (!IsPeerTrusted(peerId, inPkt.type)) continue;

            m_sync.UpdatePeerSelection(peerId, inPkt.isSelecting, inPkt.selectionStartX, inPkt.selectionStartY);
            if (m_settings.isServer) {
                m_network.SendPacketToGroup(inPkt, inPkt.groupId);
            }
        } else if (inPkt.type == NetMuxPacketType::Ping) {
            Packet pong = { m_localId, m_settings.groupId, m_sequenceCounter++, 0.0, NetMuxPacketType::Heartbeat, 0, 0, 0, false, false, 0, 0, 0, false, 0, 0, "", 0 };
            unsigned int now = (unsigned int)m_loopTimer.ElapsedMilliseconds();
            pong.x = (int)(now & 0xFFFF);
            pong.y = (int)((now >> 16) & 0xFFFF);
            m_network.SendPacket(pong);
        } else if (inPkt.type == NetMuxPacketType::Disconnect) {
            char msg[256];
            snprintf(msg, sizeof(msg), "Peer %llu disconnected explicitly.", peerId);
            ConfigGUI::LogSecurityEvent(msg);
            m_sync.RemovePeer(peerId);
            m_authService.ClearPeer(peerId);
        } else if (inPkt.type == NetMuxPacketType::Wheel) {
            if (IsPeerTrusted(peerId, inPkt.type)) {
                m_driver.SendMouseWheel(inPkt.wheelDelta, inPkt.isHorizontalWheel);
            }
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
            Packet checkPkt = { m_localId, m_settings.groupId, m_sequenceCounter++, 0.0, NetMuxPacketType::SyncCheck, peer.normalizedX, peer.normalizedY, 0, false, false, 0, 0, 0, false, 0, 0, "", 0 };
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
            Packet masterPkt = { id, peer.groupId, m_sequenceCounter++, 0.0, NetMuxPacketType::MasterStateSync, peer.normalizedX, peer.normalizedY, 0, false, false, 0, 0, 0, false, 0, 0, "", 0 };
            masterPkt.localTimestamp = m_loopTimer.ElapsedMilliseconds();
            m_network.SendPacket(masterPkt);
        }
        lastMasterSync = m_loopTimer.ElapsedMilliseconds();
    }
}

void NetMuxFramework::PerformLatencySync() {
    if (m_loopTimer.ElapsedMilliseconds() - m_lastSyncTime > 1000.0) {
        Packet syncPkt = { m_localId, m_settings.groupId, m_sequenceCounter++, 0.0, NetMuxPacketType::Sync, 0, 0, 0, false, false, 0, 0, 0, false, 0, 0, "", 0 };
        m_network.SendPacket(syncPkt);
        m_syncTimer.Reset();
        m_lastSyncTime = m_loopTimer.ElapsedMilliseconds();
    }

    static double lastPing = 0;
    if (m_loopTimer.ElapsedMilliseconds() - lastPing > 2000.0) {
        Packet pingPkt = { m_localId, m_settings.groupId, m_sequenceCounter++, 0.0, NetMuxPacketType::Ping, 0, 0, 0, false, false, 0, 0, 0, false, 0, 0, "", 0 };
        m_network.SendPacket(pingPkt);
        lastPing = m_loopTimer.ElapsedMilliseconds();
    }

    static double lastHeartbeat = 0;
    if (m_loopTimer.ElapsedMilliseconds() - lastHeartbeat > 100.0) {
        Packet hbPkt = { m_localId, m_settings.groupId, m_sequenceCounter++, 0.0, NetMuxPacketType::Heartbeat, 0, 0, 0, false, false, 0, 0, 0, false, 0, 0, "", 0 };
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

void NetMuxFramework::PerformPeerCleanup() {
    static double lastCleanup = 0;
    if (m_loopTimer.ElapsedMilliseconds() - lastCleanup > 5000.0) {
        std::vector<unsigned long long> pruned = m_sync.PruneInactivePeers(10000.0); // 10s timeout
        for (auto id : pruned) {
            char msg[256];
            snprintf(msg, sizeof(msg), "Peer %llu timed out and pruned.", id);
            ConfigGUI::LogSecurityEvent(msg);
            m_authService.ClearPeer(id);
        }
        lastCleanup = m_loopTimer.ElapsedMilliseconds();
    }
}

#ifdef __linux__
void NetMuxFramework::ProcessX11Events() {
    if (!m_xDisplay) return;
    Display* display = (Display*)m_xDisplay;
    XEvent event;
    while (XPending(display)) {
        XNextEvent(display, &event);
        if (event.type == SelectionRequest) {
            XSelectionRequestEvent* req = &event.xselectionrequest;
            XSelectionEvent sev = {0};
            sev.type = SelectionNotify;
            sev.display = req->display;
            sev.requestor = req->requestor;
            sev.selection = req->selection;
            sev.target = req->target;
            sev.property = req->property;
            sev.time = req->time;

            Atom utf8 = XInternAtom(display, "UTF8_STRING", False);
            if (req->target == utf8 || req->target == XA_STRING) {
                std::string text = m_clipboard.GetText();
                XChangeProperty(display, req->requestor, req->property, req->target, 8, PropModeReplace,
                                (unsigned char*)text.c_str(), text.size());
            } else {
                sev.property = None;
            }
            XSendEvent(display, req->requestor, True, 0, (XEvent*)&sev);
        }
    }
}
#endif

void NetMuxFramework::PerformClipboardSync() {
    static double lastCheck = 0;
    if (m_loopTimer.ElapsedMilliseconds() - lastCheck < 500.0) return;
    lastCheck = m_loopTimer.ElapsedMilliseconds();

    if (m_clipboard.HasChanged()) {
        std::string text = m_clipboard.GetText();
        double now = m_loopTimer.ElapsedMilliseconds();
        m_lastClipboardTimestamp = now;

        const size_t CHUNK_SIZE = 4000; // Leave some safety room
        int totalChunks = (int)((text.size() + CHUNK_SIZE - 1) / CHUNK_SIZE);
        if (totalChunks == 0) totalChunks = 1;

        for (int i = 0; i < totalChunks; ++i) {
            size_t offset = i * CHUNK_SIZE;
            size_t remaining = text.size() - offset;
            size_t currentChunkSize = std::min(CHUNK_SIZE, remaining);

            Packet pkt = { m_localId, m_settings.groupId, m_sequenceCounter++, now, NetMuxPacketType::ClipboardSync, 0, 0, 0, false, false, 0, 0, 0, false, 0, 0, "", 0 };
            pkt.chunkIndex = i;
            pkt.totalChunks = totalChunks;

            if (currentChunkSize > 0) {
                memcpy(pkt.payload, text.c_str() + offset, currentChunkSize);
            }
            pkt.payloadSize = (int)currentChunkSize;
            m_network.SendPacket(pkt);
        }
    }
}
