# Session Handoff - v0.1.55-alpha

## Summary of Changes
- **Linux Input Capture**: Implemented evdev monitoring in `InputEngine`. Linux machines can now capture local mouse input and broadcast it when the cursor hits configured boundaries.
- **Security Hardening**: Replaced `memcmp` with constant-time comparison in `AuthModule::VerifyResponse` to mitigate timing attacks.
- **Robust Serialization**: Refactored `PacketSerializer` to calculate `HEADER_SIZE` via a constexpr helper, improving maintainability as the protocol evolves.
- **Feature Parity**: Linux now supports both input injection (via `DriverInterface`) and input capture (via `InputEngine`).

## Technical Observations
- Linux `InputEngine` utilizes a virtual coordinate system to detect boundaries, as there is no universal way to query the global system cursor position across different desktop environments without heavy dependencies.
- `PacketSerializer` layout was updated to include placeholders and explicit sizing, ensuring binary compatibility with the previous v0.1.54-alpha protocol while being more robust.

## Next Steps
- Implement native Vendor SDK linking for ViGEmBus and Interception (currently using stubs).
- Refine Linux boundary detection by integrating optionally with X11/Wayland libs if present.
- Expand `FileTransferEngine` using the multi-part chunking protocol.

## Repository State
- Version: `v0.1.55-alpha`
- Build status: Passing (Linux verified)
- Git: Pushed to origin.
