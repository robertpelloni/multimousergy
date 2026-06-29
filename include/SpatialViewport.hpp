#include <map>
#include "OverlayEngine.hpp"
#pragma once

#ifdef _WIN32
#include <d3d11.h>
#include <directxmath.h>
#include <d3dcompiler.h>
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
    void UpdateLocalDesktopFrame(ID3D11Texture2D* frame, ID3D11Device* device);
#else
    bool Initialize(void* device) { return false; }
    void Update(float deltaTime, bool isCrossingBorder) {}
    void Render(void* context, const std::map<unsigned long long, RemoteCursorState>& peers, float localDpiScale) {}
    void UpdateLocalDesktopFrame(void* frame, void* device) {}
#endif

private:
#ifdef _WIN32
    ID3D11VertexShader* m_vertexShader = nullptr;
    ID3D11PixelShader* m_pixelShader = nullptr;
    ID3D11InputLayout* m_inputLayout = nullptr;
    ID3D11Buffer* m_vertexBuffer = nullptr;
    ID3D11Buffer* m_indexBuffer = nullptr;
    ID3D11Buffer* m_constantBuffer = nullptr;
    ID3D11SamplerState* m_samplerState = nullptr;

    struct Vertex {
        DirectX::XMFLOAT3 position;
        DirectX::XMFLOAT2 texcoord;
    };

    struct ConstantBufferType {
        DirectX::XMMATRIX world;
        DirectX::XMMATRIX view;
        DirectX::XMMATRIX projection;
    };
    DirectX::XMMATRIX m_viewMatrix;
    DirectX::XMMATRIX m_projMatrix;
    float m_currentCamX;
    float m_currentCamZ;

    ID3D11ShaderResourceView* m_localSRV;
    ID3D11ShaderResourceView* m_remoteSRV;
#endif
};
