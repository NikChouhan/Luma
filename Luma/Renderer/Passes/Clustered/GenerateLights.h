#pragma once
#include "Graphics/GfxDevice.h"

enum class LightType: u32 {POINT, DIRECTION};


struct Light
{
    SM::Vector3 Position;
    float Radius;

    SM::Vector3 Color;
    float Intensity;

	SM::Vector3 Direction;
    LightType Type;
};

std::vector<Light> GenerateLights(u32 lightCount);