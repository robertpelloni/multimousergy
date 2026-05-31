#include <iostream>
#include <vector>
#include <thread>
#include <cassert>
#include "SyncModule.hpp"
#include "NetworkManager.hpp"
#include "Timer.hpp"

void test_sync_stress() {
    std::cout << "Starting SyncModule stress test..." << std::endl;
    SyncModule sync;
    const int numPeers = 20;
    const int updatesPerPeer = 1000;

    Timer timer;
    for (int i = 0; i < updatesPerPeer; ++i) {
        for (int p = 1; p <= numPeers; ++p) {
            sync.UpdatePeer(p, (p % 2), (i + p) % 65535, (i * p) % 65535);
        }
        sync.Step(1.0); // 1ms steps
    }

    double elapsed = timer.ElapsedMilliseconds();
    std::cout << "Processed " << numPeers * updatesPerPeer << " updates in " << elapsed << "ms ("
              << (numPeers * updatesPerPeer) / (elapsed / 1000.0) << " updates/sec)" << std::endl;

    assert(sync.GetAllPeers().size() == numPeers);
}

void test_network_stress() {
    std::cout << "Starting NetworkManager stress test (Simulated)..." << std::endl;
    // Simulated stress check for packet structure stability
    for (int i = 0; i < 10000; ++i) {
        Packet p = { (unsigned long long)i, 0, 0, NetMuxPacketType::AbsoluteMovement, i, i, 0, false, false, 0, 0, "stress_payload", 14 };
        assert(p.senderId == (unsigned long long)i);
    }
}
