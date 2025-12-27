struct VSInput
{
    float3 position : POSITION;
};

struct VSOutput {
    float4 position : SV_POSITION;
    float3 texCoord : TEXCOORD;
};

cbuffer SkyBoxConstants : register(b0)
{
    float4x4 viewProj;

    uint cubemapSRVIndex;
    uint padding[3];
}

VSOutput VSMain(VSInput input)
{
    VSOutput output;
    float4 pos = mul(viewProj, float4(input.position, 1.0));
    output.position = pos.xyww; // Force max depth
    output.texCoord = input.position;
    return output;
}
#define SkyBox \
"RootFlags ( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |" \
"            CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED )," \
"RootConstants(num32BitConstants=20, b0)," \
"StaticSampler(s0,"\
"           filter = FILTER_ANISOTROPIC,"\
"           addressU = TEXTURE_ADDRESS_WRAP,"\
"           addressV = TEXTURE_ADDRESS_WRAP,"\
"           addressW = TEXTURE_ADDRESS_WRAP,"\
"           visibility = SHADER_VISIBILITY_ALL )"

SamplerState skyboxSampler : register(s0);

[RootSignature(SkyBox)]
float4 PSMain(VSOutput input) : SV_TARGET
{
    Texture2D skyboxTexture = ResourceDescriptorHeap[NonUniformResourceIndex(cubemapSRVIndex)];
    return skyboxTexture.Sample(skyboxSampler, input.texCoord);
}