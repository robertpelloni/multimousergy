import socket
import time
import struct

def simulate_client(host='127.0.0.1', port=5555):
    print(f"Connecting to {host}:{port}...")
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        s.connect((host, port))
    except Exception as e:
        print(f"Failed to connect: {e}")
        return

    # Packet structure: unsigned long long senderId, int type, int x, int y, int button, bool down
    # type 1 is AbsoluteMovement
    packet_format = 'Q i i i i ?'

    start_time = time.time()
    count = 0
    while time.time() - start_time < 10:
        # Move in a circle
        angle = count * 0.1
        nx = int(32768 + 10000 * (angle % 6.28))
        ny = int(32768 + 10000 * (angle % 6.28))

        # AbsoluteMovement = 1
        data = struct.pack(packet_format, 1001, 1, nx, ny, 0, False)
        s.send(data)
        count += 1
        time.sleep(0.01) # 100Hz

    print(f"Sent {count} packets.")
    s.close()

if __name__ == "__main__":
    # This script requires the NetMux server to be running
    print("Performance Test Script (Mock Client)")
    # simulate_client()
