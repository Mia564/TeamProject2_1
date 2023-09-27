#include "fullscreen_quad.h"
#include "shader.h"
#include "misc.h"

FullscreenQuad::FullscreenQuad(ID3D11Device* device) {
    create_vs_from_cso(device, "fullscreen_quad_vs.cso", embeddedVertexShader.ReleaseAndGetAddressOf(),
        nullptr, nullptr, 0);
    create_ps_from_cso(device, "fullscreen_quad_ps.cso", embeddedPixelShader.ReleaseAndGetAddressOf());

    HRESULT hr = S_OK;
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.ByteWidth = sizeof(Constant);
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    hr = device->CreateBuffer(&bufferDesc, nullptr, constantBuffer.GetAddressOf());
    _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
}

void FullscreenQuad::blit(ID3D11DeviceContext* immediateContext,
    ID3D11ShaderResourceView** shaderResourceView, uint32_t startSlot, uint32_t numViews,
    ID3D11PixelShader* replacedPixelShader) {
    immediateContext->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
    immediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    immediateContext->IASetInputLayout(nullptr);

    immediateContext->VSSetShader(embeddedVertexShader.Get(), 0, 0);
    replacedPixelShader ? immediateContext->PSSetShader(replacedPixelShader, 0, 0) :
        immediateContext->PSSetShader(embeddedPixelShader.Get(), 0, 0);

    immediateContext->PSSetShaderResources(startSlot, numViews, shaderResourceView);

    immediateContext->Draw(4, 0); // •`‰æˆ—
}

void FullscreenQuad::set_luminance_clamp(ID3D11DeviceContext* immediateContext, float min, float max) {
    Constant::LuminanceConstant data = { min,max };
    immediateContext->UpdateSubresource(constantBuffer.Get(), 0, 0, &data, 0, 0);
    immediateContext->PSSetConstantBuffers(0, 1, constantBuffer.GetAddressOf());
}

void FullscreenQuad::set_blur(ID3D11DeviceContext* immediateContext, float gaussianSigma, float bloomIntensity) {
    Constant::BlurConstant data = { gaussianSigma,bloomIntensity };
    immediateContext->UpdateSubresource(constantBuffer.Get(), 0, 0, &data, 0, 0);
    immediateContext->PSSetConstantBuffers(1, 1, constantBuffer.GetAddressOf());
}

void FullscreenQuad::set_tone_exposure(ID3D11DeviceContext* immediateContext, float exposure) {
    Constant::ToneExposureConstant data = { exposure };
    immediateContext->UpdateSubresource(constantBuffer.Get(), 0, 0, &data, 0, 0);
    immediateContext->PSSetConstantBuffers(2, 1, constantBuffer.GetAddressOf());
}