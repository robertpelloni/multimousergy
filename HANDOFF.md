## Session Handoff - Spatial Viewport Integration

### Actions Performed
1. Initialized the `SpatialViewport` class with DirectX 11 matrix math placeholders.
2. Updated `NetMuxFramework::Run()` to pipe cursor state data (`overlayPeers`) down to the `SpatialViewport::Render` method when D3D11 is enabled.
3. Updated `OverlayEngine` and `D3D11Overlay` to expose the D3D11 Context to the framework so it can be passed to the Spatial Viewport.
4. Bumbped version to `0.1.78-alpha` and updated the `CHANGELOG.md`.

### Known State
- Build passes on linux.
- Unit tests all pass successfully.
- Web app properly functions as a standalone viewer.

### Future considerations
- Continue expanding the `SpatialViewport` rendering logic to actually draw the 2D planes using the provided cursor state and D3D11 context.
- Implement DXGI Desktop Duplication to feed frames into the `SpatialViewport`.
- Setup WebRTC connection flow.
