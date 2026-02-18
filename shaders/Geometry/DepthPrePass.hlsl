struct VertexDP
{
    float4 position : SV_Position;
    float2 uv : TexCoord;
    float3 normal : Normal;
};

struct PerDraw
{
    row_major float4x4 worldViewProjMatrix;
    row_major float4x4 worldMatrix;
};

ConstantBuffer<PerDraw> constBuffer : register(b0);

#define DepthPrePass \
"RootFlags ( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT)," \
"RootConstants(num32BitConstants=32, b0)," \

[RootSignature(DepthPrePass)]
VertexDP DepthVS(float3 position: POSITION, float2 uv: TEXCOORD, float3 normal: NORMAL)
{
    VertexDP vertexDP;

    vertexDP.position = mul(float4(position, 1.f), constBuffer.worldViewProjMatrix);
    return vertexDP;
}
