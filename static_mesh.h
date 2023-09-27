#pragma once
#include <DirectXMath.h>
#include <d3d11.h>
#include <directxmath.h>
#include <wrl.h>
#include <vector>
#include <string>

class StaticMesh {
public:
    struct Vertex {
        DirectX::XMFLOAT3 position;
        DirectX::XMFLOAT3 normal;
        DirectX::XMFLOAT2 texcoord;
    };
    struct Constants {
        DirectX::XMFLOAT4X4 world;
        DirectX::XMFLOAT4 materialColor;
    };
    // サブセットは対応するマテリアル名、そのマテリアルを使用するメッシュのインデックス開始番号とインデックス数
    struct Subset {
        std::wstring usemtl;
        uint32_t indexStart = 0;    // start position of index buffer
        uint32_t indexCount = 0;    // number of vertices (indices)
    };
    std::vector<Subset> subsets;

    struct Material {
        std::wstring name;
        DirectX::XMFLOAT4 ka = { 0.2f,0.2f,0.2f,1.0f };
        DirectX::XMFLOAT4 kd = { 0.8f,0.8f,0.8f,1.0f };
        DirectX::XMFLOAT4 ks = { 1.0f,1.0f,1.0f,1.0f };
        std::wstring textureFilename[2];
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shaderResourceView[2];
    };
    std::vector<Material> materials;

    struct BoundingBox {
        DirectX::XMFLOAT3 min = {};
        DirectX::XMFLOAT3 max = {};
    };
    BoundingBox boundingBox = {};
private:
    Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;

    Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout;
    Microsoft::WRL::ComPtr<ID3D11Buffer> constantBuffer;

    //std::wstring textureFilename;
    //Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shaderResourceView;

    // バウンディングボックスを計算する関数
    void CalculateBoundingBox(const std::vector<Vertex>& vertices);
public:
    // default : onInvers = false
    StaticMesh(ID3D11Device* device, const wchar_t* objFilename,bool onInvers = false);
    virtual ~StaticMesh() = default;

    void render(ID3D11DeviceContext* immediateContext,
        const DirectX::XMFLOAT4X4& world, const DirectX::XMFLOAT4& materialColor);
protected:
    void create_com_buffers(ID3D11Device* device, Vertex* vertices, size_t vertexCount,
        uint32_t* indices, size_t indexCount);
};