#include "shader.h"
#include "misc.h"
#include <sstream>

// シェーダーファイルのロードをモジュール化
HRESULT create_vs_from_cso(ID3D11Device* device, const char* csoName, ID3D11VertexShader** vertexShader,
    ID3D11InputLayout** inputLayout, D3D11_INPUT_ELEMENT_DESC* inputElementDesc, UINT numElements) {
    FILE* fp = nullptr;
    fopen_s(&fp, csoName, "rb");
    _ASSERT_EXPR_A(fp, "CSO File not found");

    fseek(fp, 0, SEEK_END);
    long csoSz = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    std::unique_ptr<unsigned char[]> csoData = std::make_unique<unsigned char[]>(csoSz);
    fread(csoData.get(), csoSz, 1, fp);
    fclose(fp);

    HRESULT hr = S_OK;
    hr = device->CreateVertexShader(csoData.get(), csoSz, nullptr, vertexShader);
    _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

    if (inputLayout) {
        hr = device->CreateInputLayout(inputElementDesc, numElements, csoData.get(), csoSz, inputLayout);
        _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
    }

    return hr;
}

HRESULT create_ps_from_cso(ID3D11Device* device, const char* csoName, ID3D11PixelShader** pixelShader) {
    FILE* fp = nullptr;
    fopen_s(&fp, csoName, "rb");
    _ASSERT_EXPR_A(fp,"CSO File not found");

    fseek(fp, 0, SEEK_END);
    long csoSz = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    std::unique_ptr<unsigned char[]> csoData = std::make_unique<unsigned char[]>(csoSz);
    fread(csoData.get(), csoSz, 1, fp);
    fclose(fp);

    HRESULT hr = S_OK;
    hr = device->CreatePixelShader(csoData.get(), csoSz, nullptr, pixelShader);
    _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

    return hr;
}