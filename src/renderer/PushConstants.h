#pragma once
#include "StandardTypes.h"
#include "Common.h"
struct RenderPass
{
    XMMATRIX worldViewProj;

    XMMATRIX worldMatrix;

    u32 albedoIndex;
    u32 normalIndex;
    u32 metallicRoughnessIndex;
    u32 emissiveIndex;

    u32 accelerationStructureIndex;
    SM::Vector3 dirLightDir;

    float dirLightIntensity;
    float dirLightColor[3];

    float pointLightIntensity;
    SM::Vector3 cameraPos;

    float pointLightRadius;
    float pointLightColor[3];
};

struct DepthPPBuffer
{
    XMMATRIX worldViewProj;
    XMMATRIX worldMatrix;
};

struct ShaderEffects
{
    float resolution[2];
    float time;
    float cameraYaw;

    float cameraPitch;
    u32 uavIndex;
    u32 padding1;
    u32 padding2;

    alignas(16) SM::Vector3 cameraPos;
};

struct RTAO
{
    XMMATRIX projMatrixInv;
	XMMATRIX viewMatrixInv;

    u32 accelerationStructureIndex;
    u32 rtUavIndex;
    u32 depthIndex;
    u32 normalUavIndex;

    BOOL isEnabled;
    u32 samplesPerPixel;
    u32 padding;
    u32 padding1;
};