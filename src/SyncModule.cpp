#include "SyncModule.hpp"
#include <chrono>
#include <algorithm>
#include <cmath>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#endif

SyncModule::SyncModule() {}
SyncModule::~SyncModule() {}

void SyncModule::UpdatePeer(unsigned long long id, unsigned int groupId, int normX, int normY, double packetTimestamp, const char* name, const char* gname) {
    std::lock_guard<std::mutex> lock(m_mutex);
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
    static auto startTime = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    double timestamp = std::chrono::duration<double, std::milli>(now - startTime).count();

    peer.jitterBuffer.push_back({normX, normY, timestamp});

    // Keep buffer small (e.g. 5 points)
    if (peer.jitterBuffer.size() > 5) {
        peer.jitterBuffer.erase(peer.jitterBuffer.begin());
    }

    // Use the oldest point in the buffer for a consistent delay
    // This acts as a jitter compensation mechanism.
    HistoryPoint target = peer.jitterBuffer.front();

    peer.normalizedX = target.nx;
    peer.normalizedY = target.ny;

    int screenWidth = 1920, screenHeight = 1080;
    int screenLeft = 0, screenTop = 0;
#ifdef _WIN32
    // Handle Virtual Screen (Multi-Monitor)
    screenLeft = GetSystemMetrics(SM_XVIRTUALSCREEN);
    screenTop = GetSystemMetrics(SM_YVIRTUALSCREEN);
    screenWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    screenHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);
#endif

    float newTargetX = (float)(screenLeft + Denormalize(peer.normalizedX, screenWidth));
    float newTargetY = (float)(screenTop + Denormalize(peer.normalizedY, screenHeight));

    // Calculate velocity for prediction
    double dt = timestamp - peer.lastSeen;
    if (dt > 0) {
        peer.vx = (float)(newTargetX - peer.targetX) / (float)dt;
        peer.vy = (float)(newTargetY - peer.targetY) / (float)dt;
    }

    peer.targetX = newTargetX;
    peer.targetY = newTargetY;

    peer.lastSeen = timestamp;
    peer.isStalled = false;

    if (packetTimestamp > 0) {
        peer.e2eLatency = timestamp - packetTimestamp;
    }

    // If it's the first update, snap immediately
    if (peer.x == 0 && peer.y == 0) {
        peer.x = peer.targetX;
        peer.y = peer.targetY;
    }

    // Assign a default color based on ID if not set
    if (peer.colorR == 0 && peer.colorG == 0 && peer.colorB == 0) {
        peer.colorR = (unsigned char)((id * 50) % 255);
        peer.colorG = (unsigned char)((id * 80) % 255);
        peer.colorB = (unsigned char)((id * 110) % 255);
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

bool SyncModule::ResolveConflict(unsigned long long id, double timestamp) {
    std::lock_guard<std::mutex> lock(m_mutex);

    // Timestamp-First model for simultaneous interaction:
    // We adjust the incoming remote timestamp by its clock offset
    // to place it on our local unified timeline.
    double adjustedTimestamp = timestamp;
    if (m_peers.count(id)) {
        adjustedTimestamp += m_peers[id].clockOffset;
    }

    // First-to-Claim priority for interaction (clicks)
    if (m_activePeerId == 0) {
        m_activePeerId = id;
        m_lastActiveSwitch = adjustedTimestamp;
        return true;
    }

    if (m_activePeerId == id) {
        m_lastActiveSwitch = adjustedTimestamp;
        return true;
    }

    // SIMULTANEOUS EDITING LOGIC:
    // If a click arrives with an EARLIER adjusted timestamp than our
    // current active owner's start time, and it's within a small 'race window' (e.g. 100ms),
    // we allow the switch.
    if (adjustedTimestamp < m_lastActiveSwitch && (m_lastActiveSwitch - adjustedTimestamp < 100.0)) {
        m_activePeerId = id;
        m_lastActiveSwitch = adjustedTimestamp;
        return true;
    }

    // If another peer is active, check for 2-second interaction timeout
    if (adjustedTimestamp - m_lastActiveSwitch > 2000.0) {
        m_activePeerId = id;
        m_lastActiveSwitch = adjustedTimestamp;
        return true;
    }

    return false; // Conflict: Focus is owned by someone else with earlier priority
}

void SyncModule::SetActivePeer(unsigned long long id) {
    std::lock_guard<std::mutex> lock(m_mutex);

    // Master Lock: Prevent rapid switching (500ms cooldown)
    static auto startTime = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    double currentMs = std::chrono::duration<double, std::milli>(now - startTime).count();

    if (currentMs - m_lastActiveSwitch > 500.0) {
        m_activePeerId = id;
        m_lastActiveSwitch = currentMs;
    }
}

void SyncModule::Step(double deltaTime) {
    std::lock_guard<std::mutex> lock(m_mutex);

    static auto startTime = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    double currentMs = std::chrono::duration<double, std::milli>(now - startTime).count();

    // Coordinated smoothing factor based on deltaTime
    // Ensuring frame-rate independent interpolation
    float lerpFactor = 1.0f - std::pow(0.01f, (float)deltaTime / 1000.0f);
    if (lerpFactor > 1.0f) lerpFactor = 1.0f;
    if (lerpFactor < 0.1f) lerpFactor = 0.1f; // Minimum smoothing

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

        peer.x += (predictedTargetX - peer.x) * lerpFactor;
        peer.y += (predictedTargetY - peer.y) * lerpFactor;
    }
}
