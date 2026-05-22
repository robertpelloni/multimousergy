# CHANGELOG.md

## [0.1.2-alpha] - 2026-05-22
### Added
- Automated peer discovery via UDP broadcasting.
- Win32 settings window for interactive configuration.
- Crossover release mechanism to regain local cursor control.
- Optimized flicker-free GDI rendering using `UpdateLayeredWindow`.
- Enhanced interaction focus handling with `WindowFromPoint` and `SetForegroundWindow`.
- Support for custom cursor colors.

## [0.1.1-alpha] - 2026-05-22
### Added
- Bidirectional non-blocking UDP/TCP networking.
- Command-line argument parsing for role and boundary configuration.
- Win32 raw input (WM_INPUT) for mouse delta and button capture.
- Low-level mouse hook (WH_MOUSE_LL) for cursor suppression.
- GDI-based transparent layered window overlay for remote cursor rendering.
- "Warp-Click-Restore" logic for remote interaction focus.

## [0.1.0-alpha] - 2026-05-22
### Added
- Initial project architecture and documentation.
- Project vision and roadmap defined.
- Stub implementation of core modules (Driver, Input, Networking, UI).
