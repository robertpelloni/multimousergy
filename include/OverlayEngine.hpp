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
    void* m_hdcMem;
    void* m_hBitmap;
    void* m_hOldBitmap;
    int m_screenWidth;
    int m_screenHeight;
#endif
};
