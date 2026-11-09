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

    // Multi-file support: Allow multiple transfers for the same path
    // Resume is only used if a transfer was explicitly interrupted.

    FileTransfer transfer;
    transfer.filename = fs::path(filePath).filename().string();
    transfer.fileSize = fs::file_size(filePath);
    transfer.localPath = filePath;
    transfer.bytesTransferred = 0;
    transfer.isOutgoing = true;
    transfer.isComplete = false;
    transfer.peerId = targetPeerId;
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

bool FileTransferEngine::StartBatchTransfer(const std::vector<std::string>& filePaths, unsigned long long targetPeerId, std::vector<unsigned long long>& outTransferIds) {
    outTransferIds.clear();
    bool anyStarted = false;
    for (const auto& path : filePaths) {
        if (StartTransfer(path, targetPeerId)) {
            outTransferIds.push_back(m_nextTransferId - 1);
            anyStarted = true;
        }
    }
    return anyStarted;
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

bool FileTransferEngine::HandleFileHeader(const Packet& pkt, unsigned long long senderPeerId) {
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

    // Resume check for incoming
    unsigned long long transferId = ((unsigned long long)(uint32_t)pkt.y << 32) | (uint32_t)pkt.x;
    if (m_transfers.count(transferId)) {
        FileTransfer& existing = m_transfers[transferId];
        if (existing.hash == hash && existing.fileSize == size) {
            std::cout << "[FileTransfer] Resuming incoming transfer ID: " << transferId << std::endl;
            return true;
        }
    }

    FileTransfer transfer;
    transfer.filename = safeFilename;
    transfer.fileSize = size;
    transfer.hash = hash;
    transfer.bytesTransferred = 0;
    transfer.isOutgoing = false;
    transfer.isComplete = false;
    transfer.peerId = senderPeerId;
    transfer.tempPath = "received_" + safeFilename + ".part";

    // Clean up any stale partial file from a previous aborted transfer
    if (fs::exists(transfer.tempPath)) {
        fs::remove(transfer.tempPath);
    }

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

    if (offset + pkt.payloadSize > t.fileSize) return false;

    // Stream directly to temporary disk file to avoid high memory buffering
    std::fstream fileStream(t.tempPath, std::ios::in | std::ios::out | std::ios::binary);
    if (!fileStream.is_open()) {
        fileStream.clear();
        fileStream.open(t.tempPath, std::ios::out | std::ios::binary);
    }
    if (!fileStream.is_open()) {
        t.lastError = FileTransferError::AccessDenied;
        return false;
    }

    fileStream.seekp(offset);
    fileStream.write(pkt.payload, pkt.payloadSize);
    fileStream.close();

    t.bytesTransferred += pkt.payloadSize;

    if (t.bytesTransferred >= t.fileSize) {
        t.isComplete = true;

        // Verify Hash from disk stream
        std::string calculatedHash = CalculateHash(t.tempPath);

        if (calculatedHash != t.hash) {
            std::cerr << "[FileTransfer] Hash mismatch! Expected: " << t.hash << " Got: " << calculatedHash << std::endl;
            t.lastError = FileTransferError::HashMismatch;
            fs::remove(t.tempPath);
        } else {
            // Rename temporary file to final target destination
            std::string savePath;
            if (!m_destDir.empty()) {
                std::error_code dirEc;
                fs::create_directories(m_destDir, dirEc);
                savePath = (fs::path(m_destDir) / ("received_" + t.filename)).string();
            } else {
                savePath = "received_" + t.filename;
            }
            if (fs::exists(savePath)) {
                fs::remove(savePath);
            }
            std::error_code ec;
            fs::rename(t.tempPath, savePath, ec);
            if (ec) {
                // Fallback copy if rename fails across mounts
                fs::copy_file(t.tempPath, savePath, fs::copy_options::overwrite_existing, ec);
                fs::remove(t.tempPath, ec);
            }
            std::cout << "[FileTransfer] File saved: " << savePath << std::endl;
        }
    }

    return true;
}

std::map<unsigned long long, FileTransferEngine::TransferStatus> FileTransferEngine::GetActiveTransfers() {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::map<unsigned long long, TransferStatus> result;
    for (auto const& [id, t] : m_transfers) {
        result[id] = { t.filename, (float)t.bytesTransferred / (t.fileSize ? t.fileSize : 1), t.isComplete, t.isOutgoing, t.lastError };
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

unsigned long long FileTransferEngine::GetTransferPeerId(unsigned long long transferId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_transfers.find(transferId);
    if (it != m_transfers.end()) {
        return it->second.peerId;
    }
    return 0;
}

void FileTransferEngine::SetDestinationDirectory(const std::string& dir) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_destDir = dir;
}

std::string FileTransferEngine::GetDestinationDirectory() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_destDir;
}

std::string FileTransferEngine::CalculateHash(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return "";

    auto ctx = AuthModule::CreateContext();
    char buffer[16384];
    while (file) {
        file.read(buffer, sizeof(buffer));
        std::streamsize bytes = file.gcount();
        if (bytes > 0) {
            AuthModule::UpdateHash(ctx, buffer, (size_t)bytes);
        }
    }

    unsigned char hash[32];
    if (AuthModule::FinalizeHash(ctx, hash)) {
        return AuthModule::HashToHex(hash);
    }
    return "";
}

void FileTransferEngine::CleanupPeerTransfers(unsigned long long peerId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto it = m_transfers.begin(); it != m_transfers.end(); ) {
        if (!it->second.isComplete && (peerId == 0 || it->second.peerId == peerId)) {
            // Clean up any lingering partial files on disk
            if (!it->second.tempPath.empty() && fs::exists(it->second.tempPath)) {
                fs::remove(it->second.tempPath);
            }
            it = m_transfers.erase(it);
        } else {
            ++it;
        }
    }
}
