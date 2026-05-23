# ROADMAP.md

## Milestone 1: Core Connectivity
- [ ] Virtual HID Driver integration (ViGEmBus).
- [x] Raw Input interception and local cursor suppression logic.
- [x] Bidirectional non-blocking networking for cursor and state sync.

## Milestone 2: Independent Rendering
- [x] Transparent layered window overlay.
- [x] Optimized flicker-free `UpdateLayeredWindow` implementation.
- [x] Pre-allocated GDI resource management for low-latency rendering.
- [x] Standard cursor icon and colorized crosshair rendering.
- [ ] D3D11 hardware-accelerated overlay (Future performance optimization).

## Milestone 3: Interaction & Multiplexing
- [x] TCP state sync for clicks and buttons.
- [x] Warp-Click-Restore with window focus handling.
- [x] Crossover release mechanism for local cursor recovery.

## Milestone 4: Polishing & UI
- [x] Programmatic Win32 Configuration GUI.
- [x] Automated peer discovery via UDP broadcasting.
- [x] Multi-monitor coordinate scaling and clamping.
- [x] High-precision synchronization protocol (v0.1.11+).
- [x] Integrated benchmarking and telemetry (v0.1.16+).
 - [x] Dynamic Cursor Scaling and Peer Selection (v0.1.21+).
 - [x] Dynamic Cursor Grouping and Load Balancing (v0.1.26+).
