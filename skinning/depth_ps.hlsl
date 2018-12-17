// AMDG
// (c) John Lukasiewicz 2017

// Constant Buffer Variables
Texture2D txDiffuse : register(t0);
SamplerState samLinear : register(s0);

cbuffer CBPSUniforms : register(b0)
{
    float4 dirLightPos;
    float4 eyePos;
};

struct VS_INPUT
{
    float3 Pos         : POSITION;
    float2 Tex         : TEXCOORD0;
    float4 BoneWeights : BLENDWEIGHT0;
    uint4 BoneIndices  : BLENDINDICES0;
};

struct PS_INPUT
{
    float4 Pos         : SV_POSITION;
    float4 WorldPos    : POSITION;
    float4 LightPos    : POSITION1;
    float3 Normal      : NORMAL;
    float2 Tex         : TEXCOORD0;
};

void PS(PS_INPUT input) {}
