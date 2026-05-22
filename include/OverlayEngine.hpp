#pragma once

class OverlayEngine {
public:
    OverlayEngine();
    ~OverlayEngine();

    bool Initialize();
    void Render(int cursorX, int cursorY);
    void Shutdown();

private:
    bool m_active;
#ifdef _WIN32
    void* m_hwnd;
#endif
};
