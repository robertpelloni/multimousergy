import socket
import time
import struct
import math

# NetMux Protocol Definition
# struct Packet {
#     unsigned long long senderId; // Q (8)
#     unsigned int groupId;        // I (4)
#     double localTimestamp;       // d (8) - Likely 8-byte aligned
#     PacketType type;             // i (4)
#     int x;                       // i (4)
#     int y;                       // i (4)
#     int button;                  // i (4)
#     bool down;                   // b (1)
#     char payload[1024];          // 1024s (1024)
#     int payloadSize;             // i (4)
# };
# Total: 8 + 4 + (4 padding) + 8 + 4 + 4 + 4 + 4 + 1 + 1024 + (3 padding?) + 4 = 1072

PACKET_FORMAT = "Q I 4x d i i i i b 1024s i"

def test_sync_validation():
    print("--- NetMux E2E Synchronization Validation ---")

    server_addr = ("127.0.0.1", 5555)
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    # 1. Simulate Client A movement
    client_a_id = 1001
    group_id = 1
    target_x = 32768 # Center
    target_y = 32768

    print(f"Client A ({client_a_id}) sending AbsoluteMovement to {target_x}, {target_y}")

    # PacketType::AbsoluteMovement = 1
    packet = struct.pack(PACKET_FORMAT,
                         client_a_id, group_id, time.time(),
                         1, target_x, target_y, 0, False, b"", 0)

    sock.sendto(packet, server_addr)
    time.sleep(1) # Allow for server to process and rebroadcast

    # 2. Simulate Client B (Reporting back local perception for SyncCheck)
    # PacketType::SyncCheck = 13
    print("Client B reporting back perceived position for SyncCheck...")

    # Correct position (Zero Drift)
    check_packet = struct.pack(PACKET_FORMAT,
                               1002, group_id, time.time(),
                               13, target_x, target_y, 1001, False, b"", 0)
    sock.sendto(check_packet, server_addr)

    time.sleep(0.5)

    # Drifting position (High Drift)
    drifted_x = target_x + 500
    print(f"Client B reporting drifted position ({drifted_x}) for SyncCheck...")
    check_packet_drift = struct.pack(PACKET_FORMAT,
                                     1002, group_id, time.time(),
                                     13, drifted_x, target_y, 1001, False, b"", 0)
    sock.sendto(check_packet_drift, server_addr)

    print("E2E Validation packets sent. Check NetMux logs for 'Corrective MasterSync issued'.")

if __name__ == "__main__":
    test_sync_validation()
