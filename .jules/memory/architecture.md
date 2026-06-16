** Summary of Architecture, Patterns, and Decisions**

### 1. Core Architecture: The Two-Layer Input Model
NetMux (v0.1.60-alpha) is built on a decoupled two-layer system designed to bypass the limitations of standard OS cursor management:
*   **Capture Layer (`InputEngine`)**: Utilizes low-level Windows hooks (`WH_MOUSE_LL`) and Raw Input (`WM_INPUT`) to intercept hardware mouse data. It differentiates between multiple local physical devices and suppresses local cursor movement when the mouse crosses a defined boundary.
*   **Injection Layer (`DriverInterface`)**: Communicates with virtual HID drivers (ViGEmBus or Interception) to inject remote mouse coordinates directly into the Windows input stream. This allows the remote machine to treat the inbound network mouse as a genuine, independent hardware device.

### 2. Networking Strategy: Hybrid Protocol & State Management
The system employs a hybrid networking model to optimize for both speed and reliability:
*   **Lightweight UDP**: Handles high-frequency real-time data such as `Movement`, `AbsoluteMovement`, and `SyncCheck`. It uses independent monotonic sequence counters for **Replay Protection** without blocking the reliable stream.
*   **Reliable TCP**: Manages critical state transitions, clicks, clipboard synchronization, and handshakes.
*   **Asynchronous Connection Model**: The `NetworkManager` implements a `ConnectionState` machine (Disconnected, Connecting, Connected, Error). This ensures that protocol-level handshakes are deferred until the TCP transport is fully established, preventing race conditions common in non-blocking socket environments.

### 3. Synchronization & Authority Patterns
*   **Authoritative Server Authority**: The server is the source of truth, periodically broadcasting `MasterStateSync` packets to all clients to correct local perception drift and enforce global cursor alignment.
*   **Distributed Clock Sync**: Peers exchange heartbeats to calculate clock offsets, enabling **Timestamp-First Interaction Ownership**. This allows the system to resolve simultaneous click conflicts by honoring the peer with the earliest absolute timestamp.
*   **Gesture Integrity**: The `SyncModule` enforces logic where focus ownership is denied to peers during active selection gestures, ensuring collaborative stability.

### 4. Visual & UI Systems
*   **Hardware-Accelerated Rendering**: An `OverlayEngine` (Direct3D 11 or GDI+) paints secondary cursors directly on top of the OS. The D3D11 backend uses a 2.0x scaling factor for quad rendering to correctly map pixel dimensions to NDC space.
*   **Integrated Telemetry UI**: `ConfigGUI` provides a persistent monitor window for live telemetry (RTT, E2E Latency, Drift), security logging, and **Automated Peer Discovery** via UDP broadcasting.
*   **Single-Instance Management**: The UI stack enforces a single-window instance by verifying existing handles via `IsWindow()` before creating new ones, preventing resource bloat during framework re-initialization.

### 5. Security & Isolation Decisions
*   **Mutual Authentication**: NetMux implements a SHA-256 Challenge-Response mechanism. Both client and server must verify each other using a shared security key before protocol initialization is completed.
*   **Multi-Tenant Grouping**: Communication is isolated by `groupId`, allowing multiple independent groups of users to coexist on the same local network without cross-interaction.

### 6. Project Governance & Lifecycle
The project follows a strict roadmap-driven development model:
*   **Documentation Suite**: Maintenance of `VISION.md`, `ROADMAP.md`, `TODO.md`, and `MEMORY.md` is mandatory for architectural consistency.
*   **Zero-Friction Handoff**: Every session concludes with a `HANDOFF.md` file, ensuring that successive models can immediately resume work with full context.