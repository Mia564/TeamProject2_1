#include "geometric_primitive.hlsli"
VS_OUT main(float4 position : POSITION, float4 normal : NORMAL)
{
    VS_OUT vout;
    vout.position = mul(position, mul(world, viewProjection));
    
    normal.w = 0;
    float4 N = normalize(mul(normal, world));
    float4 L = normalize(-lightDirection);
    
    vout.color.rgb = materialColor.rgb * max(0, dot(L, N));
    vout.color.a = materialColor.a;
    return vout;
}