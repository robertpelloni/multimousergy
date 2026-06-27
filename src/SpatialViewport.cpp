#include "SpatialViewport.hpp"
#include <iostream>

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
    float targetX = isCrossingBorder ? 4.0f : 0.0f; // Shift to remote plane
    float targetZ = isCrossingBorder ? -3.0f : -5.0f; // Zoom out to see context

    float lerpSpeed = 5.0f;
    m_currentCamX += (targetX - m_currentCamX) * deltaTime * lerpSpeed;
    m_currentCamZ += (targetZ - m_currentCamZ) * deltaTime * lerpSpeed;

    // Update view matrix based on interpolated camera position
    DirectX::XMVECTOR Eye = DirectX::XMVectorSet(m_currentCamX, 0.0f, m_currentCamZ, 0.0f);
    DirectX::XMVECTOR At = DirectX::XMVectorSet(m_currentCamX, 0.0f, 0.0f, 0.0f);
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

    // Map normalized coordinates to 3D plane positions for each connected peer
    for (auto const& [id, peer] : peers) {
        float dpiRatio = localDpiScale;

        // Convert normalized coordinates (0-65535) to -1.0 to 1.0 screen space
        // Then apply scaling based on DPI to map onto the 3D plane
        float normX = (peer.x / 65535.0f);
        float normY = (peer.y / 65535.0f);

        // Calculate 3D world position
        // Assuming Plane 1 is at z=0, and Plane 2 is at z=0 with an x-offset
        // The x-offset would depend on the peer ID or relative position
        float worldX = ((normX * 2.0f) - 1.0f) * dpiRatio;
        float worldY = (1.0f - (normY * 2.0f)) * dpiRatio; // Invert Y for 3D space

        // In a full implementation, we would bind these coordinates to a vertex buffer
        // and draw the cursor sprite at (worldX, worldY, 0.0f)
    }
}
#endif
