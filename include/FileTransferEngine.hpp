#pragma once
#include "CommonTypes.hpp"
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <mutex>
#include <functional>

enum class FileTransferError {
    None,
    FileNotFound,
    AccessDenied,
    DiskFull,
    HashMismatch,
    TooLarge,
    BadAlloc
};

struct FileTransfer {
    std::string filename;
    uint64_t fileSize;
    std::string hash;
    std::string localPath;
    uint64_t bytesTransferred;
    bool isOutgoing;
    bool isComplete;
    FileTransferError lastError = FileTransferError::None;
    std::vector<char> buffer;
};

class FileTransferEngine {
public:
    FileTransferEngine();
    ~FileTransferEngine();

    // Outgoing
    bool StartTransfer(const std::string& filePath, unsigned long long targetPeerId);
    bool GetHeaderPacket(unsigned long long transferId, Packet& outPkt);
    bool GetNextChunk(unsigned long long transferId, Packet& outPkt);

    // Incoming
    bool HandleFileHeader(const Packet& pkt);
    bool HandleFileData(const Packet& pkt);

    // Monitoring
    struct TransferStatus {
        std::string filename;
        float progress;
        bool isComplete;
        bool isOutgoing;
        FileTransferError lastError;
    };
    std::map<unsigned long long, TransferStatus> GetActiveTransfers();

    bool IsHeaderSent(unsigned long long transferId);
    void SetHeaderSent(unsigned long long transferId, bool sent);

    void CleanupPeerTransfers(unsigned long long peerId);

private:
    std::mutex m_mutex;
    std::map<unsigned long long, FileTransfer> m_transfers;
    std::map<unsigned long long, bool> m_headerSent;
    unsigned long long m_nextTransferId = 1;

    std::string CalculateHash(const std::string& path);
};
