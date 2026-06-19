#include "PacketSerializer.hpp"
#include <cstring>

/**
 * NetMux Packet Serialization Layer
 *
 * Optimized for low-bandwidth environments (UDP) by multiplexing mutually exclusive
 * fields into three 32-bit "data slots" (d1, d2, d3).
 */

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
    buf.reserve(headerOnly ? HEADER_SIZE : HEADER_SIZE + pkt.payloadSize + sizeof(int32_t));
    size_t offset = 0;

    // Fixed Header: Always serialized for every packet
    WriteBuffer(buf, offset, pkt.senderId);
    WriteBuffer(buf, offset, pkt.groupId);
    WriteBuffer(buf, offset, pkt.sequenceNumber);
    WriteBuffer(buf, offset, pkt.localTimestamp);
    WriteBuffer(buf, offset, (int32_t)pkt.type);
    WriteBuffer(buf, offset, pkt.x);
    WriteBuffer(buf, offset, pkt.y);

    /**
     * MAPPING LOGIC:
     * We map specific fields from the Packet struct into generic 32-bit slots (d1-d3).
     * This reduces the fixed header size significantly (63 -> 48 bytes).
     */
    int32_t d1 = 0, d2 = 0, d3 = 0;
    switch (pkt.type) {
    case NetMuxPacketType::Click:
    case NetMuxPacketType::KeyboardEvent:
    case NetMuxPacketType::InputEvent:
        d1 = pkt.button;
        d2 = pkt.down ? 1 : 0;
        break;
    case NetMuxPacketType::Wheel:
        d1 = pkt.wheelDelta;
        d2 = pkt.isHorizontalWheel ? 1 : 0;
        break;
    case NetMuxPacketType::SelectionUpdate:
        d1 = pkt.isSelecting ? 1 : 0;
        d2 = pkt.selectionStartX;
        d3 = pkt.selectionStartY;
        break;
    case NetMuxPacketType::ClipboardSync:
    case NetMuxPacketType::FileData:
    case NetMuxPacketType::VideoFrame:
    case NetMuxPacketType::ICECandidate:
    case NetMuxPacketType::DeltaMovement: // DeltaMovement might use index/total for something else in future
        d1 = pkt.chunkIndex;
        d2 = pkt.totalChunks;
        break;
    case NetMuxPacketType::Handshake:
    case NetMuxPacketType::ResolutionUpdate:
        {
            union { float f; int32_t i; } conv;
            conv.f = pkt.dpiScale;
            d1 = conv.i;
        }
        break;
    default:
        // Movement and other header-only types use x/y (already serialized)
        break;
    }

    WriteBuffer(buf, offset, d1);
    WriteBuffer(buf, offset, d2);
    WriteBuffer(buf, offset, d3);

    if (!headerOnly) {
        WriteBuffer(buf, offset, pkt.payloadSize);
        if (pkt.payloadSize > 0) {
            size_t pSize = (pkt.payloadSize > 4096) ? 4096 : pkt.payloadSize;
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

    ReadBuffer(buffer, offset, size, outPkt.senderId);
    ReadBuffer(buffer, offset, size, outPkt.groupId);
    ReadBuffer(buffer, offset, size, outPkt.sequenceNumber);
    ReadBuffer(buffer, offset, size, outPkt.localTimestamp);
    ReadBuffer(buffer, offset, size, typeInt); outPkt.type = (NetMuxPacketType)typeInt;
    ReadBuffer(buffer, offset, size, outPkt.x);
    ReadBuffer(buffer, offset, size, outPkt.y);

    int32_t d1, d2, d3;
    ReadBuffer(buffer, offset, size, d1);
    ReadBuffer(buffer, offset, size, d2);
    ReadBuffer(buffer, offset, size, d3);

    // Reset non-header fields to safe defaults to prevent data bleeding between reused packet structs
    outPkt.button = 0;
    outPkt.down = false;
    outPkt.isSelecting = false;
    outPkt.selectionStartX = 0;
    outPkt.selectionStartY = 0;
    outPkt.wheelDelta = 0;
    outPkt.isHorizontalWheel = false;
    outPkt.chunkIndex = 0;
    outPkt.totalChunks = 0;
    outPkt.dpiScale = 1.0f;

    // Reverse mapping: generic data slots back to named Packet fields
    switch (outPkt.type) {
    case NetMuxPacketType::Click:
    case NetMuxPacketType::KeyboardEvent:
    case NetMuxPacketType::InputEvent:
        outPkt.button = d1;
        outPkt.down = (d2 != 0);
        break;
    case NetMuxPacketType::Wheel:
        outPkt.wheelDelta = d1;
        outPkt.isHorizontalWheel = (d2 != 0);
        break;
    case NetMuxPacketType::SelectionUpdate:
        outPkt.isSelecting = (d1 != 0);
        outPkt.selectionStartX = d2;
        outPkt.selectionStartY = d3;
        break;
    case NetMuxPacketType::ClipboardSync:
    case NetMuxPacketType::FileData:
    case NetMuxPacketType::VideoFrame:
    case NetMuxPacketType::ICECandidate:
        outPkt.chunkIndex = d1;
        outPkt.totalChunks = d2;
        break;
    case NetMuxPacketType::Handshake:
    case NetMuxPacketType::ResolutionUpdate:
        {
            union { float f; int32_t i; } conv;
            conv.i = d1;
            outPkt.dpiScale = conv.f;
        }
        break;
    default:
        break;
    }

    outPkt.payloadSize = 0;
    if (outPkt.IsHeaderOnly()) return offset;

    if (offset + sizeof(int32_t) > size) return 0;
    ReadBuffer(buffer, offset, size, outPkt.payloadSize);

    if (outPkt.payloadSize > 0) {
        if (outPkt.payloadSize > 4096) return 0;
        if (offset + (size_t)outPkt.payloadSize > size) return 0;

        std::memcpy(outPkt.payload, buffer + offset, outPkt.payloadSize);
        offset += outPkt.payloadSize;
    }

    return offset;
}
