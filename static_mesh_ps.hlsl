#include "static_mesh.hlsli"

Texture2D color_map : register(t0);
Texture2D normal_map : register(t1);
SamplerState pointSamplerState : register(s0);
SamplerState linerSamplerState : register(s1);
SamplerState anisotropicSamplerState : register(s2);

//#define TEST1 // 鏡面反射テスト用

float4 main(VS_OUT pin) : SV_TARGET
{
    float4 color = color_map.Sample(anisotropicSamplerState, pin.texcoord);
    
    float alpha = color.a;
    float3 N = normalize(pin.worldNormal.xyz);
    
 #if 1
    float3 T = float3(1.0001, 0, 0);
    float3 B = normalize(cross(N, T));
    T = normalize(cross(B, N));
    
    float4 normal = normal_map.Sample(linerSamplerState, pin.texcoord);
    
  
    normal = (normal * 2.0) - 1.0;
    normal.w = 0;
    N = normalize((normal.x * T) + (normal.y * B) + (normal.z * N));
 #endif
    float3 L = normalize(-lightDirection.xyz);
    float3 diffuse = color.rgb * max(0, dot(N, L));
    
    float3 V = normalize(cameraPosition.xyz - pin.worldPosition.xyz);
#ifdef TEST1
    float3 specular = pow(max(0, dot(N, normalize(V + L))), 256);
#else 
    float3 specular = pow(max(0, dot(N, normalize(V + L))), 128); // ここの第二引数で反射の値の調整
#endif
    return float4(diffuse + specular, alpha) * pin.color;
}