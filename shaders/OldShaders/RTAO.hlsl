#include "../Common/Common.hlsli"

struct RTAOParameters
{
	row_major float4x4 projMatrixInv;

	row_major float4x4 viewMatrixInv;

	uint accelerationStructureIndex;
	uint rtUavIndex;
	uint depthIndex;
	uint normalUavIndex;

	bool isEnabled;
	uint samplesPerPixel;
	uint padding;
	uint padding1;
};

SamplerState basicSampler : register(s0);
ConstantBuffer<RTAOParameters> constBuffer : register(b0);

float3 GenerateRandomHemisphereDirection(float3 normal, uint3 DTid, uint i)
{
	float seed = float(DTid.x * i);
	return RandomOnHemisphere(seed, normal);
}

[numthreads(8, 8, 1)]
void RTAOMain(uint3 DTid : SV_DispatchThreadID)
{
	RWTexture2D<float4> RTAOTextureOut = ResourceDescriptorHeap[NonUniformResourceIndex(constBuffer.rtUavIndex)];
	RWTexture2D<float4> GeometricNormalTextureOut= ResourceDescriptorHeap[NonUniformResourceIndex(constBuffer.normalUavIndex)];

	Texture2D DepthTexture = ResourceDescriptorHeap[NonUniformResourceIndex(constBuffer.depthIndex)];
	RaytracingAccelerationStructure accelerationStructure = ResourceDescriptorHeap[NonUniformResourceIndex(constBuffer.accelerationStructureIndex)];

	// generating normals live instead of sampling
	//Texture2D NormalTexture = ResourceDescriptorHeap(NonUniformResourceIndex(constBuffer.normalIndex));

	uint width, height;
	DepthTexture.GetDimensions(width, height);
	float depth = DepthTexture.Load(int3(DTid.xy, 0));
	float2 uv = (float2(DTid.xy) + 0.5) / float2(width, height);

	float3 worldPos = CalculateWorldPosFromDepth(depth, uv, constBuffer.projMatrixInv, constBuffer.viewMatrixInv);
	GeometricNormalTextureOut[DTid.xy] = float4(worldPos, 1.f);

	// sample neighbouring pixels' depth to generate accurate normals

	//float depthRight = DepthTexture.Load(int3(DTid.x + 1, DTid.y, 0));
	//float depthLeft = DepthTexture.Load(int3(DTid.x - 1, DTid.y, 0));
	//float depthDown = DepthTexture.Load(int3(DTid.x, DTid.y + 1, 0));
	//float depthUp = DepthTexture.Load(int3(DTid.x, DTid.y - 1, 0));

	//float2 uvRight = (float2(DTid.x + 1, DTid.y) + 0.5) / float2(width, height);
	//float2 uvLeft = (float2(DTid.x - 1, DTid.y) + 0.5) / float2(width, height);
	//float2 uvDown = (float2(DTid.x, DTid.y - 1) + 0.5) / float2(width, height);
	//float2 uvUp = (float2(DTid.x, DTid.y + 1) + 0.5) / float2(width, height);

	//float3 rightWorldPos = CalculateWorldPosFromDepth(depthRight, uvRight, constBuffer.projMatrixInv, constBuffer.viewMatrixInv);
	//float3 leftWorldPos = CalculateWorldPosFromDepth(depthLeft, uvLeft, constBuffer.projMatrixInv, constBuffer.viewMatrixInv);
	//float3 downWorldPos = CalculateWorldPosFromDepth(depthDown, uvDown, constBuffer.projMatrixInv, constBuffer.viewMatrixInv);
	//float3 upWorldPos = CalculateWorldPosFromDepth(depthUp, uvUp, constBuffer.projMatrixInv, constBuffer.viewMatrixInv);

	//float3 dx = rightWorldPos - leftWorldPos;  // Horizontal edge
	//float3 dy = upWorldPos - downWorldPos;

	//float3 geometricNormal = normalize(cross(dx, dy));
	//GeometricNormalTextureOut[DTid.xy] = float4(geometricNormal, 1.f);

	//if (constBuffer.isEnabled)
	//{
	//	float occlusion = 0.f;
	//	for (uint i = 0; i < constBuffer.samplesPerPixel; i++)
	//	{
	//		float3 rayDir = GenerateRandomHemisphereDirection(geometricNormal, DTid, i);

	//		RayDesc ray;
	//		ray.Origin = worldPos + geometricNormal * 0.001; // bias to avoid self-intersection
	//		ray.Direction = rayDir;
	//		ray.TMin = 0.001;
	//		ray.TMax = 0.7;

	//		RayQuery<RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH> query;
	//		RAY_FLAG flags;
	//		flags |= RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH;

	//		query.TraceRayInline(accelerationStructure, flags, 0xFF, ray);

	//		query.Proceed();

	//		if (query.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
	//		{
	//			occlusion += 1.0;
	//		}
	//	}

	//	occlusion /= float(constBuffer.samplesPerPixel);

	//	// AO = 1.0 - occlusion (1.0 = bright, 0.0 = dark)
	//	float ao = 1.0 - occlusion;

	//	RTAOTextureOut[DTid.xy] = float4(ao, ao, ao, 1.0);
	//}
}
