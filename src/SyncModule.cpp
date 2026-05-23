#include "SyncModule.hpp"
#include <chrono>

#ifdef _WIN32
#include <windows.h>
#endif

SyncModule::SyncModule() {}
SyncModule::~SyncModule() {}

void SyncModule::UpdatePeer(unsigned long long id, int normX, int normY) {
    std::lock_guard<std::mutex> lock(m_mutex);
    PeerState& peer = m_peers[id];
    peer.id = id;
    peer.normalizedX = normX;
    peer.normalizedY = normY;

    int screenWidth = 1920, screenHeight = 1080;
#ifdef _WIN32
    screenWidth = GetSystemMetrics(SM_CXSCREEN);
    screenHeight = GetSystemMetrics(SM_CYSCREEN);
#endif

    peer.x = Denormalize(normX, screenWidth);
    peer.y = Denormalize(normY, screenHeight);

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
