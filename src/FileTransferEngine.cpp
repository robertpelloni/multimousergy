#include "FileTransferEngine.hpp"
#include "AuthModule.hpp"
#include <iostream>
#include <filesystem>
#include <cstring>

namespace fs = std::filesystem;

FileTransferEngine::FileTransferEngine() {}

FileTransferEngine::~FileTransferEngine() {}

bool FileTransferEngine::StartTransfer(const std::string& filePath, unsigned long long targetPeerId) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!fs::exists(filePath)) return false;

    FileTransfer transfer;
    transfer.filename = fs::path(filePath).filename().string();
    transfer.fileSize = fs::file_size(filePath);
    transfer.localPath = filePath;
    transfer.bytesTransferred = 0;
    transfer.isOutgoing = true;
    transfer.isComplete = false;
    transfer.hash = CalculateHash(filePath);

    unsigned long long transferId = m_nextTransferId++;
    m_transfers[transferId] = transfer;
    return true;
}

bool FileTransferEngine::GetHeaderPacket(unsigned long long transferId, Packet& outPkt) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_transfers.find(transferId);
    if (it == m_transfers.end() || !it->second.isOutgoing) return false;

    FileTransfer& t = it->second;
    outPkt.type = NetMuxPacketType::FileHeader;
    std::string meta = t.filename + "|" + std::to_string(t.fileSize) + "|" + t.hash;
    std::strncpy(outPkt.payload, meta.c_str(), sizeof(outPkt.payload) - 1);
    outPkt.payloadSize = (int)meta.size();

    // Split transferId across x and y to avoid truncation
    outPkt.x = (int)(transferId & 0xFFFFFFFF);
    outPkt.y = (int)((transferId >> 32) & 0xFFFFFFFF);

    return true;
}

bool FileTransferEngine::GetNextChunk(unsigned long long transferId, Packet& outPkt) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_transfers.find(transferId);
    if (it == m_transfers.end() || !it->second.isOutgoing || it->second.isComplete) return false;

    FileTransfer& t = it->second;
    std::ifstream file(t.localPath, std::ios::binary);
    file.seekg(t.bytesTransferred);

    const size_t CHUNK_SIZE = 4000;
    char buffer[CHUNK_SIZE];
    file.read(buffer, CHUNK_SIZE);
    std::streamsize bytesRead = file.gcount();

    if (bytesRead > 0) {
        outPkt.type = NetMuxPacketType::FileData;
        outPkt.chunkIndex = (int)(t.bytesTransferred / CHUNK_SIZE);
        outPkt.totalChunks = (int)((t.fileSize + CHUNK_SIZE - 1) / CHUNK_SIZE);
        outPkt.payloadSize = (int)bytesRead;
        std::memcpy(outPkt.payload, buffer, bytesRead);

        outPkt.x = (int)(transferId & 0xFFFFFFFF);
        outPkt.y = (int)((transferId >> 32) & 0xFFFFFFFF);

        t.bytesTransferred += bytesRead;
        if (t.bytesTransferred >= t.fileSize) {
            t.isComplete = true;
        }
        return true;
    }

    return false;
}

bool FileTransferEngine::HandleFileHeader(const Packet& pkt) {
    std::lock_guard<std::mutex> lock(m_mutex);

    // Payload format: "filename|size|hash"
    std::string meta(pkt.payload, std::min((int)pkt.payloadSize, (int)sizeof(pkt.payload)));
    size_t sep1 = meta.find('|');
    size_t sep2 = meta.find('|', sep1 + 1);

    if (sep1 == std::string::npos || sep2 == std::string::npos) return false;

    std::string rawFilename = meta.substr(0, sep1);
    uint64_t size = 0;
    try {
        size = std::stoull(meta.substr(sep1 + 1, sep2 - sep1 - 1));
    } catch (...) { return false; }
    std::string hash = meta.substr(sep2 + 1);

    // SECURITY: Sanitize filename (Path Traversal Protection)
    std::string safeFilename = fs::path(rawFilename).filename().string();
    if (safeFilename.empty()) return false;

    // SECURITY: Limit file size (DoS/OOM Protection)
    const uint64_t MAX_FILE_SIZE = 100 * 1024 * 1024; // 100MB limit
    if (size > MAX_FILE_SIZE) {
        std::cerr << "[FileTransfer] Rejected: File too large (" << size << " bytes)" << std::endl;
        return false;
    }

    FileTransfer transfer;
    transfer.filename = safeFilename;
    transfer.fileSize = size;
    transfer.hash = hash;
    transfer.bytesTransferred = 0;
    transfer.isOutgoing = false;
    transfer.isComplete = false;

    try {
        transfer.buffer.resize(transfer.fileSize);
    } catch (const std::bad_alloc&) {
        return false;
    }

    unsigned long long transferId = ((unsigned long long)(uint32_t)pkt.y << 32) | (uint32_t)pkt.x;
    m_transfers[transferId] = transfer;

    std::cout << "[FileTransfer] Receiving file: " << transfer.filename << " (" << transfer.fileSize << " bytes) ID: " << transferId << std::endl;
    return true;
}

bool FileTransferEngine::HandleFileData(const Packet& pkt) {
    std::lock_guard<std::mutex> lock(m_mutex);

    unsigned long long transferId = ((unsigned long long)(uint32_t)pkt.y << 32) | (uint32_t)pkt.x;
    auto it = m_transfers.find(transferId);
    if (it == m_transfers.end() || it->second.isOutgoing || it->second.isComplete) return false;

    FileTransfer& t = it->second;
    const size_t CHUNK_SIZE = 4000;
    if (pkt.chunkIndex < 0) return false;
    size_t offset = (size_t)pkt.chunkIndex * CHUNK_SIZE;

    if (offset + pkt.payloadSize > t.buffer.size()) return false;

    std::memcpy(t.buffer.data() + offset, pkt.payload, pkt.payloadSize);
    t.bytesTransferred += pkt.payloadSize;

    if (t.bytesTransferred >= t.fileSize) {
        t.isComplete = true;

        // Verify Hash
        std::string data(t.buffer.begin(), t.buffer.end());
        unsigned char hash[32];
        AuthModule::ComputeSHA256(data, hash);
        std::string calculatedHash = AuthModule::HashToHex(hash);

        if (calculatedHash != t.hash) {
            std::cerr << "[FileTransfer] Hash mismatch! Expected: " << t.hash << " Got: " << calculatedHash << std::endl;
        } else {
            // Save to disk
            std::string savePath = "received_" + t.filename;
            std::ofstream outFile(savePath, std::ios::binary);
            if (outFile.is_open()) {
                outFile.write(t.buffer.data(), t.buffer.size());
                std::cout << "[FileTransfer] File saved: " << savePath << std::endl;
            }
        }
        t.buffer.clear();
        t.buffer.shrink_to_fit();
    }

    return true;
}

std::map<unsigned long long, FileTransferEngine::TransferStatus> FileTransferEngine::GetActiveTransfers() {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::map<unsigned long long, TransferStatus> result;
    for (auto const& [id, t] : m_transfers) {
        result[id] = { t.filename, (float)t.bytesTransferred / t.fileSize, t.isComplete, t.isOutgoing };
    }
    return result;
}

bool FileTransferEngine::IsHeaderSent(unsigned long long transferId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_headerSent[transferId];
}

void FileTransferEngine::SetHeaderSent(unsigned long long transferId, bool sent) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_headerSent[transferId] = sent;
}

std::string FileTransferEngine::CalculateHash(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return "";

    std::vector<char> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    std::string data(buffer.begin(), buffer.end());

    unsigned char hash[32];
    if (AuthModule::ComputeSHA256(data, hash)) {
        return AuthModule::HashToHex(hash);
    }
    return "";
}
