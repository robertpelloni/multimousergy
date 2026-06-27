#pragma once

#ifdef _WIN32
#include <d3d11.h>
#include <directxmath.h>
#endif

class SpatialViewport {
public:
    SpatialViewport();
    ~SpatialViewport();

#ifdef _WIN32
    bool Initialize(ID3D11Device* device);
    void Update(float deltaTime, bool isCrossingBorder);
    void Render(ID3D11DeviceContext* context, const std::map<unsigned long long, RemoteCursorState>& peers, float localDpiScale);

    void SetLocalDesktopTexture(ID3D11ShaderResourceView* srv) { m_localSRV = srv; }
    void SetRemoteDesktopTexture(ID3D11ShaderResourceView* srv) { m_remoteSRV = srv; }
#else
    bool Initialize(void* device) { return false; }
    void Update(float deltaTime, bool isCrossingBorder) {}
    void Render(void* context) {}
#endif

private:
#ifdef _WIN32
    DirectX::XMMATRIX m_viewMatrix;
    DirectX::XMMATRIX m_projMatrix;
    float m_currentCamX;
    float m_currentCamZ;

    ID3D11ShaderResourceView* m_localSRV;
    ID3D11ShaderResourceView* m_remoteSRV;
#endif
};
