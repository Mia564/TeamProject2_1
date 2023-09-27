#include "sprite.hlsli"
// ピクセルシェーダ：色の描画
Texture2D colorMap : register(t0);
SamplerState pointSamplerState : register(s0);
SamplerState linearSamplerState : register(s1);
SamplerState anisotropicSamplerState : register(s2);

float4 main(VS_OUT pin) : SV_TARGET // SV_TARGETなどのセマンティック = 意味付け
{
    float4 color = colorMap.Sample(anisotropicSamplerState, pin.texcoord);
    float alpha = color.a;
#if 1
    // Inverse gamma process
    const float GAMMA = 2.2;
    color.rgb = pow(color.rgb, GAMMA);
#endif
    return float4(color.rgb, alpha) * pin.color;
}