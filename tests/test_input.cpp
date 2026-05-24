#include <iostream>
#include <cassert>
#include "InputEngine.hpp"

void test_boundary_logic() {
    std::cout << "Testing Input Boundary detection..." << std::endl;
    InputEngine input;
    Config cfg;
    cfg.boundaryX = 100;
    cfg.isLeft = true;

    input.Initialize(cfg);
    assert(input.IsAtBoundary(50, 50) == true);
    assert(input.IsAtBoundary(150, 50) == false);

    cfg.isLeft = false;
    input.Initialize(cfg);
    assert(input.IsAtBoundary(150, 50) == true);
    assert(input.IsAtBoundary(50, 50) == false);
}

void test_input_packet_queuing() {
    std::cout << "Testing Input Packet queuing..." << std::endl;
    InputEngine input;
    // Mock interaction logic or manual queue push if accessible
    // Since pendingPackets is private, we verify via framework or check state
}
