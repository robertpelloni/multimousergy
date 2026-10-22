# MultiMousergy (NetMux)

MultiMousergy — internally still **NetMux** (`CMakeLists.txt`:
`project(NetMux VERSION 0.1.90)`) — is a Windows C++17 application for
sharing one mouse across two networked PCs: move your cursor off the edge
of one machine's display and it appears on the other machine as an
independent second cursor, injected through virtual HID drivers, with a
D3D11/GDI+ overlay drawing the remote cursor locally.

## What's implemented

Real code, ~150 KB of C++ under `src/`:

- `InputEngine.cpp` — low-level mouse hook / raw-input capture
- `DriverInterface.cpp` — virtual HID injection (ViGEmBus / Interception
  support exists but is **off by default** in CMake:
  `ENABLE_VIGEM` / `ENABLE_INTERCEPTION` / `NETMUX_USE_NATIVE_DRIVERS`)
- `NetworkManager.cpp` / `PacketSerializer.cpp` — UDP cursor tracking +
  TCP state sync
- `OverlayEngine.cpp` / `D3D11Overlay.cpp` — the remote-cursor overlay
- `SyncModule.cpp` — server-authoritative state correction
- `ClipboardModule.cpp`, `FileTransferEngine.cpp`, `ConfigManager.cpp`,
  `SystemTray.cpp`, `DesktopCapture.cpp`
- `SpatialViewport.cpp`, `VideoEncoder/Decoder.cpp`, `WebRTCManager.cpp`,
  `WebcamCapture.cpp` — the spatial-workspace and video pieces; these are
  the smallest files in the tree (WebRTCManager is ~2 KB) and are best
  described as skeletal. The old README's feature list (3D-composited
  spatial workspace, integrated high-fps WebRTC video chat) states the
  design goal, not the shipped state.

Config is via `netmux.cfg`; `build.bat` / `run_server.bat` /
`run_client.bat` wrap the CMake build and launch. The `borg.db` /
`hypernexus.db` files at the root are agent-harness session artifacts, not
application data.

## Building and running

Prerequisites: Windows 10+, Visual Studio 2022, and the ViGEmBus driver on
target machines for virtual-HID injection.

```bash
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

```bash
NetMux.exe --server --port 5555 --boundary-x 1919 --left
NetMux.exe --client <Server_IP> --port 5555 --boundary-x 0
```

## Status

Prototype, v0.1.90. Created May 2026; latest commits are automated agent
checkpoints (October 2026). MIT licensed (see `LICENSE`). Design docs:
`VISION.md`, `ROADMAP.md`, `HANDOFF.md`.
