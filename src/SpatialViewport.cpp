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
    if (!context) return;

    // TODO: Implement D3D11 3D plane rendering.
    // This requires setting up vertex/pixel shaders, constant buffers, and drawing quads.
    // For now, we focus on the WebRTC media pipeline integration to supply the textures.

    if (m_localSRV) {
        // Draw Local Desktop Plane using m_localSRV
    }

    int planeIndex = 1;
    for (auto const& [id, peer] : peers) {
        float dpiRatio = localDpiScale;

        // Map normalized coordinates (0-65535) to -1.0 to 1.0 screen space
        float normX = (peer.x / 65535.0f);
        float normY = (peer.y / 65535.0f);

        float worldX = ((normX * 2.0f) - 1.0f) * dpiRatio;
        float worldY = (1.0f - (normY * 2.0f)) * dpiRatio;

        // TODO: Obtain remote SRV from WebRTC media pipeline manager
        // ID3D11ShaderResourceView* remoteSRV = WebRTCManager::GetInstance()->GetRemoteDesktopTexture(id);

        if (m_remoteSRV) {
            // Draw remote desktop quad
        }

        planeIndex++;
    }
}

void SpatialViewport::UpdateLocalDesktopFrame(ID3D11Texture2D* frame, ID3D11Device* device) {
    if (!frame || !device) return;

    if (m_localSRV) {
        m_localSRV->Release();
        m_localSRV = nullptr;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;

    device->CreateShaderResourceView(frame, &srvDesc, &m_localSRV);
}

#endif
