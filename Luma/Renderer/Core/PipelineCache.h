#pragma once
#include <vector>
#include "unordered_map"

#include "Graphics/D3D12/Shader.h"
#include "Graphics/D3D12/Pipeline.h"

struct PipelineCache
{
	PipelineCache(const GfxDevice& gfxDevice);
	~PipelineCache();

	PipelineCache(const PipelineCache&) = delete;
	PipelineCache& operator=(const PipelineCache&) = delete;

	// Shader management
	[[nodiscard]] ShaderHandle LoadShader(const ShaderDesc& desc, const std::string& name = "");
	Shader* GetShader(ShaderHandle handle);
	[[nodiscard]] const Shader* GetShader(ShaderHandle handle) const;
	void UnloadShader(ShaderHandle handle);

	// Pipeline management

	static ComPtr<ID3D12RootSignature> CreateRootSignatureFromBlob(const GfxDevice& gfxDevice, IDxcBlob* get);

	[[nodiscard]] PipelineHandle CreatePipeline(const GraphicsPipelineDesc& desc, const std::string& name);
	[[nodiscard]] PipelineHandle CreatePipeline(const ComputePipelineDesc& desc, const std::string& name);

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

		std::vector<ShaderHandle> shaderHandles;

		PipelineType type;
		GraphicsPipelineDesc graphicsDesc;
		ComputePipelineDesc computeDesc;
	};

	const GfxDevice& gfxDevice_;
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

	// internal pipeline creation helpers
	[[nodiscard]] Pipeline CreateGraphicsPSO(const GraphicsPipelineDesc& graphicsPipelineDesc);
	[[nodiscard]] Pipeline CreateComputePSO(const ComputePipelineDesc& computePipelineDesc);

	// handle shader/pipeline mgmt
	[[nodiscard]] ShaderHandle AllocateShaderHandle();
	[[nodiscard]] PipelineHandle AllocatePipelineHandle();
	[[nodiscard]] bool IsShaderHandleValid(ShaderHandle handle) const;
	[[nodiscard]] bool IsPipelineHandleValid(PipelineHandle handle) const;
};