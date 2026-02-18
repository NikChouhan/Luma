struct Light
{
	float3 Position;
	float  Radius;
	float3 Color;
	float  Intensity;
	uint   Type; // 0 = Point, 1 = Spot, 2 = Directional (I will only do point rn)
	float3 Direction;
};
