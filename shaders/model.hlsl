#include "Common.hlsl"
#include "PBRCalc.hlsl"
struct VSInput
{
    float3 _position : POSITION;
    float2 _uv : TEXCOORD;
    float3 _normal : NORMAL;
};

struct PSInput
{
    float4 _position : SV_POSITION;
    float2 _uv : TEXCOORD;
    float3 _normal : NORMAL;
    float3 _worldPos : TEXCOORD1;
};

SamplerState samplerDiffuse : register(s0);
// normals will be generated through the geometry
// SamplerState samplerNormal : register(s1);

cbuffer PerDraw : register(b0)
{
    row_major float4x4 _worldViewProjMatrix;

    row_major float4x4 _worldMatrix;

    uint _albedoIndex;
    uint _normalIndex;
    uint _metallicRoughNessIndex;
    uint _emissiveIndex;

    uint _accelerationStructureIndex;
    float3 _dirLightDir;

    float _dirLightIntensity;
    float3 _dirLightColor;

    float _pointLightIntensity;
    float3 _cameraPos;

    float _pointLightRadius;
    float3 _pointLightColor;
}

PSInput VSMain(VSInput input)
{
    PSInput result;
    result._position = mul(float4(input._position, 1.0f), _worldViewProjMatrix);
    result._normal =  mul(input._normal, (float3x3)_worldMatrix);
    result._uv = input._uv;
    result._worldPos = (mul(float4(input._position, 1.), _worldMatrix));
    return result;
}
/* add other root constants with macro defined Root Constants
 * I will be doing it per shader. Won't be sharing root constants among shaders
 * If I need to I will prolly include it with Common.hlsl and serialize it
 * to .rso maybe (root signature output file). Then get it read in the c++
 * code and do whatever is necessary. Won't need it tho
*/

#define Raster \
"RootFlags ( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |" \
"            CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED )," \
"RootConstants(num32BitConstants=52, b0)," \
"StaticSampler(s0,"\
"           filter = FILTER_ANISOTROPIC,"\
"           addressU = TEXTURE_ADDRESS_WRAP,"\
"           addressV = TEXTURE_ADDRESS_WRAP,"\
"           addressW = TEXTURE_ADDRESS_WRAP,"\
"           visibility = SHADER_VISIBILITY_ALL )"

[RootSignature(Raster)]
float4 PSMain(PSInput input) : SV_TARGET
{
    float2 uv = input._uv;

    Texture2D albedoTex = ResourceDescriptorHeap[NonUniformResourceIndex(_albedoIndex)];
    float4 albedoColor = albedoTex.Sample(samplerDiffuse, uv);

    float amb = .9f;
    float3 finalColor = float3(amb, amb, amb) * albedoColor.rgb;

	if (_metallicRoughNessIndex)
	{
	    Texture2D metallicRoughnessTex = ResourceDescriptorHeap[NonUniformResourceIndex(_metallicRoughNessIndex)];
        float2 metallicRoughness = metallicRoughnessTex.Sample(samplerDiffuse, uv).rg;
        float metallic = metallicRoughness.x;
        float roughness = metallicRoughness.y;

        finalColor *= (1.0 - metallic * 0.5);
	}
    if (_emissiveIndex)
    {
        Texture2D emissiveTex = ResourceDescriptorHeap[NonUniformResourceIndex(_emissiveIndex)];
        float3 emissive = emissiveTex.Sample(samplerDiffuse, uv).rgb;

        finalColor += emissive;
    }
    if (_normalIndex)
    {
        Texture2D normalTex = ResourceDescriptorHeap[NonUniformResourceIndex(_normalIndex)];
        //float3 N = normalTex.Load(uv);
    }

    // TODO: generated normals later soon
    // RWTexture2D<float4> normalTex = ResourceDescriptorHeap[NonUniformResourceIndex(_normalIndex)];

    return float4(finalColor, albedoColor.a);
}