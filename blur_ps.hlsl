#include "fullscreen_quad.hlsli"
#define POINT 0
#define LINEAR 1
#define ANISOTROPIC 2
SamplerState samplerStates[3] : register(s0);
Texture2D textureMaps[4] : register(t0);
float4 main(VS_OUT pin) : SV_TARGET
{
    uint mipLevel = 0, width, height, numberOfLevels;
    textureMaps[1].GetDimensions(mipLevel, width, height, numberOfLevels);
    
    float4 color = textureMaps[0].Sample(samplerStates[ANISOTROPIC], pin.texcoord);
    float alpha = color.a;
    
    float3 blurColor = 0;
    float gaussianKernelTotal = 0;
    
    const int gaussianHalfKernelSize = 3;
    [unroll]
    for (int x = -gaussianHalfKernelSize; x <= +gaussianHalfKernelSize; x += 1)
    {
        [unroll]
        for (int y = -gaussianHalfKernelSize; y <= +gaussianHalfKernelSize; y += 1)
        {
            float gaussianKernel = exp(-(x * x + y * y) / (2.0 * gaussianSigma * gaussianSigma)) /
            (2 * 3.14159265358979 * gaussianSigma * gaussianSigma);
            blurColor += textureMaps[1].Sample(samplerStates[LINEAR], pin.texcoord +
            float2(x * 1.0 / width, y * 1.0 / height)).rgb * gaussianKernel;
            gaussianKernelTotal += gaussianKernel;
        }
    }
    blurColor /= gaussianKernelTotal;
    color.rgb += blurColor * bloomIntensity;
    
#if 1
    // Tone mapping : HDR -> SDR
    color.rgb = 1 - exp(-color.rgb * exposure);
#endif

#if 1
    // Gamma process
    const float GAMMA = 2.2;
    color.rgb = pow(color.rgb, 1.0 / GAMMA);
#endif    
    
    return float4(color.rgb, alpha);
}