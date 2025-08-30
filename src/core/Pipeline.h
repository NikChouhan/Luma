#pragma once
#include "GfxDevice.h"
#include <Shader.h>

struct Pipeline
{
    ComPtr<ID3D12PipelineState> _pipelineState;
    ComPtr<ID3D12RootSignature> _rootSignature;
};

struct PipelineDesc
{
    Shaders _shaders;
    bool _enableDepthTest;
    bool _enableStencilTest;
};

Pipeline CreatePipeline(GfxDevice& gfxDevice, PipelineDesc pipelineDesc);
void DestroyPipeline(GfxDevice& gfxDevice, Pipeline& pipeline);