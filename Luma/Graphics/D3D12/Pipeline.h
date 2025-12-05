#pragma once
#include "Graphics/GfxDevice.h"

#include "RootSignature.h"

struct Swapchain;

enum class PipelineType : u8
{
	GRAPHICS,
    COMPUTE
};

typedef std::initializer_list<u32> Shaders;
struct Pipeline
{
    ComPtr<ID3D12PipelineState> pipelineState;
    ComPtr<ID3D12RootSignature> rootSignature;

    void Release();
};


struct PipelineDesc
{
    PipelineType pipelineType{};
    RootSignDesc::RSType rootSignType;
    Shaders shaderIndex;
    BOOL enableDepthTest;
    BOOL enableStencilTest{};
    BOOL isDepthPrePass = false;
};

void CompilePipelineInternal(const GfxDevice& gfxDevice, const Swapchain& swapChain, Pipeline& pipeline, const PipelineDesc& pipelineDesc);
Pipeline CreatePipeline(const GfxDevice& gfxDevice, Swapchain& swapChain, PipelineDesc& pipelineDesc);
void DestroyPipeline(GfxDevice& gfxDevice, Pipeline& pipeline);