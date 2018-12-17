// AMDG
// (c) John Lukasiewicz 2017

// Constant Buffer Variables
StructuredBuffer<matrix> boneTransforms : register(t0);

cbuffer CBVSUniforms : register(b0)
{
    matrix View;
    matrix LightView;
};

cbuffer cbChangeOnResize : register(b1)
{
    matrix Projection;
    matrix LightProjection;
};

cbuffer cbChangesEveryFrame : register(b2)
{
    matrix World;
    float4 vMeshColor;
};

struct VS_INPUT
{
    float3 Pos         : POSITION;
    float3 Normal      : NORMAL;
    float2 Tex         : TEXCOORD0;
    float4 BoneWeights : BLENDWEIGHT0;
    uint4 BoneIndices  : BLENDINDICES0;
};

struct PS_INPUT
{
    float4 Pos         : SV_POSITION;
    float4 WorldPos    : POSITION;
    float4 LightPos    : TEXCOORD1;
    float3 Normal      : NORMAL;
    float2 Tex         : TEXCOORD0;
};

PS_INPUT VS(VS_INPUT input)
{
    matrix boneTransform = input.BoneWeights.x * boneTransforms[input.BoneIndices.x];
    boneTransform += input.BoneWeights.y * boneTransforms[input.BoneIndices.y];
    boneTransform += input.BoneWeights.z * boneTransforms[input.BoneIndices.z];
    boneTransform += input.BoneWeights.w * boneTransforms[input.BoneIndices.w];
    //matrix boneTransform = (matrix(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1));
    PS_INPUT output = (PS_INPUT)0;
    output.WorldPos = mul(float4(input.Pos, 1), boneTransform);
    output.WorldPos = mul(output.WorldPos, World);

    output.Pos = mul(output.WorldPos, View);
    output.LightPos = mul(output.WorldPos, LightView);

    output.Pos = mul(output.Pos, Projection);
    output.LightPos = mul(output.LightPos, LightProjection);

    output.Tex = input.Tex;

    output.Normal = mul(float3(input.Normal), boneTransform);
    output.Normal = mul(output.Normal, World);

    return output;
}