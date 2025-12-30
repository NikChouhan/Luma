#include "../LightsCommon.hlsl"

struct Cluster
{
	float4 minPoint;
	float4 maxPoint;
};

struct LightAssignCluster
{
	uint3 clusterInputData;	// contains WG size in x,y,z dimensions
	uint  clusterUAVIndex;

	uint globalLightListTextureUAVIndex;	// 3d texture storing offset and count of lights per cluster
	uint globalLightCounterUAVIndex;	    // global light counter buffer
	uint globalLightIndexBufferUAVIndex; 	// using the offset and light counter values per cluster extract the light indices for it
											// Buffer to store actual light indices in a long buffer chain
	uint lightCount; // Total number of lights in scene

	uint globalLightsStructuredBufferSRVIndex;
	uint padding[3];
};

ConstantBuffer<LightAssignCluster> LightAssignCluster : register(b0);

#define THREADS_PER_GROUP 256

groupshared uint LocalLightIndexList[1024];
groupshared uint LocalLightCount;

bool LightSphereAABBIntersect(float3 lightPos, float lightRadius, Cluster cluster)
{
	float3 minPoint = cluster.minPoint.xyz;
	float3 maxPoint = cluster.maxPoint.xyz;

	// closest point on AABB to sphere center
	// imagine it like a kind of projection of the light spehere center on the AABB
	// not exactly (cuz no dir for projection) but kinda, if its outside 
	float3 closestPoint = clamp(lightPos, minPoint, maxPoint);

	// distance squared from sphere center to closest point
	float3 diff = lightPos - closestPoint;
	float distanceSquared = dot(diff, diff);

	return distanceSquared <= (lightRadius * lightRadius);
}

[numthreads(THREADS_PER_GROUP, 1, 1)]
void CSLightAssignCluster(uint3 DTid : SV_DispatchThreadID,
	uint3 groupID : SV_GroupID,
	uint3 groupThreadID : SV_GroupThreadID)
{
	uint threadIdx = groupThreadID.x;

	// Initialize shared memory counter (only for first thread cuz once per WG u know)
	if (threadIdx == 0)
	{
		LocalLightCount = 0;
	}

	GroupMemoryBarrierWithGroupSync();

	// cluster index
	uint clusterIndex = groupID.x +
		groupID.y * LightAssignCluster.clusterInputData.x +
		groupID.z * LightAssignCluster.clusterInputData.x * LightAssignCluster.clusterInputData.y;

	// cluster AABB, all threads in one WG read same cluster
	RWStructuredBuffer<Cluster> clusters = ResourceDescriptorHeap[NonUniformResourceIndex(LightAssignCluster.clusterUAVIndex)];
	Cluster currentCluster = clusters[clusterIndex];

	StructuredBuffer<Light> gLights = ResourceDescriptorHeap[NonUniformResourceIndex(LightAssignCluster.globalLightsStructuredBufferSRVIndex)];

	// 1 light per thread in a strided pattern
	// doing 256 lights per call, can be easily increased
	for (uint lightIdx = threadIdx; lightIdx < LightAssignCluster.lightCount; lightIdx += THREADS_PER_GROUP)
	{
		Light light = gLights[lightIdx];

		bool checkIntersect = LightSphereAABBIntersect(light.Position, light.Radius, currentCluster);

		if (checkIntersect)
		{
			// Atomically add light index to local list
			uint localIndex;
			InterlockedAdd(LocalLightCount, 1, localIndex);

			// Store in groupshared if there's space
			if (localIndex < LightAssignCluster.lightCount)
			{
				LocalLightIndexList[localIndex] = lightIdx;
			}
		}
	}

	// wait for all threads in the WG
	GroupMemoryBarrierWithGroupSync();

	// let one thread handle the copying to buffer stuff
	// parallelizing it will be detrimental, too many thread local copies of same data
	// With no useful performance gain, it will likely just overload VGPRs
	if (threadIdx == 0)
	{
		uint lightsToWrite = min(LocalLightCount, LightAssignCluster.lightCount);

		// Get global offset atomically
		RWStructuredBuffer<uint> GlobalLightCounter = ResourceDescriptorHeap[NonUniformResourceIndex(LightAssignCluster.globalLightCounterUAVIndex)];
		uint globalOffset;
		InterlockedAdd(GlobalLightCounter[0], lightsToWrite, globalOffset);

		// store offset and count in global texture (per cluster)
		// size of GlobalLightIndexList texture should be 16x9x24
		RWTexture3D<uint2> GlobalLightIndexList = ResourceDescriptorHeap[NonUniformResourceIndex(LightAssignCluster.globalLightListTextureUAVIndex)];
		GlobalLightIndexList[groupID] = uint2(globalOffset, lightsToWrite);

		// copy local light indices to global buffer
		RWStructuredBuffer<uint> GlobalLightIndexBuffer = ResourceDescriptorHeap[NonUniformResourceIndex(LightAssignCluster.globalLightIndexBufferUAVIndex)];
		for (uint i = 0; i < lightsToWrite; i++)
		{
			GlobalLightIndexBuffer[globalOffset + i] = LocalLightIndexList[i];
		}
		/* henceforth load light indices per cluster in the final frag shader, using the offset
		 * and count from the GlobalLightIndexList, perform the lights calculations and
		 *  enjoy free perf ig?? A man can dream :D
		 */
	}
}