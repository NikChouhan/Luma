#include "Common.hlsl"
#include "PBRCalc.hlsl"
#include "TraceShadowRay.hlsl"

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
SamplerState samplerNormal : register(s1);

struct PerDraw
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
};

ConstantBuffer<PerDraw> constBuffer : register(b0);

PSInput VSMain(VSInput input)
{
    PSInput result;
    result._position = mul(float4(input._position, 1.0f), constBuffer._worldViewProjMatrix);
    result._normal =  mul(input._normal, (float3x3)constBuffer._worldMatrix);
    result._uv = input._uv;
    result._worldPos = (mul(float4(input._position, 1.), constBuffer._worldMatrix));
    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    RaytracingAccelerationStructure accelStruct = ResourceDescriptorHeap[NonUniformResourceIndex(constBuffer._accelerationStructureIndex)];
    Texture2D albedoTex = ResourceDescriptorHeap[NonUniformResourceIndex(constBuffer._albedoIndex)];
    Texture2D metallicRoughnessTex = ResourceDescriptorHeap[NonUniformResourceIndex(constBuffer._metallicRoughNessIndex)];
    Texture2D emissiveTex = ResourceDescriptorHeap[NonUniformResourceIndex(constBuffer._emissiveIndex)];

    RWTexture2D<float4> normalTex = ResourceDescriptorHeap[NonUniformResourceIndex(72)];

    float2 uv = input._uv;
	//uv.x = 1.0 - uv.x;
	//uv.y = 1.0 - uv.y;
	//uv = uv.yx;


    float4 albedoColor = albedoTex.Sample(samplerDiffuse, uv);
    float2 metallicRoughness = metallicRoughnessTex.Sample(samplerDiffuse, uv).rg;
    float metallic = metallicRoughness.x;
    float roughness = metallicRoughness.y;
    float3 emissive = emissiveTex.Sample(samplerDiffuse, uv).rgb;

    float3 N = normalTex.Load(uv);
    float3 V = normalize(constBuffer._cameraPos - input._worldPos);
    float3 surfaceOffset = input._worldPos + N * 0.1;

    float3 Lo = float3(0.0, 0.0, 0.0);

    /*float attenuation = 1.0 / (distance * distance);
    attenuation *= pow(max(1.0 - (distance / constBuffer._pointLightRadius), 0.0), 2.0);*/

    float shadow = 1.f;
    float3 radiance = constBuffer._pointLightColor * constBuffer._pointLightIntensity;
    Lo += CalculatePBR(N, V, float3(0.,0.,0.), radiance, albedoColor.rgb, metallic, roughness, 1.0) * shadow;
    float amb = .9f;
    float3 ambient = float3(amb, amb, amb) * albedoColor.rgb * (1.0 - metallic * 0.5);

    float3 finalColor = ambient + Lo + emissive;

    // Tone mapping (simple Reinhard)
    finalColor = finalColor / (finalColor + float3(1.0, 1.0, 1.0));
    return float4(finalColor, albedoColor.a);
}