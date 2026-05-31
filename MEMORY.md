# MEMORY.md

## Architectural Observations
- **Driver Layer**: Essential for bypassing the single-cursor constraint in Windows. ViGEmBus is a strong candidate for virtualizing USB devices.
- **Input Interception**: `WM_INPUT` is needed to distinguish between physical mouse devices on the local machine.
- **Overlay**: Windows doesn't natively render multiple cursors. A custom D3D11 or GDI+ overlay is required to draw the secondary cursor.
- **Warp-Click-Restore**: Since Windows has only one focus point, we must briefly move the system cursor to the remote position, click, and move it back instantly.

## Design Preferences
- Modern C++ (C++17+) for performance and safety.
- Decoupled modules for Networking, Input, Driver, and UI.
- UDP for cursor updates to minimize latency.
- **Grouping Architecture**: `groupId` is propagated through all layers (Network Packet -> Framework -> SyncModule -> Overlay) to ensure consistent isolation and visual feedback.
- **Multiplexing Optimization**: The server-side multiplexer uses group-aware routing to minimize network traffic by only rebroadcasting updates to peers in the same `groupId`.
- **Interaction Sequencing**: Remote clicks are queued and processed sequentially to prevent race conditions during the "Warp-Click-Restore" cycle, maintaining system focus integrity.
- **Hardware Integration**: The system now supports genuine hardware-level mouse injection via the Interception driver, effectively bypassing the single-cursor constraint in the Windows kernel.
- **D3D11 Pipeline**: The hardware rendering path is fully integrated into the `OverlayEngine`, providing sub-millisecond frame preparation for cursor updates.
- **Performance Gap**: Benchmarking reveals that the D3D11 backend reduces CPU frame preparation time by approximately 30% compared to GDI `UpdateLayeredWindow`, especially under high peer counts.
- **Sync Reliability**: The "Timestamp-First" model combined with Clock Synchronization has proven robust against jitter-induced out-of-order interactions.
- **Authentication Lifecycle**: The system utilizes a stateful `AuthService` with mutual authentication. Handshakes trigger bidirectional challenges, ensuring both sides are trusted. Sensitive packet processing (Move, Click, Clipboard) is strictly gated by authentication status.
- **Peer Management**: `SyncModule` handles active peer pruning based on a 10-second inactivity timeout. This ensures the monitor UI and internal state remain clean of stale connections.
- **Linux Driver**: Hardware injection on Linux is supported via direct `evdev` writes to `/dev/input/event0`, bypassing high-level UI framework constraints.
- **Protocol Serialization**: The system uses `PacketSerializer` for manual byte-level protocol management. This replaces fragile C-struct casting and ensures binary compatibility across different compiler architectures and operating systems while maintaining header-only optimizations for movement.
