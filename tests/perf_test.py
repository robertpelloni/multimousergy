import socket
import time
import struct

def simulate_client(host='127.0.0.1', port=9999):
    print(f"Connecting to {host}:{port} via UDP...")
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    # Packet structure (matching C++ alignment):
    # unsigned long long senderId (Q - 8)
    # unsigned int groupId (I - 4)
    # (4 bytes padding)
    # double localTimestamp (d - 8)
    # int type (i - 4)
    # int x (i - 4)
    # int y (i - 4)
    # int button (i - 4)
    # bool down (? - 1)
    # (3 bytes padding)
    # char payload[1024] (1024s - 1024)
    # int payloadSize (i - 4)
    # (4 bytes padding for struct alignment to 8 bytes)

    packet_format = 'Q I 4x d i i i i ? 3x 1024s i 4x'

    try:
        print(f"Packet size: {struct.calcsize(packet_format)} bytes")
    except Exception as e:
        print(f"Format error: {e}")
        return

    start_time = time.time()
    count = 0
    # AbsoluteMovement = 1
    # Handshake = 7

    # Send Handshake first
    handshake = struct.pack(packet_format, 1001, 0, time.time()*1000, 7, 0, 0, 0, False, b"Python-Mock", 11)
    s.sendto(handshake, (host, port))

    print("Streaming AbsoluteMovement packets...")
    while time.time() - start_time < 5:
        # Move in a circle
        angle = count * 0.1
        nx = int(32768 + 15000 * (angle % 6.28))
        ny = int(32768 + 15000 * (angle % 6.28))

        timestamp = time.time() * 1000
        data = struct.pack(packet_format, 1001, 0, timestamp, 1, nx, ny, 0, False, b"", 0)
        s.sendto(data, (host, port))
        count += 1
        time.sleep(0.01) # 100Hz

    print(f"Sent {count} packets.")
    s.close()

if __name__ == "__main__":
    simulate_client()
