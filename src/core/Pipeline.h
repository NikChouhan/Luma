#pragma once
#include "GfxDevice.h"
#include <Shader.h>

struct Swapchain;

struct Pipeline
{
    ComPtr<ID3D12PipelineState> _pipelineState;
    ComPtr<ID3D12RootSignature> _rootSignature;
};

struct PipelineDesc
{
    Shaders _shaders;
    BOOL _enableDepthTest;
    BOOL _enableStencilTest = FALSE;

};

Pipeline CreatePipeline(GfxDevice& gfxDevice, Swapchain& swapChain, PipelineDesc pipelineDesc);
void DestroyPipeline(GfxDevice& gfxDevice, Pipeline& pipeline);