struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
};

SamplerState Sampler : register(s0);

struct PerDraw
{
    uint materialIndex;
    uint3 padding;
    row_major float4x4 worldViewProjMatrix;
    row_major float4x4 worldMatrix;
};

ConstantBuffer<PerDraw> constBuffer : register(b0);

PSInput VSMain(float3 position : POSITION, float2 uv : TEXCOORD, float3 normal : NORMAL)
{
    PSInput result;

    result.position = mul(float4(position, 1.0f), constBuffer.worldViewProjMatrix);
    result.normal = mul(float4(normal, 0.0f), constBuffer.worldMatrix).xyz;
    result.uv = uv;
    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    float3 normal = normalize(input.normal);
    float3 lightDir = normalize(float3(0.0f, 1.0f, 1.0f));
    float diffuse = max(dot(normal, lightDir), 0.0f);

    Texture2D<float4> tex = ResourceDescriptorHeap[NonUniformResourceIndex(constBuffer.materialIndex)];
    float4 texColor = tex.Sample(Sampler, input.uv);

    float ambientIntensity = 0.1;

    float3 ambient = texColor.rgb * ambientIntensity;
    float3 diffuseColor = texColor.rgb * diffuse;
    float3 finalColor = ambient + diffuseColor;
    
    return float4(finalColor, texColor.a);
}