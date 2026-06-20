## Session Handoff - UI Rebuild to Electron

### Actions Performed
1. Stripped out the old native `ConfigGUI` C++ Win32 dependencies.
2. Refactored `main.cpp` and `NetMuxFramework.cpp` to run purely headlessly.
3. Added a new `ui/` directory containing a modern Electron front-end.
4. Set up IPC mapping between the C++ standard output and the Electron application context via standard pipes and `contextBridge`.
5. Created telemetry logs handling so the Electron app can receive minimap rendering updates from the sync component.
6. Handled application lifecycle so the child process terminates cleanly when the UI is closed.

### Known State
- Build passes on linux. Needs CI check for cross-platform compliance (X11 removed).
- Unit tests all pass successfully.
- Web app properly functions as a standalone viewer.

### Future considerations
- Need to expand the web UI feature parity with the old config GUI.
- Port advanced configuration like auto-connect, clipboard sync state, desktop capturing preferences.
