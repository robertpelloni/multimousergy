# NetMux Session Handoff - v0.1.114-alpha

## Repository Synchronization Summary (2026-11-04)

### 3D Spatial Raycasting & Plane Intersection (v0.1.114-alpha)
- **VR/AR Spatial 3D Cursor Engine**:
  - Implemented `Ray3D`, `ScreenToRay`, and `IntersectRayWithAnchorPlane` in `SpatialViewport` to project screen inputs into 3D world rays and compute exact normalized UV cursor coordinates across spatial peer planes.
  - Added unit test `test_spatial_raycasting_and_intersection()` in `tests/test_main.cpp`.

### Wayland Integration & Portal-Based Input Capture (v0.1.113-alpha)
- **Wayland Display Server & Portal Session Manager**:
  - Implemented `WaylandPortalManager` (`DetectDisplayServer`, `IsWaylandSession`, `IsPortalAvailable`, `RequestInputCaptureSession`, `ReleaseInputCaptureSession`, `InjectVirtualPointerEvent`) supporting modern secure Linux Wayland desktop environments.
  - Added unit test `test_wayland_portal_manager()` in `tests/test_main.cpp`.

### Linux Virtual Hardware Mouse Emulation via uinput (v0.1.112-alpha)
- **Virtual HID Hardware Emulation**:
  - Implemented `InitUInputDevice` in `DriverInterface`, constructing a clean virtual USB mouse node on `/dev/uinput` with fallback device node scanning.
  - Added unit test `test_driver_interface_virtual_device()` in `tests/test_main.cpp`.

### Multi-Monitor Virtual Display Layout Mapping (v0.1.111-alpha)
- **Virtual Canvas to Physical Display Mapper**:
  - Implemented `MonitorDescriptor`, `AddLocalMonitor`, `ClearLocalMonitors`, and `MapVirtualToPhysicalMonitor` in `SyncModule` to accurately resolve spanning multi-monitor virtual coordinates to physical monitor IDs and local pixel coordinates.
  - Added unit test `test_multimonitor_layout_mapping()` in `tests/test_main.cpp`.

### Group-Scoped Interaction Ownership (v0.1.110-alpha)
- **Multi-Team Isolated Ownership Model**:
  - Implemented `ResolveGroupInteraction` and `GetGroupActivePeer` in `SyncModule`, maintaining per-group active interaction ownership states and timestamps so independent teams sharing the same network retain full local focus and uninterrupted simultaneous control.
  - Added unit test `test_group_scoped_interaction_ownership()` in `tests/test_main.cpp`.

### SpatialAnchor 3D Node Tracking (v0.1.109-alpha)
- **Spatial Node Anchoring**:
  - Defined `SpatialAnchor` struct and implemented dynamic anchor mapping (`SetSpatialAnchor`, `GetSpatialAnchor`, `SetActiveAnchorFocus`, `RemoveSpatialAnchor`) in `SpatialViewport`.
  - Added unit test `test_spatial_viewport_anchors()` in `tests/test_main.cpp`.

### Directional Spatial Audio Panning (v0.1.108-alpha)
- **Spatial Voice Audio Panning**:
  - Implemented `CalculateSpatialAudioGain` and `ApplySpatialAudioPan` in `WebRTCManager` delivering constant-power stereo panning based on peer cursor horizontal coordinates (0 = left, 32767 = center, 65535 = right).
  - Added unit test `test_spatial_audio_panning()` in `tests/test_main.cpp`.

### Multi-File Batch Transfer Queueing & Enhanced Staging (v0.1.107-alpha)
- **Batch File Transfer Engine**:
  - Added `StartBatchTransfer` to `FileTransferEngine` and `QueueBatchFileTransfer` to `NetMuxFramework`.
  - Updated Command Dashboard UI (`ui/index.html`) to stage multi-file drops with aggregate size calculations, individual file queueing, and visual batch status progress.
  - Added unit test `test_file_transfer_batch_queue()` in `tests/test_main.cpp`.

### Dynamic Resolution & DPI-Aware Coordinate Scaling (v0.1.106-alpha)
- **Coordinate Projection Engine**:
  - Implemented `ScalePeerCoordinates` in `SyncModule` to dynamically project normalized 0-65535 cursor positions between heterogeneous client displays (e.g. 4K 3840x2160 at 1.5x DPI to 1080p 1920x1080 at 1.0x DPI).
  - Added unit test `test_dynamic_resolution_dpi_scaling()` in `tests/test_main.cpp`.

### Integrated Video Chat Stream Synchronization (v0.1.105-alpha)
- **Synchronized Media Stream Architecture**:
  - Implemented `PeerMediaStream` struct in `WebRTCManager.hpp` tracking incoming H.264 video frame packets, PCM audio chunks, presentation timestamps (PTS), and frame index sequences.
  - Added synchronizer APIs (`SyncPeerMediaStreams`, `GetPeerMediaStream`, `ClearPeerMediaStream`) to maintain lip-sync and lock audio/video presentation clocks across participating peers.
  - Added live `Video Chat: Standby / Active` status badge directly onto the live minimap stats bar in `ui/index.html`.
  - Added unit test `test_integrated_video_chat_sync()` in `tests/test_main.cpp`.

### Spatial Audio Pipeline & UI Webcam Toggles (v0.1.104-alpha)
- **WebRTC Spatial Audio**:
  - Implemented `EnableAudioTrack()`, `PushAudioFrame()`, and `SetAudioFrameCallback()` in `WebRTCManager`.
  - Added explicit UI switches for "Enable Spatial Audio Channel" and "Enable Peer Webcam Stream" in `ui/index.html` with bidirectional persistence to `netmux.cfg`.
  - Added unit test `test_webrtc_audio_pipeline()`.

### File Transfer Queueing & Telemetry Staging (v0.1.103-alpha)
- **Engine Queue & Headless Telemetry**:
  - Added `QueueFileTransfer()` API on `NetMuxFramework`.
  - Serialized active file transfers (`transfers` array) to stdout JSON telemetry in headless mode.
  - Real-time progress bar rendering in the Command Dashboard (`ui/index.html`).
  - Added unit test `test_file_transfer_engine_queue_and_telemetry()`.

### Continuous Drag-and-Drop Selection Synchronization (v0.1.102-alpha)
- Attached drag selection metadata (`isSelecting`, `selectionStartX`, `selectionStartY`) directly to `AbsoluteMovement` and `DeltaMovement` packets.
- Rendered live dashed cyan bounding boxes on the minimap canvas during continuous multi-cursor selections.

### Multi-Team Group Isolation & Authoritative Drift Arbitration (v0.1.99-alpha – v0.1.101-alpha)
- Added `SendPacketToGroup()` in `NetworkManager` for server-side packet isolation across collaborative teams.
- Authoritative Euclidean coordinate drift arbitration (>1500 units) snapping client cursors with `MasterStateSync`.

### Hardware Media Codecs & Native WebRTC Video (v0.1.94-alpha – v0.1.98-alpha)
- Windows Media Foundation webcam capture enumeration in `WebcamCapture.cpp`.
- Hardware H.264 MFT encoder/decoder pipeline in `VideoEncoder.cpp` / `VideoDecoder.cpp`.
- Native DataChannel video streaming in `WebRTCManager.cpp`.

### Unified Command Dashboard & CLI Modernization (v0.1.91-alpha – v0.1.93-alpha)
- Single-page responsive Command Dashboard (`ui/index.html`, `ui/main.js`, `ui/preload.js`).
- System Tray with background process lifecycle controls.
- Interactive screen edge boundary visualizer in `SpatialViewport.cpp`.
- Glowing dual-layer focus halo rings in `D3D11Overlay.cpp`.
- Adaptive delta compression (1:25 anchor ratio) in `InputEngine.cpp`.

## Project State
- **Version**: `v0.1.105-alpha`
- **Build**: CMake + GCC/Clang/MSVC, 100% CTest pass rate (22/22 unit tests passing).
- Working tree clean.

## Notes for Successor
- All Milestone 6 items (H.264 codecs, WebRTC native integration, spatial audio, and integrated video chat stream synchronizer) are fully implemented and verified.
- The Electron frontend runs headless C++ netmux instances and consumes JSON telemetry via stdout.
