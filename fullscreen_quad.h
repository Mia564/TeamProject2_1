#pragma once

#include <d3d11.h>
#include <wrl.h>
#include <cstdint>
#include <DirectXMath.h>

class FullscreenQuad {
public:
    FullscreenQuad(ID3D11Device* device);
    virtual ~FullscreenQuad() = default;

    struct Constant {
        struct LuminanceConstant {
            float min;
            float max;
            int pad[2];
        };
        LuminanceConstant luminanceConstant = {};
        struct BlurConstant {
            float GaussianSigma;
            float bloomIntensity;
            int pad[2];
        };
        BlurConstant blurConstant = {};
        struct ToneExposureConstant {
            float exposure;
            int pad[3];
        };
        ToneExposureConstant toneExposureConstant = {};
    };

private:
    Microsoft::WRL::ComPtr<ID3D11VertexShader> embeddedVertexShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> embeddedPixelShader;
    Microsoft::WRL::ComPtr<ID3D11Buffer> constantBuffer;

public:
    void blit(ID3D11DeviceContext* immediateContextbool, ID3D11ShaderResourceView** shaderResourceView,
        uint32_t startSlot, uint32_t numViews, ID3D11PixelShader* replacedPixelShader = nullptr);
    void set_luminance_clamp(ID3D11DeviceContext* immediateContext, float min, float max);
    void set_blur(ID3D11DeviceContext* immediateContext, float gaussianSigma, float bloomIntensity);
    void set_tone_exposure(ID3D11DeviceContext* immediateContext, float exposure);
};