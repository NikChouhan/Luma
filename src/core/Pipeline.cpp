#include "Pipeline.h"

#include "Buffer.h"
#include "Swapchain.h"

#include "RootSignature.h"

Pipeline CreatePipeline(GfxDevice& gfxDevice, Swapchain& swapChain, PipelineDesc pipelineDesc)
{
    Pipeline pipeline{};
    // create the root signature
    /*
     TODO: root signature should be separated from the pipeline creation,
     it can be reused between multiple pipelines. Also, samplers should be per texture(?)
     and not per root signature xd
	 */
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
		}
    };

    pipeline._rootSignature = CreateRootSignature(gfxDevice, { ._isDepthPrePass = pipelineDesc._isDepthPrePass })._rootSignature;

    // Describe and create the PSO
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { .pInputElementDescs = inputElementDescs, .NumElements = _countof(inputElementDescs) };
    psoDesc.pRootSignature = pipeline._rootSignature.Get();

    for (const Shader& shader : pipelineDesc._shaders)
    {
        if (shader._type == Type::VERTEX)
        {
            D3D12_SHADER_BYTECODE bytecode{};
            bytecode.BytecodeLength = shader._pBlob->GetBufferSize();
            bytecode.pShaderBytecode = shader._pBlob->GetBufferPointer();
            psoDesc.VS = bytecode;
        }
        else if (shader._type == Type::PIXEL)
        {
            D3D12_SHADER_BYTECODE bytecode{};
            bytecode.BytecodeLength = shader._pBlob->GetBufferSize();
            bytecode.pShaderBytecode = shader._pBlob->GetBufferPointer();
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

    if (pipelineDesc._enableRasterizer == TRUE)
    {
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = swapChain._renderTargets[0]->GetDesc().Format;
    }

    else
    {
        psoDesc.NumRenderTargets = 0;
    }

    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    psoDesc.RasterizerState.FrontCounterClockwise = true;
    psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;

    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC1(D3D12_DEFAULT);
    psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    psoDesc.DepthStencilState.DepthEnable = pipelineDesc._enableDepthTest;
    psoDesc.DepthStencilState.StencilEnable = pipelineDesc._enableStencilTest;
    psoDesc.DSVFormat = swapChain._depthStencil->GetDesc().Format;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.SampleDesc.Count = 1;
    DX_ASSERT(gfxDevice._device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipeline._pipelineState)));

    return pipeline;
}
