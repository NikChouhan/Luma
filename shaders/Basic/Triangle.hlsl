#include "../Common/Common.hlsli"

struct VSInput
{
    float3 position : POSITION;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float3 color : COLOR;
};

#define TriangleRoot \
"RootFlags ( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT )"

[RootSignature(TriangleRoot)]
VSOutput VS_Main(VSInput input)
{
    VSOutput output;
    output.position = float4(input.position, 1.0f);
    output.color = input.position * 0.5f + 0.5f;
    return output;
}

[RootSignature(TriangleRoot)]
float4 PS_Main(VSOutput input) : SV_TARGET
{
    return float4(input.color, 1.0f);
}