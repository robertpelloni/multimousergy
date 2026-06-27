#include "NetMuxFramework.hpp"
#include "AuthModule.hpp"
#include "Logger.hpp"

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

    // Ensure SyncModule knows our local ID for filtering
    m_sync.UpdateLocalState(m_localId, m_settings.groupId, 0, 0, false, 0, 0);

    if (m_settings.isServer) {
        if (!m_network.StartServer(m_settings.port)) {
            Logger::Log(LogLevel::Error, "Failed to start server on port " + std::to_string(m_settings.port));
            return false;
        }
        Logger::Log(LogLevel::Info, "Server started on port " + std::to_string(m_settings.port));
    } else {
        if (!m_network.Connect(m_settings.remoteIp, m_settings.port)) {
            Logger::Log(LogLevel::Error, "Failed to connect to " + m_settings.remoteIp + ":" + std::to_string(m_settings.port));
            return false;
        }
        Logger::Log(LogLevel::Info, "Connecting to " + m_settings.remoteIp + ":" + std::to_string(m_settings.port));

        // Handshake: Defer until TCP connection is confirmed in Run loop
        if (m_network.GetTcpState() == ConnectionState::Connected) {
            Packet handshake = { m_localId, m_settings.groupId, m_sequenceCounter++, 0.0, NetMuxPacketType::Handshake, 0, 0, 0, false, false, 0, 0, 0, false, 0, 0, 1.0f, "", 0 };
            std::string meta = m_settings.sessionName + "|" + m_settings.groupName + "|" + m_settings.displayName;
            strncpy(handshake.payload, meta.c_str(), sizeof(handshake.payload) - 1);
            handshake.payloadSize = (int)meta.size();
            m_network.SendPacket(handshake);
        }
    }

    if (!m_overlay.Initialize()) {
        std::cerr << "Failed to initialize overlay components." << std::endl;
        return false;
    }

    if (!m_settings.cursorThemePath.empty()) {
        m_overlay.LoadCursorTheme(m_settings.cursorThemePath);
    }

#ifdef _WIN32
    if (m_settings.useD3D11) {
        if (!m_spatialViewport.Initialize((ID3D11Device*)m_overlay.GetD3D11Context())) {
            std::cerr << "[Warning] Failed to initialize Spatial Viewport." << std::endl;
        }
        if (!m_capture.Initialize()) {
            std::cerr << "[Warning] Failed to initialize DXGI Desktop Duplication." << std::endl;
        }
    }
#endif

    if (!m_webrtc.Initialize()) {
        std::cerr << "[Warning] WebRTC initialization failed. High-bandwidth media streams will be unavailable." << std::endl;
    }

    if (!m_driver.Initialize(m_settings.driverType)) {
        std::cout << "[Warning] Virtual HID Driver not available. Using software fallback." << std::endl;
    }

    // Broadcast initial resolution and DPI
    int sw = 1920;
    int sh = 1080;
    float dpi = 1.0f;
#ifdef _WIN32
    sw = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    sh = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    // Retrieve system DPI scale
    HDC hdc = GetDC(NULL);
    if (hdc) {
        dpi = (float)GetDeviceCaps(hdc, LOGPIXELSX) / 96.0f;
        ReleaseDC(NULL, hdc);
    }
#endif
    Packet resPkt = { m_localId, m_settings.groupId, m_sequenceCounter++, 0.0, NetMuxPacketType::ResolutionUpdate, sw, sh, 0, false, false, 0, 0, 0, false, 0, 0, dpi, "", 0 };
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

    ConnectionState lastTcpState = m_network.GetTcpState();

    while (m_running) {
        double dt = frameTimer.ElapsedMilliseconds();
        frameTimer.Reset();

        ConnectionState currentTcpState = m_network.GetTcpState();
        if (!m_settings.isServer && lastTcpState == ConnectionState::Connecting && currentTcpState == ConnectionState::Connected) {
            std::cout << "[Framework] TCP Connected. Sending handshake..." << std::endl;
            Packet handshake = { m_localId, m_settings.groupId, m_sequenceCounter++, 0.0, NetMuxPacketType::Handshake, 0, 0, 0, false, false, 0, 0, 0, false, 0, 0, 1.0f, "", 0 };
            std::string meta = m_settings.sessionName + "|" + m_settings.groupName + "|" + m_settings.displayName;
            strncpy(handshake.payload, meta.c_str(), sizeof(handshake.payload) - 1);
            handshake.payloadSize = (int)meta.size();
            m_network.SendPacket(handshake);
        }
        lastTcpState = currentTcpState;

        PerformLatencySync();
        PerformDiscoveryBroadcast();
        PerformClipboardSync();
        PerformMasterStateSync();
        PerformSyncCheck();
        PerformFileTransfer();
        PerformPeerCleanup();
#ifdef __linux__
        ProcessX11Events();
#endif
        ProcessOutgoingPackets();
        ProcessIncomingPackets();
        ProcessInteractionQueue();

        m_sync.Step(dt);

        // Update MultiMousergy Spatial Viewport
        m_spatialViewport.Update((float)dt / 1000.0f, m_input.IsCaptured());

#ifdef _WIN32
        if (m_settings.useD3D11) {
            if (m_capture.AcquireFrame()) {
                ID3D11Texture2D* frame = m_capture.GetCurrentFrameTexture();
                if (frame) {
                    ID3D11ShaderResourceView* srv = nullptr;
                    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
                    srvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
                    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                    srvDesc.Texture2D.MostDetailedMip = 0;
                    srvDesc.Texture2D.MipLevels = 1;

                    // Note: In a real app we'd cache this SRV, creating it per-frame is inefficient
                    // We also need the device to create the SRV, which we'd get from the texture
                    // For now, this hooks the systems together conceptually
                    // m_spatialViewport.SetLocalDesktopTexture(srv);
                }
                m_capture.ReleaseFrame();
            }
        }
#endif

        // Update input capture permission based on connectivity
        auto allPeers = m_sync.GetAllPeers();
        bool hasExternalPeers = false;
        for (auto const& [id, p] : allPeers) {
            if (id != m_localId) {
                hasExternalPeers = true;
                break;
            }
        }
        m_input.SetPeerConnected(hasExternalPeers);

        // Output UI Telemetry to stdout (JSON format)
        static double lastUIUpdate = 0;
        if (m_loopTimer.ElapsedMilliseconds() - lastUIUpdate > 100.0) {
            std::map<unsigned long long, PeerState> peers = m_sync.GetAllPeers();
            std::cout << "{\"type\":\"telemetry\",\"peers\":[";
            bool first = true;
            for (auto& p : peers) {
                if (!first) std::cout << ",";
                std::cout << "{\"id\":" << p.first << ",\"x\":" << p.second.x << ",\"y\":" << p.second.y << "}";
                first = false;
            }
            std::cout << "]}" << std::endl;
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
                // Client-side filtering: Only show peers in our group
                if (!m_settings.isServer && peer.groupId != m_settings.groupId) {
                    continue;
                }

                // ALWAYS render the peer on the overlay (even ourselves)
                // This provides the "Mux" feel where everyone sees everyone's cursor shape/color.
                overlayPeers[id] = { (int)peer.x, (int)peer.y, peer.colorR, peer.colorG, peer.colorB, peer.groupId, peer.isSelecting, peer.isConflictBlocked, (int)peer.selStartX, (int)peer.selStartY };

                // Active cursor visual indicator (White for owner, original for others)
                if (id == activeId) {
                    overlayPeers[id].r = 255; overlayPeers[id].g = 255; overlayPeers[id].b = 255;
                }
            }
            m_overlay.SetActivePeer(activeId);
            m_overlay.RenderPeers(overlayPeers);
            
#ifdef _WIN32
            // Delegate spatial rendering to SpatialViewport if D3D11 is used
            if (m_settings.useD3D11 && m_overlay.GetD3D11Context() != nullptr) {

                m_spatialViewport.Render((ID3D11DeviceContext*)m_overlay.GetD3D11Context(), overlayPeers);

                // Get local DPI scale
                float localDpi = 1.0f;
                HDC hdc = GetDC(NULL);
                if (hdc) {
                    localDpi = (float)GetDeviceCaps(hdc, LOGPIXELSX) / 96.0f;
                    ReleaseDC(NULL, hdc);
                }
                m_spatialViewport.Render((ID3D11DeviceContext*)m_overlay.GetD3D11Context(), overlayPeers, localDpi);

            }
#endif

            m_renderTimer.Reset();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void NetMuxFramework::UpdateSessionMetadata(const std::string& name, unsigned int groupId) {
    m_settings.sessionName = name;
    m_settings.groupId = groupId;

    Packet update = { m_localId, m_settings.groupId, m_sequenceCounter++, 0.0, NetMuxPacketType::SessionUpdate, 0, 0, 0, false, false, 0, 0, 0, false, 0, 0, 1.0f, "", 0 };
    std::string meta = m_settings.sessionName + "|" + m_settings.groupName + "|" + m_settings.displayName;
    strncpy(update.payload, meta.c_str(), sizeof(update.payload) - 1);
    update.payloadSize = (int)meta.size();
    m_network.SendPacket(update);

    std::cout << "[Framework] Local session updated: " << name << " (Group: " << groupId << ")" << std::endl;
}

void NetMuxFramework::Shutdown() {
    if (m_running) {
        m_running = false;

        // Broadcast disconnect to peers
        Packet discPkt = { m_localId, m_settings.groupId, m_sequenceCounter++, 0.0, NetMuxPacketType::Disconnect, 0, 0, 0, false, false, 0, 0, 0, false, 0, 0, 1.0f, "", 0 };
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
                    Packet focusPkt = { m_localId, m_settings.groupId, m_sequenceCounter++, 0.0, NetMuxPacketType::FocusUpdate, 0, 0, 0, false, false, 0, 0, 0, false, 0, 0, 1.0f, "", 0 };
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
        outPkt.senderId = m_localId;
        outPkt.groupId = m_settings.groupId;
        outPkt.sequenceNumber = m_sequenceCounter++;
        outPkt.localTimestamp = m_loopTimer.ElapsedMilliseconds();
        
        // Always sync local state for rendering on our own overlay/minimap
        if (outPkt.type == NetMuxPacketType::AbsoluteMovement || outPkt.type == NetMuxPacketType::SelectionUpdate) {
            m_sync.UpdateLocalState(m_localId, m_settings.groupId, outPkt.x, outPkt.y, outPkt.isSelecting, outPkt.selectionStartX, outPkt.selectionStartY);
        }

        // Always broadcast to peers so they can see us globally
        m_network.SendPacket(outPkt);
    }
}

bool NetMuxFramework::IsPeerTrusted(unsigned long long peerId, NetMuxPacketType type) {
    if (m_settings.securityKey.empty()) return true;

    PeerState peer;
    if (m_sync.GetPeerState(peerId, peer)) {
        if (peer.isAuthenticated) return true;
    }

    // Auto-Challenge
    static std::map<unsigned long long, double> lastChallengeTime;
    double now = m_loopTimer.ElapsedMilliseconds();

    if (now - lastChallengeTime[peerId] > 2000.0) {
        Packet challenge = { m_localId, m_settings.groupId, m_sequenceCounter++, 0.0, NetMuxPacketType::AuthChallenge, 0, 0, 0, false, false, 0, 0, 0, false, 0, 0, 1.0f, "", 0 };
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

        // Replay Protection
        bool isUdpPacket = (inPkt.type == NetMuxPacketType::Movement || inPkt.type == NetMuxPacketType::AbsoluteMovement ||
                            inPkt.type == NetMuxPacketType::Heartbeat || inPkt.type == NetMuxPacketType::SyncCheck ||
                            inPkt.type == NetMuxPacketType::DeltaMovement);

        if (isUdpPacket) {
            if (m_lastSequence.count(peerId)) {
                unsigned int lastSeq = m_lastSequence[peerId];
                // Sequence wrap-around handling
                // If sequence difference is negative and large, it's likely a wrap-around
                int diff = (int)(inPkt.sequenceNumber - lastSeq);

                // Allow sequence if it's strictly greater, OR if it wrapped around (e.g., diff < -0x7FFFFFFF)
                if (diff <= 0 && diff > -100000) {
                    continue; // Drop replayed or out-of-order old packet
                }
            }
            m_lastSequence[peerId] = inPkt.sequenceNumber;
        } else {
            // TCP Replay Cache for strict exactly-once semantics
            // Clean up old entries periodically, not every packet
            double now = m_loopTimer.ElapsedMilliseconds();
            static double lastCleanupTime = 0;
            auto& cache = m_tcpReplayCache[peerId];
            if (now - lastCleanupTime > 10000.0) { // Cleanup every 10 seconds
                lastCleanupTime = now;
                for (auto it = cache.begin(); it != cache.end();) {
                    if (now - it->second > 60000.0) { // 60 seconds retention
                        it = cache.erase(it);
                    } else {
                        ++it;
                    }
                }
            }
            if (cache.count(inPkt.sequenceNumber)) {
                std::cerr << "[Security] TCP Replay attack detected from peer " << peerId << " seq " << inPkt.sequenceNumber << std::endl;
                continue; // Drop packet
            }
            cache[inPkt.sequenceNumber] = now;
        }

        if (inPkt.type == NetMuxPacketType::Movement) {
            if (IsPeerTrusted(peerId, inPkt.type)) {
                m_driver.SendMouseMovement(inPkt.x, inPkt.y);
                if (m_settings.isServer) m_network.SendPacketToGroup(inPkt, inPkt.groupId);
            }
        } else if (inPkt.type == NetMuxPacketType::DeltaMovement) {
            if (!IsPeerTrusted(peerId, inPkt.type)) continue;

            PeerState oldPeer;
            if (m_sync.GetPeerState(peerId, oldPeer)) {
                m_sync.UpdatePeer(peerId, inPkt.groupId, oldPeer.normalizedX + inPkt.x, oldPeer.normalizedY + inPkt.y, inPkt.localTimestamp);
                m_overlayDirty = true;
                if (m_settings.isServer) m_network.SendPacketToGroup(inPkt, inPkt.groupId);
            }
        } else if (inPkt.type == NetMuxPacketType::AbsoluteMovement) {
            if (!IsPeerTrusted(peerId, inPkt.type)) continue;

            PeerState oldPeer;
            m_sync.GetPeerState(peerId, oldPeer);

            int sw = 1920, sh = 1080;
            int sl = 0, st = 0;
#ifdef _WIN32
            sl = GetSystemMetrics(SM_XVIRTUALSCREEN);
            st = GetSystemMetrics(SM_YVIRTUALSCREEN);
            sw = GetSystemMetrics(SM_CXVIRTUALSCREEN);
            sh = GetSystemMetrics(SM_CYVIRTUALSCREEN);
#endif
            float nx = (float)(sl + SyncModule::Denormalize(inPkt.x, sw));
            float ny = (float)(st + SyncModule::Denormalize(inPkt.y, sh));

            long dx = (long)(nx - oldPeer.targetX);
            long dy = (long)(ny - oldPeer.targetY);

            m_sync.UpdatePeer(peerId, inPkt.groupId, inPkt.x, inPkt.y, inPkt.localTimestamp);
            m_overlayDirty = true;
            
            if (peerId == m_sync.GetActivePeer()) {
                m_driver.SendMouseMovement(dx, dy);
            }

            if (m_settings.isServer) m_network.SendPacketToGroup(inPkt, inPkt.groupId);

        } else if (inPkt.type == NetMuxPacketType::Click) {
            if (IsPeerTrusted(peerId, inPkt.type)) {
                m_sync.UpdatePeerButtons(peerId, inPkt.button, inPkt.down);
                std::lock_guard<std::mutex> lock(m_interactionMutex);
                m_interactionQueue.push({peerId, inPkt.button, inPkt.down, inPkt.localTimestamp, inPkt.groupId});
                if (m_settings.isServer) m_network.SendPacketToGroup(inPkt, inPkt.groupId);
            }
        } else if (inPkt.type == NetMuxPacketType::Sync) {
            m_sync.RefreshPeer(peerId, inPkt.groupId); // Keep alive
            if (m_settings.isServer) {
                m_network.SendPacket(inPkt);
            } else {
                double rtt = m_syncTimer.ElapsedMilliseconds();
                m_sync.UpdateLatency(peerId, rtt);
            }
        } else if (inPkt.type == NetMuxPacketType::Heartbeat) {
            if (!IsPeerTrusted(peerId, inPkt.type)) continue;
            m_sync.RefreshPeer(peerId, inPkt.groupId); // Keep alive

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
                    if (m_settings.isServer) m_network.SendPacket(inPkt);
                }
            }
        } else if (inPkt.type == NetMuxPacketType::Handshake) {
            int size = std::min(inPkt.payloadSize, (int)sizeof(inPkt.payload));
            std::string meta(inPkt.payload, size);
            size_t sep1 = meta.find('|');
            size_t sep2 = meta.find('|', sep1 + 1);

            std::string remoteName = (sep1 != std::string::npos) ? meta.substr(0, sep1) : meta;
            std::string remoteGroupName = (sep1 != std::string::npos && sep2 != std::string::npos) ? meta.substr(sep1 + 1, sep2 - sep1 - 1) :
                                         (sep1 != std::string::npos ? meta.substr(sep1 + 1) : "");
            std::string remoteDisplayName = (sep2 != std::string::npos) ? meta.substr(sep2 + 1) : "";

            std::cout << "[Network] Handshake from peer: " << remoteName << " (" << remoteDisplayName << ") ID: " << peerId << " Group: " << remoteGroupName << std::endl;
            m_sync.UpdatePeer(peerId, inPkt.groupId, 0, 0, 0, remoteName.c_str(), remoteGroupName.c_str(), remoteDisplayName.c_str());
            m_lastSequence[peerId] = inPkt.sequenceNumber;
            Packet challenge = { m_localId, m_settings.groupId, m_sequenceCounter++, 0.0, NetMuxPacketType::AuthChallenge, 0, 0, 0, false, false, 0, 0, 0, false, 0, 0, 1.0f, "", 0 };
            challenge.x = m_authService.CreateChallenge(peerId);
            m_network.SendPacket(challenge);
            if (m_settings.isServer) {
                Packet reply = { m_localId, m_settings.groupId, m_sequenceCounter++, 0.0, NetMuxPacketType::Handshake, 0, 0, 0, false, false, 0, 0, 0, false, 0, 0, 1.0f, "", 0 };
                std::string sMeta = m_settings.sessionName + "|" + m_settings.groupName + "|" + m_settings.displayName;
                strncpy(reply.payload, sMeta.c_str(), sizeof(reply.payload) - 1);
                reply.payloadSize = (int)sMeta.size();
                m_network.SendPacket(reply);
            }
        } else if (inPkt.type == NetMuxPacketType::AuthChallenge) {
            int nonce = inPkt.x;
            unsigned char hash[32];
            AuthModule::GenerateResponse(nonce, m_settings.securityKey, hash);
            Packet authPkt = { m_localId, m_settings.groupId, m_sequenceCounter++, 0.0, NetMuxPacketType::AuthResponse, 0, 0, 0, false, false, 0, 0, 0, false, 0, 0, 1.0f, "", 0 };
            memcpy(authPkt.payload, hash, 32);
            authPkt.payloadSize = 32;
            m_network.SendPacket(authPkt);
        } else if (inPkt.type == NetMuxPacketType::AuthResponse) {
            bool authenticated = m_settings.securityKey.empty();
            if (!authenticated && inPkt.payloadSize == 32) {
                authenticated = m_authService.VerifyResponse(peerId, m_settings.securityKey, (const unsigned char*)inPkt.payload);
            }
            if (authenticated) {
                m_sync.SetAuthenticated(peerId, true);
                std::cout << "[Security] Peer " << peerId << " authenticated." << std::endl;
            }
        } else if (inPkt.type == NetMuxPacketType::FocusUpdate) {
            m_sync.SetActivePeer((unsigned long long)inPkt.button);
            if (m_settings.isServer) m_network.SendPacketToGroup(inPkt, inPkt.groupId);
        } else if (inPkt.type == NetMuxPacketType::SessionUpdate) {
            if (!IsPeerTrusted(peerId, inPkt.type)) continue;
            m_sync.RefreshPeer(peerId, inPkt.groupId); // Refresh
            if (m_settings.isServer) m_network.SendPacketToGroup(inPkt, inPkt.groupId);
        } else if (inPkt.type == NetMuxPacketType::ResolutionUpdate) {
            if (!IsPeerTrusted(peerId, inPkt.type)) continue;
            m_sync.UpdatePeerResolution(peerId, inPkt.x, inPkt.y, inPkt.dpiScale);
            if (m_settings.isServer) m_network.SendPacketToGroup(inPkt, inPkt.groupId);
        } else if (inPkt.type == NetMuxPacketType::SelectionUpdate) {
            if (!IsPeerTrusted(peerId, inPkt.type)) continue;
            m_sync.UpdatePeerSelection(peerId, inPkt.isSelecting, inPkt.selectionStartX, inPkt.selectionStartY);
            if (m_settings.isServer) m_network.SendPacketToGroup(inPkt, inPkt.groupId);
        } else if (inPkt.type == NetMuxPacketType::Ping) {
            m_sync.RefreshPeer(peerId, inPkt.groupId);
            Packet pong = { m_localId, m_settings.groupId, m_sequenceCounter++, 0.0, NetMuxPacketType::Heartbeat, 0, 0, 0, false, false, 0, 0, 0, false, 0, 0, 1.0f, "", 0 };
            unsigned int now = (unsigned int)m_loopTimer.ElapsedMilliseconds();
            pong.x = (int)(now & 0xFFFF);
            pong.y = (int)((now >> 16) & 0xFFFF);
            m_network.SendPacket(pong);
        } else if (inPkt.type == NetMuxPacketType::Disconnect) {
            m_sync.RemovePeer(peerId);
            if (m_settings.isServer) m_network.SendPacketToGroup(inPkt, inPkt.groupId);
        } else if (inPkt.type == NetMuxPacketType::Wheel) {
            if (IsPeerTrusted(peerId, inPkt.type)) {
                m_driver.SendMouseWheel(inPkt.wheelDelta, inPkt.isHorizontalWheel);
                if (m_settings.isServer) m_network.SendPacketToGroup(inPkt, inPkt.groupId);
            }
        } else if (inPkt.type == NetMuxPacketType::SyncCheck) {
            if (m_settings.isServer) {
                // If we receive a sync check, we should potentially rebroadcast or reply with MasterStateSync
                // but for now we just refresh the peer's timeout.
                m_sync.RefreshPeer(peerId, inPkt.groupId);
            }
        } else if (inPkt.type == NetMuxPacketType::MasterStateSync) {
            // Server Authority: Update local perception of remote peer
            if (!m_settings.isServer) {
                m_sync.UpdatePeer(peerId, inPkt.groupId, inPkt.x, inPkt.y, inPkt.localTimestamp);
            }
        } else if (inPkt.type == NetMuxPacketType::FileHeader) {
            if (IsPeerTrusted(peerId, inPkt.type)) {
                m_fileTransfer.HandleFileHeader(inPkt);
                if (m_settings.isServer) m_network.SendPacketToGroup(inPkt, inPkt.groupId);
            }
        } else if (inPkt.type == NetMuxPacketType::FileData) {
            if (IsPeerTrusted(peerId, inPkt.type)) {
                m_fileTransfer.HandleFileData(inPkt);
                if (m_settings.isServer) m_network.SendPacketToGroup(inPkt, inPkt.groupId);
            }
        } else if (inPkt.type == NetMuxPacketType::KeyboardEvent) {
            if (IsPeerTrusted(peerId, inPkt.type)) {
                m_driver.SendKeyboardKey(inPkt.button, inPkt.down);
                if (m_settings.isServer) m_network.SendPacketToGroup(inPkt, inPkt.groupId);
            }
        }
    }
}

void NetMuxFramework::PerformSyncCheck() {
    if (m_settings.isServer) return;
    static double lastSyncCheck = 0;
    if (m_loopTimer.ElapsedMilliseconds() - lastSyncCheck > 500.0) {
        auto peers = m_sync.GetAllPeers();
        for (auto const& [id, peer] : peers) {
            Packet checkPkt = { m_localId, m_settings.groupId, m_sequenceCounter++, 0.0, NetMuxPacketType::SyncCheck, peer.normalizedX, peer.normalizedY, 0, false, false, 0, 0, 0, false, 0, 0, 1.0f, "", 0 };
            checkPkt.button = (int)id;
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
            Packet masterPkt = { id, peer.groupId, m_sequenceCounter++, m_loopTimer.ElapsedMilliseconds(), NetMuxPacketType::MasterStateSync, peer.normalizedX, peer.normalizedY, 0, false, false, 0, 0, 0, false, 0, 0, 1.0f, "", 0 };
            m_network.SendPacket(masterPkt);
        }
        lastMasterSync = m_loopTimer.ElapsedMilliseconds();
    }
}

void NetMuxFramework::PerformLatencySync() {
    if (m_loopTimer.ElapsedMilliseconds() - m_lastSyncTime > 1000.0) {
        Packet syncPkt = { m_localId, m_settings.groupId, m_sequenceCounter++, 0.0, NetMuxPacketType::Sync, 0, 0, 0, false, false, 0, 0, 0, false, 0, 0, 1.0f, "", 0 };
        m_network.SendPacket(syncPkt);
        m_syncTimer.Reset();
        m_lastSyncTime = m_loopTimer.ElapsedMilliseconds();
    }
    static double lastPing = 0;
    if (m_loopTimer.ElapsedMilliseconds() - lastPing > 2000.0) {
        Packet pingPkt = { m_localId, m_settings.groupId, m_sequenceCounter++, 0.0, NetMuxPacketType::Ping, 0, 0, 0, false, false, 0, 0, 0, false, 0, 0, 1.0f, "", 0 };
        m_network.SendPacket(pingPkt);
        lastPing = m_loopTimer.ElapsedMilliseconds();
    }
    static double lastHeartbeat = 0;
    if (m_loopTimer.ElapsedMilliseconds() - lastHeartbeat > 100.0) {
        Packet hbPkt = { m_localId, m_settings.groupId, m_sequenceCounter++, 0.0, NetMuxPacketType::Heartbeat, 0, 0, 0, false, false, 0, 0, 0, false, 0, 0, 1.0f, "", 0 };
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
        std::vector<unsigned long long> pruned = m_sync.PruneInactivePeers(10000.0);
        for (auto id : pruned) {
            m_authService.ClearPeer(id);
            m_fileTransfer.CleanupPeerTransfers(id);
            m_clipboard.CleanupPeer(id);
            m_clipboardReassembly.erase(id);
        }
        lastCleanup = m_loopTimer.ElapsedMilliseconds();
    }
}

#ifdef __linux__
void NetMuxFramework::ProcessX11Events() {
    if (!m_xDisplay) return;
    XEvent ev;
    while (XPending((Display*)m_xDisplay)) {
        XNextEvent((Display*)m_xDisplay, &ev);
        m_clipboard.HandleX11Event(&ev);
    }
}
#endif

void NetMuxFramework::PerformFileTransfer() {
    auto transfers = m_fileTransfer.GetActiveTransfers();
    for (auto const& [id, status] : transfers) {
        if (status.isOutgoing && !status.isComplete) {
            Packet pkt = { m_localId, m_settings.groupId, m_sequenceCounter++, m_loopTimer.ElapsedMilliseconds(), NetMuxPacketType::FileData, 0, 0, 0, false, false, 0, 0, 0, false, 0, 0, 1.0f, "", 0 };

            // Check if we need to send header first
            if (!m_fileTransfer.IsHeaderSent(id)) {
                if (m_fileTransfer.GetHeaderPacket(id, pkt)) {
                    m_network.SendPacket(pkt);
                    m_fileTransfer.SetHeaderSent(id, true);
                }
            } else {
                if (m_fileTransfer.GetNextChunk(id, pkt)) {
                    m_network.SendPacket(pkt);
                }
            }
        }
    }
}

void NetMuxFramework::PerformClipboardSync() {
    static double lastCheck = 0;
    if (m_loopTimer.ElapsedMilliseconds() - lastCheck < 500.0) return;
    lastCheck = m_loopTimer.ElapsedMilliseconds();
    if (m_clipboard.HasChanged()) {
        std::string text = m_clipboard.GetText();
        double now = m_loopTimer.ElapsedMilliseconds();
        const size_t CHUNK_SIZE = 4000;
        int totalChunks = (int)((text.size() + CHUNK_SIZE - 1) / CHUNK_SIZE);
        if (totalChunks == 0) totalChunks = 1;
        for (int i = 0; i < totalChunks; ++i) {
            size_t offset = i * CHUNK_SIZE;
            size_t remaining = text.size() - offset;
            size_t currentChunkSize = std::min(CHUNK_SIZE, remaining);
            Packet pkt = { m_localId, m_settings.groupId, m_sequenceCounter++, now, NetMuxPacketType::ClipboardSync, 0, 0, 0, false, false, 0, 0, 0, false, 0, 0, 1.0f, "", 0 };
            pkt.chunkIndex = i;
            pkt.totalChunks = totalChunks;
            if (currentChunkSize > 0) memcpy(pkt.payload, text.c_str() + offset, currentChunkSize);
            pkt.payloadSize = (int)currentChunkSize;
            m_network.SendPacket(pkt);
        }
    }
}
