#pragma once
#include "GfxDevice.h"
#include <Shader.h>

struct RootSign;
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
    std::vector<u32> _shaderIndex{};
    BOOL _enableDepthTest;
    BOOL _enableStencilTest{};
    BOOL _isDepthPrePass = false;
};

void CompilePipelineInternal(const GfxDevice& gfxDevice, const Swapchain& swapChain, Pipeline& pipeline, Resources* resources, const PipelineDesc& pipelineDesc);
Pipeline CreatePipeline(GfxDevice& gfxDevice, Swapchain& swapChain, Resources* resources, const PipelineDesc& pipelineDesc);
void DestroyPipeline(GfxDevice& gfxDevice, Pipeline& pipeline);