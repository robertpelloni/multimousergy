# CHANGELOG.md

## [0.1.44-alpha] - 2025-01-24
### Added
- Implemented stateful `AuthService` for secure challenge-response management.
- Added Mutual Authentication: Both client and server now challenge each other.
- Hardened security: unauthenticated peers are strictly dropped from sensitive state updates.
- Added CLI support for security keys (`--key` / `-k`).
- Integrated comprehensive unit tests for `AuthService`.

## [0.1.43-alpha] - 2025-01-24
### Added
- Implemented Network Bandwidth Optimization (header-only movement packets).
- Expanded ConfigGUI with real-time Drift and E2E Latency telemetry.
- Implemented "Focus Halo" visual feedback for active interaction owner in both D3D11 and GDI.
- Added framework re-initialization signaling from ConfigGUI.

## [0.1.42-alpha] - 2025-01-24
### Added
- Implemented "Gesture Integrity" in conflict resolution.
- Added visual feedback for interaction conflicts (Red 'X').
- Added "Focus Halo" to highlight active interaction owner.
- Implemented Adaptive Smoothing for better low-latency tracking.
- Verified simultaneous edit resolution with concurrency tests.

## [0.1.41-alpha] - 2025-01-24
### Added
- Robust SHA-256 challenge-response authentication (Windows/Linux support).
- Manual Driver Selection in Configuration GUI (Auto/Interception/ViGEmBus).
- Enhanced network reliability with partial send handling (SafeSend).
- Fixed potential buffer over-reads in network packet processing.
- Added SO_REUSEADDR for improved server recovery during restarts.

## [0.1.40-alpha] - 2025-01-24
### Added
- Finalized hardware-accelerated Direct3D 11 rendering pipeline.
- Implemented "Timestamp-First" conflict resolution for simultaneous multi-user interactions.
- Integrated distributed clock synchronization via UDP heartbeats.
- Added support for secure challenge-response authentication.
- Enhanced DriverInterface with ViGEmBus and Interception stubs.
- Conducted full E2E performance audit with automated benchmarking.

## [0.1.0-alpha] - 2025-01-22
### Added
- Initial project architecture and documentation.
- Core modules: Input, Network, Sync, Overlay, Driver Interface.
- Hybrid UDP/TCP networking for cursor movement and state.
- Coordinate normalization and interpolation (SyncModule).
- Hardware-accelerated Direct3D 11 cursor rendering.
- Programmatic Win32 Configuration GUI.
- Clipboard synchronization.
- Integration test suite.
- Dynamic Resolution Scaling support.
- Hardware driver abstraction (Interception/ViGEmBus).
