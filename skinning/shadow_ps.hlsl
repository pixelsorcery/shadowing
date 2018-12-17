
struct PS_INPUT
{
    float4 Pos         : SV_POSITION;
    float4 WorldPos    : POSITION;
    float4 LightPos    : POSITION1;
    float3 Normal      : NORMAL;
    float2 Tex         : TEXCOORD0;
};

void main(PS_INPUT input) {}