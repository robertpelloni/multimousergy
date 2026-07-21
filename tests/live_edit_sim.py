import socket
import time
import struct

PACKET_FORMAT = "Q I 4x d i i i i b 1024s i"
FMT_SEL = "Q I 4x d i i i i b b 2x i i 1024s i"

def simulate_editing():
    server_addr = ("127.0.0.1", 5555)
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    peers = [1001, 1002]

    print("Simulating simultaneous editing from two peers...")

    # 0. Initial registration
    pkt_a_init = struct.pack(PACKET_FORMAT, 1001, 1, time.time(), 1, 1000, 1000, 0, False, b"", 0)
    sock.sendto(pkt_a_init, server_addr)
    pkt_b_init = struct.pack(PACKET_FORMAT, 1002, 1, time.time(), 1, 2000, 2000, 0, False, b"", 0)
    sock.sendto(pkt_b_init, server_addr)
    time.sleep(0.1)

    # 1. Peer A starts selection
    pkt_a_sel = struct.pack(FMT_SEL, 1001, 1, time.time(), 14, 1000, 1000, 0, False, True, 500, 500, b"", 0)
    sock.sendto(pkt_a_sel, server_addr)

    # 1.5 Peer A clicks to claim focus
    pkt_a_click = struct.pack(PACKET_FORMAT, 1001, 1, time.time(), 2, 1000, 1000, 0, True, b"", 0)
    sock.sendto(pkt_a_click, server_addr)

    # 2. Peer B moves (Global visibility)
    pkt_b_move = struct.pack(PACKET_FORMAT, 1002, 1, time.time(), 1, 2000, 2000, 0, False, b"", 0)
    sock.sendto(pkt_b_move, server_addr)

    time.sleep(0.1)

    # 3. Peer B tries to click while A is selecting (Should be denied by Gesture Integrity)
    pkt_b_click = struct.pack(PACKET_FORMAT, 1002, 1, time.time(), 2, 2000, 2000, 0, True, b"", 0)
    sock.sendto(pkt_b_click, server_addr)

    time.sleep(0.1)

    # 4. Peer A releases selection
    pkt_a_rel = struct.pack(FMT_SEL, 1001, 1, time.time(), 14, 1500, 1500, 0, False, False, 500, 500, b"", 0)
    sock.sendto(pkt_a_rel, server_addr)

    print("Simulation packets sent. Peer B's click should have been denied due to Peer A's active selection.")

if __name__ == "__main__":
    simulate_editing()
