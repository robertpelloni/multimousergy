# CHANGELOG.md

## [0.1.41-alpha] - 2026-05-24
### Added
- Integrated the Interception library for hardware-level cursor injection, replacing the software-level fallback stub.
- Updated `DriverInterface` to utilize `interception_send` for independent remote cursor movement and clicks, bypassing single-cursor kernel limitations.
- Refactored `InputEngine` state access to use encapsulated getters/setters for improved class safety.
- Updated `CMakeLists.txt` to dynamically link the Interception library and ensure DLL propagation.
- Marked Milestone 1 (Virtual HID Driver) and D3D11 integration as completed in the project ROADMAP and TODO files.

## [0.1.40-alpha] - 2025-01-22
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
