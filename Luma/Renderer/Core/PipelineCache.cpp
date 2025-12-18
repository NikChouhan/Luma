#include "PipelineCache.h"

static D3D12_INPUT_ELEMENT_DESC kStandardInputLayout[] =
{
    {
        .SemanticName = "POSITION", .SemanticIndex = 0, .Format = DXGI_FORMAT_R32G32B32_FLOAT, .InputSlot = 0,
        .AlignedByteOffset = 0, .InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
        .InstanceDataStepRate = 0
    },
    {
        .SemanticName = "TEXCOORD", .SemanticIndex = 0, .Format = DXGI_FORMAT_R32G32_FLOAT, .InputSlot = 0,
        .AlignedByteOffset = 12, .InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
        .InstanceDataStepRate = 0
    },
    {
        .SemanticName = "NORMAL", .SemanticIndex = 0, .Format = DXGI_FORMAT_R32G32B32_FLOAT, .InputSlot = 0,
        .AlignedByteOffset = 20, .InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
        .InstanceDataStepRate = 0
    },
};

PipelineCache::PipelineCache(const GfxDevice& gfxDevice) : gfxDevice_(gfxDevice), dxcRes_(ShaderCompiler())
{

}

PipelineCache::~PipelineCache()
{
    for (auto& managed : pipelines_) {
        if (managed.pipeline.pso) {
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
    Shader* shader = &shaders_.at(handle.index).shader;
    return shader;
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

ComPtr<ID3D12RootSignature> PipelineCache::CreateRootSignatureFromBlob(const GfxDevice& gfxDevice, IDxcBlob* shaderBlob)
{
    if (!shaderBlob) return nullptr;

    ComPtr<ID3D12RootSignature> rootSignature;

    // The D3D12 runtime can parse the Root Signature directly from the compiled shader blob
    HRESULT hr = gfxDevice.device_->CreateRootSignature(
        0,                                      // NodeMask (0 for single GPU)
        shaderBlob->GetBufferPointer(),         // Pointer to the shader bytecode
        shaderBlob->GetBufferSize(),            // Size of the shader bytecode
        IID_PPV_ARGS(&rootSignature)
    );

    if (FAILED(hr))
    {
        // This usually happens if the shader was compiled WITHOUT a [RootSignature(...)] attribute
        // or if the embedded signature is invalid.
        DX_ASSERT(hr);
        return nullptr;
    }

    return rootSignature;
}

PipelineHandle PipelineCache::CreatePipeline(const GraphicsPipelineDesc& desc, const std::string& name)
{
    if (!name.empty())
    {
        auto it = pipelineNameMap_.find(name);
        if (it != pipelineNameMap_.end() && IsPipelineHandleValid(it->second))
            return it->second;
    }

    PipelineHandle handle = AllocatePipelineHandle();
    u32 index = handle.index;

    Pipeline newPipeline = CreateGraphicsPSO(desc);

    if (index >= pipelines_.size())
    {
        pipelines_.resize(index + 1);
    }

    std::vector<ShaderHandle> shaders;
    if (IsShaderHandleValid(desc.vertexShader)) shaders.push_back(desc.vertexShader);
    if (IsShaderHandleValid(desc.pixelShader)) shaders.push_back(desc.pixelShader);

    pipelines_.at(index) = ManagedPipeline{
        .pipeline = newPipeline,
        .handle = handle,
        .name = name,
        .shaderHandles = shaders,
        .type = PipelineType::GRAPHICS,
        .graphicsDesc = desc,
        .computeDesc = {} // empty
    };

    if (!name.empty()) pipelineNameMap_[name] = handle;

    return handle;
}

PipelineHandle PipelineCache::CreatePipeline(const ComputePipelineDesc& desc, const std::string& name)
{
    if (!name.empty())
    {
        auto it = pipelineNameMap_.find(name);
        if (it != pipelineNameMap_.end() && IsPipelineHandleValid(it->second))
            return it->second;
    }

    PipelineHandle handle = AllocatePipelineHandle();
    u32 index = handle.index;

    Pipeline newPipeline = CreateComputePSO(desc);

    if (index >= pipelines_.size())
    {
        pipelines_.resize(index + 1);
    }

    std::vector<ShaderHandle> shaders;
    shaders.push_back(desc.computeShader);

    pipelines_.at(index) = ManagedPipeline{
        .pipeline = newPipeline,
        .handle = handle,
        .name = name,
        .shaderHandles = shaders,
        .type = PipelineType::COMPUTE,
        .graphicsDesc = {}, // empty
        .computeDesc = desc
    };

    if (!name.empty()) pipelineNameMap_[name] = handle;

    return handle;
}

Pipeline PipelineCache::CreateGraphicsPSO(const GraphicsPipelineDesc& desc)
{
    Pipeline pipeline;

    Shader* vs = this->GetShader(desc.vertexShader);
    Shader* ps = this->GetShader(desc.pixelShader);
    assert(vs && "Vertex Shader is mandatory");

    /* TODO: Currently I am only passing VS blob cuz my shaders will have a common root signature
     * declared at the top. Both will share it
     * Guess I was wrong. If Pixel shader has the [RootSignature[<define>]] part at the top of its
     * definition, vs blob cant have access to it. Rather define it at the top of both definitions
     * OR
     * Always define it above ps def, and once its reflected to c++ code, it will be used
     * nonetheless, and if ps doesn't exist (depth pre pass for ex) define it above vs def
	*/
    if (ps) pipeline.rootSign = CreateRootSignatureFromBlob(gfxDevice_, ps->pBlob.Get());
    else pipeline.rootSign = CreateRootSignatureFromBlob(gfxDevice_, vs->pBlob.Get());

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.pRootSignature = pipeline.rootSign.Get();

    psoDesc.VS = CD3DX12_SHADER_BYTECODE({ vs->pBlob->GetBufferPointer(), vs->pBlob->GetBufferSize()});
    if (ps) psoDesc.PS = CD3DX12_SHADER_BYTECODE(ps->pBlob->GetBufferPointer(), ps->pBlob->GetBufferSize());

    if (desc.inputLayout.empty()) {
        psoDesc.InputLayout = {.pInputElementDescs = kStandardInputLayout, .NumElements = _countof(kStandardInputLayout) };
    }
    else {
        psoDesc.InputLayout = { desc.inputLayout.data(), (UINT)desc.inputLayout.size() };
    }

    // this is the opaque blend state i.e default
    auto& blend = psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    if (desc.blendMode == BlendMode::ALPHA_BLEND) {
        blend.RenderTarget[0].BlendEnable = TRUE;
        blend.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
        blend.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
        blend.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    }
    // TODO: handle other blend states, don't need it for now so leaving it empty

    // Rasterizer State
    auto& rast = psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    if (desc.rasterMode == RasterMode::SOLID_NONE_CULL) rast.CullMode = D3D12_CULL_MODE_NONE;
    if (desc.rasterMode == RasterMode::WIREFRAME) rast.FillMode = D3D12_FILL_MODE_WIREFRAME;
    rast.FrontCounterClockwise = TRUE;

    // Depth Stencil
    auto& depth = psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    if (desc.depthMode == DepthMode::NONE) {
        depth.DepthEnable = FALSE;
    }
    else if (desc.depthMode == DepthMode::READ_ONLY) 
    {
        depth.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    }

    // Topology
    switch (desc.topology)
	{
        case Topology::TRIANGLES: psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; pipeline.primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST; break;
        case Topology::LINES:     psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;     pipeline.primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_LINELIST;     break;
        case Topology::POINTS:    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;    pipeline.primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_POINTLIST;    break;
    }

    // Formats
    if (desc.rtvFormat == DXGI_FORMAT_UNKNOWN)
    {
        psoDesc.NumRenderTargets = 0;
    }
    else
    {
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = desc.rtvFormat;
    }
    psoDesc.DSVFormat = desc.dsvFormat;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.SampleDesc.Count = 1;

    DX_ASSERT(gfxDevice_.device_->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipeline.pso)));

    return pipeline;
}

Pipeline PipelineCache::CreateComputePSO(const ComputePipelineDesc& desc)
{
    Pipeline pipeline;

    Shader* cs = this->GetShader(desc.computeShader);
    pipeline.rootSign = CreateRootSignatureFromBlob(gfxDevice_, cs->pBlob.Get());

    D3D12_COMPUTE_PIPELINE_STATE_DESC csoDesc{};
	csoDesc.pRootSignature = pipeline.rootSign.Get();

    D3D12_SHADER_BYTECODE cShaderBytecode{};
    cShaderBytecode.BytecodeLength = cs->pBlob->GetBufferSize();
    cShaderBytecode.pShaderBytecode = cs->pBlob->GetBufferPointer();
    csoDesc.CS = cShaderBytecode;

    csoDesc.NodeMask = 0;

    DX_ASSERT(gfxDevice_.device_->CreateComputePipelineState(&csoDesc, IID_PPV_ARGS(&pipeline.pso)));

    return pipeline;
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

void PipelineCache::ReloadPipeline(PipelineHandle handle)
{
    if (!IsPipelineHandleValid(handle)) {
        return;
    }

    auto& managed = pipelines_[handle.index];

    managed.pipeline.Release();

    // Switch on type to rebuild
    if (managed.type == PipelineType::GRAPHICS)
    {
        managed.pipeline = CreateGraphicsPSO(managed.graphicsDesc);
    }
    else if (managed.type == PipelineType::COMPUTE)
    {
        managed.pipeline = CreateComputePSO(managed.computeDesc);
    }
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