#pragma once
#include "Graphics/GfxDevice.h"

struct CommandList
{
	explicit CommandList(const GfxDevice& gfxDevice);
	~CommandList();

	GfxDevice gfxDevice_;
	ComPtr<ID3D12GraphicsCommandList10> commandList_;
};
