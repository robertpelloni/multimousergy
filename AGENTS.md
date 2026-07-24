<!-- [TORMENTNEXUS_AUTO_INJECTED] -->
> [!IMPORTANT]
> You are running within the TormentNexus environment. You MUST use your available tools frequently and proactively for researching, editing, executing, and validating your work. Always prioritize tool execution.

# NetMux: Cross-Network Multi-Cursor System

## Objective
Implement an open-source, decoupled, networking multi-cursor system for Windows 10. The system must allow two independent physical PCs, each running an instance of this software over a local network, to cross display boundaries with their local mouse hardware without stealing or hijacking the remote machine's native system cursor. The remote machine must display and process the inbound network cursor as a distinct, second independent cursor instance capable of asynchronous clicks and movement.

## Core Architecture Requirements
The system uses a decoupled two-layer system to prevent low-level hook conflicts.

[Local Hardware Mouse]
       │
       ▼ (WH_MOUSE_LL / Raw Input)
[PC 1 Controller Service] ──(Network Packet)──> [PC 2 Controller Service]
                                                       │
                                                       ▼ (Virtual Bus Write)
                                                [Virtual HID Driver (VigemBus/Interception)]
                                                       │
                                                       ▼
                                                [Windows Device Manager (Hardware ID Generated)]
                                                       │
                                                       ▼
                                                [PC 2 Custom Multiplexer Rendering Overlay]

## Implementation Blueprint
1. **Initialize Windows Driver Layer (Kernel-Space Injection)**: Utilize ViGEmBus or Interception to instantiate a virtual hardware-level HID USB mouse link.
2. **Build Low-Level Hardware Interception Engine**: Use WM_INPUT (Raw Input API) via RegisterRawInputDevices() to differentiate input by hardware device paths. Use WH_MOUSE_LL to suppress local behavior at boundaries.
3. **Implement Asynchronous Network Multiplexing Layer**: Lightweight UDP for real-time tracking, absolute TCP for click/state synchronization.
4. **Build the Multi-Cursor UI Overlay Engine**: Custom hardware-accelerated overlay loop (Direct3D 11 or layered window) to paint secondary mouse cursor.
5. **Simulate Target Click Multiplexing**: Implement a programmatic warp-click-restore cycle to execute click behaviors without losing local context.

## Coding Standards
- C++17 or later.
- Use Windows API for low-level tasks.
- Decouple driver interface from networking logic.
- Robust error handling and logging.
