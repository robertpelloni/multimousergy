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
