# Session Handoff - v0.1.56-alpha

## Summary of Changes
- **Native X11 Clipboard**: Implemented direct `Xlib` calls in `ClipboardModule.cpp` to read the system clipboard on Linux, significantly reducing overhead compared to `xclip` process spawning.
- **Enhanced Linux Capture**: Integrated `XQueryPointer` into `InputEngine.cpp` to synchronize virtual coordinates with the real system cursor, allowing for reliable boundary-based input capture.
- **Driver Interface Refactoring**: Updated `DriverInterface` and `CMakeLists.txt` to support optional linking against native Interception and ViGEmBus SDKs via the `NETMUX_USE_NATIVE_DRIVERS` macro.
- **Build System**: Updated `CMakeLists.txt` to detect and link against `X11` libraries on Linux.

## Technical Observations
- The Linux X11 clipboard implementation includes a 100ms timeout loop to handle asynchronous `SelectionNotify` events.
- Boundary detection on Linux now uses the global root window coordinates from X11, resolving the "drift" issue inherent in relative-only tracking.
- `NETMUX_USE_NATIVE_DRIVERS` allows developers to compile the project without needing the physical drivers installed, while still having the logic ready for deployment.

## Next Steps
- Implement native X11 clipboard *writing* (currently still using `xclip` due to X11 ownership complexity).
- Finalize the `FileTransferEngine` using the established chunking protocol.
- Research Wayland-specific input capture alternatives for future-proofing Linux support.

## Repository State
- Version: `v0.1.56-alpha`
- Build status: Passing (X11 dependency added to Linux build)
- Git: Pushed to origin.
