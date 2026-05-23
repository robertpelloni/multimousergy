#pragma once
#include <map>
#include <mutex>
#include "NetworkManager.hpp"

struct PeerState {
    unsigned long long id;
    int x; // Denormalized screen coordinate
    int y;
    int normalizedX; // 0-65535
    int normalizedY;
    unsigned char colorR;
    unsigned char colorG;
    unsigned char colorB;
    double lastSeen;
    double latency;
};

class SyncModule {
public:
    SyncModule();
    ~SyncModule();

    void UpdatePeer(unsigned long long id, int normX, int normY);
    void UpdateLatency(unsigned long long id, double latency);

    bool GetPeerState(unsigned long long id, PeerState& state);
    std::map<unsigned long long, PeerState> GetAllPeers();

    static int Normalize(int val, int max);
    static int Denormalize(int val, int max);

    // Conflict Resolution: returns true if local state should be overridden
    bool ResolveConflict(unsigned long long id, int normX, int normY, double timestamp);

    void SetActivePeer(unsigned long long id) { m_activePeerId = id; }
    unsigned long long GetActivePeer() const { return m_activePeerId; }

private:
    std::map<unsigned long long, PeerState> m_peers;
    unsigned long long m_activePeerId = 0;
    std::mutex m_mutex;
};
