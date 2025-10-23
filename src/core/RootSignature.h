#pragma once
#include "GfxDevice.h"

struct Pipeline;

struct RootSign
{
	ComPtr<ID3D12RootSignature> _rootSignature;
};

struct RootSignDesc
{
	bool _isDepthPrePass = false;
};

RootSign CreateRootSignature(GfxDevice& gfxDevice, RootSignDesc desc);
