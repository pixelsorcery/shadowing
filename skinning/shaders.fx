//--------------------------------------------------------------------------------------
// File: shaders.fx
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------
//VERTEX SHADER CONSTANTS
StructuredBuffer<matrix> boneTransforms : register(t0);

cbuffer CBUniforms : register(b0)
{
    matrix View;
};

cbuffer cbChangeOnResize : register(b1)
{
    matrix Projection;
};

cbuffer cbChangesEveryFrame : register(b2)
{
    matrix World;
    float4 vMeshColor;
};

//PIXEL SHADER
Texture2D txDiffuse : register(t0);
SamplerState samLinear : register(s0);

//--------------------------------------------------------------------------------------
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
    float2 Tex         : TEXCOORD0;
};


//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
PS_INPUT VS(VS_INPUT input)
{
    matrix boneTransform = input.BoneWeights.x * boneTransforms[input.BoneIndices.x];
    boneTransform += input.BoneWeights.y * boneTransforms[input.BoneIndices.y];
    boneTransform += input.BoneWeights.z * boneTransforms[input.BoneIndices.z];
    boneTransform += input.BoneWeights.w * boneTransforms[input.BoneIndices.w];
    //matrix boneTransform = (matrix(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1));
    PS_INPUT output = (PS_INPUT)0;
    output.Pos = mul(float4(input.Pos,1), boneTransform);
    output.Pos = mul(output.Pos, World);
    output.Pos = mul(output.Pos, View);
    output.Pos = mul(output.Pos, Projection);
    output.Tex = input.Tex;

    return output;
}


//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 PS(PS_INPUT input) : SV_Target
{
    return txDiffuse.Sample(samLinear, input.Tex);
}
