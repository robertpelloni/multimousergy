#include <iostream>
#include <cassert>
#include <fstream>
#include <cstring>
#include "FileTransferEngine.hpp"
#include "AuthModule.hpp"
#include <filesystem>

void test_file_transfer_engine() {
    std::cout << "Running FileTransferEngine integration tests..." << std::endl;

    // 1. Setup mock files
    std::string testFile = "test_transfer.bin";
    std::string testFileLarge = "test_transfer_large.bin";

    std::string mockData = "NetMux Mock File Transfer Data Verification 1234567890.";
    std::ofstream out(testFile, std::ios::binary);
    out.write(mockData.c_str(), mockData.size());
    out.close();

    // Create a mock large file (e.g. 10MB) to test chunking and edge cases
    std::vector<char> largeData(10 * 1024 * 1024, 'A');
    std::ofstream outLarge(testFileLarge, std::ios::binary);
    outLarge.write(largeData.data(), largeData.size());
    outLarge.close();

    // Calculate expected hash
    unsigned char expectedHashBytes[32];
    AuthModule::ComputeSHA256(mockData, expectedHashBytes);
    std::string expectedHash = AuthModule::HashToHex(expectedHashBytes);

    // 2. Instantiate Outgoing and Incoming Engines
    FileTransferEngine sender;
    FileTransferEngine receiver;

    // --- TEST 1: STANDARD TRANSFER ---
    unsigned long long peerId = 999;
    assert(sender.StartTransfer(testFile, peerId));
    auto active = sender.GetActiveTransfers();
    assert(!active.empty());
    unsigned long long transferId = active.begin()->first;

    Packet headerPkt;
    assert(sender.GetHeaderPacket(transferId, headerPkt));
    assert(receiver.HandleFileHeader(headerPkt));

    Packet dataPkt;
    while (sender.GetNextChunk(transferId, dataPkt)) {
        assert(receiver.HandleFileData(dataPkt));
    }

    auto incomingActive = receiver.GetActiveTransfers();
    assert(!incomingActive.empty());
    auto incomingStatus = incomingActive[transferId];
    assert(incomingStatus.isComplete == true);
    assert(incomingStatus.lastError == FileTransferError::None);

    // --- TEST 2: INTERRUPTED TRANSFER (RESUME) ---
    std::cout << "Testing Interrupted Transfer Resume..." << std::endl;
    assert(sender.StartTransfer(testFileLarge, peerId));
    auto activeLarge = sender.GetActiveTransfers();
    unsigned long long transferIdLarge = 0;
    for (auto const& [id, t] : activeLarge) {
        if (t.filename == "test_transfer_large.bin") transferIdLarge = id;
    }
    assert(transferIdLarge != 0);

    Packet headerPktLarge;
    assert(sender.GetHeaderPacket(transferIdLarge, headerPktLarge));
    assert(receiver.HandleFileHeader(headerPktLarge));

    // Send half the chunks
    int chunkCount = 0;
    while (sender.GetNextChunk(transferIdLarge, dataPkt)) {
        assert(receiver.HandleFileData(dataPkt));
        chunkCount++;
        if (chunkCount >= 1000) break; // Interrupt!
    }

    // Simulate connection drop and resume: receiver gets the header again
    assert(receiver.HandleFileHeader(headerPktLarge)); // Should log "Resuming incoming transfer" and return true

    // Complete the remaining chunks
    while (sender.GetNextChunk(transferIdLarge, dataPkt)) {
        assert(receiver.HandleFileData(dataPkt));
    }

    auto incomingActiveLarge = receiver.GetActiveTransfers();
    auto incomingStatusLarge = incomingActiveLarge[transferIdLarge];
    assert(incomingStatusLarge.isComplete == true);
    assert(incomingStatusLarge.lastError == FileTransferError::None);

    // 7. Cleanup
    std::filesystem::remove(testFile);
    std::filesystem::remove("received_" + testFile);
    std::filesystem::remove(testFileLarge);
    std::filesystem::remove("received_" + testFileLarge);

    std::cout << "FileTransferEngine integration tests passed successfully!" << std::endl;
}
