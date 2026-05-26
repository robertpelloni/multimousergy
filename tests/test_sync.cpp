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

void test_simultaneous_edit_resolution() {
    std::cout << "Testing simultaneous edit resolution..." << std::endl;
    SyncModule sync;

    unsigned long long peerA = 1001;
    unsigned long long peerB = 1002;

    sync.UpdatePeer(peerA, 1, 0, 0);
    sync.UpdatePeer(peerB, 1, 0, 0);

    // Peer A has 10ms offset, Peer B has 50ms offset
    sync.UpdateLatency(peerA, 20.0);
    sync.UpdateClockOffset(peerA, 1000.0, 1020.0); // Offset = 10

    sync.UpdateLatency(peerB, 20.0);
    sync.UpdateClockOffset(peerB, 1000.0, 1060.0); // Offset = 50

    // Simulating simultaneous clicks
    double arrivalLocal = 2000.0;

    // Click from B arrived 'later' in physical time but occurred 'earlier'
    // Peer A click occurred at 2000 - 10 = 1990
    // Peer B click occurred at 2000 - 50 = 1950

    bool bWon = sync.ResolveInteraction(peerB, arrivalLocal);
    // Peer A click was at 1990, Peer B click was at 1950.
    // If A claims it first, B should steal it because B is older.

    sync.SetActivePeer(0); // Reset
    // Note: ResolveInteraction uses current clock state internally if available
    bool aClaimed = sync.ResolveInteraction(peerA, arrivalLocal);
    // m_lastActiveSwitch is now 1990 (arrivalLocal - offsetA = 2000 - 10)

    bool bStole = sync.ResolveInteraction(peerB, arrivalLocal);
    // Peer B adjusted is 1950. 1950 < 1990.

    std::cout << "Peer A (Adjusted 1990) claimed first. Peer B (Adjusted 1950) trying to steal..." << std::endl;
    std::cout << "Active Peer: " << sync.GetActivePeer() << " (Expected " << peerB << ")" << std::endl;

    assert(aClaimed == true);
    assert(bStole == true);
    assert(sync.GetActivePeer() == peerB);

    std::cout << "Simultaneous edit resolution verified." << std::endl;
}
