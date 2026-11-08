## [0.1.109-alpha] - 2026-11-04

### Added
- SpatialAnchor 3D Node Tracking in SpatialViewport:
  - Defined `SpatialAnchor` struct and implemented `SetSpatialAnchor`, `GetSpatialAnchor`, `SetActiveAnchorFocus`, and `RemoveSpatialAnchor` in `SpatialViewport`.
  - Added unit test `test_spatial_viewport_anchors()` in `tests/test_main.cpp`.

## [0.1.108-alpha] - 2026-11-04

### Added
- Directional Spatial Audio Panning:
  - Implemented `CalculateSpatialAudioGain` and `ApplySpatialAudioPan` in `WebRTCManager` applying constant-power stereo panning laws based on peer cursor coordinates.
  - Added unit test `test_spatial_audio_panning()` in `tests/test_main.cpp`.

## [0.1.107-alpha] - 2026-11-04

### Added
- Multi-File Batch Transfer Queueing & Enhanced Drag-and-Drop Staging:
  - Added `StartBatchTransfer()` to `FileTransferEngine` and `QueueBatchFileTransfer()` to `NetMuxFramework`.
  - Updated Command Dashboard UI (`ui/index.html`) dropzone to process multi-file batches, reporting total payload sizes, queuing individual transfers, and updating batch status indicators.
  - Added unit test `test_file_transfer_batch_queue()` in `tests/test_main.cpp`.

## [0.1.106-alpha] - 2026-11-04

### Added
- Dynamic Resolution & DPI-Aware Coordinate Scaling:
  - Implemented `ScalePeerCoordinates()` in `SyncModule` to accurately project normalized (0-65535) coordinates across peers with distinct display resolutions and DPI scales.
  - Added unit test `test_dynamic_resolution_dpi_scaling()` in `tests/test_main.cpp`.

## [0.1.105-alpha] - 2026-11-04

### Added
- Integrated Video Chat Media Stream Synchronization:
  - Implemented `PeerMediaStream` struct and synchronized stream management in `WebRTCManager` (`SyncPeerMediaStreams`, `GetPeerMediaStream`, `ClearPeerMediaStream`) to coordinate video packets, spatial audio, and presentation timestamps.
  - Added real-time Video Chat status badge to the live spatial minimap stats bar in the Electron Command Dashboard.
  - Added unit test `test_integrated_video_chat_sync()` in `tests/test_main.cpp`.

## [0.1.104-alpha] - 2026-11-04

### Added
- WebRTC Spatial Audio Track Pipeline & Dashboard Integration:
  - Implemented `EnableAudioTrack()`, `PushAudioFrame()`, and `SetAudioFrameCallback()` in `WebRTCManager`.
  - Added "Enable Spatial Audio Channel" and "Enable Peer Webcam Stream" controls with descriptive tooltips to the Spatial Viewport card in `ui/index.html`.
  - Persisted `enable_audio` and `enable_webcam` flags across UI and `netmux.cfg`.
  - Added unit test `test_webrtc_audio_pipeline()` in `tests/test_main.cpp`.

## [0.1.103-alpha] - 2026-11-04

### Added
- File Transfer Engine Telemetry & Dashboard Integration:
  - Added `QueueFileTransfer()` API on `NetMuxFramework` for programmatic initiation of chunked multi-part file streaming.
  - Telemetry JSON serialization of active file transfer progress (`transfers` array) dispatched to stdout in headless mode.
  - Real-time file transfer progress bar and status indicator inside the Command Dashboard (`ui/index.html`).
  - Added unit test `test_file_transfer_engine_queue_and_telemetry()` verifying queueing, chunk transmission, completion, and hash verification.

## [0.1.102-alpha] - 2026-11-04

### Added
- Continuous Multi-Peer Drag Selection Synchronization:
  - Attached selection metadata directly to `AbsoluteMovement` and `DeltaMovement` packets in `InputEngine.cpp`.
  - Added real-time bounding box rendering on the Command Dashboard minimap canvas.
  - Added unit test `test_continuous_drag_selection_sync()` validating drag coordinates and release state transitions.

## [0.1.101-alpha] - 2026-11-04

### Added
- Multi-Team Secure Isolation & Group Routing:
  - Scoped server-side `FocusUpdate` and `MasterStateSync` rebroadcasts strictly to each peer's `groupId` via `SendPacketToGroup()`.
  - Added dynamic active group isolation indicator badge on the Live Multi-Peer Minimap in `ui/index.html`.
  - Added unit test `test_multi_team_group_isolation()` in `tests/test_main.cpp`.

## [0.1.100-alpha] - 2026-11-04

### Added
- Authoritative Synchronization & Drift Arbitration:
  - Server-side Euclidean coordinate drift tracking and arbitration during `SyncCheck` packet evaluations.
  - Automatic `MasterStateSync` reconciliation when client coordinate discrepancy exceeds threshold (> 1500 units).
  - Client-side coordinate snapping with drift calculation and correction counter tracking in `SyncModule`.
  - Live sync drift and authoritative correction metrics added to headless JSON telemetry and the Command Dashboard performance metrics bar (`ui/index.html`).
  - Added unit test `test_authoritative_drift_arbitration()` in `tests/test_main.cpp`.

## [0.1.99-alpha] - 2026-11-04

### Added
- Collaborative Team Group Isolation Dashboard UI:
  - De-aggregated "Session / Group" field in `ui/index.html` into independent "Session Name", "Group Name", and "Group ID" controls.
  - Linked bidirectional config loading and saving with `netmux.cfg` for team group names.
  - Added descriptive tooltips explaining team/tenant workspace segmentation.

## [0.1.98-alpha] - 2026-11-04

### Added
- Native WebRTC Desktop Video Integration (`WebRTCManager.hpp`, `WebRTCManager.cpp`):
  - Embedded `VideoEncoder` and `VideoDecoder` directly into `WebRTCManager` for hardware-accelerated local desktop frame compression.
  - Implemented `EncodeLocalDesktopFrame` with H.264 bitstream packet delivery over low-latency DataChannels.
  - Added configurable incoming video frame callbacks (`SetVideoFrameCallback`) and `DecodeRemoteVideoPacket`.
  - Added `test_webrtc_manager_pipeline()` unit test in `tests/test_main.cpp`.

## [0.1.97-alpha] - 2026-11-04

### Added
- Complete H.264 Video Codec Pipeline (`VideoEncoder`, `VideoDecoder`):
  - Configurable resolution, bitrate, fps, and low-latency profiles via `VideoEncoderConfig` and `VideoDecoderConfig`.
  - Windows Media Foundation H.264 MFT setup, sample buffer feeding, and DXGI output extraction.
  - Cross-platform NAL unit stream verification, Annex B prefix validation, and corrupted stream rejection.
  - Added `test_video_codec_pipeline()` in `tests/test_main.cpp`.
- Updated `TODO.md` and `ROADMAP.md` marking H.264 encoder/decoder complete.

## [0.1.96-alpha] - 2026-11-04

### Added
- Collaborative Selection Bounding Box Rendering:
  - Updated `NetMuxFramework.cpp` to stream `selStartX` and `selStartY` in JSON telemetry.
  - Enhanced Electron live minimap in `ui/index.html` to render live selection bounding rectangles with dashed borders and fill while dragging.

## [0.1.95-alpha] - 2026-11-04

### Added
- Integrated `test_group_isolation()` unit test into `test_main.cpp` suite, verifying server group-aware packet filtering.
- Group-Aware Clipboard Relay: Updated server clipboard forwarding in `NetMuxFramework.cpp` to use `SendPacketToGroup` instead of broadcast to ensure multi-team isolation.
- Fixed boundary coordinate parsing in UI config extraction to safely accept 0 without defaulting to 1919.

## [0.1.94-alpha] - 2026-11-04

### Added

- Hardware Webcam Capture Device Enumeration (`WebcamCapture.hpp`, `WebcamCapture.cpp`):
  - Implemented Windows Media Foundation `IMFMediaSource` device enumeration via `MFEnumDeviceSources` with `MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID`.
  - Added `WebcamDeviceInfo` metadata tracking device index, friendly names, and symbolic endpoint IDs.
  - Added unit test `test_webcam_capture()` in `tests/test_main.cpp` validating device enumeration, activation, reading, and cleanup lifecycle across platforms.

## [0.1.93-alpha] - 2026-11-04

### Added

- Focus Halo Visual Feedback Refinement (`D3D11Overlay.cpp`, `ui/index.html`):
  - Upgraded active peer cursor indicator in D3D11Overlay with dual-layer electric cyan animated focus halo ring.
  - Added matching live focus halo pulse rings in the dashboard minimap canvas around the active input owner.
- Bandwidth Optimization (`InputEngine.cpp`):
  - Implemented adaptive delta compression throttling (1:25 absolute anchor ratio during continuous high-velocity motions).
  - Added sub-pixel zero-delta redundant packet suppression to conserve network traffic.

## [0.1.92-alpha] - 2026-11-04

### Added

- Interactive Screen Edge Boundary Visualizer in Spatial Viewport (`SpatialViewport.hpp`, `SpatialViewport.cpp`):
  - Dynamic D3D11 boundary line geometry rendering directly between local and remote workspace planes with neon electric cyan highlight.
  - Linked boundary configuration (`SetBoundaryConfig`, `GetBoundaryConfig`) directly between `NetMuxFramework` and `SpatialViewport`.
  - Interactive boundary edge visualizer with real-time drag adjustments in both the configuration dashboard and live minimap (`ui/index.html`).

## [0.1.91-alpha] - 2026-11-04

### Added

- Comprehensive unified single-page Command Dashboard redesign (`ui/index.html`, `ui/main.js`, `ui/preload.js`):
  - Consolidated all controls, screens, boundary configuration, live minimap, diagnostics, file staging dropzone, and performance metrics onto one cohesive dashboard.
  - Added descriptive labels and contextual tooltips `[?]` explaining all settings (network identity, virtual HID drivers, pre-shared key, 3D viewport, and boundary hops).
  - Wired full two-way config persistence (`loadConfig` and `saveConfig` IPC) directly synchronizing with `netmux.cfg`.
  - Added Electron native System Tray integration with menu actions (Show Dashboard, Stop NetMux Backend, and Quit MultiMousergy) with double-click window toggle.
  - Added expanded CLI arguments in `src/main.cpp` (`--driver-type`, `--display-name`, `--session-name`, `--group-name`, `--group-id`, `--peer-color`, `--sel-color`, `--cursor-theme`, `--cursor-scale`, `--spatial`) for complete UI-to-backend parity.
  - Organized and modernized build/run scripts under `scripts/` (`build.sh`, `run_server.sh`, `run_client.sh`, `build.bat`, `run_server.bat`, `run_client.bat`), archiving legacy files nondestructively.

## [0.1.90-alpha] - 2026-08-03

### Added

- System tray icon with programmatically-drawn cyan mouse cursor
- Right-click context menu: Show Stats, Toggle Overlay, About, Exit
- Hover tooltip shows mode (Server/Client) and live peer count (updates every 2s)
- Double-click tray icon shows version balloon notification
- Tray exit request cleanly shuts down the framework
- `shell32` linked for Shell_NotifyIcon API

## [0.1.89-alpha] - 2026-07-15

### Added

- Repository reconciliation: all feature branches merged into main (netmux-initial-architecture, refactor-ui-network-stack, jules-implement-dxgi, jules-implement-x11, fix-simultaneous-cursors, jules-14870789006794460373)
- Self-peer filtering in perf logs, MasterStateSync, SyncCheck, and spatial rendering
- DesktopCapture success/error logging and proper destructor cleanup
- D3D11 spatial pipeline success confirmation logging

### Fixed

- CMake project version now synced with VERSION.md (0.1.89)
- Logger truncates log on startup (was append-only, growing unbounded)
- Cached hasExternalPeers check (was calling GetAllPeers every frame)
- SpatialViewport::Render guarded on !overlayPeers.empty()

## [0.1.88-alpha] - 2026-06-29

### Added

- Integrated WebRTC SDP/ICE handshake and DXGI texture encoding
- Cross-platform driver compatibility with macOS CoreGraphics stub

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
