#pragma once
#include "GfxDevice.h"

struct Pipeline;

struct RootSign
{
	ComPtr<ID3D12RootSignature> _rootSignature;
};

struct RootSignDesc
{
    enum class RSType { SHADER_EFFECTS, DEPTHPP, RTAO, RENDER };
    RSType _type;
};

RootSign CreateRootSignature(const GfxDevice& gfxDevice, RootSignDesc desc);
