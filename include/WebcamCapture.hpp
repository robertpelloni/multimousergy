#pragma once

#include <string>
#include <vector>
#include <cstdint>

#ifdef _WIN32
#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#endif

struct WebcamDeviceInfo {
    int index;
    std::string name;
    std::string endpointId;
};

class WebcamCapture {
public:
    WebcamCapture();
    ~WebcamCapture();

    // Enumerate all connected video capture hardware devices
    std::vector<WebcamDeviceInfo> EnumerateDevices();

    // Initialize capture pipeline with a specific device index (default: 0)
    bool Initialize(int deviceIndex = 0);

    // Stop current stream and release Media Foundation resources
    void Shutdown();

    // Check if capture pipeline is currently active
    bool IsCapturing() const;

    // Pump frames from the hardware source reader
    void Update();

    // Retrieve active device details
    WebcamDeviceInfo GetCurrentDeviceInfo() const;

private:
    bool m_isCapturing;
    WebcamDeviceInfo m_activeDevice;
    std::vector<WebcamDeviceInfo> m_deviceList;

#ifdef _WIN32
    IMFMediaSource* m_mediaSource = nullptr;
    IMFSourceReader* m_sourceReader = nullptr;
#endif
};
