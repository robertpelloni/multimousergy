# CHANGELOG.md

## [0.1.79-alpha] - 2026-06-26
### Added
- Integrated D3D11 Desktop Duplication API capture loop for the Spatial Viewport rendering pipeline.

## [0.1.77-alpha] - 2026-06-25
### Fixed
- Fixed an issue where the TCP replay cache pruning would cause a performance issue during high volume transfers by limiting cleanup to once every 10 seconds.

## [0.1.71-alpha] - 2025-01-24
### Added
- Implemented Dynamic Resolution Scaling and DPI Awareness.
- Enhanced protocol to broadcast DPI scale factors.
- Updated ConfigGUI with remote resolution and DPI telemetry in the peer list.

## [0.1.70-alpha] - 2025-01-24
### Added
- Refined ConfigGUI UX: persistent status bar, dynamic button text, and "Connect to Selected" button.
- Enhanced Network Resilience: automatic cleanup of partial file/clipboard data from dead peers.
- Integrated `STATUSCLASSNAME` for real-time connection telemetry.

## [0.1.69-alpha] - 2025-01-24
### Added
- Implemented Friendly Name broadcasting and peer-list prioritization.
- Added "Start Minimized to Tray" global preference.
- Enhanced protocol handshake to include user-defined display names.
### Fixed
- Improved UI child window management for multi-tab environments.

## [0.1.68-alpha] - 2025-01-24
### Added
- Refactored ConfigGUI to a modular tab-driven architecture.
- Implemented Delta Compression for high-frequency cursor updates.
- Transitioned DriverInterface to dynamic SDK loading (ViGEmBus/Interception).
- Added "Global Settings" tab with customizable "Display Name" support.
- Optimized tab switching with child window visibility management.

## [0.1.67-alpha] - 2025-01-24
### Added
- Implemented System Tray support (Windows) with minimize-to-tray and context menu.
- Integrated a comprehensive Diagnostics Logger with UI "Logs" tab and file output.
- Unified packet serialization with `IsHeaderOnly` optimization.
- Added a "Quit" button and improved UI layout polish.

## [0.1.66-alpha] - 2025-01-24
### Added
- Implemented interactive Peer Selection via the GUI Minimap.
- Added descriptive UI Tooltips for all major controls in ConfigGUI.
- Enhanced File Transfer Engine for multi-file concurrent transfers.
- Added local machine info (Hostname/IP) to the connection panel.
### Fixed
- Improved Tabbed GUI layout and child window visibility management.

## [0.1.65-alpha] - 2025-01-24
### Added
- Modernized ConfigGUI with a Tabbed Interface for better scalability.
- Implemented automatic driver detection (ViGEmBus/Interception) with UI feedback.
- Enhanced File Transfer Engine with basic Resume support and error reporting.
- Improved SyncModule robustness for peer re-entry and collision handling.
- Added automatic network discovery scan on startup.

## [0.1.64-alpha] - 2025-01-24
### Added
- Implemented full Keyboard Synchronization (capture and injection).
- Added Machine Info (Hostname/IP) to ConfigGUI Connection Settings.
- Implemented native Linux X11 event dispatching for improved clipboard stability.
- Enhanced ConfigGUI layout with improved group box organization.

## [0.1.63-alpha] - 2025-01-24
### Added
- Implemented Auto-Reconnect logic for resilient connection recovery.
- Added Peer Color customization controls to ConfigGUI.
- Improved network state robustness with graceful disconnection handling.
- Enhanced ConfigGUI with "Clear History" and expanded layout.

## [0.1.62-alpha] - 2025-01-24
### Added
- Implemented SHA-256 file integrity verification for file transfers.
- Added "Recent Servers" history dropdown to ConfigGUI.
- Improved real-time UI feedback for connection states.
### Fixed
- Optimized ConfigGUI listboxes to prevent flickering and reduce CPU overhead.

## [0.1.61-alpha] - 2025-01-24
### Added
- Implemented File Transfer Engine for transparent drag-and-drop file sharing via multi-part protocol.
- Integrated file transfer progress monitoring into ConfigGUI.
- Added asynchronous file chunking and reassembly logic.

## [0.1.60-alpha] - 2025-01-24
### Fixed
- Refactored UI and Network stacks for robust connection handling and single-instance window management.
- Implemented asynchronous handshake logic to ensure TCP connectivity before protocol initialization.
- Reorganized ConfigGUI layout for improved UX and accessibility.
- Fixed multiple window instance bug in ConfigGUI.

## [0.1.59-alpha] - 2025-01-24
### Fixed
- Fixed simultaneous cursor visibility by implementing continuous broadcasting of local cursor coordinates.
- Added `RefreshPeer` to `SyncModule` to prevent keep-alive packets from resetting peer positions to (0,0).
- Integrated `SyncCheck` handling in `NetMuxFramework` for improved peer lifecycle stability.
- Verified multi-cursor synchronization with an expanded test suite.

## [0.1.58-alpha] - 2025-01-24
### Fixed
- Fixed simultaneous cursor visibility by implementing continuous broadcasting of local cursor coordinates. (Initial attempt)
- Corrected D3D11 cursor scaling by accounting for the 2.0 multiplier in NDC space (-1.0 to 1.0).
- Enforced `HWND_TOPMOST` status for the overlay window across both GDI and D3D11 backends.
- Improved coordinate normalization accuracy using actual virtual screen metrics (`SM_CXVIRTUALSCREEN`, `SM_CYVIRTUALSCREEN`).
- Fixed default cursor color generation to prevent conflicts with GDI transparency colorkey (RGB 0,0,0).
- Refined peer connectivity detection logic in `NetMuxFramework`.
- Added descriptive logging for peer lifecycle events (Join, Prune, Remove) in `SyncModule`.
- Implemented authoritative cursor synchronization via server-side `MasterStateSync` enforcement.

## [0.1.57-alpha] - 2025-01-24
### Added
- Implement native X11 persistent display management (optimized connection reuse).
- Implement native X11 clipboard writing and event dispatch (serve data without `xclip`).
- Modernize Unicode conversion with manual UTF-8/UTF-16 bridging (removed deprecated headers).
- Add placeholders for File Transfer protocol integration.

## [0.1.56-alpha] - 2025-01-24
### Added
- Implement native X11 clipboard reading on Linux (optimized over `xclip`).
- Enhance Linux input capture with real X11 cursor synchronization (`XQueryPointer`).
- Refactor `DriverInterface` to support optional native SDK linking (Interception/ViGEmBus).

## [0.1.55-alpha] - 2025-01-24
### Added
- Implement Linux Input Capture in `InputEngine` via `evdev`.
- Implement constant-time comparison in `AuthModule` to mitigate timing attacks.
- Refactor `PacketSerializer` to use a robust `GetHeaderSize` constexpr helper.

## [0.1.54-alpha] - 2025-01-24
### Added
- Implement High-Capacity Clipboard Chunking (unlimited size via multi-part packets).
- Add full D3D11 Custom Cursor Theme support (texture synchronization).
- Optimized D3D11 rendering with dynamic texture updates from Win32 bitmaps.

## [0.1.53-alpha] - 2025-01-24
### Added
- Wired Selection RGB color customization and Cursor Theme browsing to ConfigGUI.
- Refactored `ClipboardModule` with portable `std::wstring_convert` and Linux `xclip` support.
- Added comprehensive unit tests for `ClipboardModule` (Unicode/Hashing).

## [0.1.52-alpha] - 2025-01-24
### Added
- Expand network protocol payload capacity to 4096 bytes for larger clipboard transfers.
- Implement Unicode (UTF-8/UTF-16) clipboard synchronization support via `CF_UNICODETEXT`.
- Add hash-based clipboard change detection for optimized performance.
- Implement temporal clipboard conflict resolution using unified framework timelines.

## [0.1.51-alpha] - 2025-01-24
### Added
- Implemented full Mouse Wheel support (Vertical & Horizontal).
- Enhanced Linux driver with Automated Input Device Discovery (ioctl axis detection).
- Integrated selection rect color customization into all render backends.

## [0.1.50-alpha] - 2025-01-24
### Added
- Implemented Dynamic Cursor Theme loading (.bmp support).
- Added Selection Rectangle color customization.
- Expanded ConfigGUI and ConfigManager to support new personalization features.

## [0.1.49-alpha] - 2025-01-24
### Added
- Refactored `ConfigManager` to use robust `key=value` persistent storage.
- Integrated Automated Peer Discovery into ConfigGUI with auto-fill support.
- Improved UI layout for enhanced telemetry and discovery visibility.

## [0.1.48-alpha] - 2025-01-24
### Added
- Implemented Replay Attack Protection via sequence numbers.
- Expanded ConfigGUI with a real-time Security Event Log.
- Enhanced SyncModule with persistent peer metadata (Total Packets, Last Auth).

## [0.1.47-alpha] - 2025-01-24
### Added
- Implemented Auto-Challenge mechanism for seamless re-authentication.
- Enhanced ConfigGUI with detailed mutual authentication telemetry (LOCKED/OPEN/AUTH).
- Added explicit `Disconnect` packet for graceful session termination.
- Centralized security enforcement via `IsPeerTrusted` helper.

## [0.1.46-alpha] - 2025-01-24
### Added
- Implemented formal network serialization layer via `PacketSerializer`.
- Replaced raw memory casting with byte-level serialization for cross-platform binary compatibility.
- Preserved movement update bandwidth optimization in the serialization layer.

## [0.1.45-alpha] - 2025-01-24
### Added
- Implemented Peer Lifecycle Management: Inactive peers are now pruned after a timeout.
- Added Linux hardware injection support via `evdev` (`/dev/input/event0`).
- Integrated `AuthService` state cleanup during peer pruning.

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
