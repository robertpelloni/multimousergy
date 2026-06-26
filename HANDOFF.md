## Session Handoff - Spatial Viewport Integration & DXGI Desktop Duplication

### Actions Performed
1. Integrated DXGI Desktop Duplication API into `NetMuxFramework::Run()` to capture screen frames continuously when D3D11 is enabled.
2. Verified that the `DesktopCapture` class implements `AcquireFrame` and `ReleaseFrame` correctly.
3. Updated `NetMuxFramework::Initialize()` to start both the `DesktopCapture` module and the `WebRTCManager` to prepare for the 3D Composition Engine and media streams (Milestone 6).
4. Bumped version to `0.1.79-alpha` and added entries to `CHANGELOG.md`.

### Known State
- Build passes on linux.
- Unit tests all pass successfully.
- SpatialViewport class has D3D11 initialization and matrices calculated.
- `NetMuxFramework` is passing cursor data to the Spatial Viewport.

### Future considerations
- Continue expanding the `SpatialViewport` rendering logic to actually draw the 2D planes using the provided cursor state and D3D11 context.
- Start passing the captured DXGI frames to the WebRTC video stream.
