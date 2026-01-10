struct ClusterGeometryData
{
    float3 vertices[8];
    uint2x3 indices[6];
};

struct VSInput
{
    uint vertexID : SV_VertexID;
    uint instanceID : SV_InstanceID;
};

struct VSOutput
{
    float4 position : SV_Position;
    float3 color : COLOR;
};

struct ClusterRender
{
    float4x4 viewProj;

    uint clusterGeometryStructuredBufferSRVIndex;
    float3 padding;
};

ConstantBuffer<ClusterRender> vizData : register(b1);

#define ClusterRenderRS \
"RootFlags ( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |" \
"            CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED )," \
"RootConstants(num32BitConstants=20, b1)" \

VSOutput VSMain(VSInput input)
{
    StructuredBuffer<ClusterGeometryData> clusterGeo = ResourceDescriptorHeap[vizData.clusterGeometryStructuredBufferSRVIndex];

    uint clusterID = input.instanceID;
	ClusterGeometryData cluster = clusterGeo[clusterID];
    
    uint triangleIdx = input.vertexID / 3;
    uint vertInTri = input.vertexID % 3;
    
    uint faceIdx = triangleIdx / 2;
    uint triInFace = triangleIdx % 2;
    uint vertIdx = cluster.indices[faceIdx][triInFace][vertInTri];
    
    float3 posVS = cluster.vertices[vertIdx];

    float4 posCS = mul(vizData.viewProj, float4(posVS, 1.0));
    
    VSOutput output;
    output.position = posCS;
    output.color = float3(1., 0., 0.);
    return output;
}

[RootSignature(ClusterRenderRS)]
float4 PSMain(VSOutput input) : SV_Target
{
    return float4(input.color, 1.0);
}