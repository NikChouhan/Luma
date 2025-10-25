#pragma once
#include "GfxDevice.h"
#include <Shader.h>

struct RootSign;
struct Swapchain;

enum PipelineType : u8
{
	GRAPHICS,
    COMPUTE
};


struct Pipeline
{
    ComPtr<ID3D12PipelineState> _pipelineState;
    ComPtr<ID3D12RootSignature> _rootSignature;
};

struct PipelineDesc
{
    PipelineType _pipelineType{};
    Shaders _shaders;
    BOOL _enableDepthTest;
    BOOL _enableStencilTest{};
    BOOL _isDepthPrePass = false;
};

Pipeline CreatePipeline(GfxDevice& gfxDevice, Swapchain& swapChain, PipelineDesc pipelineDesc);
void DestroyPipeline(GfxDevice& gfxDevice, Pipeline& pipeline);