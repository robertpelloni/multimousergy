# ROADMAP.md

## Milestone 1: Core Connectivity (COMPLETED)
- [x] Virtual HID Driver integration (Interception).
- [x] Raw Input interception and local cursor suppression logic.
- [x] Bidirectional non-blocking networking for cursor and state sync.

## Milestone 2: Independent Rendering (COMPLETED)
- [x] Transparent layered window overlay.
- [x] Optimized flicker-free `UpdateLayeredWindow` implementation.
- [x] Pre-allocated GDI resource management for low-latency rendering.
- [x] Standard cursor icon and colorized crosshair rendering.
- [x] D3D11 hardware-accelerated overlay implementation (v0.1.39+).

## Milestone 3: Interaction & Multiplexing (COMPLETED)
- [x] TCP state sync for clicks and buttons.
- [x] Warp-Click-Restore with window focus handling.
- [x] Crossover release mechanism for local cursor recovery.

## Milestone 4: Polishing & UI (COMPLETED)
- [x] Programmatic Win32 Configuration GUI.
- [x] Automated peer discovery via UDP broadcasting.
- [x] Multi-monitor coordinate scaling and clamping.
- [x] High-precision synchronization protocol (v0.1.11+).
- [x] Integrated benchmarking and telemetry (v0.1.16+).
 - [x] Dynamic Cursor Scaling and Peer Selection (v0.1.21+).
 - [x] Dynamic Cursor Grouping and Load Balancing (v0.1.26+).
 - [x] Dynamic Session Management and Live Monitoring (v0.1.27+).
 - [x] Real-Time Synchronization Layer (Clock Sync & Drift Correction) (v0.1.31+).
 - [x] High-Frequency Authoritative Master State Sync (v0.1.32+).
 - [x] Automated Testing and Stability Verification (v0.1.33+).
 - [x] Dynamic Group Metadata and Management (v0.1.35+).
 - [x] Load Balancing and Stability Polish (v0.1.36+).
 - [x] Advanced Input Distribution and Concurrency Tests (v0.1.37+).
 - [x] Cross-Platform Abstraction and Multi-OS Foundation (v0.1.38+).
 - [x] D3D11 Hardware Acceleration (v0.1.39+).
 - [x] Final Alpha Stability & Sync Validation (v0.1.40+).
