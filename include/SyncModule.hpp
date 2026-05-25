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
    unsigned int groupId;
    char groupName[64];
    char sessionName[64];
    float x; // Current denormalized screen coordinate
    float y;
    float targetX; // Target denormalized coordinate for interpolation
    float targetY;
    int screenWidth; // Remote screen resolution for normalization context
    int screenHeight;
    int normalizedX; // 0-65535
    int normalizedY;
    unsigned char colorR;
    unsigned char colorG;
    unsigned char colorB;
    double lastSeen;
    double latency;
    double e2eLatency;
    double clockOffset; // Offset to convert remote timestamps to local timeline
    bool isStalled;
    float vx; // Velocity pixels/ms
    float vy;
    std::vector<HistoryPoint> jitterBuffer;
};

class SyncModule {
public:
    SyncModule();
    ~SyncModule();

    void UpdatePeer(unsigned long long id, unsigned int groupId, int normX, int normY, double packetTimestamp = 0, const char* name = nullptr, const char* groupName = nullptr);
    void UpdatePeerResolution(unsigned long long id, int width, int height);
    void UpdateLatency(unsigned long long id, double latency);
    void UpdateClockOffset(unsigned long long id, double remoteTime, double localTime);

    bool GetPeerState(unsigned long long id, PeerState& state);
    std::map<unsigned long long, PeerState> GetAllPeers();

    static int Normalize(int val, int max);
    static int Denormalize(int val, int max);

    // Conflict Resolution: returns true if interaction is permitted
    bool ResolveConflict(unsigned long long id, double timestamp);

    // Focus validation for arbitrary input events
    bool DispatchInputEvent(unsigned long long id, double timestamp);

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
