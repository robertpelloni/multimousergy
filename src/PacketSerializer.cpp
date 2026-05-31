#include "PacketSerializer.hpp"
#include <cstring>

// Helper to write to buffer and advance offset
template<typename T>
static void WriteBuffer(std::vector<uint8_t>& buf, size_t& offset, const T& val) {
    if (offset + sizeof(T) > buf.size()) buf.resize(offset + sizeof(T));
    std::memcpy(buf.data() + offset, &val, sizeof(T));
    offset += sizeof(T);
}

// Helper to read from buffer and advance offset
template<typename T>
static void ReadBuffer(const uint8_t* buf, size_t& offset, size_t totalSize, T& val) {
    if (offset + sizeof(T) <= totalSize) {
        std::memcpy(&val, buf + offset, sizeof(T));
        offset += sizeof(T);
    }
}

std::vector<uint8_t> PacketSerializer::Serialize(const Packet& pkt, bool headerOnly) {
    std::vector<uint8_t> buf;
    size_t offset = 0;

    WriteBuffer(buf, offset, pkt.senderId);
    WriteBuffer(buf, offset, pkt.groupId);
    WriteBuffer(buf, offset, pkt.sequenceNumber);
    WriteBuffer(buf, offset, pkt.localTimestamp);
    WriteBuffer(buf, offset, (int32_t)pkt.type);
    WriteBuffer(buf, offset, pkt.x);
    WriteBuffer(buf, offset, pkt.y);
    WriteBuffer(buf, offset, pkt.button);
    WriteBuffer(buf, offset, (uint8_t)pkt.down);
    WriteBuffer(buf, offset, (uint8_t)pkt.isSelecting);
    WriteBuffer(buf, offset, pkt.selectionStartX);
    WriteBuffer(buf, offset, pkt.selectionStartY);

    if (!headerOnly) {
        WriteBuffer(buf, offset, pkt.payloadSize);
        if (pkt.payloadSize > 0) {
            size_t pSize = (pkt.payloadSize > 1024) ? 1024 : pkt.payloadSize;
            if (offset + pSize > buf.size()) buf.resize(offset + pSize);
            std::memcpy(buf.data() + offset, pkt.payload, pSize);
            offset += pSize;
        }
    }

    return buf;
}

size_t PacketSerializer::Deserialize(const uint8_t* buffer, size_t size, Packet& outPkt) {
    if (size < HEADER_SIZE) return 0;

    size_t offset = 0;
    int32_t typeInt;
    uint8_t downByte, isSelectingByte;

    ReadBuffer(buffer, offset, size, outPkt.senderId);
    ReadBuffer(buffer, offset, size, outPkt.groupId);
    ReadBuffer(buffer, offset, size, outPkt.sequenceNumber);
    ReadBuffer(buffer, offset, size, outPkt.localTimestamp);
    ReadBuffer(buffer, offset, size, typeInt); outPkt.type = (NetMuxPacketType)typeInt;
    ReadBuffer(buffer, offset, size, outPkt.x);
    ReadBuffer(buffer, offset, size, outPkt.y);
    ReadBuffer(buffer, offset, size, outPkt.button);
    ReadBuffer(buffer, offset, size, downByte); outPkt.down = (downByte != 0);
    ReadBuffer(buffer, offset, size, isSelectingByte); outPkt.isSelecting = (isSelectingByte != 0);
    ReadBuffer(buffer, offset, size, outPkt.selectionStartX);
    ReadBuffer(buffer, offset, size, outPkt.selectionStartY);

    // Default payload state
    outPkt.payloadSize = 0;

    // Only movement/absolute are guaranteed to be header-only
    bool isHeaderOnlyType = (outPkt.type == NetMuxPacketType::Movement || outPkt.type == NetMuxPacketType::AbsoluteMovement);
    if (isHeaderOnlyType) return offset;

    // For other types, we expect a payloadSize field
    if (offset + sizeof(int32_t) > size) return 0; // Need more data for payloadSize

    ReadBuffer(buffer, offset, size, outPkt.payloadSize);

    if (outPkt.payloadSize > 0) {
        if (outPkt.payloadSize > 1024) return 0; // Protocol error/corrupted
        if (offset + (size_t)outPkt.payloadSize > size) return 0; // Need more data for payload

        std::memcpy(outPkt.payload, buffer + offset, outPkt.payloadSize);
        offset += outPkt.payloadSize;
    }

    return offset;
}
