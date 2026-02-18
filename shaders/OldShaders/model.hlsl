#include "../Common/Common.hlsli"
#include "PBRCalc.hlsl"

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
// normals will be generated through the geometry
// SamplerState samplerNormal : register(s1);

struct PerDraw
{
    row_major float4x4 worldViewProjMatrix;
    row_major float4x4 inverseViewProj;

    uint albedoIndex;
    uint normalIndex;
    uint metallicRoughNessIndex;
    uint emissiveIndex;

    float2 ScreenResolution;
    uint clusterIndex;
    uint depthSRVIndex;
    
    uint lightListCounterBufferSRVIndex;
    uint lightListTextureSRVIndex;
    uint lightIndicesBufferSRVIndex;
    uint globalLightStructuredBufferSRVIndex;

    float farZ;
    float nearZ;
    float2 padding;

	float3 cameraPos;
    float padding2;
};

ConstantBuffer<PerDraw> perDraw : register(b1);

VSOutput VSMain(VSInput input)
{
    VSOutput result;
    result.position = mul(float4(input.position, 1.0f), perDraw.worldViewProjMatrix);
    //result.normal =  mul(input.normal, (float3x3)perDraw.worldMatrix);
    result.uv = input.uv;
    //result.worldPos = (mul(float4(input.position, 1.), perDraw.worldMatrix));
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
"RootConstants(num32BitConstants=52, b1)," \
"StaticSampler(s0,"\
"           filter = FILTER_ANISOTROPIC,"\
"           addressU = TEXTURE_ADDRESS_WRAP,"\
"           addressV = TEXTURE_ADDRESS_WRAP,"\
"           addressW = TEXTURE_ADDRESS_WRAP,"\
"           visibility = SHADER_VISIBILITY_ALL )"\

uint3 GetClusterID(int2 pixelCoord, float depth)
{
    uint clusterX = pixelCoord.x / 16;
    uint clusterY = pixelCoord.y / 9;

    uint clusterZ = ( 24) / log2(perDraw.farZ / perDraw.nearZ) * (log2(depth) - log2(perDraw.nearZ));
    return uint3(clusterX, clusterY, clusterZ);
}

[RootSignature(Raster)]
float4 PSMain(VSOutput input) : SV_TARGET
{
    float2 uv = input.uv;
	int2 pixelCoord = int2(uv * perDraw.ScreenResolution.xy);

    Texture2D albedoTex = ResourceDescriptorHeap[NonUniformResourceIndex(perDraw.albedoIndex)];
    float4 albedoColor = albedoTex.Sample(samplerDiffuse, uv);

    Texture2D normalTex = ResourceDescriptorHeap[NonUniformResourceIndex(perDraw.normalIndex)];
    float3 normal = normalTex.Load(int3(pixelCoord, 0)).xyz;

    Texture2D depthTex = ResourceDescriptorHeap[NonUniformResourceIndex(perDraw.depthSRVIndex)];
    float depth = depthTex.Load(int3(pixelCoord, 0)).r;
    uint3 clusterID = GetClusterID(pixelCoord, input.position.w);

    uint clusterIndex = clusterID.x +
				        16 * clusterID.y +
				        16 * 9 * clusterID.z;

    StructuredBuffer<Light> LightBuffer = ResourceDescriptorHeap[NonUniformResourceIndex(perDraw.globalLightStructuredBufferSRVIndex)];

    // find offset and light size for the particular cluster
    Texture3D<uint2> GlobalLightIndexList = ResourceDescriptorHeap[NonUniformResourceIndex(perDraw.lightListTextureSRVIndex)];
    uint2 offsetLight = GlobalLightIndexList[clusterID];
    uint offset = offsetLight.x;
    uint lightsThisCluster = offsetLight.y;

    float3 worldPos = CalculateWorldPosFromDepth(depth, uv, perDraw.inverseViewProj);

    float metallic;
    float roughness;
    if (perDraw.metallicRoughNessIndex != UINT_MAX_VALUE)
    {
        Texture2D metallicRoughnessTex = ResourceDescriptorHeap[NonUniformResourceIndex(perDraw.metallicRoughNessIndex)];
        float2 metallicRoughness = metallicRoughnessTex.Sample(samplerDiffuse, uv).rg;
        metallic = metallicRoughness.x;
        roughness = metallicRoughness.y;
    }


    float3 emissive = 0.f;
    //if (perDraw.emissiveIndex != UINT_MAX_VALUE)
    //{
    //    Texture2D emissiveTex = ResourceDescriptorHeap[NonUniformResourceIndex(perDraw.emissiveIndex)];
    //    emissive = emissiveTex.Sample(samplerDiffuse, uv).rgb;
    //}
    StructuredBuffer<uint> GlobalLightIndexBuffer = ResourceDescriptorHeap[NonUniformResourceIndex(perDraw.lightIndicesBufferSRVIndex)];

    float3 lighting = float3(0, 0, 0);

    for (uint i = 0; i < lightsThisCluster; i++)
    {
        uint listIndex = offset + i;
        uint lightIndex = GlobalLightIndexBuffer[listIndex];
        Light light = LightBuffer[lightIndex];

        float V = worldPos - perDraw.cameraPos;

        lighting += CalculatePBRLighting(light, perDraw.cameraPos, worldPos, normal, albedoColor.rgb, metallic, roughness);
    }
    //return float4(lighting, albedoColor.a);
    float amb = 1.4f;
    float3 ambient = float3(amb, amb, amb) * albedoColor.rgb;// * (1. - metallic);

    float3 finalColor = ambient + lighting.rgb+ emissive;

    // TODO: generated normals later soon
    // RWTexture2D<float4> normalTex = ResourceDescriptorHeap[NonUniformResourceIndex(_normalIndex)];

    return float4(finalColor, albedoColor.a);
}
