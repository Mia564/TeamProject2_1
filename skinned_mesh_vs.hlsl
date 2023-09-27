#include "skinned_mesh.hlsli"
VS_OUT main(VS_IN vin)
{
    // ’¸“_‚Æ–@ü‚Ì•ÏŠ·ˆ—
    vin.normal.w = 0;
    float sigma = vin.tangent.w;
    vin.tangent.w = 0;
    
    float4 blendedPosition = { 0, 0, 0, 1 };
    float4 blendedNormal = { 0, 0, 0, 0 };
    float4 blendedTangent = { 0, 0, 0, 0 };
    
    for (int boneIndex = 0; boneIndex < 4; ++boneIndex)
    {
        blendedPosition += vin.boneWeights[boneIndex]
        * mul(vin.position, boneTransforms[vin.boneIndices[boneIndex]]);
        blendedNormal += vin.boneWeights[boneIndex]
        * mul(vin.normal, boneTransforms[vin.boneIndices[boneIndex]]);
        blendedTangent += vin.boneWeights[boneIndex]
        * mul(vin.tangent, boneTransforms[vin.boneIndices[boneIndex]]);
    }
    vin.position = float4(blendedPosition.xyz, 1.0f);
    vin.normal = float4(blendedNormal.xyz, 0.0f);
    vin.tangent = float4(blendedTangent.xyz, 0.0f);
    
    VS_OUT vout;
    vout.position = mul(vin.position, mul(world, viewProjection));
    vout.worldPosition = mul(vin.position, world);
    vout.worldNormal = normalize(mul(vin.normal, world));
    vout.worldTangent = normalize(mul(vin.tangent, world));
    vout.worldTangent.w = sigma;
    vout.texcoord = vin.texcoord;
#if 1
    vout.color = materialColor;
#else
    vout.color = 0;
    const float4 boneColors[4] = {
        {1,0,0,1 },
        {0,1,0,1 },
        {0,0,1,1 },
        {1,1,1,1 },
    };
    for (int boneIndex = 0; boneIndex < 4; ++boneIndex)
    {
        vout.color += boneColors[vin.boneIndices[boneIndex] % 4]
        * vin.boneWeights[boneIndex];
    }
#endif
    
    return vout;
}