#include "fullscreen_quad.hlsli"

#define POINT 0
#define LINEAR 1
#define ANISOTROPIC 2
SamplerState samplerStates[3] : register(s0);
Texture2D textureMap : register(t0);
float4 main(VS_OUT pin) : SV_TARGET
{
    return textureMap.Sample(samplerStates[LINEAR], pin.texcoord);
}