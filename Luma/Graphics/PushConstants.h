#pragma once
#include "StandardTypes.h"
#include "Core/Common.h"

struct RTAO
{
	DirectX::XMMATRIX projMatrixInv;
    DirectX::XMMATRIX viewMatrixInv;

    u32 accelerationStructureIndex;
    u32 rtUavIndex;
    u32 depthIndex;
    u32 normalUavIndex;

    BOOL isEnabled;
    u32 samplesPerPixel;
    u32 padding;
    u32 padding1;
};