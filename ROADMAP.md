# ROADMAP.md

## Milestone 1: Core Connectivity (COMPLETED ARCHITECTURE)
- [x] Virtual HID Driver abstraction layer.
- [x] Linux hardware injection support (evdev).
- [x] Raw Input interception and local cursor suppression.
- [x] Bidirectional hybrid UDP/TCP networking.

## Milestone 2: Advanced Rendering (COMPLETED)
- [x] Direct3D 11 hardware-accelerated overlay.
- [x] Dynamic cursor scaling and textured sprite rendering.
- [x] Custom Cursor Themes support.

## Milestone 3: Real-Time Synchronization (COMPLETED)
- [x] Distributed Clock Synchronization (Unified Timeline).
- [x] "Timestamp-First" Interaction Ownership & Conflict Resolution.
- [x] Real-Time Sync Guard (Drift reporting and correction).

## Milestone 4: Collaborative Features (COMPLETED)
- [x] Collaborative Selection and Drag Sync.
- [x] Secure SHA-256 Mutual Authentication & Auto-Challenge.
- [x] Replay Protection & Security Event Logging.
- [x] Persistent Monitor UI with live telemetry.

## Milestone 5: Optimization & Polish (IN PROGRESS)
- [x] Multi-Monitor DPI Awareness.
- [x] Bandwidth Optimization (Header-only movement packets).
- [x] Focus Halo visual feedback.
- [x] Formal Protocol Serialization.
- [ ] Native Vendor SDK linking for Drivers.
