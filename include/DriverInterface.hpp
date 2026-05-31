#pragma once
#include <string>
#include "NetworkManager.hpp"

class DriverInterface {
public:
    DriverInterface();
    ~DriverInterface();

    bool Initialize(NetMuxDriverType type = NetMuxDriverType::Auto);
    void Shutdown();

    bool SendMouseMovement(long dx, long dy);
    bool SendMouseButton(int button, bool down);
    bool SendMouseWheel(int delta, bool horizontal);

private:
    bool m_initialized;
    NetMuxDriverType m_type;
    long m_lastX = 0;
    long m_lastY = 0;
    int m_buttonState = 0; // bitmask
    bool m_useNativeDrivers = false;

#ifdef _WIN32
    void* m_context; // Interception context
    int m_device;    // Virtual mouse device ID
    void* m_vigemClient; // ViGEm client handle
    void* m_vigemPad;    // ViGEm target handle
#endif
#ifdef __linux__
    int m_device;
#endif
};
