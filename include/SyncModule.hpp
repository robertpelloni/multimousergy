#pragma once
#include <map>
#include <mutex>
#include <vector>
#include "NetworkManager.hpp"

struct HistoryPoint {
    int nx;
    int ny;
    double timestamp;
};

struct PeerState {
    unsigned long long id;
    float x; // Current denormalized screen coordinate
    float y;
    float targetX; // Target denormalized coordinate for interpolation
    float targetY;
    int normalizedX; // 0-65535
    int normalizedY;
    unsigned char colorR;
    unsigned char colorG;
    unsigned char colorB;
    double lastSeen;
    double latency;
    double e2eLatency;
    bool isStalled;
    float vx; // Velocity pixels/ms
    float vy;
    std::vector<HistoryPoint> jitterBuffer;
};

class SyncModule {
public:
    SyncModule();
    ~SyncModule();

    void UpdatePeer(unsigned long long id, int normX, int normY, double packetTimestamp = 0);
    void UpdateLatency(unsigned long long id, double latency);

    bool GetPeerState(unsigned long long id, PeerState& state);
    std::map<unsigned long long, PeerState> GetAllPeers();

    static int Normalize(int val, int max);
    static int Denormalize(int val, int max);

    // Conflict Resolution: returns true if local state should be overridden
    bool ResolveConflict(unsigned long long id, int normX, int normY, double timestamp);

    // Interpolation step
    void Step(double deltaTime);

    void SetActivePeer(unsigned long long id);
    unsigned long long GetActivePeer() const { return m_activePeerId; }

private:
    std::map<unsigned long long, PeerState> m_peers;
    unsigned long long m_activePeerId = 0;
    double m_lastActiveSwitch = 0;
    std::mutex m_mutex;
};
