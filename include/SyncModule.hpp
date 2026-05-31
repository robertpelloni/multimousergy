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
    bool isSelecting;
    float selStartX;
    float selStartY;
    float x; // Current denormalized screen coordinate
    float y;
    float targetX; // Target denormalized coordinate for interpolation
    float targetY;
    int screenWidth; // Remote screen resolution for normalization context
    int screenHeight;
    int normalizedX; // 0-65535
    int normalizedY;
    bool isAuthenticated;
    bool isConflictBlocked;
    int buttonState; // Bitmask of current button states
    unsigned char colorR;
    unsigned char colorG;
    unsigned char colorB;
    double lastSeen;
    double latency;
    double e2eLatency;
    float drift;
    double clockOffset; // Offset to convert remote timestamps to local timeline
    bool isStalled;
    float vx; // Velocity pixels/ms
    float vy;
    unsigned int totalPacketsReceived;
    double lastAuthTime;
    std::vector<HistoryPoint> jitterBuffer;
};

class SyncModule {
public:
    SyncModule();
    ~SyncModule();

    void UpdatePeer(unsigned long long id, unsigned int groupId, int normX, int normY, double packetTimestamp = 0, const char* name = nullptr, const char* groupName = nullptr);
    void UpdatePeerButtons(unsigned long long id, int button, bool down);
    void UpdatePeerSelection(unsigned long long id, bool selecting, int startX, int startY);
    void SetAuthenticated(unsigned long long id, bool auth);
    void UpdatePeerResolution(unsigned long long id, int width, int height);
    void UpdateDrift(unsigned long long id, float drift);
    void UpdateLatency(unsigned long long id, double latency);
    void UpdateClockOffset(unsigned long long id, double remoteTime, double localTime);

    // Identifies and removes peers that haven't sent updates within the timeout.
    // Returns a list of pruned peer IDs.
    std::vector<unsigned long long> PruneInactivePeers(double timeoutMs);
    void RemovePeer(unsigned long long id);

    void UpdateLocalState(unsigned long long localId, unsigned int groupId, int normX, int normY, bool isSelecting, int selStartX, int selStartY);

    bool GetPeerState(unsigned long long id, PeerState& state);
    std::map<unsigned long long, PeerState> GetAllPeers();

    static int Normalize(int val, int max);
    static int Denormalize(int val, int max);

    // Interaction Ownership: returns true if interaction is permitted
    bool ResolveInteraction(unsigned long long id, double timestamp, bool isRelease = false);

    // Focus validation for arbitrary input events
    bool DispatchInputEvent(unsigned long long id, double timestamp);

    // Interpolation step
    void Step(double deltaTime);

    void SetActivePeer(unsigned long long id);
    unsigned long long GetActivePeer() const { return m_activePeerId; }
    unsigned long long GetLocalId() const { return m_localId; }

private:
    std::map<unsigned long long, PeerState> m_peers;
    unsigned long long m_activePeerId = 0;
    unsigned long long m_localId = 0;
    double m_lastActiveSwitch = 0;
    std::mutex m_mutex;
};
