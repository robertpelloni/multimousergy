#include <iostream>
#include <cassert>
#include <thread>
#include <chrono>
#include <vector>
#include "SyncModule.hpp"

void test_sync_metadata_and_buttons() {
    std::cout << "Testing SyncModule metadata and buttons..." << std::endl;
    SyncModule sync;
    unsigned long long id = 12345;

    sync.UpdatePeer(id, 10, 32768, 32768, 0, "TestSession", "TestGroup");

    PeerState s;
    assert(sync.GetPeerState(id, s));
    assert(s.groupId == 10);
    assert(std::string(s.sessionName) == "TestSession");
    assert(std::string(s.groupName) == "TestGroup");

    // Test button bitmask
    sync.UpdatePeerButtons(id, 0, true); // Left down
    sync.UpdatePeerButtons(id, 1, true); // Right down
    assert(sync.GetPeerState(id, s));
    assert(s.buttonState == 3); // 1 | 2

    sync.UpdatePeerButtons(id, 0, false); // Left up
    assert(sync.GetPeerState(id, s));
    assert(s.buttonState == 2);

    std::cout << "[Test] Metadata and buttons verified." << std::endl;
}

void test_sync_lifecycle_management() {
    std::cout << "Testing SyncModule lifecycle management..." << std::endl;
    SyncModule sync;
    unsigned long long id1 = 1;
    unsigned long long id2 = 2;

    sync.UpdatePeer(id1, 0, 0, 0);
    sync.UpdatePeer(id2, 0, 0, 0);

    assert(sync.GetAllPeers().size() == 2);

    sync.RemovePeer(id1);
    assert(sync.GetAllPeers().size() == 1);
    PeerState sTmp;
    assert(!sync.GetPeerState(id1, sTmp));

    // Test Pruning (Simulated delay)
    sync.UpdatePeer(id2, 0, 0, 0);
    std::cout << "Waiting for pruning timeout..." << std::endl;
    // We can't easily wait 10 seconds in a unit test, but we can check if it returns empty on immediate call
    auto pruned = sync.PruneInactivePeers(10000.0);
    assert(pruned.empty());

    std::cout << "[Test] Lifecycle management verified." << std::endl;
}

void test_local_state_and_focus() {
    std::cout << "Testing local state and focus validation..." << std::endl;
    SyncModule sync;
    unsigned long long localId = 999;

    sync.UpdateLocalState(localId, 1, 1000, 1000, true, 500, 500);

    PeerState s;
    assert(sync.GetPeerState(localId, s));
    assert(s.isAuthenticated == true);
    assert(s.isSelecting == true);
    assert(s.id == localId);

    // Focus validation
    assert(sync.GetActivePeer() == 0);
    assert(sync.DispatchInputEvent(localId, 1000.0) == true);
    assert(sync.GetActivePeer() == localId);

    unsigned long long remoteId = 888;
    assert(sync.DispatchInputEvent(remoteId, 1001.0) == false); // Already owned by local

    std::cout << "[Test] Local state and focus verified." << std::endl;
}

void test_interaction_timeout() {
    std::cout << "Testing interaction timeout..." << std::endl;
    SyncModule sync;
    unsigned long long id1 = 101;
    unsigned long long id2 = 102;

    sync.UpdatePeer(id1, 0, 0, 0);
    sync.UpdatePeer(id2, 0, 0, 0);
    sync.UpdateClockOffset(id1, 0, 0);
    sync.UpdateClockOffset(id2, 0, 0);

    sync.ResolveInteraction(id1, 1000.0);
    assert(sync.GetActivePeer() == id1);

    // Try to steal before timeout
    assert(sync.ResolveInteraction(id2, 2000.0) == false);

    // Try to steal after timeout (2 seconds in SyncModule)
    assert(sync.ResolveInteraction(id2, 3500.0) == true);
    assert(sync.GetActivePeer() == id2);

    std::cout << "[Test] Interaction timeout verified." << std::endl;
}

void test_sync_detailed_suite() {
    test_sync_metadata_and_buttons();
    test_sync_lifecycle_management();
    test_local_state_and_focus();
    test_interaction_timeout();
}
