struct VS_OUT
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};
cbuffer LUMINANCE_CONSTANT_BUFFER : register(b0)
{
    float min;
    float max;
};
cbuffer BLUR_CONSTANT_BUFFER : register(b1)
{
    float gaussianSigma;
    float bloomIntensity; // ‹­‚³
}
cbuffer TONE_MAPPING_CONSTANT_BUFFER : register(b2)
{
    float exposure;
}