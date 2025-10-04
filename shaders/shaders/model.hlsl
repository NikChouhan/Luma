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
    result.normal = mul(constBuffer.worldMatrix, float4(normal, 0.0f)).xyz;
    result.uv = uv;
    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    // uv test (pass)
    //return float4(input.uv.x, input.uv.y, 0.0f, 1.0f);

    // normals test (pass)
    float3 normal = normalize(input.normal);
    float3 outputNormal = normal * 0.5f + 0.5f;
    return float4(outputNormal, 1.0f);

    //Texture2D<float4> tex = ResourceDescriptorHeap[NonUniformResourceIndex(constBuffer.materialIndex)];
    //float4 texColor = tex.Sample(Sampler, input.uv);
    
    //return float4(texColor);
}