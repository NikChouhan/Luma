#include "PipelineCache.h"

PipelineCache::PipelineCache(const GfxDevice& gfxDevice, Swapchain& swapchain) : gfxDevice_(gfxDevice), swapchain_(swapchain), dxcRes_(ShaderCompiler())
{

}

PipelineCache::~PipelineCache()
{
    for (auto& managed : pipelines_) {
        if (managed.pipeline.pipelineState) {
            managed.pipeline.Release();
        }
    }

    
    for (auto& managed : shaders_) {
        if (managed.shader.pBlob) {
            managed.shader.Release();
        }
    }
}

ShaderHandle PipelineCache::LoadShader(const ShaderDesc& desc, const std::string& name)
{
    ShaderHandle handle = AllocateShaderHandle();

    u32 index = handle.index;

    Shader shader{};
    CompileShaderInternal(gfxDevice_, dxcRes_, shader, desc);
    shader.index = handle.index;

    if (index >= shaders_.size())
        shaders_.resize(index + 1);

    shaders_.at(index) = ManagedShader{
        .shader = shader,
        .handle = handle,
        .name = name,
        .desc = desc
    };

    return handle;
}

Shader* PipelineCache::GetShader(ShaderHandle handle)
{
    if (!IsShaderHandleValid(handle))
        return nullptr;
    Shader shader = shaders_.at(handle.index).shader;
    return &shader;
}

const Shader* PipelineCache::GetShader(ShaderHandle handle) const
{
    if (!IsShaderHandleValid(handle))
        return nullptr;
    Shader shader = shaders_.at(handle.index).shader;
    return &shader;
}

void PipelineCache::UnloadShader(ShaderHandle handle)
{
    if (!IsShaderHandleValid(handle))
        return;
    u32 index = handle.index;

    shaderGenerations_.at(index)++;
    shaderFreeList_.push_back(index);
    shaders_.at(index) = ManagedShader{};
}

PipelineHandle PipelineCache::CreatePipeline(const PipelineDesc& desc, const std::string& name)
{
    if (!pipelineNameMap_.empty())
    {
        auto it = pipelineNameMap_.find(name);
        if (it != pipelineNameMap_.end() && IsPipelineHandleValid(it->second))
            return it->second;
    }

    PipelineHandle handle = AllocatePipelineHandle();
    u32 index = handle.index;

    std::vector<ShaderHandle> shaderHandles;
    for (auto shaderHandle : GetShaderByName(name))
    {
        shaderHandles.push_back(shaderHandle);
    }

    Pipeline pipeline{};
    CompilePipelineInternal(this, gfxDevice_, swapchain_, pipeline, desc);

    if (index >= pipelines_.size())
    {
        pipelines_.resize(index + 1);
    }

    pipelines_.at(index) = ManagedPipeline{
	    .pipeline = pipeline,
	    .handle = handle,
	    .name = name,
	    .desc = desc,
	    .shaderHandles = shaderHandles
    };

    if (!name.empty()) 
    {
        pipelineNameMap_[name] = handle;
    }

    return handle;
}

Pipeline* PipelineCache::GetPipeline(PipelineHandle handle)
{
    if (!IsPipelineHandleValid(handle))
    {
        return nullptr;
    }
    return &pipelines_.at(handle.index).pipeline;
}

const Pipeline* PipelineCache::GetPipeline(PipelineHandle handle) const
{
    if (!IsPipelineHandleValid(handle))
    {
        return nullptr;
    }
    return &pipelines_.at(handle.index).pipeline;
}

void PipelineCache::DestroyPipeline(PipelineHandle handle)
{
    if (!IsPipelineHandleValid(handle))
        return;
    u32 index = handle.index;

    pipelineGenerations_.at(index)++;
    pipelineFreeList_.push_back(index);
    pipelines_.at(index) = ManagedPipeline{};
}

ShaderHandle PipelineCache::GetShaderByName(const std::string& name) const
{
    if (!name.empty())
    {
        auto it = shaderNameMap_.find(name);
        if (it != shaderNameMap_.end() && IsShaderHandleValid(it->second))
            return it->second;
    }
    return g_invalidShaderHandle;
}

PipelineHandle PipelineCache::GetPipelineByName(const std::string& name) const
{
    auto it = pipelineNameMap_.find(name);
    if (it != pipelineNameMap_.end() && IsPipelineHandleValid(it->second))
        return it->second;
    return g_invalidPipelineHandle;
}

void PipelineCache::ReloadShader(ShaderHandle handle)
{
    if (!IsShaderHandleValid(handle))
    {
        return;
    }

    auto& managed = shaders_.at(handle.index);
    managed.shader.Release();

    CompileShaderInternal(gfxDevice_, dxcRes_, managed.shader, managed.desc);

    // rebuild dependent pipelines
    for (auto& pipelineManaged : pipelines_) {
        for (ShaderHandle sh : pipelineManaged.shaderHandles) {
            if (sh == handle) {
                ReloadPipeline(pipelineManaged.handle);
                break;
            }
        }
    }
}

void PipelineCache::ReloadPipeline(PipelineHandle handle) {
    if (!IsPipelineHandleValid(handle)) {
        return;
    }

    auto& managed = pipelines_[handle.index];

    managed.pipeline.Release();
    CompilePipelineInternal(gfxDevice_, swapchain_, managed.pipeline, managed.desc);
}

ShaderHandle PipelineCache::AllocateShaderHandle()
{
    u32 index{};
    if (!shaderFreeList_.empty())
    {
	    index = shaderFreeList_.back();
        shaderGenerations_.at(index)++;
        shaderFreeList_.pop_back();
    }
    else
    {
        index = shaderGenerations_.size();
        shaderGenerations_.push_back(0);
    }
    return ShaderHandle{ index, shaderGenerations_.at(index) };
}

PipelineHandle PipelineCache::AllocatePipelineHandle()
{
    u32 index{};
    if (!pipelineFreeList_.empty())
    {
        index = pipelineFreeList_.back();
        pipelineGenerations_.at(index)++;
        pipelineFreeList_.pop_back();
    }
    else
    {
        index = pipelineGenerations_.size();
        pipelineGenerations_.push_back(0);
    }
    return PipelineHandle{ index, pipelineGenerations_.at(index) };
}

bool PipelineCache::IsShaderHandleValid(ShaderHandle handle) const
{
    return handle.index < shaderGenerations_.size() &&
        shaderGenerations_[handle.index] == handle.generation;
}

bool PipelineCache::IsPipelineHandleValid(PipelineHandle handle) const
{
    return handle.index < pipelineGenerations_.size() &&
        pipelineGenerations_[handle.index] == handle.generation;
}