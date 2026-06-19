# MultiMousergy (formerly NetMux) - Project Architecture & Memory

## 1. Core Vision & Concept
MultiMousergy is a highly advanced, decentralized multi-cursor system that allows independent PCs to share mouse and keyboard input across network boundaries. Unlike standard KVM tools, it does not hijack the remote user's cursor. Instead, it virtualizes hardware input to create a cohesive, shared spatial workspace where remote cursors exist independently alongside local ones.

## 2. Decoupled Two-Layer Input Architecture
*   **Capture Layer:** Intercepts physical input and suppresses boundary transitions locally. Uses low-level OS hooks (`WH_MOUSE_LL`, `WM_INPUT` on Windows; `XQueryPointer`, `evdev` on Linux).
*   **Injection Layer:** Receives network data and bypasses OS cursor limits by writing coordinates directly to a Virtual HID Driver.
    *   *Windows:* Employs dynamic SDK loading for ViGEmBus and Interception.
    *   *Linux:* Injects via `uinput` and `evdev`.

## 3. Hybrid Networking Protocol
*   **UDP for Movement:** Utilizes lightweight, header-only UDP packets with delta compression for high-frequency cursor movement to ensure low bandwidth and ultra-low latency.
*   **TCP for State & Data:** Reliable transport is used for auth, clicks, window focus synchronization, clipboard sharing, and file transfers. 
*   **Non-Blocking I/O:** `NetworkManager` is implemented as a state-driven machine using `select()` to avoid blocking the main thread or UI, heavily reducing CPU spin loops.

## 4. MasterStateSync & Distributed Coordination
*   **Authoritative Loop:** A server-side MasterStateSync normalizes coordinates across different screen geometries, DPI scaling variations, and refresh rates.
*   **Timestamp-First Resolution:** Resolves input conflicts (e.g., simultaneous remote clicks) using distributed clock synchronization to maintain a unified jitter buffer and timeline.

## 5. Security & High-Capacity Data
*   **Authentication:** Mutual challenge-response authentication via constant-time SHA-256 verification.
*   **Data Integrity:** Large payloads (like clipboard data and file transfers) use multi-part chunking with streaming SHA-256 integrity verification.

## 6. Advanced Rendering & Spatial Viewport (In Progress)
*   **Hardware Overlay:** A Direct3D 11 compositing engine (`OverlayEngine`/`D3D11Overlay`) manages hardware-accelerated cursor sprite rendering.
*   **SpatialViewport:** A 3D environment initialized with `XMMATRIX` math that places local and remote desktop streams onto 3D planes. When cursors cross borders, the camera smoothly pans and zooms, giving a physical spatial layout to networked machines.

## 7. Next-Generation Media Pipeline
*   **WebRTC Integration:** Integrating WebRTC (SDP/ICE) for P2P data channels and A/V streams.
*   **DXGI & Windows Media Foundation:** Implementing DXGI Desktop Duplication (`DesktopCapture`) and Media Foundation (`WebcamCapture`) pipelines to capture low-latency local frames, encode them, and stream them to remote spatial planes.