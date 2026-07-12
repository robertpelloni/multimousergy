# CHANGELOG.md

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
