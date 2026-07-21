#include <iostream>
#include <cassert>
#include <vector>
#include <thread>
#include <chrono>
#include "SyncModule.hpp"
#include "CommonTypes.hpp"

// This test simulates the interaction between multiple peers and authoritative server state.
void test_authoritative_sync_consistency() {
    std::cout << "Testing Authoritative MasterStateSync consistency..." << std::endl;
    SyncModule clientSync;
    unsigned long long remotePeerId = 1001;
    unsigned int groupId = 1;

    // 1. Initial State: Remote peer reported at (10000, 10000)
    clientSync.UpdatePeer(remotePeerId, groupId, 10000, 10000);

    PeerState state;
    assert(clientSync.GetPeerState(remotePeerId, state));
    assert(state.normalizedX == 10000);

    // 2. Simulate Local Drift/Prediction:
    // In a real scenario, the client might have predicted movement or missed an update.
    // We'll manually 'drift' the target in SyncModule (Simulated by a new movement packet from peer)
    clientSync.UpdatePeer(remotePeerId, groupId, 15000, 15000);
    assert(clientSync.GetPeerState(remotePeerId, state));
    assert(state.normalizedX == 15000);

    // 3. Authoritative Correction: Server sends MasterStateSync with correct position (12000)
    // Client-side NetMuxFramework calls UpdatePeer with MasterStateSync data.
    clientSync.UpdatePeer(remotePeerId, groupId, 12000, 12000);

    assert(clientSync.GetPeerState(remotePeerId, state));
    assert(state.normalizedX == 12000);
    std::cout << "[Test] MasterStateSync correction verified." << std::endl;
}

void test_multithreaded_sync_integrity() {
    std::cout << "Testing multithreaded SyncModule integrity..." << std::endl;
    SyncModule sync;
    const int numPeers = 20;
    const int updates = 1000;
    std::vector<std::thread> threads;

    for (int i = 0; i < numPeers; ++i) {
        threads.emplace_back([&sync, i, updates]() {
            unsigned long long id = 2000 + i;
            for (int j = 0; j < updates; ++j) {
                sync.UpdatePeer(id, 0, j, j);
            }
        });
    }

    for (auto& t : threads) t.join();

    auto peers = sync.GetAllPeers();
    assert(peers.size() == (size_t)numPeers);
    for (int i = 0; i < numPeers; ++i) {
        PeerState s;
        assert(sync.GetPeerState(2000 + i, s));
        assert(s.normalizedX == updates - 1);
    }
    std::cout << "[Test] Multithreaded integrity verified." << std::endl;
}

int main_consistency_tests() {
    test_authoritative_sync_consistency();
    test_multithreaded_sync_integrity();
    return 0;
}
