# ROADMAP.md

## Milestone 1: Core Connectivity (COMPLETED ARCHITECTURE)
- [x] Virtual HID Driver abstraction layer.
- [x] Linux hardware injection support (evdev).
- [x] Raw Input interception and local cursor suppression.
- [x] Bidirectional hybrid UDP/TCP networking.

## Milestone 2: Advanced Rendering (COMPLETED)
- [x] Direct3D 11 hardware-accelerated overlay.
- [x] Dynamic cursor scaling and textured sprite rendering.
- [x] Custom Cursor Themes support (.bmp loading).
- [x] Selection Color customization.
- [x] Mouse Wheel Support (Vertical & Horizontal).

## Milestone 3: Real-Time Synchronization (COMPLETED)
- [x] Distributed Clock Synchronization (Unified Timeline).
- [x] "Timestamp-First" Interaction Ownership & Conflict Resolution.
- [x] Real-Time Sync Guard (Drift reporting and correction).

## Milestone 4: Collaborative Features (COMPLETED)
- [x] Collaborative Selection and Drag Sync.
- [x] Secure SHA-256 Mutual Authentication & Auto-Challenge.
- [x] Replay Protection & Security Event Logging.
- [x] Automated Peer Discovery & UI Selection.
- [x] Persistent Monitor UI with live telemetry.

## Milestone 5: Optimization & Polish (IN PROGRESS)
- [x] Linux Input Capture (evdev).
- [x] Security Hardening (Constant-time auth verification).
- [x] Multi-Monitor DPI Awareness.
- [x] Bandwidth Optimization (Header-only movement packets).
- [x] Focus Halo visual feedback.
- [x] Formal Protocol Serialization.
- [x] Unicode Clipboard & High-Capacity Payloads.
- [x] Real-time UI Personalization (RGB/Themes).
- [x] Unlimited Clipboard Capacity (Multi-part Chunking).
- [x] Full D3D11 Custom Cursor Support.
- [x] Native Linux X11 Integration (Clipboard & Cursor Position).
- [x] Authoritative Synchronization (Server-side position enforcement).
- [x] UI & Network Stack Refactor (Robust connection handling).
 - [x] File Transfer Engine: Implement drag-and-drop file sharing via multi-part protocol.
- [ ] Native Vendor SDK linking for Drivers.
