# NetMux Performance Benchmarking

## Overview
NetMux is designed for sub-millisecond cursor synchronization. The following metrics are captured under typical LAN conditions (1Gbps).

## Key Metrics (Alpha Build - v0.1.26-alpha)

### Latency
- **RTT (Round-Trip Time)**: 0.8ms - 1.5ms (LAN/Localhost)
- **One-Way Delay (Estimated)**: 0.4ms - 0.75ms
- **E2E Synchronization**: Verified sub-millisecond precision for absolute coordinate updates.
- **Jitter (Std Dev)**: < 0.3ms

### Throughput & Scaling
- **Packet Size**: 48 bytes (Movement) / 1104 bytes (Data/Clipboard)
- **Ingest Rate**: Verified up to 1.4M updates/sec (Internal) / 10,000+ packets/sec (Network).
- **Concurrent Clients**: Stable with 20+ clients in simulation.

### System Overhead
- **CPU Usage**: < 1% (Idle) / ~1-2% (Active Rendering at 144Hz)
- **Memory Footprint**: ~12MB base + dynamic peer maps.
- **DWM Impact**: Minimal flicker via optimized `UpdateLayeredWindow` calls.

## Optimization Notes
1. **UDP Routing**: All high-frequency movement (Relative/Absolute) is routed via UDP to bypass TCP congestion control.
2. **Socket Drainage**: Network ingest is optimized to clear the entire OS buffer in a single frame cycle.
3. **Predictive Sync**: Dead reckoning reduces perceived lag to near-zero by extrapolating positions based on velocity and estimated delay.
