# VISION.md: MultiMousergy

## Project Goal
MultiMousergy (formerly NetMux) is a collaborative spatial desktop environment that merges multi-cursor multiplexing with real-time video chat and seamless display-border transitions.
Unlike traditional KM sharing tools (Synergy, Mouse Without Borders) that hijack the remote cursor,
NetMux provides true cursor isolation by presenting remote inputs as independent virtual hardware devices.

## Core Pillars
1. **True Independence**: Remote users do not lose control of their cursor when a network mouse enters their screen.
2. **Hardware-Level Injection**: Bypassing high-level OS limitations by using virtual HID drivers (ViGEmBus/Interception).
3. **Seamless Experience**: Zero-latency feel for remote cursor movement and clicks.
4. **Multiplexing**: Intelligent warp-click-restore logic to maintain window focus and interaction without stealing local cursor context.
5. **Secure Isolation (Grouping)**: Intelligent cursor grouping to allow multiple independent teams or workspaces to coexist on the same network without cross-interaction.
6. **Hardware-Accelerated Rendering**: Direct3D 11 backend for ultra-low latency cursor overlays, ensuring visual feedback matches the 1000Hz+ polling rates of modern gaming mice.
7. **Unified Clock Synchronization**: Distributed clock coordination to resolve race conditions and interaction order across multiple networked machines.
8. **Multi-Platform Foundation**: A unified distributed input framework capable of bridging Windows, Linux, and macOS environments.
9. **High-Capacity Collaboration**: Unlimited data sharing (clipboard, files) via intelligent protocol chunking and reassembly.
10. **Spatial Workspace Viewport**: A 3D-composited desktop space that zooms and pans between peers, creating a unified physical-virtual workspace.
11. **Integrated Media Streams**: Real-time WebRTC-powered video and audio tracks synchronized with cursor focus and spatial position.
