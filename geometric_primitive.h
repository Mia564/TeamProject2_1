#pragma once
#include <DirectXMath.h>
#include <d3d11.h>
#include <directxmath.h>
#include <wrl.h>

class GeometricPrimitive {
public:
    struct Vertex {
        DirectX::XMFLOAT3 position;
        DirectX::XMFLOAT3 normal;
    };
    struct Constants {
        DirectX::XMFLOAT4X4 world;
        DirectX::XMFLOAT4 materialColor;
    };
private:
    Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;

    Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout;
    Microsoft::WRL::ComPtr<ID3D11Buffer> constantBuffer;

public:
    GeometricPrimitive(ID3D11Device* device);
    virtual ~GeometricPrimitive() = default;

    void render(ID3D11DeviceContext* immediateContext,
        const DirectX::XMFLOAT4X4& world, const DirectX::XMFLOAT4& materialColor);
protected:
    void create_com_buffers(ID3D11Device* device, Vertex* vertices, size_t vertexCount,
        uint32_t* indices, size_t indexCount);
};