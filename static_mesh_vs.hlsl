#include "static_mesh.hlsli"
VS_OUT main(float4 position : POSITION, float4 normal : NORMAL, float2 texcoord : TEXCOORD)
{
    VS_OUT vout;
    vout.position = mul(position, mul(world, viewProjection));
    vout.worldPosition = mul(position, world);

    normal.w = 0;
    float4 N = normalize(mul(normal,world));
    //float4 L = normalize(-lightDirection);
    vout.worldNormal = N;
    vout.color = materialColor;
    
    vout.texcoord = texcoord;
    
    return vout;
}