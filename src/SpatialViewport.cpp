#include "SpatialViewport.hpp"
#include <iostream>
#include <cmath>

SpatialViewport::SpatialViewport()
#ifdef _WIN32
    : m_currentCamX(0.0f), m_currentCamZ(0.0f), m_localSRV(nullptr), m_remoteSRV(nullptr)
#endif
{
}

SpatialViewport::~SpatialViewport() {}

#ifdef _WIN32
bool SpatialViewport::Initialize(ID3D11Device* device) {
    std::cout << "[SpatialViewport] Initializing 3D Composition Engine (D3D11)..." << std::endl;

    // Set up View and Projection matrices for 3D space
    DirectX::XMVECTOR Eye = DirectX::XMVectorSet(0.0f, 0.0f, -5.0f, 0.0f);
    DirectX::XMVECTOR At = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
    DirectX::XMVECTOR Up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    m_viewMatrix = DirectX::XMMatrixLookAtLH(Eye, At, Up);
    m_projMatrix = DirectX::XMMatrixPerspectiveFovLH(DirectX::XM_PIDIV4, 16.0f / 9.0f, 0.01f, 100.0f);

    return true;
}

void SpatialViewport::Update(float deltaTime, bool isCrossingBorder) {
    // Boundary-triggered Spatial Animation (The "Zoom")
    // Enhance smoothing logic to make it more seamless
    float targetX = isCrossingBorder ? 2.5f : 0.0f; // Shift slightly towards the remote plane
    float targetZ = isCrossingBorder ? -6.0f : -4.0f; // Zoom out more for context

    // Smooth step easing (could use smoothstep or similar for better feel)
    float lerpSpeed = 8.0f; // Slightly faster, but smoother easing

    // Simple exponential smoothing
    m_currentCamX += (targetX - m_currentCamX) * (1.0f - std::exp(-lerpSpeed * deltaTime));
    m_currentCamZ += (targetZ - m_currentCamZ) * (1.0f - std::exp(-lerpSpeed * deltaTime));

    // Update view matrix based on interpolated camera position
    // Adding a slight tilt to the camera for a 3D effect
    DirectX::XMVECTOR Eye = DirectX::XMVectorSet(m_currentCamX, 0.5f, m_currentCamZ, 0.0f); // Slightly elevated
    DirectX::XMVECTOR At = DirectX::XMVectorSet(m_currentCamX * 0.5f, 0.0f, 0.0f, 0.0f);  // Look slightly towards center
    DirectX::XMVECTOR Up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    m_viewMatrix = DirectX::XMMatrixLookAtLH(Eye, At, Up);
}




void SpatialViewport::Render(ID3D11DeviceContext* context, const std::map<unsigned long long, RemoteCursorState>& peers, float localDpiScale) {

    // 1. Set Vertex/Pixel Shaders for 2D planes in 3D space
    // 2. Bind m_localSRV (Local Desktop) to Plane 1
    // 3. Bind m_remoteSRV (Remote WebRTC Stream) to Plane 2 (offset in X)
    // 4. Apply View/Proj constant buffers
    // 5. Draw planes using Anisotropic filtering to prevent blurriness

    // Stub implementation for now until WebRTC/DXGI frames are fully wired in.
    if (!context) return;





    // Example of using DPI scale for cursor rendering in 3D space
    for (auto const& [id, peer] : peers) {
        float dpiRatio = localDpiScale;
        // In a real implementation we would adjust the cursor's world position based on dpiRatio
        // float worldX = (peer.x * dpiRatio) / screenWidth;
    }

}
#endif
