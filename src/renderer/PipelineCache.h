#pragma once
#include <vector>
#include "unordered_map"

#include "Shader.h"
#include "Pipeline.h"

struct ShaderHandle
{
	u32 index : 24;
	u32 generation : 8;

	bool operator==(const ShaderHandle& other) const
	{
		return index == other.index && generation == other.generation;
	}
};

struct PipelineHandle
{
	u32 index : 24;
	u32 generation : 8;

	bool operator==(const PipelineHandle& other) const
	{
		return index == other.index && generation == other.generation;
	}
};

static constexpr ShaderHandle g_invalidShaderHandle = { 0xFFFFFF, 0xFF };
static constexpr PipelineHandle g_invalidPipelineHandle = { 0xFFFFFF, 0xFF };

struct PipelineCache
{
	PipelineCache(const GfxDevice& gfxDevice, Swapchain& swapchain);
	~PipelineCache();

	PipelineCache(const PipelineCache&) = delete;
	PipelineCache& operator=(const PipelineCache&) = delete;

	// Shader management
	[[nodiscard]] ShaderHandle LoadShader(const ShaderDesc& desc, const std::string& name = "");
	Shader* GetShader(ShaderHandle handle);
	[[nodiscard]] const Shader* GetShader(ShaderHandle handle) const;
	void UnloadShader(ShaderHandle handle);

	// Pipeline management
	[[nodiscard]] PipelineHandle CreatePipeline(const PipelineDesc& desc, const std::string& name = "");
	Pipeline* GetPipeline(PipelineHandle handle);
	[[nodiscard]] const Pipeline* GetPipeline(PipelineHandle handle) const;
	void DestroyPipeline(PipelineHandle handle);

	// Named lookups (for JSON references)
	[[nodiscard]] ShaderHandle GetShaderByName(const std::string& name) const;
	[[nodiscard]] PipelineHandle GetPipelineByName(const std::string& name) const;

	// Hot reload support
	void ReloadShader(ShaderHandle handle);
	void ReloadPipeline(PipelineHandle handle);

private:
	struct ManagedShader
	{
		Shader shader;
		ShaderHandle handle;
		std::string name;
		ShaderDesc desc;
	};

	struct ManagedPipeline
	{
		Pipeline pipeline;
		PipelineHandle handle;
		std::string name;
		PipelineDesc desc;
		std::vector<ShaderHandle> shaderHandles; // track pipeline specific shaders
	};

	const GfxDevice& gfxDevice_;
	const Swapchain& swapchain_;
	DXCRes dxcRes_;

	// shader storage
	std::vector<ManagedShader> shaders_;
	std::vector<u8> shaderGenerations_;
	std::vector<u32> shaderFreeList_;
	std::unordered_map<std::string, ShaderHandle> shaderNameMap_;

	// pipeline storage
	std::vector<ManagedPipeline> pipelines_;
	std::vector<u8> pipelineGenerations_;
	std::vector<u32> pipelineFreeList_;
	std::unordered_map<std::string, PipelineHandle> pipelineNameMap_;

	// handle shader/pipeline mgmt
	[[nodiscard]] ShaderHandle AllocateShaderHandle();
	[[nodiscard]] PipelineHandle AllocatePipelineHandle();
	[[nodiscard]] bool IsShaderHandleValid(ShaderHandle handle) const;
	[[nodiscard]] bool IsPipelineHandleValid(PipelineHandle handle) const;
};