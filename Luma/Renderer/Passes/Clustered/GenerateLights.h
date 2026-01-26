#pragma once
#include <vector>

#include "Core/Common.h"
#include "StandardTypes.h"

enum class LightType: u32 {POINT, DIRECTION};

struct Light
{
    SM::Vector3 Position;
    float Radius;

    DirectX::SimpleMath::Vector3 Color;
    float Intensity;

	SM::Vector3 Direction;
    LightType Type;
};

std::vector<Light> GenerateLights(u32 lightCount);