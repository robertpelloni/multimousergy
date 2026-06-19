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
    bool SendKeyboardKey(int key, bool down);
    bool SendMouseWheel(int delta, bool horizontal);

    static bool IsDriverInstalled(NetMuxDriverType type);

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

    // Function pointer cache
    void* m_hInterception = nullptr;
    void* m_hViGEm = nullptr;
    void* m_interception_send = nullptr;
    void* m_interception_destroy_context = nullptr;

    void* m_vigem_target_x360_update = nullptr;
#endif

#ifdef __linux__
    int m_device;
#endif
};
