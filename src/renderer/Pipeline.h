#pragma once
#include "GfxDevice.h"
#include <Shader.h>

#include "RootSignature.h"

struct Swapchain;

enum class PipelineType : u8
{
	GRAPHICS,
    COMPUTE
};


struct Pipeline
{
    ComPtr<ID3D12PipelineState> _pipelineState;
    ComPtr<ID3D12RootSignature> _rootSignature;

    void Release();
};

typedef std::initializer_list<u32> ShaderIndex;

struct PipelineDesc
{
    PipelineType _pipelineType{};
    RootSignDesc::RSType _rootSignType;
    std::vector<u32> _shaderIndex{};
    BOOL _enableDepthTest;
    BOOL _enableStencilTest{};
    BOOL _isDepthPrePass = false;
};

void CompilePipelineInternal(const GfxDevice& gfxDevice, const Swapchain& swapChain, Pipeline& pipeline, Resources* resources, PipelineDesc& pipelineDesc);
Pipeline CreatePipeline(const GfxDevice& gfxDevice, Swapchain& swapChain, Resources* resources, PipelineDesc& pipelineDesc);
void DestroyPipeline(GfxDevice& gfxDevice, Pipeline& pipeline);