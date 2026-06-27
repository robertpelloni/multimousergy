## Session Handoff - Milestone 5 Polish

### Actions Performed
1. Optimized the `MasterStateSync` loop in `NetMuxFramework.cpp` to cache the last broadcasted position and only send updates when the position changes, reducing unnecessary network traffic.
2. Finalized the Native Linux X11 integration in `InputEngine.cpp` by adding a fallback/supplement using `XQueryPointer` to reliably fetch the absolute cursor position, ensuring robust cross-platform cursor support.
3. Bumped version to `0.1.81-alpha` and updated the `CHANGELOG.md`.

### Known State
- Build passes on linux.
- Unit tests all pass successfully.
- Milestone 5 features, including the File Transfer Engine and Multi-Monitor DPI Awareness, are confirmed as fully implemented.

### Future considerations
- Continue expanding the `SpatialViewport` rendering logic to actually draw the 2D planes using the provided cursor state and D3D11 context.
- Implement the actual D3D11 vertex/index buffers and draw calls in `SpatialViewport::Render` to draw the 3D planes.
- Implement the `WebRTCManager` to receive the remote desktop stream and feed it into `m_remoteSRV`.
