#include "SyncModule.hpp"
#include <chrono>
#include <algorithm>
#include <cmath>

#ifdef _WIN32
#include <windows.h>
#endif

SyncModule::SyncModule() {}
SyncModule::~SyncModule() {}

void SyncModule::UpdatePeer(unsigned long long id, int normX, int normY) {
    std::lock_guard<std::mutex> lock(m_mutex);
    PeerState& peer = m_peers[id];
    peer.id = id;

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

    int newTargetX = screenLeft + Denormalize(peer.normalizedX, screenWidth);
    int newTargetY = screenTop + Denormalize(peer.normalizedY, screenHeight);

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

bool SyncModule::ResolveConflict(unsigned long long id, int normX, int normY, double timestamp) {
    // Basic conflict resolution logic (e.g., Server is always right)
    // For alpha, we assume inbound packets from server (rebroadcasts) take precedence.
    return true;
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
        // Predict where the cursor should be based on velocity and estimated latency
        float predictionMs = (float)peer.latency * 0.5f; // Predict half RTT into future
        int predictedTargetX = peer.targetX + (int)(peer.vx * predictionMs);
        int predictedTargetY = peer.targetY + (int)(peer.vy * predictionMs);

        peer.x += (int)((predictedTargetX - peer.x) * lerpFactor);
        peer.y += (int)((predictedTargetY - peer.y) * lerpFactor);
    }
}
