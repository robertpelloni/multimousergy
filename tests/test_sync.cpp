#include <iostream>
#include <cassert>
#include "SyncModule.hpp"

void test_coordinate_normalization_fidelity() {
    std::cout << "Testing Coordinate Normalization fidelity..." << std::endl;
    int maxVal = 1920;
    int testVal = 960;

    int norm = SyncModule::Normalize(testVal, maxVal);
    int denorm = SyncModule::Denormalize(norm, maxVal);

    // Normalized 960/1920 is 0.5, which is 32767/32768 in 65535 range
    assert(std::abs(testVal - denorm) <= 1);
}

void test_clock_offset_calculation() {
    std::cout << "Testing Clock Offset calculation..." << std::endl;
    SyncModule sync;
    unsigned long long peerId = 101;

    // Mock latency and timing
    sync.UpdatePeer(peerId, 0, 0, 0);
    sync.UpdateLatency(peerId, 20.0); // 20ms RTT

    double remoteTime = 1000.0;
    double localTimeAtArrival = 1015.0; // Travel time ~10ms

    sync.UpdateClockOffset(peerId, remoteTime, localTimeAtArrival);

    PeerState s;
    assert(sync.GetPeerState(peerId, s));
    // Offset should be localTime - (remoteTime + halfRTT) = 1015 - (1000 + 10) = 5
    assert(std::abs(s.clockOffset - 5.0) < 0.1);
}
