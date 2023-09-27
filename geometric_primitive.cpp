#include "geometric_primitive.h"
#include "shader.h"
#include "misc.h"

GeometricPrimitive::GeometricPrimitive(ID3D11Device* device) {
    Vertex vertices[24] = {};
    // サイズが1.0の正立方体データを作成する(重心を原点とする)。正立方体のコントロールポイント数は 8 個、
    // 1 つのコントロールポイントの位置には法線の向きが違う頂点が 3 個あるので頂点情報の総数は 8x3=24 個、
    // 頂点情報配列（vertices）にすべて頂点の位置・法線情報を格納する。

    // 手前面
    vertices[0].position = { 0,1,0 };       // 左上
    vertices[1].position = { 1,1,0 };       // 右上
    vertices[2].position = { 1,0,0 };       // 右下
    vertices[3].position = { 0,0,0 };       // 左下

    vertices[0].normal = { 0,0,-1 };        // 左上
    vertices[1].normal = { 0,0,-1 };        // 右上
    vertices[2].normal = { 0,0,-1 };        // 右下
    vertices[3].normal = { 0,0,-1 };        // 左下

    // 奥面
    vertices[4].position = { 0,1,1 };       // 左上
    vertices[5].position = { 1,1,1 };       // 右上
    vertices[6].position = { 1,0,1 };       // 右下
    vertices[7].position = { 0,0,1 };       // 左下

    vertices[4].normal = { 0,0,1 };         // 左上
    vertices[5].normal = { 0,0,1 };         // 右上
    vertices[6].normal = { 0,0,1 };         // 右下
    vertices[7].normal = { 0,0,1 };         // 左下

    // 上面
    vertices[8].position = { 1,1,1 };       // 左上
    vertices[9].position = { 0,1,1 };       // 右上
    vertices[10].position = { 1,1,0 };      // 右下
    vertices[11].position = { 0,1,0 };      // 左下

    vertices[8].normal = { 0,1,0 };         // 左上
    vertices[9].normal = { 0,1,0 };         // 右上
    vertices[10].normal = { 0,1,0 };        // 右下
    vertices[11].normal = { 0,1,0 };        // 左下

    // 下面
    vertices[12].position = { 0,0,0 };      // 左上
    vertices[13].position = { 1,0,0 };      // 右上
    vertices[14].position = { 0,0,1 };      // 右下
    vertices[15].position = { 1,0,1 };      // 左下

    vertices[12].normal = { 0,-1,0 };       // 左上
    vertices[13].normal = { 0,-1,0 };       // 右上
    vertices[14].normal = { 0,-1,0 };       // 右下
    vertices[15].normal = { 0,-1,0 };       // 左下

    // 左面
    vertices[16].position = { 0,1,1 };      // 左上
    vertices[17].position = { 0,1,0 };      // 右上
    vertices[18].position = { 0,0,0 };      // 右下
    vertices[19].position = { 0,0,1 };      // 左下

    vertices[16].normal = { -1,0,0 };       // 左上
    vertices[17].normal = { -1,0,0 };       // 右上
    vertices[18].normal = { -1,0,0 };       // 右下
    vertices[19].normal = { -1,0,0 };       // 左下

    // 右面
    vertices[20].position = { 1,1,0 };      // 左上
    vertices[21].position = { 1,1,1 };      // 右上
    vertices[22].position = { 1,0,1 };      // 右下
    vertices[23].position = { 1,0,0 };      // 左下

    vertices[20].normal = { 1,0,0 };        // 左上
    vertices[21].normal = { 1,0,0 };        // 右上
    vertices[22].normal = { 1,0,0 };        // 右下
    vertices[23].normal = { 1,0,0 };        // 左下

    uint32_t indices[36] = {};
    // 正立方体は6面持ち、1つの面は2つの3角形ポリゴンで構成されるので3角形ポリゴンの総数は6×2＝12個、
    // 正立方体を描画するために12回の3角ポリゴン描画が必要、よって参照される頂点情報は12×3＝36回、
    // 3角形ポリゴンが参照する頂点情報のインデックス(頂点番号)を描画順に配列(indices)に格納する。
    // 時計回りが表面になるように格納すること。

    // 手前面 0~3
    indices[0] = 0; indices[1] = 1; indices[2] = 2; 
    indices[3] = 0; indices[4] = 2; indices[5] = 3;

    // 奥面 4~7
    indices[6] = 4; indices[7] = 6; indices[8] = 5;
    indices[9] = 4; indices[10] = 7; indices[11] = 6;

    // 上面 8~11
    indices[12] = 11; indices[13] = 9; indices[14] = 8;
    indices[15] = 8; indices[16] = 10; indices[17] = 11;

    // 下面 12~15
    indices[18] = 12; indices[19] = 13; indices[20] = 14;
    indices[21] = 13; indices[22] = 15; indices[23] = 14;

    // 左面 16~19
    indices[24] = 16; indices[25] = 17; indices[26] = 18;
    indices[27] = 16; indices[28] = 18; indices[29] = 19;

    // 左面 20~23
    indices[30] = 20; indices[31] = 21; indices[32] = 22;
    indices[33] = 20; indices[34] = 22; indices[35] = 23;

    create_com_buffers(device, vertices, 24, indices, 36);

    HRESULT hr = S_OK;

    D3D11_INPUT_ELEMENT_DESC inputElementDesc[] = {
        {"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT, 0,
        D3D11_APPEND_ALIGNED_ELEMENT,D3D11_INPUT_PER_VERTEX_DATA,0},
        {"NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,
        D3D11_APPEND_ALIGNED_ELEMENT,D3D11_INPUT_PER_VERTEX_DATA,0},
    };
    
    create_vs_from_cso(device, "geometric_primitive_vs.cso", vertexShader.GetAddressOf(),
        inputLayout.GetAddressOf(), inputElementDesc, ARRAYSIZE(inputElementDesc));
    create_ps_from_cso(device, "geometric_primitive_ps.cso", pixelShader.GetAddressOf());

    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.ByteWidth = sizeof(Constants);
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    hr = device->CreateBuffer(&bufferDesc, nullptr, constantBuffer.GetAddressOf());
    _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
}

void GeometricPrimitive::create_com_buffers(ID3D11Device* device, Vertex* vertices, size_t vertexCount,
    uint32_t* indices, size_t indexCount) {
    HRESULT hr = S_OK;

    D3D11_BUFFER_DESC bufferDesc = {};
    D3D11_SUBRESOURCE_DATA subresourceData = {};
    bufferDesc.ByteWidth = static_cast<UINT>(sizeof(Vertex) * vertexCount);
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bufferDesc.CPUAccessFlags = 0;
    bufferDesc.MiscFlags = 0;
    bufferDesc.StructureByteStride = 0;
    subresourceData.pSysMem = vertices;
    subresourceData.SysMemPitch = 0;
    subresourceData.SysMemSlicePitch = 0;
    hr = device->CreateBuffer(&bufferDesc, &subresourceData, vertexBuffer.ReleaseAndGetAddressOf());
    _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

    bufferDesc.ByteWidth = static_cast<UINT>(sizeof(uint32_t) * indexCount);
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    subresourceData.pSysMem = indices;
    hr = device->CreateBuffer(&bufferDesc, &subresourceData, indexBuffer.ReleaseAndGetAddressOf());
    _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
}

void GeometricPrimitive::render(ID3D11DeviceContext* immediateContext,
    const DirectX::XMFLOAT4X4& world, const DirectX::XMFLOAT4& materialColor) {
    uint32_t stride = sizeof(Vertex);
    uint32_t offset = 0;
    immediateContext->IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), &stride, &offset);
    immediateContext->IASetIndexBuffer(indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
    immediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    immediateContext->IASetInputLayout(inputLayout.Get());

    immediateContext->VSSetShader(vertexShader.Get(), nullptr, 0);
    immediateContext->PSSetShader(pixelShader.Get(), nullptr, 0);

    Constants data = { world, materialColor };
    immediateContext->UpdateSubresource(constantBuffer.Get(), 0, 0, &data, 0, 0);
    immediateContext->VSSetConstantBuffers(0, 1, constantBuffer.GetAddressOf());

    D3D11_BUFFER_DESC bufferDesc = {};
    indexBuffer->GetDesc(&bufferDesc);
    immediateContext->DrawIndexed(bufferDesc.ByteWidth / sizeof(uint32_t), 0, 0);
}