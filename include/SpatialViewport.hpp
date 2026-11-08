#include <map>
#include "OverlayEngine.hpp"
#pragma once

#ifdef _WIN32
#include <d3d11.h>
#include <directxmath.h>
#include <d3dcompiler.h>
#endif

struct SpatialAnchor {
    unsigned long long peerId = 0;
    float posX = 0.0f;
    float posY = 0.0f;
    float posZ = 0.0f;
    float yaw = 0.0f;
    float pitch = 0.0f;
    float roll = 0.0f;
    float scale = 1.0f;
    bool hasActiveFocus = false;
};

class SpatialViewport {
public:
    SpatialViewport();
    ~SpatialViewport();

    void SetSpatialAnchor(unsigned long long peerId, float x, float y, float z, float scale = 1.0f);
    bool GetSpatialAnchor(unsigned long long peerId, SpatialAnchor& outAnchor) const;
    void SetActiveAnchorFocus(unsigned long long peerId);
    void RemoveSpatialAnchor(unsigned long long peerId);

#ifdef _WIN32
    bool Initialize(ID3D11Device* device);
    void Update(float deltaTime, bool isCrossingBorder);
    void Render(ID3D11DeviceContext* context, const std::map<unsigned long long, RemoteCursorState>& peers, float localDpiScale);

    void SetLocalDesktopTexture(ID3D11ShaderResourceView* srv) { m_localSRV = srv; }
    void SetRemoteDesktopTexture(ID3D11ShaderResourceView* srv) { m_remoteSRV = srv; }
    void UpdateLocalDesktopFrame(ID3D11Texture2D* frame, ID3D11Device* device);
    void SetBoundaryConfig(int boundaryX, int boundaryY, bool isLeft) {
        m_boundaryX = boundaryX;
        m_boundaryY = boundaryY;
        m_isLeft = isLeft;
    }
    void GetBoundaryConfig(int& boundaryX, int& boundaryY, bool& isLeft) const {
        boundaryX = m_boundaryX;
        boundaryY = m_boundaryY;
        isLeft = m_isLeft;
    }
#else
    bool Initialize(void* device) { return false; }
    void Update(float deltaTime, bool isCrossingBorder) {}
    void Render(void* context, const std::map<unsigned long long, RemoteCursorState>& peers, float localDpiScale) {}
    void UpdateLocalDesktopFrame(void* frame, void* device) {}
    void SetBoundaryConfig(int boundaryX, int boundaryY, bool isLeft) {
        m_boundaryX = boundaryX;
        m_boundaryY = boundaryY;
        m_isLeft = isLeft;
    }
    void GetBoundaryConfig(int& boundaryX, int& boundaryY, bool& isLeft) const {
        boundaryX = m_boundaryX;
        boundaryY = m_boundaryY;
        isLeft = m_isLeft;
    }
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

    ID3D11Buffer* m_boundaryVertexBuffer = nullptr;
    ID3D11PixelShader* m_boundaryPixelShader = nullptr;
#endif

    int m_boundaryX = 1919;
    int m_boundaryY = 0;
    bool m_isLeft = true;

    std::map<unsigned long long, SpatialAnchor> m_anchors;
    unsigned long long m_activeFocusPeer = 0;
};
