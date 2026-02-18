float length2(float2 v)
{
	return dot(v, v);
}
float length2(float3 v)
{
	return dot(v, v);
}
float length2(float4 v)
{
	return dot(v, v);
}

float3 CalculateWorldPosFromDepth(float depth, float2 uv, float4x4 invViewProj)
{
	float2 ndc;
	ndc.x = uv.x * 2.0 - 1.0;
	ndc.y = (uv.y) * 2.0 - 1.0;

	// Convert depth to NDC
	float z = depth * 2.0 - 1.;

	float4 clipSpacePosition = float4(ndc, z, 1.0);

	float4 worldSpacePosition = mul(invViewProj, clipSpacePosition);
	worldSpacePosition.xyz /= worldSpacePosition.w;
	return worldSpacePosition.xyz;
}
// taken from UE 5.6
uint StrongIntegerHash(uint x)
{
	// From https://github.com/skeeto/hash-prospector
	// bias = 0.16540778981744320
	x ^= x >> 16;
	x *= 0xa812d533;
	x ^= x >> 15;
	x *= 0xb278e4ad;
	x ^= x >> 17;
	return x;
}

float Rand(inout uint Seed)
{
	// Counter based PRNG -- safer than most small-state PRNGs since we use the random values directly here.
	Seed += 1;
	uint Output = StrongIntegerHash(Seed);
	// take low 24 bits
	return (Output & 0xFFFFFF) * 5.96046447754e-08; // * 2^-24
}
//
float3 RandomInUnitSphere(inout float seed)
{
	float3 p;
	do {
		p = float3(Rand(seed) * 2.0f - 1.0f, Rand(seed) * 2.0f - 1.0f, Rand(seed) * 2.0f - 1.0f);
	} while (dot(p, p) >= 1.0f); // Rejection sampling to ensure it's within the unit sphere
	return p;
}

float3 RandomOnHemisphere(inout float seed, float3 normal)
{
	float3 randomDir = RandomInUnitSphere(seed);
	// Ensure the direction is in the same hemisphere as the norm(al)
	if (dot(randomDir, normal) < 0.0f) 
	{
		randomDir = -randomDir;
	}
	return normalize(randomDir);
}

#define UINT_MAX_VALUE 4294967295
