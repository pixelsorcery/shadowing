// AMDG
// (c) John Lukasiewicz 2017

// Constant Buffer Variables
Texture2D txDiffuse : register(t0);
Texture2D txShadowMap : register(t1);

SamplerState samLinear : register(s0);
SamplerState samClamp : register(s1);

cbuffer CBPSUniforms : register(b0)
{
    float4 dirLightPos;
    float4 eyePos;
};

struct PS_INPUT
{
    float4 Pos         : SV_POSITION;
    float4 WorldPos    : POSITION;
    float4 LightPos    : TEXCOORD1;
    float3 Normal      : NORMAL;
    float2 Tex         : TEXCOORD0;
};


float4 PS(PS_INPUT input) : SV_Target
{
    float4 color = (1.0f, 1.0f, 1.0f, 1.0f);
    float shininess = 32.0f;
    float intensity = 0.0f; // looks terrible on skin
    float4 outputColor;
    float4 texColor = txDiffuse.Sample(samLinear, input.Tex);
    float3 normal = normalize(input.Normal);
    float3 light = normalize(-dirLightPos);
    float3 reflection = normalize(reflect(light, normal));
    float3 view = normalize(eyePos - input.WorldPos);
    float bias = 0.0005f;

    // re-homogenize lightpos
    input.LightPos.xyz /= input.LightPos.w;
    
    // transform clip space to texture space
    input.LightPos.x = input.LightPos.x / 2.0f + 0.5f;
    input.LightPos.y = input.LightPos.y /-2.0f + 0.5f;

    float shadowMapDepth = txShadowMap.Sample(samClamp, input.LightPos.xy).r;
    shadowMapDepth += bias;

    float4 ambient = float4(0.3f, 0.3f, 0.3f, 1.0f);
    float4 diffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);;
    float4 specular = float4(0.0f, 0.0f, 0.0f, 0.0f);;

    if (shadowMapDepth >= input.LightPos.z)
    {
        diffuse = saturate(dot(normal, -light));
        specular = intensity * pow(saturate(dot(reflection, view)), shininess);
    }

    outputColor = color * texColor * (diffuse + specular + ambient);

    //outputColor = float4(input.LightPos.x, input.LightPos.y, input.LightPos.z, 1.0);

    return outputColor;
}

