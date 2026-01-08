struct Cluster
{
	float4 minPoint;
	float4 maxPoint;
};

// just put it into bindless
// RWStructuredBuffer<Cluster> cluster : register(u0);

struct ComputeAABBData
{
	float4x4 inverseProj;
	uint3 clusterInputData; // contains workgroup size number in x, y, z
	float zNear;

	uint2 screenDimensions;
	float zFar;
	uint clusterUAVIndex;
};

ConstantBuffer<ComputeAABBData> computeAABBData : register(b0);


float4 ScreenToView(float4 pointInSS);
float3 LineIntersectionToZPlane(float3 eyePos, float3 viewSpacePos, float zDistance);

// every workgroup processes a cluster
// TODO: using 1 thread per WG rn needs optimisation soon

#define ComputeAABB_RS \
"RootFlags ( CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED) ," \
"RootConstants(num32BitConstants=24, b0)" \

[RootSignature(ComputeAABB_RS)]
[numthreads(1,1,1)]
void ClusterMain(uint3 DtId : SV_DispatchThreadID,
				 uint3 groupID : SV_GroupID,
				 uint3 groupThreadID : SV_GroupThreadID)
{
	/* groupID : ID of workgroup (mapped as wavefront or its integer multiple inside hw)
	 * dispatchThreadID : ID of the thread in global terms wrt Dispatch call
	 * groupThreadID : ID of the thread inside the current workgroup (or local workgroup)
	 */
	const float3 eyePos = float3(0., 0., 0.);
	uint clusterSizePx = computeAABBData.screenDimensions.x/ computeAABBData.clusterInputData.x;	// for 1920x180p its 1920/16 = 120
	uint clusterIndex = groupID.x +
						groupID.y * computeAABBData.clusterInputData.x +
						groupID.z * computeAABBData.clusterInputData.x * computeAABBData.clusterInputData.y;
	// max & min point in screen space
	float4 maxPointSS = float4((groupID.xy + uint2(1, 1)) * clusterSizePx, 0., 1.);
	float4 minPointSS = float4((groupID.xy) * clusterSizePx, 0., 1.);
	// convert to view space
	float3 maxPointVS = ScreenToView(maxPointSS).xyz;
	float3 minPointVS = ScreenToView(minPointSS).xyz;
	// near/far values of the cluster in view space
	float clusterNear = computeAABBData.zNear * pow(computeAABBData.zFar / computeAABBData.zNear, groupID.z / float(computeAABBData.clusterInputData.z));
	float clusterFar = computeAABBData.zNear * pow(computeAABBData.zFar / computeAABBData.zNear, (groupID.z +1) / float(computeAABBData.clusterInputData.z));

	// find the 4 intersection points, wrt camera to the far/near plane
	float3 minPointNear = LineIntersectionToZPlane(eyePos, minPointVS, clusterNear);
	float3 minPointFar = LineIntersectionToZPlane(eyePos, minPointVS, clusterFar);
	float3 maxPointNear = LineIntersectionToZPlane(eyePos, maxPointVS, clusterNear);
	float3 maxPointFar = LineIntersectionToZPlane(eyePos, maxPointVS, clusterFar);

	float3 minPointAABB = min(min(minPointNear, minPointFar), min(maxPointNear, maxPointFar));
	float3 maxPointAABB = max(max(minPointNear, minPointFar), max(maxPointNear, maxPointFar));

	RWStructuredBuffer<Cluster> clusters = ResourceDescriptorHeap[NonUniformResourceIndex(computeAABBData.clusterUAVIndex)];
	clusters[clusterIndex].minPoint = float4(minPointAABB, 0.);
	clusters[clusterIndex].maxPoint = float4(maxPointAABB, 0.);
}

float3 LineIntersectionToZPlane(float3 eyePos, float3 viewSpacePos, float zDistance)
{
	float3 normal = float3(0.0, 0.0, 1.0);
	float3 ab = viewSpacePos - eyePos;

	float t = (zDistance - dot(normal, eyePos)) / dot(normal, ab);

	// Computing the actual xyz position of the point along the line
	float3 result = eyePos + t * ab;
	return result;
}

float4 ClipToView(float4 clip)
{
	// View space transform
	float4 view = mul(computeAABBData.inverseProj, clip);

	view = view / view.w;
	return view;
}

float4 ScreenToView(float4 pointInSS)
{
	// to NDC
	float2 texCoord = pointInSS.xy / computeAABBData.screenDimensions.xy;
	float2 ndc;
	ndc.x = texCoord.x * 2.0 - 1.0;
	ndc.y = (1.0 - texCoord.y) * 2.0 - 1.0;
	//ndc.y = (texCoord.y) * 2.0 - 1.0;

	// to clip space
	float4 clip = float4(ndc, pointInSS.z, 1.);

	return ClipToView(clip);
}
/*TODO: Should not be here, but after learning compute shaders now
 * please write a texture downsampling compute shader
 * Then a LOD compute shader
 * +
*/
