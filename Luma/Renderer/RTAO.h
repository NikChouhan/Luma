#pragma once
#include "Graphics/GfxDevice.h"
#include "Graphics/D3D12/Pipeline.h"
#include "Graphics/D3D12/Shader.h"

struct RTAOPass
{
	Shader _rtaoShader;
	Pipeline _rtaoPipeline;

	void ExecuteRTAOPass(const GfxDevice& gfxDevice, FrameSync& frameSync);
};

RTAOPass InitRTAOResources(const GfxDevice& gfxDevice, FrameSync& frameSync, Swapchain& swapchain, DXCRes& dxcRes, Resources* resources);