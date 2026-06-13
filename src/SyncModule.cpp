#include "SyncModule.hpp"
#include <chrono>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

static auto s_syncStartTime = std::chrono::steady_clock::now();

SyncModule::SyncModule() {}
SyncModule::~SyncModule() {}

void SyncModule::SetAuthenticated(unsigned long long id, bool auth) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_peers.count(id)) {
        m_peers[id].isAuthenticated = auth;
        if (auth) {
            auto now = std::chrono::steady_clock::now();
            m_peers[id].lastAuthTime = std::chrono::duration<double, std::milli>(now - s_syncStartTime).count();
        }
    }
}

void SyncModule::UpdatePeerResolution(unsigned long long id, int width, int height) {
    std::lock_guard<std::mutex> lock(m_mutex);
    PeerState& peer = m_peers[id];
    peer.screenWidth = width;
    peer.screenHeight = height;
}

void SyncModule::UpdateDrift(unsigned long long id, float drift) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_peers.count(id)) {
        m_peers[id].drift = drift;
    }
}

void SyncModule::UpdatePeerButtons(unsigned long long id, int button, bool down) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_peers.count(id)) {
        if (down) m_peers[id].buttonState |= (1 << button);
        else m_peers[id].buttonState &= ~(1 << button);
    }
}

void SyncModule::UpdatePeerSelection(unsigned long long id, bool selecting, int startX, int startY) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_peers.count(id)) {
        PeerState& peer = m_peers[id];
        peer.isSelecting = selecting;

        int sw = 1920, sh = 1080;
#ifdef _WIN32
        sw = GetSystemMetrics(SM_CXVIRTUALSCREEN);
        sh = GetSystemMetrics(SM_CYVIRTUALSCREEN);
#endif
        peer.selStartX = (float)Denormalize(startX, sw);
        peer.selStartY = (float)Denormalize(startY, sh);
    }
}

void SyncModule::UpdatePeer(unsigned long long id, unsigned int groupId, int normX, int normY, double packetTimestamp, const char* name, const char* gname) {
    std::lock_guard<std::mutex> lock(m_mutex);
    bool isNew = (m_peers.find(id) == m_peers.end());
    PeerState& peer = m_peers[id];
    peer.id = id;
    peer.groupId = groupId;
    if (name) {
        strncpy(peer.sessionName, name, sizeof(peer.sessionName) - 1);
        peer.sessionName[sizeof(peer.sessionName) - 1] = '\0';
    }
    if (gname) {
        strncpy(peer.groupName, gname, sizeof(peer.groupName) - 1);
        peer.groupName[sizeof(peer.groupName) - 1] = '\0';
    }

    // Add to jitter buffer with current local timestamp
    auto now = std::chrono::steady_clock::now();
    double timestamp = std::chrono::duration<double, std::milli>(now - s_syncStartTime).count();

    peer.jitterBuffer.push_back({normX, normY, timestamp});

    // LOW LATENCY MODE: Keep buffer very small (1 point for immediate movement)
    if (peer.jitterBuffer.size() > 1) {
        peer.jitterBuffer.erase(peer.jitterBuffer.begin());
    }

    // Use the oldest point in the buffer for a consistent delay
    // This acts as a jitter compensation mechanism.
    HistoryPoint target = peer.jitterBuffer.front();

    peer.normalizedX = target.nx;
    peer.normalizedY = target.ny;

    // LOCAL CONTEXT: Use our resolution for denormalization
    int localScreenWidth = 1920, localScreenHeight = 1080;
    int screenLeft = 0, screenTop = 0;
#ifdef _WIN32
    // Handle Virtual Screen (Multi-Monitor)
    screenLeft = GetSystemMetrics(SM_XVIRTUALSCREEN);
    screenTop = GetSystemMetrics(SM_YVIRTUALSCREEN);
    localScreenWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    localScreenHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);
#endif

    // SCALE RESOLUTION: Aspect-ratio aware scaling if remote resolution is known.
    float denormX = (float)Denormalize(peer.normalizedX, localScreenWidth);
    float denormY = (float)Denormalize(peer.normalizedY, localScreenHeight);

    if (peer.screenWidth > 0 && peer.screenHeight > 0) {
        float scaleX = (float)localScreenWidth / (float)peer.screenWidth;
        float scaleY = (float)localScreenHeight / (float)peer.screenHeight;

        // Use uniform scaling to preserve aspect ratio if desired,
        // but for KM tools, direct mapping to screen percentage is usually preferred.
        // We'll stick to percentage-based mapping (Denormalize already does this).
    }

    float newTargetX = (float)(screenLeft + denormX);
    float newTargetY = (float)(screenTop + denormY);

    // Calculate velocity for prediction
    double dt = timestamp - peer.lastSeen;
    if (dt > 0) {
        peer.vx = (float)(newTargetX - peer.targetX) / (float)dt;
        peer.vy = (float)(newTargetY - peer.targetY) / (float)dt;
    }

    peer.targetX = newTargetX;
    peer.targetY = newTargetY;

    if (isNew) {
        std::cout << "[Sync] New peer joined: " << id << (name ? " (" : "") << (name ? name : "") << (name ? ")" : "") << std::endl;
    }

    peer.lastSeen = timestamp;
    peer.isConflictBlocked = false; // Reset block status on movement
    // By default, new peers are not authenticated
    if (peer.id != 0 && peer.lastSeen == timestamp) {
         // peer.isAuthenticated = false; // Initialized by compiler or map ctor
    }
    peer.isStalled = false;

    if (packetTimestamp > 0) {
        peer.e2eLatency = timestamp - packetTimestamp;
    }
    peer.totalPacketsReceived++;

    // If it's the first update, snap immediately
    if (peer.x == 0 && peer.y == 0) {
        peer.x = peer.targetX;
        peer.y = peer.targetY;
    }

    // Assign a default color based on ID if not set
    if (peer.colorR == 0 && peer.colorG == 0 && peer.colorB == 0) {
        peer.colorR = (unsigned char)(((id * 50) % 200) + 55);
        peer.colorG = (unsigned char)(((id * 80) % 200) + 55);
        peer.colorB = (unsigned char)(((id * 110) % 200) + 55);
    }
}

void SyncModule::RefreshPeer(unsigned long long id, unsigned int groupId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_peers.count(id)) {
        m_peers[id].lastSeen = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - s_syncStartTime).count();
        m_peers[id].isStalled = false;
    } else {
        // If they don't exist, we can't really "refresh" them with a valid position.
        // We'll let the next AbsoluteMovement create them.
    }
}

void SyncModule::UpdateLatency(unsigned long long id, double latency) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_peers.count(id)) {
        m_peers[id].latency = latency;
    }
}

void SyncModule::UpdateClockOffset(unsigned long long id, double remoteTime, double localTime) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_peers.count(id)) {
        PeerState& peer = m_peers[id];
        // RemoteTime arrived at LocalTime.
        // Estimated travel time is latency/2 (half RTT).
        double travelTime = peer.latency * 0.5;
        double estimatedRemoteTimeAtArrival = remoteTime + travelTime;
        peer.clockOffset = localTime - estimatedRemoteTimeAtArrival;
    }
}

double SyncModule::GetAdjustedTimestamp(unsigned long long id, double remoteTimestamp) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_peers.count(id)) {
        return remoteTimestamp - m_peers[id].clockOffset;
    }
    return remoteTimestamp;
}

std::vector<unsigned long long> SyncModule::PruneInactivePeers(double timeoutMs) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<unsigned long long> pruned;

    // Use current time from the same steady_clock reference as UpdatePeer
    double currentMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - s_syncStartTime).count();

    for (auto it = m_peers.begin(); it != m_peers.end(); ) {
        if (it->first != m_localId && (currentMs - it->second.lastSeen) > timeoutMs) {
            std::cout << "[Sync] Pruning inactive peer: " << it->first << " (" << it->second.sessionName << ")" << std::endl;
            pruned.push_back(it->first);
            if (m_activePeerId == it->first) m_activePeerId = 0;
            it = m_peers.erase(it);
        } else {
            ++it;
        }
    }
    return pruned;
}

void SyncModule::RemovePeer(unsigned long long id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_peers.count(id)) {
        std::cout << "[Sync] Removing peer: " << id << " (" << m_peers[id].sessionName << ")" << std::endl;
        m_peers.erase(id);
    }
    if (m_activePeerId == id) m_activePeerId = 0;
}

void SyncModule::UpdateLocalState(unsigned long long localId, unsigned int groupId, int normX, int normY, bool isSelecting, int startX, int startY) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_localId = localId;
    PeerState& peer = m_peers[localId];
    peer.id = localId;
    peer.groupId = groupId;
    peer.normalizedX = normX;
    peer.normalizedY = normY;
    peer.isSelecting = isSelecting;

    int sw = 1920, sh = 1080;
    int sl = 0, st = 0;
#ifdef _WIN32
    sl = GetSystemMetrics(SM_XVIRTUALSCREEN);
    st = GetSystemMetrics(SM_YVIRTUALSCREEN);
    sw = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    sh = GetSystemMetrics(SM_CYVIRTUALSCREEN);
#endif

    peer.targetX = (float)(sl + Denormalize(normX, sw));
    peer.targetY = (float)(st + Denormalize(normY, sh));
    
    // Snap local state immediately for responsiveness
    peer.x = peer.targetX;
    peer.y = peer.targetY;
    
    peer.isAuthenticated = true; // Local is always authenticated

    if (isSelecting) {
        peer.selStartX = (float)(sl + Denormalize(startX, sw));
        peer.selStartY = (float)(st + Denormalize(startY, sh));
    }

    peer.lastSeen = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - s_syncStartTime).count();
    
    if (peer.colorR == 0 && peer.colorG == 0 && peer.colorB == 0) {
        peer.colorR = 255; peer.colorG = 255; peer.colorB = 255; // White for local
    }
}

bool SyncModule::GetPeerState(unsigned long long id, PeerState& state) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_peers.count(id)) {
        state = m_peers[id];
        return true;
    }
    return false;
}

std::map<unsigned long long, PeerState> SyncModule::GetAllPeers() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_peers;
}

int SyncModule::Normalize(int val, int max) {
    if (max <= 1) return 0;
    return (val * 65535) / (max - 1);
}

int SyncModule::Denormalize(int val, int max) {
    if (max <= 1) return 0;
    return (val * (max - 1)) / 65535;
}

bool SyncModule::ResolveInteraction(unsigned long long id, double timestamp, bool isRelease) {
    std::lock_guard<std::mutex> lock(m_mutex);

    // Interaction Ownership Model:
    // Uses unified timelines to handle simultaneous edits.
    double adjustedTimestamp = timestamp;
    if (m_peers.count(id)) {
        adjustedTimestamp -= m_peers[id].clockOffset; // SUBTRACT offset to get physical occurrence time
    }

    if (isRelease) {
        if (m_activePeerId == id) {
            // Soft Release: Keep focus for a small cooldown to prevent jitter
            m_lastActiveSwitch = adjustedTimestamp;
        }
        return true;
    }

    // Initial claim
    if (m_activePeerId == 0) {
        m_activePeerId = id;
        m_lastActiveSwitch = adjustedTimestamp;
        return true;
    }

    // Ownership check
    if (m_activePeerId == id) {
        m_lastActiveSwitch = adjustedTimestamp;
        return true;
    }

    // Conflict: Simultaneous Click Resolution
    // If another click arrived with an earlier timestamp during the race window,
    // we must respect the physical order of events.
    if (adjustedTimestamp < m_lastActiveSwitch) {
        // GESTURE INTEGRITY: Prevent focus theft if the current owner is actively selecting/dragging.
        if (m_peers.count(m_activePeerId) && m_peers[m_activePeerId].isSelecting) {
            std::cout << "[Sync] Conflict: " << id << " denied. Current owner " << m_activePeerId << " is in active gesture." << std::endl;
            return false;
        }

        // Preemptive Switch: The new interaction is actually older (occurred first)
        std::cout << "[Sync] Preemptive Ownership: " << id << " stealing from " << m_activePeerId << " (Diff: " << (m_lastActiveSwitch - adjustedTimestamp) << "ms)" << std::endl;
        m_activePeerId = id;
        m_lastActiveSwitch = adjustedTimestamp;
        return true;
    }

    // Interaction Timeout (2 seconds)
    if (adjustedTimestamp - m_lastActiveSwitch > 2000.0) {
        m_activePeerId = id;
        m_lastActiveSwitch = adjustedTimestamp;
        return true;
    }

    // If we reached here, focus is owned by someone else
    if (m_peers.count(id)) m_peers[id].isConflictBlocked = true;
    return false;
}

bool SyncModule::DispatchInputEvent(unsigned long long id, double timestamp) {
    std::lock_guard<std::mutex> lock(m_mutex);

    // Only allow input events from the currently active focus owner
    // This provides a consistent single-user control model for complex actions.
    if (m_activePeerId == id) {
        m_lastActiveSwitch = timestamp; // Reset timeout
        return true;
    }

    // If no one is active, allow the new user to claim it
    if (m_activePeerId == 0) {
        m_activePeerId = id;
        m_lastActiveSwitch = timestamp;
        return true;
    }

    return false;
}

void SyncModule::SetActivePeer(unsigned long long id) {
    std::lock_guard<std::mutex> lock(m_mutex);

    // Master Lock: Prevent rapid switching (500ms cooldown)
    auto now = std::chrono::steady_clock::now();
    double currentMs = std::chrono::duration<double, std::milli>(now - s_syncStartTime).count();

    m_activePeerId = id;
    m_lastActiveSwitch = currentMs;
}

void SyncModule::Step(double deltaTime) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto now = std::chrono::steady_clock::now();
    double currentMs = std::chrono::duration<double, std::milli>(now - s_syncStartTime).count();

    // Coordinated smoothing factor based on deltaTime
    // Ensuring frame-rate independent interpolation
    float lerpFactor = 1.0f - std::pow(0.0000001f, (float)deltaTime / 1000.0f);
    if (lerpFactor > 1.0f) lerpFactor = 1.0f;
    if (lerpFactor < 0.4f) lerpFactor = 0.4f; // High minimum smoothing for responsiveness

    for (auto& [id, peer] : m_peers) {
        // Stall detection (3 seconds timeout)
        if (currentMs - peer.lastSeen > 3000.0) {
            peer.isStalled = true;
            peer.vx = 0; peer.vy = 0;
        }

        // Apply Prediction (Dead Reckoning)
        // Predict where the cursor should be based on velocity and synchronized latency.
        // We use the clock offset to calculate exactly how 'old' the target coordinate is
        // relative to the unified timeline.
        double packetAge = currentMs - (peer.lastSeen + peer.clockOffset);
        if (packetAge < 0) packetAge = 0;

        float predictionMs = (float)packetAge + (float)peer.latency * 0.5f;
        int predictedTargetX = (int)peer.targetX + (int)(peer.vx * predictionMs);
        int predictedTargetY = (int)peer.targetY + (int)(peer.vy * predictionMs);

        // ADAPTIVE SMOOTHING:
        // Increase responsiveness (lerpFactor) if drift is high, or if latency is very low.
        float adaptiveLerp = lerpFactor;
        if (peer.drift > 500) adaptiveLerp = std::min(1.0f, adaptiveLerp * 2.0f);
        if (peer.latency < 5.0) adaptiveLerp = std::min(1.0f, adaptiveLerp * 1.5f);

        peer.x += (predictedTargetX - peer.x) * adaptiveLerp;
        peer.y += (predictedTargetY - peer.y) * adaptiveLerp;
    }
}
