#include "Common.hlsl"
#include "PBRCalc.hlsl"

#include "LightsCommon.hlsl"
struct VSInput
{
    float3 position : POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
};

struct PSInput
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
    row_major float4x4 worldMatrix;
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
};

ConstantBuffer<PerDraw> perDraw : register(b0);

PSInput VSMain(VSInput input)
{
    PSInput result;
    result.position = mul(float4(input.position, 1.0f), perDraw.worldViewProjMatrix);
    result.normal =  mul(input.normal, (float3x3)perDraw.worldMatrix);
    result.uv = input.uv;
    result.worldPos = (mul(float4(input.position, 1.), perDraw.worldMatrix));
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
"RootConstants(num32BitConstants=44, b0)," \
"StaticSampler(s0,"\
"           filter = FILTER_ANISOTROPIC,"\
"           addressU = TEXTURE_ADDRESS_WRAP,"\
"           addressV = TEXTURE_ADDRESS_WRAP,"\
"           addressW = TEXTURE_ADDRESS_WRAP,"\
"           visibility = SHADER_VISIBILITY_ALL )"

uint3 GetClusterID(float2 pixelCoord, float2 depth)
{
    uint clusterX = pixelCoord.x / 16;
    uint clusterY = pixelCoord.y / 9;

    uint clusterZ = (16 * 9 * 24) / log2(perDraw.farZ / perDraw.nearZ) * (log2(depth) - log2(perDraw.nearZ));
    return uint3(clusterX, clusterY, clusterZ);
}

[RootSignature(Raster)]
float4 PSMain(PSInput input) : SV_TARGET
{
    float2 uv = input.uv;

    Texture2D albedoTex = ResourceDescriptorHeap[NonUniformResourceIndex(perDraw.albedoIndex)];
    float3 albedoColor = albedoTex.Sample(samplerDiffuse, uv).xyz;

	Texture2D normalTex = ResourceDescriptorHeap[NonUniformResourceIndex(perDraw.normalIndex)];
    float3 normal = normalTex.Load(uv);

    Texture2D depthTex = ResourceDescriptorHeap[NonUniformResourceIndex(perDraw.depthSRVIndex)];
    float2 depth = depthTex.Load(uv);


    float2 pixelCoord = uv * perDraw.ScreenResolution.xy;
	uint3 clusterID = GetClusterID(pixelCoord, depth);

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

    float3 lighting = float3(0, 0, 0);

    for (uint i = 0; i < lightsThisCluster; i++)
    {
        uint lightIndex = offset + i;
        Light light = LightBuffer[lightIndex];

        float3 lightDir = light.Position - worldPos;
        float distance = length(lightDir);

        if (distance < light.Radius)
        {
            lightDir /= distance;
            float attenuation = saturate(1.0 - (distance / light.Radius));
            attenuation *= attenuation;

            float NdotL = saturate(dot(normal, lightDir));
            lighting += albedoColor * light.Color * light.Intensity * NdotL * attenuation;
        }
    }

    float amb = 0.5f;
    float3 finalColor = float3(amb, amb, amb) * lighting.rgb;

	if (perDraw.metallicRoughNessIndex)
	{
	    Texture2D metallicRoughnessTex = ResourceDescriptorHeap[NonUniformResourceIndex(perDraw.metallicRoughNessIndex)];
        float2 metallicRoughness = metallicRoughnessTex.Sample(samplerDiffuse, uv).rg;
        float metallic = metallicRoughness.x;
        float roughness = metallicRoughness.y;

        finalColor *= (1.0 - metallic * 0.5);
	}
    if (perDraw.emissiveIndex)
    {
        Texture2D emissiveTex = ResourceDescriptorHeap[NonUniformResourceIndex(perDraw.emissiveIndex)];
        float3 emissive = emissiveTex.Sample(samplerDiffuse, uv).rgb;

        finalColor += emissive *6;
    }

    // TODO: generated normals later soon
    // RWTexture2D<float4> normalTex = ResourceDescriptorHeap[NonUniformResourceIndex(_normalIndex)];

    return float4(finalColor, albedoColor.a);
}
