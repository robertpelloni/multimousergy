# NetMux Session Handoff - v0.1.60-alpha

## Summary of Changes
- **Network Stack Refactor**: Implemented `ConnectionState` (Disconnected, Connecting, Connected, Error) in `NetworkManager`. Handshake packets are now deferred until the TCP connection is explicitly verified via `select` and `SO_ERROR` checks.
- **UI UX Overhaul**: Reorganized `ConfigGUI` into logical group boxes. Renamed "Save & Start" to "Apply & Connect" for clarity. Added a "Disconnect" button.
- **Single-Instance UI**: Fixed a bug where multiple overlapping windows were created during re-initialization by using `IsWindow()` checks in `ConfigGUI`.
- **Performance Optimization**: `NetworkManager::SafeSend` now uses `select()` to wait for socket writability instead of busy-looping on `WSAEWOULDBLOCK`.

## State of the Project
- **Version**: `v0.1.60-alpha`
- **Tests**: All unit tests in `NetMuxTests` pass.
- **Next Milestone**: Milestone 5 (File Transfer Engine) is the primary remaining task in the current roadmap.

## Architectural Notes for Successor
- The framework relies on a restart cycle triggered by `ConfigGUI`. When "Apply & Connect" or "Disconnect" is clicked, `s_restartRequested` signals `main.cpp` to shutdown and re-initialize the framework.
- Handshake logic is now state-driven in `NetMuxFramework::Run`. Ensure any new protocol initialization steps follow this pattern.
