# CHANGELOG.md

## [0.1.24-alpha] - 2025-01-24
### Added
- D3D11 hardware-accelerated overlay backend for ultra-smooth rendering.
- Refactored OverlayEngine to support multiple rendering backends (GDI/D3D11).
- Backend selection toggle in ConfigGUI.
- High-refresh rate support (144Hz+) via hardware acceleration.

## [0.1.23-alpha] - 2025-01-24
### Added
- E2E latency benchmarking and telemetry (RTT/E2E metrics).
- CSV export for performance data (BENCHMARK_RESULTS.csv).
- Switched SyncModule to floating-point coordinates for precision.

## [0.1.22-alpha] - 2025-01-24
### Added
- Automated stress tests for SyncModule and NetworkManager.
- Final end-to-end performance benchmarking and validation.
- Stability audit for high-frequency concurrent traffic.
- Updated PERFORMANCE.md with v0.1.22-alpha metrics.

## [0.1.21-alpha] - 2025-01-24
### Added
- Dynamic cursor scaling via ConfigGUI.
- Enhanced ConfigGUI with peer discovery list and active status.
- Support for StretchBlt-based remote cursor scaling.
- Final multi-terminal performance validation.

## [0.1.20-alpha] - 2025-01-24
### Added
- Multi-client UDP routing map for high-frequency movement distribution.
- Formal network handshake protocol for client metadata exchange.
- Targeted UDP transmission support for multi-user sessions.
- Verified system stability for concurrent movement and state sync.

## [0.1.17-alpha] - 2025-01-24
### Added
- Final comprehensive E2E validation and stability audit.
- Full roadmap synchronization and task finalization.
- Integrated multi-user concurrency and clipboard sync verification.

## [0.1.16-alpha] - 2025-01-24
### Added
- Integrated benchmarking mode via `--bench` flag.
- Detailed performance telemetry logging (jitter, frame statistics).
- Generated PERFORMANCE.md with documented alpha benchmarks.
- Cleaned up unused GDI resources in OverlayEngine.

## [0.1.20-alpha] - 2025-01-24
### Added
- Multi-client UDP routing map for high-frequency movement distribution.
- Formal network handshake protocol for client metadata exchange.
- Targeted UDP transmission support for multi-user sessions.
- Verified system stability for concurrent movement and state sync.

## [0.1.17-alpha] - 2025-01-24
### Added
- Final comprehensive E2E validation and stability audit.
- Full roadmap synchronization and task finalization.
- Integrated multi-user concurrency and clipboard sync verification.

## [0.1.16-alpha] - 2025-01-24
### Added
- Integrated benchmarking mode via `--bench` flag.
- Detailed performance telemetry logging (jitter, frame statistics).
- Generated PERFORMANCE.md with documented alpha benchmarks.
- Cleaned up unused GDI resources in OverlayEngine.

## [0.1.15-alpha] - 2025-01-24
### Added
- Comprehensive E2E performance validation and metric logging.
- Optimized NetworkManager socket buffer drainage for minimal input lag.
- Enhanced performance instrumentation (frame time, peer latency, user count).
- Validated system stability under concurrent multi-client high-frequency load.

## [0.1.14-alpha] - 2025-01-24
### Added
- Cross-network clipboard synchronization (text-based).
- Expanded Packet structure with 1024-byte data payloads.
- Integrated clipboard polling and remote update application.

## [0.1.13-alpha] - 2025-01-24
### Added
- Multi-monitor support via Virtual Screen metrics (SM_XVIRTUALSCREEN, etc.).
- Refined DriverInterface with clear stub signaling and architecture notes.
- Integrated multi-monitor coordinate normalization and clamping.
- Verified system stability for heterogeneous display arrangements.

## [0.1.12-alpha] - 2025-01-24
### Added
- Formal E2E test case for coordinate transformation fidelity.
- Optimized zero-latency routing for AbsoluteMovement packets.
- Priority rebroadcasting in server mode for global state consistency.
- Refined temporal coordination and packet processing loop.

## [0.1.11-alpha] - 2026-05-23
### Added
- High-precision clock synchronization protocol using 100ms Heartbeat packets.
- Velocity-based Predictive Interpolation (Dead Reckoning) in SyncModule.
- Optimized continuous rendering in OverlayEngine driven by the sync engine.
- Enhanced RTT latency tracking and one-way network delay estimation.
- Improved temporal coordination using deltaTime-adjusted interpolation.

## [0.1.10-alpha] - 2026-05-23
### Added
- Automated multi-client concurrency simulation test.
- Stall detection mechanism in SyncModule (3s timeout).
- Dynamic 'Active User' display in ConfigGUI.
- Refined multi-peer activity tracking and status monitoring.

## [0.1.9-alpha] - 2026-05-23
### Added
- Comprehensive edge-case unit tests for cursor handling.
- Coordinate normalization tests for extreme resolutions.
- Jitter buffer overflow protection and verification.
- CLI color propagation to remote cursor overlay.
- Robust NetworkManager receive logic for server-mode stability.

## [0.1.8-alpha] - 2026-05-23
### Added
- Performance instrumentation logging frame deltas.
- Coordinated smoothing using exponential decay interpolation.
- Master Lock mechanism for active cursor switching.
- Python-based E2E performance validation script.

## [0.1.7-alpha] - 2026-05-23
### Added
- LERP interpolation in SyncModule for fluid cursor movement.
- Jitter buffer for handling network delay and packet reordering.
- Delta-time based temporal coordination in main loop.

## [0.1.6-alpha] - 2026-05-23
### Added
- Dedicated SyncModule for centralized state management.
- Cursor ownership and 'Active Peer' Designation.
- Active User status indicator in ConfigGUI.

## [0.1.5-alpha] - 2026-05-22
### Added
- Absolute coordinate normalization (0-65535).
- Server-side state rebroadcasting for conflict resolution.
- Optimized multi-packet network ingest.

## [0.1.4-alpha] - 2026-05-22
### Added
- Interactive peer selection in ConfigGUI.
- Real-time latency monitoring integration.

## [0.1.3-alpha] - 2026-05-22
### Added
- Multi-peer support in NetworkManager.
- Bitmap-based cursor rendering (arrow icons).
- Capture-release safety mechanism using raw input deltas.

## [0.1.2-alpha] - 2026-05-22
### Added
- Automated peer discovery via UDP broadcasting.
- Win32 settings window for interactive configuration.
- Crossover release mechanism to regain local cursor control.
- Optimized flicker-free GDI rendering using `UpdateLayeredWindow`.
- Enhanced interaction focus handling with `WindowFromPoint` and `SetForegroundWindow`.
- Support for custom cursor colors.

## [0.1.1-alpha] - 2026-05-22
### Added
- Bidirectional non-blocking UDP/TCP networking.
- Command-line argument parsing for role and boundary configuration.
- Win32 raw input (WM_INPUT) for mouse delta and button capture.
- Low-level mouse hook (WH_MOUSE_LL) for cursor suppression.
- GDI-based transparent layered window overlay for remote cursor rendering.
- "Warp-Click-Restore" logic for remote interaction focus.

## [0.1.0-alpha] - 2026-05-22
### Added
- Initial project architecture and documentation.
- Project vision and roadmap defined.
- Stub implementation of core modules (Driver, Input, Networking, UI).
