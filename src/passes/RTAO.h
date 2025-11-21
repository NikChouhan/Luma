#pragma once
#include "GfxDevice.h"
#include "Pipeline.h"

struct Pipeline;
struct Shader;

struct RTAOPass
{
	Shader _rtaoShader;
	Pipeline _rtaoPipeline;

	void ExecuteRTAOPass(const GfxDevice& gfxDevice, FrameSync& frameSync);
};

RTAOPass InitRTAOResources(const GfxDevice& gfxDevice, FrameSync& frameSync, Swapchain& swapchain, DXCRes& dxcRes, Resources* resources);