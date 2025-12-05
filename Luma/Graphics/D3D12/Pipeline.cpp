#include "Graphics/D3D12/Pipeline.h"

#include "Graphics/D3D12/Buffer.h"
#include "Renderer/Resources.h"
#include "Graphics/D3D12/Swapchain.h"

#include "Graphics/D3D12/RootSignature.h"
#include "Graphics/D3D12/Shader.h"

void Pipeline::Release()
{
    if (pipelineState) {
        pipelineState.Reset();
    }
    if (rootSignature) {
        rootSignature.Reset();
    }
}

void CompilePipelineInternal(const GfxDevice& gfxDevice,const Swapchain& swapChain, Pipeline& pipeline, const PipelineDesc& pipelineDesc)
{
    pipeline.rootSignature = CreateRootSignature(gfxDevice, { ._type = pipelineDesc.rootSignType })._rootSignature;

    if (pipelineDesc.pipelineType == PipelineType::GRAPHICS)
    {
        // define the vertex input layout
        D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
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

        // Describe and create the PSO
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.InputLayout = { .pInputElementDescs = inputElementDescs, .NumElements = _countof(inputElementDescs) };
        psoDesc.pRootSignature = pipeline.rootSignature.Get();

        for (const u32 shaderIndex : pipelineDesc.shaderIndex)
        {
            Shader shader = resources->shaders[shaderIndex];
            if (shader.type == Type::VERTEX)
            {
                D3D12_SHADER_BYTECODE bytecode{};
                bytecode.BytecodeLength = shader.pBlob->GetBufferSize();
                bytecode.pShaderBytecode = shader.pBlob->GetBufferPointer();
                psoDesc.VS = bytecode;
            }
            else if (shader.type == Type::PIXEL)
            {
                D3D12_SHADER_BYTECODE bytecode{};
                bytecode.BytecodeLength = shader.pBlob->GetBufferSize();
                bytecode.pShaderBytecode = shader.pBlob->GetBufferPointer();
                psoDesc.PS = bytecode;
            }
        }

        /* D3D12_RASTERIZER_DESC rasterizer
         {
             .FillMode = D3D12_FILL_MODE_SOLID,
             .CullMode = D3D12_CULL_MODE_NONE,
             .FrontCounterClockwise = TRUE,
             .DepthBias = FALSE,
             .DepthBiasClamp = 0,
             .SlopeScaledDepthBias = 0,
             .DepthClipEnable = TRUE,
             .MultisampleEnable = FALSE,
             .AntialiasedLineEnable = TRUE,
             .ForcedSampleCount = 0,
             .ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF
         };*/
         //  Will be replacing with proper resources provided directly
         //  in the CreatePipeline call. For now, this should work
        if (pipelineDesc.isDepthPrePass == TRUE)
        {
            psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC1(D3D12_DEFAULT);
            psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
            psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;

            psoDesc.NumRenderTargets = 0;
        }
        else
        {
            psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC1(D3D12_DEFAULT);
            psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_EQUAL;
            psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

            psoDesc.NumRenderTargets = 1;
            psoDesc.RTVFormats[0] = swapChain._renderTargets[0]->GetDesc().Format;
        }

        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
        psoDesc.RasterizerState.FrontCounterClockwise = true;
        psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;

        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);


        psoDesc.DepthStencilState.DepthEnable = pipelineDesc.enableDepthTest;
        psoDesc.DepthStencilState.StencilEnable = pipelineDesc.enableStencilTest;
        psoDesc.DSVFormat = swapChain._depthStencil->GetDesc().Format;
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.SampleDesc.Count = 1;
        DX_ASSERT(gfxDevice.device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipeline.pipelineState)));
    }

    if (pipelineDesc.pipelineType == PipelineType::COMPUTE)
    {
        D3D12_COMPUTE_PIPELINE_STATE_DESC csoDesc{};
        /*assert(pipelineDesc._shaders.begin()->_type == Type::COMPUTE && pipelineDesc._shaders.size() == 1);*/

        for (const u32 shaderIndex : pipelineDesc.shaderIndex)
        {
            Shader shader = resources->shaders[shaderIndex];

            D3D12_SHADER_BYTECODE cShaderBytecode{};
            cShaderBytecode.BytecodeLength = shader.pBlob->GetBufferSize();
            cShaderBytecode.pShaderBytecode = shader.pBlob->GetBufferPointer();
            csoDesc.CS = cShaderBytecode;
        }
        csoDesc.NodeMask = 0;
        csoDesc.pRootSignature = pipeline.rootSignature.Get();
        DX_ASSERT(gfxDevice.device->CreateComputePipelineState(&csoDesc, IID_PPV_ARGS(&pipeline.pipelineState)));
    }
}

Pipeline CreatePipeline(const GfxDevice& gfxDevice, Swapchain& swapChain, PipelineDesc& pipelineDesc)
{
    Pipeline pipeline{};

    CompilePipelineInternal(gfxDevice, swapChain, pipeline, pipelineDesc);

    resources->pipelines.push_back(pipeline);
    resources->pipelineParams.push_back(pipelineDesc);
    return pipeline;
}
