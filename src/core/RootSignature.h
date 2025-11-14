#pragma once
#include "GfxDevice.h"

struct Pipeline;

struct RootSign
{
	ComPtr<ID3D12RootSignature> _rootSignature;
};

struct RootSignDesc
{
    enum RSType { DEPTH_PRE_PASS, RT_LIGHT, RASTER , SHADER_EFFECT};
    RSType _type;
};

RootSign CreateRootSignature(const GfxDevice& gfxDevice, RootSignDesc desc);
