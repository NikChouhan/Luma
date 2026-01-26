#include "GenerateLights.h"

#include <random>

#include "Graphics/Globals.h"

static SM::Vector3 LerpColor(SM::Vector3 a, SM::Vector3 b, float t)
{
	return SM::Vector3{
		a.x + (b.x - a.x) * t,
		a.y + (b.y - a.y) * t,
		a.z + (b.z - a.z) * t
	};
}

std::vector<Light> GenerateLights(u32 lightCount)
{
	std::vector<Light> lights;
	lights.reserve(lightCount);

	// algorithm to generate lights within the scene bound
	// the lights will be divided into three layers, ceiling, columns, and ground

	std::random_device rd;
	std::mt19937 gen(rd());

	SM::Vector3 center = (SponzaAABB::max + SponzaAABB::min) * 0.5;
	SM::Vector3 size = (SponzaAABB::max - SponzaAABB::min);

	DirectX::BoundingBox sponzaBoundingBox(center, size);

	u32 numLayers = 3;
	u32 lightsPerLayer = lightCount / numLayers;

	for (u32 layer = 0; layer < numLayers; ++layer)
	{
		float layerMinY = SponzaAABB::min.y + (size.y / numLayers) * layer;
		float layerMaxY = SponzaAABB::min.y + (size.y / numLayers) * (layer + 1);

		std::uniform_real_distribution<float> xDist(SponzaAABB::max.x, SponzaAABB::min.x);
		std::uniform_real_distribution<float> yDist(layerMinY, layerMaxY);
		std::uniform_real_distribution<float> zDist(SponzaAABB::min.z, SponzaAABB::max.z);
		std::uniform_real_distribution<float> radiusDist(100.0f, 1000);
		std::uniform_real_distribution<float> intensityDist(80000.f, 1000000.0f);

		// TODO: can't do the following cuz no overload for SM::Vector3 :/, will prolly add it later
		// for now doing it as x,y,z is enough
		//std::uniform_real_distribution<SM::Vector3> directionDist({ 0., 0., 0. }, { 1.,1.,1. });
		std::uniform_real_distribution<float> distanceDistX(0., 24);
		std::uniform_real_distribution<float> distanceDistY(0., 8);
		std::uniform_real_distribution<float> distanceDistZ(0., 9.);

		float distanceX = distanceDistX(gen);
		float distanceY = distanceDistY(gen);
		float distanceZ = distanceDistZ(gen);

		u32 lightsThisLayer = (layer == numLayers - 1)
			? (lightCount - lights.size())
			: lightsPerLayer;

		for (u32 i = 0; i < lightsThisLayer; ++i)
		{
			Light light;
			light.Position = SM::Vector3{ xDist(gen), yDist(gen), zDist(gen) };
			light.Radius = radiusDist(gen);

			// TODO: dt based manipulation to change light color with time
			float heightFactor = static_cast<float>(layer) / numLayers;
			light.Color = LerpColor(
				SM::Vector3{ 0.,0.,1.},
				SM::Vector3{ 1.,0.,0. },
				heightFactor
			);

			light.Intensity = intensityDist(gen);
			light.Type = LightType::POINT;
			light.Direction = SM::Vector3{distanceX, distanceY, distanceZ};
			light.Direction.Normalize();

			lights.push_back(light);
		}
	}
	return lights;
}
