# NetMux Memory - v0.1.71-alpha

## UI and Network Stack Refactor
The UI and Network stacks were significantly refactored to resolve race conditions and usability issues.
- **Asynchronous Connection State**: `NetworkManager` now explicitly tracks `ConnectionState`. This prevents the framework from sending handshake packets before the TCP socket is fully connected.
- **Non-Blocking Logic**: `ReceivePacket` now uses `select` to poll for connection completion on the client side. `SafeSend` also uses `select` to wait for writability instead of busy-looping, which reduces CPU usage during network congestion.
- **Win32 Window Management**: `ConfigGUI` now implements a single-instance check using `IsWindow`. This prevents the "multiple windows" bug where every re-initialization would spawn a new overlapping window.
- **UX Reorganization**: Controls are now grouped logically using group boxes. Ambiguous labels like "Save & Start" have been replaced with "Apply & Connect", and a "Disconnect" button was added to provide a clear exit path for active sessions.

## Architectural Lessons
- **Deferred Handshaking**: Relying on the return value of `connect()` is insufficient for non-blocking sockets. Explicit state tracking and polling via `select` are mandatory for reliable protocol initialization.
- **Win32 Message Passing**: Using `WM_USER + 1` for re-initialization signals is effective but requires careful cleanup of window handles to avoid resource leaks.
- **Resource Lifecycle Guarding**: In decentralized systems, pruning dead peers must trigger recursive cleanup of all associated data structures (file buffers, reassembly queues) to prevent memory exhaustion.
- **UX State Synchronization**: Dynamic UI elements (like button text or status bars) should be driven by the core `ConnectionState` machine to ensure visual feedback matches internal logic.
## Merge Resolution Architectural Lessons
* The UI/Network stack refactor safely isolates the `NetMuxFramework` from the raw connection handlers.
* The `NetworkManager` effectively prevents interleaving by buffering on a per-client basis. Integrating new network flows like WebRTC SDPs into the existing `NetworkManager::SendPacket` pipelines requires careful `std::min` clamping of `payloadSize` against the `payload` buffer (e.g. `sizeof(payload) - 1`) to prevent OOB reads by remote peers parsing strings out of standard statically-sized packet packets.
* The `FileTransferEngine` can be reliably simulated in tests without relying on `NetworkManager` by generating mock `NetMuxPacketType::FileHeader` and `FileData` packets, passing them directly to a local receiver instance, and asserting on `isComplete` and `lastError` enums for SHA-256 verification and chunk reassembly.
* The File Transfer Engine's interruption/resume logic relies on validating the SHA-256 hash and exact file size transmitted in the initial `FileHeader` packet. If a connection drops, resending the header allows the receiver to seamlessly continue appending `FileData` chunks to the existing buffer without discarding previously downloaded data.
