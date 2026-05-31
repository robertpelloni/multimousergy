#pragma once
#include "CommonTypes.hpp"
#include <vector>
#include <cstdint>

class PacketSerializer {
public:
    // Constant for the header size (fixed portion of the packet)
    static constexpr size_t GetHeaderSize() {
        return sizeof(unsigned long long) + // senderId
               sizeof(unsigned int) +       // groupId
               sizeof(unsigned int) +       // sequenceNumber
               sizeof(double) +             // localTimestamp
               sizeof(int32_t) +            // type
               sizeof(int) * 3 +            // x, y, button
               sizeof(uint8_t) * 2 +        // down, isSelecting
               sizeof(int) * 2 +            // selectionStartX, selectionStartY
               sizeof(int) +                // wheelDelta
               sizeof(uint8_t) +            // isHorizontalWheel
               sizeof(int) * 2;             // chunkIndex, totalChunks
    }

    static constexpr size_t HEADER_SIZE = 8 + 4 + 4 + 8 + 4 + 4 + 4 + 4 + 1 + 1 + 4 + 4 + 4 + 1 + 4 + 4;

    // Serializes a Packet struct into a byte buffer.
    // If headerOnly is true, only fields up to 'payload' are serialized.
    static std::vector<uint8_t> Serialize(const Packet& pkt, bool headerOnly = false);

    // Deserializes a byte buffer into a Packet struct.
    // Returns the number of bytes consumed, or 0 if more data is needed/error.
    static size_t Deserialize(const uint8_t* buffer, size_t size, Packet& outPkt);
};
