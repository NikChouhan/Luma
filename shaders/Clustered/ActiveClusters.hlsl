#include "../Common.hlsl"

/*
 *The pixel shader for this pass is very simple. First, the index of the
volume tile is computed from the pixel's screen space position and the view
space depth. Then the volume tile for the pixel is marked as active. To mark
the current volume tile as active, a list of boolean flags is updated in the
pixel shader. Each entry of the list represents one volume tile in the grid.
*/

struct ActiveCluster
{
    row_major float4x4 worldViewProjMatrix;

    row_major float4x4 worldMatrix;
};

ConstantBuffer<ActiveCluster> activeCluster : register(b0);


uint CalculateClusterIndex(float3 pixelSSposition, float depth)
{
	
}

void CSMarkActiveCluster(uint3 DTid : SV_DispatchThreadID, uint3 groupID : SV_GroupID)
{
	
}