# DEPLOY.md

## Prerequisites
- Windows 10 or later.
- Visual Studio 2022 with C++ Desktop Development workload.
- ViGEmBus Driver installed on target machines.
- CMake 3.20+

## Build Instructions
1. Clone the repository.
2. Open the folder in Visual Studio or use CMake to generate project files.
3. Build the `NetMux` project in Release mode.

## Running
1. Run `NetMux.exe` on both PCs.
2. Optionally launch the Electron UI from `ui/` with `npm start` for graphical configuration.
3. Configure IP addresses and screen boundaries via the Electron UI or `netmux.cfg`.
4. Ensure firewall allows UDP/TCP traffic on the designated port (default: 5555).
