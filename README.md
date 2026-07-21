# NetMux (PolyPointer)

NetMux is an open-source, decoupled, cross-network multi-cursor system for Windows 10. It allows two independent physical PCs to cross display boundaries with their local mouse hardware without stealing or hijacking the remote machine's native system cursor.

## Key Features
- **Independent Cursors**: The remote machine displays and processes the inbound network cursor as a distinct, second independent cursor instance.
- **Authoritative Synchronization**: Implements a server-authoritative model via `MasterStateSync` to periodically correct local perception drift and ensure perfect alignment across all peers.
- **Continuous Visibility**: Continuous local cursor broadcasting ensures all peers remain visible in real-time, regardless of active input capture state.
- **Hardware-Level Injection**: Utilizes virtual HID drivers (ViGEmBus/Interception on Windows, evdev on Linux) to bypass native single-cursor kernel constraints.
- **Asynchronous Networking**: Lightweight UDP for real-time cursor tracking and TCP for reliable click/state synchronization.
- **Transparent Overlay**: Custom hardware-accelerated Direct3D 11 and GDI+ overlay loops paint secondary mouse cursors directly on top of the OS interface.
- **Warp-Click-Restore**: Programmatic cycle to execute click behaviors on remote windows without losing local cursor context or focus.

## Architecture
NetMux operates on a two-layer system:
1. **Controller Service**: Intercepts local hardware mouse movement via `WH_MOUSE_LL` and `Raw Input` (WM_INPUT).
2. **Virtual Bus Link**: Feeds coordinates to a Virtual HID Driver on the remote machine, ensuring Windows sees it as genuine hardware activity.

## Getting Started

### Prerequisites
- Windows 10 or later.
- [ViGEmBus Driver](https://github.com/ViGEm/ViGEmBus) installed on target machines.
- Visual Studio 2022 (for building).

### Building
You can use the provided `build.bat` or CMake:
```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

### Usage
Run `NetMux.exe` on both computers.

**On the Server PC:**
```bash
NetMux.exe --server --port 5555 --boundary-x 1919 --left
```

**On the Client PC:**
```bash
NetMux.exe --client <Server_IP> --port 5555 --boundary-x 0
```

## Documentation
- [VISION.md](VISION.md): Project goals and core pillars.
- [ROADMAP.md](ROADMAP.md): Major milestones and structural plans.
- [MEMORY.md](MEMORY.md): Architectural observations and design preferences.
- [DEPLOY.md](DEPLOY.md): Detailed deployment and environment setup instructions.

## License
MIT License. See [LICENSE](LICENSE) for details.
