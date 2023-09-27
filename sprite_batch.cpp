#include "sprite_batch.h"
#include "misc.h"
#include "texture.h"
#include "shader.h"

#include <sstream>
#include <WICTextureLoader.h>

SpriteBatch::SpriteBatch(ID3D11Device* device, const wchar_t* filename,size_t maxSprites)
: maxVertices(maxSprites * 6) {
    HRESULT hr = S_OK;

    std::unique_ptr<Vertex[]> vertices = std::make_unique<Vertex[]>(maxVertices);

    // 頂点バッファのオブジェクトの生成
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.ByteWidth = sizeof(Vertex) * maxVertices; // ここに入れるメモリサイズはデータの数ぶん
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    bufferDesc.MiscFlags = 0;
    bufferDesc.StructureByteStride = 0;
    D3D11_SUBRESOURCE_DATA subresourceData = {}; // 初期化データ
    subresourceData.pSysMem = vertices.get();
    subresourceData.SysMemPitch = 0;
    subresourceData.SysMemSlicePitch = 0;

    hr = device->CreateBuffer(&bufferDesc, &subresourceData, vertexBuffer.GetAddressOf());
    _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

    // 頂点シェーダーオブジェクトの生成
    const char* csoName = "sprite_vs.cso";

    // 入力レイアウトオブジェクトの生成
    D3D11_INPUT_ELEMENT_DESC inputElementDesc[]{
        {"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,
        D3D11_APPEND_ALIGNED_ELEMENT,D3D11_INPUT_PER_VERTEX_DATA,0},
        {"COLOR",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,
        D3D11_APPEND_ALIGNED_ELEMENT,D3D11_INPUT_PER_VERTEX_DATA,0},
        {"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,
         D3D11_APPEND_ALIGNED_ELEMENT,D3D11_INPUT_PER_VERTEX_DATA,0},
    };

    create_vs_from_cso(device, csoName, vertexShader.GetAddressOf(), inputLayout.GetAddressOf(), inputElementDesc, _countof(inputElementDesc));

    // ピクセルシェーダーオブジェクトの生成
    csoName = "sprite_ps.cso";

    create_ps_from_cso(device, csoName, pixelShader.GetAddressOf());

    // 画像ファイルのロードとシェーダーリソースビューオブジェクト(ID3D11ShaderResourceView)の生成
    // テクスチャ情報(D3D11_TEXTURE2D_DESC)の取得
    load_texture_from_file(device, filename, shaderResourceView.GetAddressOf(), &texture2dDesc);
    release_all_textures();
}

// spriteクラスのrenderメンバ関数の実装
void SpriteBatch::render(ID3D11DeviceContext* immediateContext,
    float dx, float dy,                     // 矩形の左上の座標(スクリーン座標系)
    float dw, float dh,                     // 矩形のサイズ(スクリーン座標系)
    float r, float g, float b, float a,     // 矩形の描画色
    float angle/*degree*/                   // 回転角
) {
    render(immediateContext, dx, dy, dw, dh, r, g, b, a, angle, 0.0f, 0.0f, static_cast<float>(texture2dDesc.Width), static_cast<float>(texture2dDesc.Height));
}

void SpriteBatch::render(ID3D11DeviceContext* immediateContext,
    float dx, float dy, float dw, float dh,
    float r, float g, float b, float a,
    float angle,/*degree*/
    float sx, float sy, float sw, float sh
) {
    // スクリーン(ビューポート)のサイズを取得する
    D3D11_VIEWPORT viewport = {};
    UINT numViewports = 1;
    immediateContext->RSGetViewports(&numViewports, &viewport);

    // left-top
    float x0 = dx;
    float y0 = dy;
    // right-top
    float x1 = dx + dw;
    float y1 = dy;
    // left-bottom
    float x2 = dx;
    float y2 = dy + dh;
    // right-bottom
    float x3 = dx + dw;
    float y3 = dy + dh;

    // 回転の中心を矩形の中心点にした場合
    float cx = dx + dw * 0.5f;
    float cy = dy + dh * 0.5f;
    rotate(x0, y0, cx, cy, angle);
    rotate(x1, y1, cx, cy, angle);
    rotate(x2, y2, cx, cy, angle);
    rotate(x3, y3, cx, cy, angle);

    // スクリーン座標系からNDCへの座標変換をおこなう
    x0 = 2.0f * x0 / viewport.Width - 1.0f;
    y0 = 1.0f - 2.0f * y0 / viewport.Height;
    x1 = 2.0f * x1 / viewport.Width - 1.0f;
    y1 = 1.0f - 2.0f * y1 / viewport.Height;
    x2 = 2.0f * x2 / viewport.Width - 1.0f;
    y2 = 1.0f - 2.0f * y2 / viewport.Height;
    x3 = 2.0f * x3 / viewport.Width - 1.0f;
    y3 = 1.0f - 2.0f * y3 / viewport.Height;

    // UV座標
    float u0 = sx / texture2dDesc.Width;
    float v0 = sy / texture2dDesc.Height;
    float u1 = (sx + sw) / texture2dDesc.Width;
    float v1 = (sy + sh) / texture2dDesc.Height;

    vertices.push_back({ { x0,y0,0},{r,g,b,a},{u0,v0} });
    vertices.push_back({ { x1,y1,0},{r,g,b,a},{u1,v0} });
    vertices.push_back({ { x2,y2,0},{r,g,b,a},{u0,v1} });
    vertices.push_back({ { x2,y2,0},{r,g,b,a},{u0,v1} });
    vertices.push_back({ { x1,y1,0},{r,g,b,a},{u1,v0} });
    vertices.push_back({ { x3,y3,0},{r,g,b,a},{u1,v1} });
}

void SpriteBatch::render(ID3D11DeviceContext* immediateContext,
    float dx, float dy,                     // 矩形の左上の座標(スクリーン座標系)
    float dw, float dh                      // 矩形のサイズ(スクリーン座標系)
) {
    render(immediateContext, dx, dy, dw, dh, 1.0f, 1.0f, 1.0f, 1.0f, 0, 0.0f, 0.0f, static_cast<float>(texture2dDesc.Width), static_cast<float>(texture2dDesc.Height));
}


void SpriteBatch::begin(ID3D11DeviceContext* immediateContext) {
    vertices.clear();
    // シェーダーのバインド
    immediateContext->VSSetShader(vertexShader.Get(), nullptr, 0);
    immediateContext->PSSetShader(pixelShader.Get(), nullptr, 0);
    immediateContext->PSSetShaderResources(0, 1, shaderResourceView.GetAddressOf());
}

void SpriteBatch::end(ID3D11DeviceContext* immediateContext) {
    // 計算結果で頂点バッファオブジェクトを更新する
    HRESULT hr = S_OK;
    D3D11_MAPPED_SUBRESOURCE mappedSubresource = {};
    hr = immediateContext->Map(vertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubresource);
    _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

    size_t vertexCount = vertices.size();
    _ASSERT_EXPR(maxVertices >= vertexCount, "Buffer overflow");
    Vertex* data = reinterpret_cast<Vertex*>(mappedSubresource.pData);

    if (data != nullptr) {
        const Vertex* p = vertices.data();
        memcpy_s(data, maxVertices * sizeof(Vertex), p, vertexCount * sizeof(Vertex));
    }
    immediateContext->Unmap(vertexBuffer.Get(), 0);

    // 頂点バッファーのバインド
    UINT stride = sizeof(Vertex); // ここは型のサイズを入れる
    UINT offset = 0;
    immediateContext->IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), &stride, &offset);
    // プリミティブタイプおよびデータの順序に関する情報のバインド
    immediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    // 入力レイアウトオブジェクトのバインド
    immediateContext->IASetInputLayout(inputLayout.Get());

    // プリミティブの描画
    immediateContext->Draw(static_cast<UINT>(vertexCount), 0);
}

SpriteBatch::~SpriteBatch() {

}