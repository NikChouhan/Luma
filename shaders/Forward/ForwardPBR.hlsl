#include "../Common/Common.hlsli"

struct VSInput
{
    float3 position : POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
    float3 worldPos : TEXCOORD1;
};

SamplerState samplerDiffuse : register(s0);

struct PerDraw
{
    row_major float4x4 worldViewProjMatrix;

    uint albedoIndex;
    uint normalIndex;
    uint metallicRoughNessIndex;
    uint emissiveIndex;
};

ConstantBuffer<PerDraw> perDraw : register(b1);

VSOutput VSMain(VSInput input)
{
    VSOutput result;
    result.position = mul(float4(input.position, 1.0f), perDraw.worldViewProjMatrix);
    result.uv = input.uv;
    result.normal = input.normal;
    return result;
}

#define ForwardPBR \
"RootFlags ( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |" \
"            CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED )," \
"RootConstants(num32BitConstants=32, b1)," \
"StaticSampler(s0,"\
"           filter = FILTER_ANISOTROPIC,"\
"           addressU = TEXTURE_ADDRESS_WRAP,"\
"           addressV = TEXTURE_ADDRESS_WRAP,"\
"           addressW = TEXTURE_ADDRESS_WRAP,"\
"           visibility = SHADER_VISIBILITY_ALL )"\


[RootSignature(ForwardPBR)]
float4 PSMain(VSOutput input) : SV_TARGET
{
    Texture2D albedoTex = ResourceDescriptorHeap[NonUniformResourceIndex(perDraw.albedoIndex)];
    float4 albedo = albedoTex.Sample(samplerDiffuse, input.uv);
    return albedo;
}