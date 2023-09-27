#include "fullscreen_quad.hlsli"

#define POINT 0
#define LINEAR 1
#define ANISOTROPIC 2
SamplerState samplerStates[3] : register(s0);
Texture2D textureMaps[4] : register(t0);

float4 main(VS_OUT pin) : SV_TARGET
{
    float4 color = textureMaps[0].Sample(samplerStates[ANISOTROPIC], pin.texcoord);
    float alpha = color.a;
    color.rgb = smoothstep(min, max, dot(color.rgb, float3(0.299, 0.587, 0.114))) * color.rgb;
    return float4(color.rgb, alpha);
}