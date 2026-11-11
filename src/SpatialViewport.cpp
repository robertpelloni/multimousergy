#include "SpatialViewport.hpp"
#include "WebRTCManager.hpp"
#include <iostream>
#include <cmath>

SpatialViewport::SpatialViewport()
#ifdef _WIN32
    : m_currentCamX(0.0f), m_currentCamZ(0.0f), m_localSRV(nullptr), m_remoteSRV(nullptr),
      m_vertexShader(nullptr), m_pixelShader(nullptr), m_inputLayout(nullptr),
      m_vertexBuffer(nullptr), m_indexBuffer(nullptr), m_constantBuffer(nullptr),
      m_samplerState(nullptr)
#endif
{
}

void SpatialViewport::SetSpatialAnchor(unsigned long long peerId, float x, float y, float z, float scale) {
    SpatialAnchor& anchor = m_anchors[peerId];
    anchor.peerId = peerId;
    anchor.posX = x;
    anchor.posY = y;
    anchor.posZ = z;
    anchor.scale = scale;
    if (m_activeFocusPeer == peerId) {
        anchor.hasActiveFocus = true;
    }
}

bool SpatialViewport::GetSpatialAnchor(unsigned long long peerId, SpatialAnchor& outAnchor) const {
    auto it = m_anchors.find(peerId);
    if (it != m_anchors.end()) {
        outAnchor = it->second;
        return true;
    }
    return false;
}

void SpatialViewport::SetActiveAnchorFocus(unsigned long long peerId) {
    m_activeFocusPeer = peerId;
    for (auto& pair : m_anchors) {
        pair.second.hasActiveFocus = (pair.first == peerId);
    }
}

void SpatialViewport::RemoveSpatialAnchor(unsigned long long peerId) {
    m_anchors.erase(peerId);
    if (m_activeFocusPeer == peerId) {
        m_activeFocusPeer = 0;
    }
}

void SpatialViewport::ScreenToRay(int screenX, int screenY, int viewportWidth, int viewportHeight, Ray3D& outRay) {
    if (viewportWidth <= 0 || viewportHeight <= 0) return;

    // Convert screen coordinates to Normalized Device Coordinates (NDC) [-1, 1]
    float ndcX = (2.0f * screenX) / viewportWidth - 1.0f;
    float ndcY = 1.0f - (2.0f * screenY) / viewportHeight;

    // Camera sits at (0, 0, -5) with 45-deg FOV perspective projection
    outRay.originX = 0.0f;
    outRay.originY = 0.0f;
    outRay.originZ = -5.0f;

    float aspect = (float)viewportWidth / (float)viewportHeight;
    float tanHalfFov = 0.41421356f; // tan(pi / 8) for ~45 deg FOV

    outRay.dirX = ndcX * aspect * tanHalfFov;
    outRay.dirY = ndcY * tanHalfFov;
    outRay.dirZ = 1.0f;

    // Normalize direction vector
    float len = std::sqrt(outRay.dirX * outRay.dirX + outRay.dirY * outRay.dirY + outRay.dirZ * outRay.dirZ);
    if (len > 0.0f) {
        outRay.dirX /= len;
        outRay.dirY /= len;
        outRay.dirZ /= len;
    }
}

bool SpatialViewport::IntersectRayWithAnchorPlane(const Ray3D& ray, const SpatialAnchor& anchor, float& outU, float& outV) {
    // Plane is oriented along Z = anchor.posZ with normal (0, 0, -1) facing camera
    float planeZ = anchor.posZ;
    if (std::abs(ray.dirZ) < 1e-6f) return false;

    // Ray equation: P = origin + t * dir
    float t = (planeZ - ray.originZ) / ray.dirZ;
    if (t < 0.0f) return false; // Behind ray origin

    float hitX = ray.originX + t * ray.dirX;
    float hitY = ray.originY + t * ray.dirY;

    // Plane extents scaled by anchor.scale (default plane is 2x2 centered at anchor.posX, anchor.posY)
    float halfWidth = (anchor.scale > 0.0f ? anchor.scale : 1.0f);
    float halfHeight = halfWidth;

    float localX = hitX - anchor.posX;
    float localY = hitY - anchor.posY;

    if (localX < -halfWidth || localX > halfWidth || localY < -halfHeight || localY > halfHeight) {
        return false; // Missed the anchor plane quad
    }

    // Map to normalized UV coordinates [0.0, 1.0]
    outU = (localX + halfWidth) / (2.0f * halfWidth);
    outV = (halfHeight - localY) / (2.0f * halfHeight);

    return true;
}

SpatialViewport::~SpatialViewport() {
#ifdef _WIN32
    if (m_boundaryPixelShader) m_boundaryPixelShader->Release();
    if (m_boundaryVertexBuffer) m_boundaryVertexBuffer->Release();
    if (m_samplerState) m_samplerState->Release();
    if (m_constantBuffer) m_constantBuffer->Release();
    if (m_indexBuffer) m_indexBuffer->Release();
    if (m_vertexBuffer) m_vertexBuffer->Release();
    if (m_inputLayout) m_inputLayout->Release();
    if (m_pixelShader) m_pixelShader->Release();
    if (m_vertexShader) m_vertexShader->Release();
#endif
}

#ifdef _WIN32
bool SpatialViewport::Initialize(ID3D11Device* device) {
    std::cout << "[SpatialViewport] Initializing 3D Composition Engine (D3D11)..." << std::endl;

    // Set up View and Projection matrices for 3D space
    DirectX::XMVECTOR Eye = DirectX::XMVectorSet(0.0f, 0.0f, -5.0f, 0.0f);
    DirectX::XMVECTOR At = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
    DirectX::XMVECTOR Up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    m_viewMatrix = DirectX::XMMatrixLookAtLH(Eye, At, Up);
    m_projMatrix = DirectX::XMMatrixPerspectiveFovLH(DirectX::XM_PIDIV4, 16.0f / 9.0f, 0.01f, 100.0f);

    if (!device) return false;

    // 1. Create Quad Vertices
    Vertex vertices[] = {
        { DirectX::XMFLOAT3(-1.0f,  1.0f, 0.0f), DirectX::XMFLOAT2(0.0f, 0.0f) }, // Top-left
        { DirectX::XMFLOAT3( 1.0f,  1.0f, 0.0f), DirectX::XMFLOAT2(1.0f, 0.0f) }, // Top-right
        { DirectX::XMFLOAT3(-1.0f, -1.0f, 0.0f), DirectX::XMFLOAT2(0.0f, 1.0f) }, // Bottom-left
        { DirectX::XMFLOAT3( 1.0f, -1.0f, 0.0f), DirectX::XMFLOAT2(1.0f, 1.0f) }  // Bottom-right
    };

    D3D11_BUFFER_DESC vertexDesc = {};
    vertexDesc.Usage = D3D11_USAGE_DEFAULT;
    vertexDesc.ByteWidth = sizeof(Vertex) * 4;
    vertexDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA vertexData = {};
    vertexData.pSysMem = vertices;
    device->CreateBuffer(&vertexDesc, &vertexData, &m_vertexBuffer);

    // 2. Create Quad Indices
    unsigned long indices[] = { 0, 1, 2, 2, 1, 3 };
    D3D11_BUFFER_DESC indexDesc = {};
    indexDesc.Usage = D3D11_USAGE_DEFAULT;
    indexDesc.ByteWidth = sizeof(unsigned long) * 6;
    indexDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA indexData = {};
    indexData.pSysMem = indices;
    device->CreateBuffer(&indexDesc, &indexData, &m_indexBuffer);

    // 3. Create Constant Buffer
    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.Usage = D3D11_USAGE_DYNAMIC;
    cbDesc.ByteWidth = sizeof(ConstantBufferType);
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    device->CreateBuffer(&cbDesc, nullptr, &m_constantBuffer);

    // 4. Create Sampler State
    D3D11_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.MaxAnisotropy = 8;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
    device->CreateSamplerState(&samplerDesc, &m_samplerState);

    // Compile basic shaders
    const char* vertexShaderSrc =
        "cbuffer ConstantBuffer : register(b0) {"
        "    matrix World;"
        "    matrix View;"
        "    matrix Projection;"
        "}"
        "struct VS_INPUT {"
        "    float3 Pos : POSITION;"
        "    float2 Tex : TEXCOORD0;"
        "};"
        "struct PS_INPUT {"
        "    float4 Pos : SV_POSITION;"
        "    float2 Tex : TEXCOORD0;"
        "};"
        "PS_INPUT main(VS_INPUT input) {"
        "    PS_INPUT output = (PS_INPUT)0;"
        "    float4 pos = float4(input.Pos, 1.0f);"
        "    pos = mul(pos, World);"
        "    pos = mul(pos, View);"
        "    pos = mul(pos, Projection);"
        "    output.Pos = pos;"
        "    output.Tex = input.Tex;"
        "    return output;"
        "}";

    const char* pixelShaderSrc =
        "Texture2D txDiffuse : register(t0);"
        "SamplerState samLinear : register(s0);"
        "struct PS_INPUT {"
        "    float4 Pos : SV_POSITION;"
        "    float2 Tex : TEXCOORD0;"
        "};"
        "float4 main(PS_INPUT input) : SV_Target {"
        "    return txDiffuse.Sample(samLinear, input.Tex);"
        "}";

    ID3DBlob* vsBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;
    HRESULT hr = D3DCompile(vertexShaderSrc, strlen(vertexShaderSrc), nullptr, nullptr, nullptr, "main", "vs_4_0", 0, 0, &vsBlob, &errorBlob);
    if (SUCCEEDED(hr)) {
        device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_vertexShader);

        D3D11_INPUT_ELEMENT_DESC layout[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
        };
        device->CreateInputLayout(layout, 2, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &m_inputLayout);
        vsBlob->Release();
    } else {
        if (errorBlob) {
            std::cerr << "[SpatialViewport] VS Error: " << (char*)errorBlob->GetBufferPointer() << std::endl;
            errorBlob->Release();
        }
    }

    ID3DBlob* psBlob = nullptr;
    hr = D3DCompile(pixelShaderSrc, strlen(pixelShaderSrc), nullptr, nullptr, nullptr, "main", "ps_4_0", 0, 0, &psBlob, &errorBlob);
    if (SUCCEEDED(hr)) {
        device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_pixelShader);
        psBlob->Release();
    } else {
        if (errorBlob) {
            std::cerr << "[SpatialViewport] PS Error: " << (char*)errorBlob->GetBufferPointer() << std::endl;
            errorBlob->Release();
        }
    }

    // 5. Create Boundary Line Pixel Shader (Neon Cyan / Electric Blue highlight for interactive edge boundaries)
    const char* boundaryPixelShaderSrc =
        "struct PS_INPUT {"
        "    float4 Pos : SV_POSITION;"
        "    float2 Tex : TEXCOORD0;"
        "};"
        "float4 main(PS_INPUT input) : SV_Target {"
        "    return float4(0.0f, 0.82f, 1.0f, 0.85f);"
        "}";

    ID3DBlob* boundaryPsBlob = nullptr;
    hr = D3DCompile(boundaryPixelShaderSrc, strlen(boundaryPixelShaderSrc), nullptr, nullptr, nullptr, "main", "ps_4_0", 0, 0, &boundaryPsBlob, &errorBlob);
    if (SUCCEEDED(hr)) {
        device->CreatePixelShader(boundaryPsBlob->GetBufferPointer(), boundaryPsBlob->GetBufferSize(), nullptr, &m_boundaryPixelShader);
        boundaryPsBlob->Release();
    } else {
        if (errorBlob) {
            std::cerr << "[SpatialViewport] Boundary PS Error: " << (char*)errorBlob->GetBufferPointer() << std::endl;
            errorBlob->Release();
        }
    }

    // 6. Create Dynamic Vertex Buffer for Interactive Edge Boundary Line
    Vertex boundaryVertices[] = {
        { DirectX::XMFLOAT3(-1.0f,  1.0f, 0.0f), DirectX::XMFLOAT2(0.0f, 0.0f) },
        { DirectX::XMFLOAT3(-0.96f, 1.0f, 0.0f), DirectX::XMFLOAT2(1.0f, 0.0f) },
        { DirectX::XMFLOAT3(-1.0f, -1.0f, 0.0f), DirectX::XMFLOAT2(0.0f, 1.0f) },
        { DirectX::XMFLOAT3(-0.96f,-1.0f, 0.0f), DirectX::XMFLOAT2(1.0f, 1.0f) }
    };
    D3D11_BUFFER_DESC boundaryDesc = {};
    boundaryDesc.Usage = D3D11_USAGE_DYNAMIC;
    boundaryDesc.ByteWidth = sizeof(Vertex) * 4;
    boundaryDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    boundaryDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    D3D11_SUBRESOURCE_DATA boundaryData = {};
    boundaryData.pSysMem = boundaryVertices;
    device->CreateBuffer(&boundaryDesc, &boundaryData, &m_boundaryVertexBuffer);

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

    // Set buffers and topology
    unsigned int stride = sizeof(Vertex);
    unsigned int offset = 0;
    context->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);
    context->IASetIndexBuffer(m_indexBuffer, DXGI_FORMAT_R32_UINT, 0);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Set shaders and sampler (assuming they were created)
    if (m_vertexShader) context->VSSetShader(m_vertexShader, nullptr, 0);
    if (m_pixelShader) context->PSSetShader(m_pixelShader, nullptr, 0);
    if (m_samplerState) context->PSSetSamplers(0, 1, &m_samplerState);

    // 1. Draw Local Desktop Plane
    if (m_localSRV) {
        D3D11_MAPPED_SUBRESOURCE mappedResource;
        if (SUCCEEDED(context->Map(m_constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource))) {
            ConstantBufferType* dataPtr = (ConstantBufferType*)mappedResource.pData;

            // Local plane is centered at world origin
            dataPtr->world = DirectX::XMMatrixTranspose(DirectX::XMMatrixIdentity());
            dataPtr->view = DirectX::XMMatrixTranspose(m_viewMatrix);
            dataPtr->projection = DirectX::XMMatrixTranspose(m_projMatrix);

            context->Unmap(m_constantBuffer, 0);
            context->VSSetConstantBuffers(0, 1, &m_constantBuffer);
        }

        context->PSSetShaderResources(0, 1, &m_localSRV);
        context->DrawIndexed(6, 0, 0);
    }

    // 2. Draw Remote Desktop Planes
    int planeIndex = 1;
    for (auto const& [id, peer] : peers) {
        // Obtain remote SRV from WebRTC media pipeline manager
        ID3D11ShaderResourceView* remoteSRV = WebRTCManager::GetInstance()->GetRemoteDesktopTexture(id);

        if (remoteSRV) {
            D3D11_MAPPED_SUBRESOURCE mappedResource;
            if (SUCCEEDED(context->Map(m_constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource))) {
                ConstantBufferType* dataPtr = (ConstantBufferType*)mappedResource.pData;

                // Offset each remote plane in X space
                DirectX::XMMATRIX worldMatrix = DirectX::XMMatrixTranslation(planeIndex * 2.5f, 0.0f, 0.0f);

                dataPtr->world = DirectX::XMMatrixTranspose(worldMatrix);
                dataPtr->view = DirectX::XMMatrixTranspose(m_viewMatrix);
                dataPtr->projection = DirectX::XMMatrixTranspose(m_projMatrix);

                context->Unmap(m_constantBuffer, 0);
                context->VSSetConstantBuffers(0, 1, &m_constantBuffer);
            }

            context->PSSetShaderResources(0, 1, &remoteSRV);
            context->DrawIndexed(6, 0, 0);
        }

        planeIndex++;
    }

    // 3. Render Interactive Screen Edge Boundary Visualizer
    if (m_boundaryVertexBuffer && m_boundaryPixelShader) {
        // Update boundary vertices based on m_isLeft and m_boundaryX configuration
        float edgeX = m_isLeft ? -1.0f : 1.0f;
        float lineWidth = 0.04f;
        float x1 = m_isLeft ? edgeX : (edgeX - lineWidth);
        float x2 = m_isLeft ? (edgeX + lineWidth) : edgeX;

        D3D11_MAPPED_SUBRESOURCE mappedBoundary;
        if (SUCCEEDED(context->Map(m_boundaryVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedBoundary))) {
            Vertex* v = (Vertex*)mappedBoundary.pData;
            v[0] = { DirectX::XMFLOAT3(x1,  1.0f, 0.001f), DirectX::XMFLOAT2(0.0f, 0.0f) };
            v[1] = { DirectX::XMFLOAT3(x2,  1.0f, 0.001f), DirectX::XMFLOAT2(1.0f, 0.0f) };
            v[2] = { DirectX::XMFLOAT3(x1, -1.0f, 0.001f), DirectX::XMFLOAT2(0.0f, 1.0f) };
            v[3] = { DirectX::XMFLOAT3(x2, -1.0f, 0.001f), DirectX::XMFLOAT2(1.0f, 1.0f) };
            context->Unmap(m_boundaryVertexBuffer, 0);
        }

        // Draw boundary line overlay with glowing boundary shader
        context->IASetVertexBuffers(0, 1, &m_boundaryVertexBuffer, &stride, &offset);
        context->PSSetShader(m_boundaryPixelShader, nullptr, 0);
        context->DrawIndexed(6, 0, 0);
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
