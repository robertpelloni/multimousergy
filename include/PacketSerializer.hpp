#pragma once
#include "CommonTypes.hpp"
#include <vector>
#include <cstdint>

class PacketSerializer {
public:
    // Serializes a Packet struct into a byte buffer.
    // If headerOnly is true, only fields up to 'payload' are serialized.
    static std::vector<uint8_t> Serialize(const Packet& pkt, bool headerOnly = false);

    // Deserializes a byte buffer into a Packet struct.
    // Returns the number of bytes consumed, or 0 if more data is needed/error.
    static size_t Deserialize(const uint8_t* buffer, size_t size, Packet& outPkt);

    // Constant for the header size (fixed portion of the packet)
    // 8(senderId) + 4(groupId) + 4(sequenceNumber) + 8(localTimestamp) + 4(type)
    // + 4(x) + 4(y) + 4(button) + 1(down) + 1(isSelecting) + 4(selStartX) + 4(selStartY)
    // + 4(wheelDelta) + 1(isHWheel) + 4(chunkIndex) + 4(totalChunks)
    static constexpr size_t HEADER_SIZE = 8 + 4 + 4 + 8 + 4 + 4 + 4 + 4 + 1 + 1 + 4 + 4 + 4 + 1 + 4 + 4;
};
