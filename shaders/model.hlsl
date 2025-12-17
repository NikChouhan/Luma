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
/* add other root constants with macro defined Root Constants
 * I will be doing it per shader. Won't be sharing root constants among shaders
 * If I need to I will prolly include it with Common.hlsl and serialize it
 * to .rso maybe (root signature output file). Then get it read in the c++
 * code and do whatever is necessary. Won't need it tho
*/

// #define MainRS \
// "RootFlags ( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | ... )," \
// "DescriptorTable ( SRV(t0), visibility = SHADER_VISIBILITY_PIXEL )," \
// "DescriptorTable ( Sampler(s0), visibility = SHADER_VISIBILITY_PIXEL )"

// [RootSignature(MainRS)]
float4 PSMain(PSInput input) : SV_TARGET
{
    Texture2D albedoTex = ResourceDescriptorHeap[NonUniformResourceIndex(constBuffer._albedoIndex)];
    float2 uv = input._uv;
    float4 albedoColor = albedoTex.Sample(samplerDiffuse, uv);
    float3 finalColor = albedoColor;
    // Tone mapping (simple Reinhard)
    //finalColor = finalColor / (finalColor + float3(1.0, 1.0, 1.0));
    return float4(finalColor, albedoColor.a);
}