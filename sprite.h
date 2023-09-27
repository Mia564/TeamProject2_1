#pragma once
#include <d3d11.h>
#include <directxmath.h>
#include <wrl.h>
#include <string>


class Sprite {
private:
    // メンバ変数
    Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout;
    Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shaderResourceView;
    D3D11_TEXTURE2D_DESC texture2dDesc;
public:
    // メンバ関数
    void render(ID3D11DeviceContext* immediateContext,
        float dx,float dy,float dw,float dh,
        float r,float g,float b,float a,
        float angle
    );

    void render(ID3D11DeviceContext* immediateContext,
        float dx, float dy, float dw, float dh,
        float r, float g, float b, float a,
        float angle,/*degree*/
        float sx, float sy, float sw, float sh
    );

    void render(ID3D11DeviceContext* immediateContext,
        float dx, float dy, float dw, float dh
    );

    void textout(ID3D11DeviceContext* immediateContext, std::string s,
        float x, float y, float w, float h, float r, float g, float b, float a);

    // 点(x,y)が点(cx,cy)を中心に角(angle)で回転した時の座標を計算する関数(inline)
    void rotate(float& x, float& y, float cx, float cy, float angle) {
        x -= cx;
        y -= cy;

        float cos = cosf(DirectX::XMConvertToRadians(angle));
        float sin = sinf(DirectX::XMConvertToRadians(angle));
        float tx = x, ty = y;
        x = cos * tx + -sin * ty;
        y = sin * tx + cos * ty;

        x += cx;
        y += cy;
    }
    // コンストラクタ・デストラクタ
    Sprite(ID3D11Device* device,const wchar_t* filename);
    ~Sprite();
    // 頂点フォーマット
    struct Vertex {
        DirectX::XMFLOAT3 position;
        DirectX::XMFLOAT4 color;
        DirectX::XMFLOAT2 texcoord;
    };
};

