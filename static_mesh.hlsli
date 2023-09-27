struct VS_OUT
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 texcoord : TEXCOORD;
    float4 worldPosition : POSITION;
    float4 worldNormal : NORMAL;
};
cbuffer OBJECT_CONSTANT_BUFFER : register(b0)
{
    row_major float4x4 world;
    float4 materialColor;
};
cbuffer SCENE_CONSTANT_BUFFER : register(b1)
{
    row_major float4x4 viewProjection;
    float4 lightDirection;
    float4 cameraPosition;
};