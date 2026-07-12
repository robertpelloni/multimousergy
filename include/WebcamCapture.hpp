#pragma once

#ifdef _WIN32
#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#endif

class WebcamCapture {
public:
    WebcamCapture();
    ~WebcamCapture();

    bool Initialize();
    void Update();

private:
#ifdef _WIN32
    IMFMediaSource* m_mediaSource = nullptr;
    IMFSourceReader* m_sourceReader = nullptr;
#endif
};
