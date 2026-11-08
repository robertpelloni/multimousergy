#pragma once
#include <string>
#include <vector>

enum class WaylandSessionType {
    X11,
    WaylandNative,
    WaylandPortal,
    Unknown
};

class WaylandPortalManager {
public:
    WaylandPortalManager();
    ~WaylandPortalManager();

    static WaylandSessionType DetectDisplayServer();
    static bool IsWaylandSession();
    static bool IsPortalAvailable();

    bool RequestInputCaptureSession(std::string& outSessionHandle);
    bool ReleaseInputCaptureSession(const std::string& sessionHandle);
    bool InjectVirtualPointerEvent(int dx, int dy, int buttonMask);

    bool IsActive() const { return m_active; }

private:
    bool m_active = false;
    std::string m_currentSession;
};
