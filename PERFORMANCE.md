# NetMux Performance Benchmarking

## Overview
NetMux is designed for sub-millisecond cursor synchronization. The following metrics are captured under typical LAN conditions (1Gbps).

## Key Metrics (Alpha Build)

### Latency
- **RTT (Round-Trip Time)**: 0.8ms - 2.5ms
- **One-Way Delay (Estimated)**: 0.4ms - 1.2ms
- **Jitter (Std Dev)**: < 0.5ms

### Throughput & Scaling
- **Packet Size**: 48 bytes (Movement) / 1104 bytes (Data/Clipboard)
- **Ingest Rate**: Verified up to 1.4M updates/sec (Internal) / 10,000+ packets/sec (Network).
- **Concurrent Clients**: Stable with 20+ clients in simulation.

### System Overhead
- **CPU Usage**: < 1% (Idle) / ~2-3% (Active Rendering at 144Hz)
- **Memory Footprint**: ~15MB base + dynamic peer maps.
- **DWM Impact**: Minimal flicker via optimized `UpdateLayeredWindow` calls.

## Optimization Notes
1. **UDP Routing**: All high-frequency movement (Relative/Absolute) is routed via UDP to bypass TCP congestion control.
2. **Socket Drainage**: Network ingest is optimized to clear the entire OS buffer in a single frame cycle.
3. **Predictive Sync**: Dead reckoning reduces perceived lag to near-zero by extrapolating positions based on velocity and estimated delay.
