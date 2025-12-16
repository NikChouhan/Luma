#pragma once

#include "Graphics/GfxDevice.h"
#include "Renderer/Core/Resources.h"

using RenderGraphNodeHandle = u32;

struct PipelineCache;

// input/output of a node
struct RenderGraphResource
{
	ResourceHandle resourceHandle;
	u32 refCount = 0;

	const char* name = nullptr;
};

struct RenderPassDesc
{
	std::vector<RenderGraphResource> inputs;
	std::vector<RenderGraphResource> outputs;
};

struct RenderGraph
{
	void DeclareResource(const std::string name, ResourceHandle);
	void AddPass(RenderPassDesc renderPassDesc);
};

struct RenderGraphBuilder
{
	ResourceManager* resourceManager;
	PipelineCache* pipelineCache;
	RenderGraphBuilder(ResourceManager* resourceManager_, PipelineCache* pipelineCache_)
		: resourceManager(resourceManager_), pipelineCache(pipelineCache_) {}

};