# IDEAS.md

## Future Enhancements
- **Multi-Platform Support (Priority)**: Refactor core modules into a platform-agnostic library. Implement `uinput` for Linux and `CGEvent` for macOS to provide native hardware injection.
- **Wayland Integration**: Research Portal-based input capture for high-security Linux environments.
- **Clipboard Sharing**: Sync clipboard content across the network.
- **File Drag-and-Drop**: Transparently move files between PCs via the network cursor.
- **Custom Cursor Themes**: Allow users to skin the secondary cursors.
- **Multi-Monitor Awareness**: Intelligent handling of multi-monitor setups on either side of the network.
- **Port to Rust**: Rewriting core logic in Rust for memory safety and modern concurrency.
