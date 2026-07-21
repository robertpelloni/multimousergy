## [0.1.88-alpha] - 2026-06-29
### Added
- Integrated WebRTC SDP/ICE handshake and DXGI texture encoding
- Cross-platform driver compatibility with macOS CoreGraphics stub

## [0.1.82-alpha] - 2026-06-27
### Changed
- Clarified completion of Milestone 4 (Collaborative Selection and Drag Sync) and Milestone 5 (File Transfer Engine).

## [0.1.81-alpha] - 2026-06-27
### Added
- Implemented `XQueryPointer` fallback in `InputEngine` for native Linux X11 integration to ensure robust cursor capturing.

## [0.1.80-alpha] - 2026-06-27
### Added
- Marked DXGI Desktop Duplication, 3D Composition Engine, and Boundary Animation as completed in the Roadmap.

## [0.1.79-alpha] - 2026-06-26
### Added
- Integrated D3D11 Desktop Duplication API capture loop for the Spatial Viewport rendering pipeline.

## [0.1.77-alpha] - 2026-06-25
### Fixed
- Fixed an issue where the TCP replay cache pruning would cause a performance issue during high volume transfers by limiting cleanup to once every 10 seconds.

## [0.1.75-alpha] - 2025-01-24
### Added
- Implemented Linux X11 fallback for `DesktopCapture` using `XGetImage` for low-latency native frame acquisition.
- Set up core architecture headers and structure for WebRTC Integration and Spatial Viewport features.
- Wired X11 Frame Capture for Spatial Viewport.
- Finalized File Transfer Engine with large file handling (10MB+), chunk reassembly, and robust interruption/resume logic tied to SHA-256 integrity checks.
- Implemented Streaming SHA-256 integrity verification for the File Transfer Engine.
- Optimized network protocol by reducing fixed header size from 63 to 48 bytes via multiplexing.
- Enhanced DriverInterface with dynamic SDK loading and function pointer caching for Interception and ViGEmBus.
- Introduced architectural skeletons for MultiMousergy Spatial Evolution (D3D11 Viewport, DXGI Capture).
- Extended protocol to support DPI scaling, Friend Names, and Video bitstreams.

### Fixed
- Refactored NetworkManager into a state-driven machine with non-blocking I/O to prevent UI freezes.
- Improved ConfigGUI with single-instance enforcement and modular tabbed architecture.
- Resolved race conditions in the TCP handshake protocol.
- Optimized cursor broadcasting to ensure continuous visibility across all peers.

## [0.1.0-alpha] - 2025-01-22
### Added
- Initial project architecture and documentation.
- Hybrid UDP/TCP networking for cursor movement and state.
- Coordinate normalization and interpolation (SyncModule).
- Programmatic Win32 Configuration GUI.
