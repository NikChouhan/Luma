#include "Pipeline.h"

#include "Buffer.h"
#include "Swapchain.h"

Pipeline CreatePipeline(GfxDevice& gfxDevice, Swapchain& swapChain, PipelineDesc pipelineDesc)
{
    Pipeline pipeline{};
    // create the root signature
    /*
     TODO: root signature should be separated from the pipeline creation,
     it can be reused between multiple pipelines. Also, samplers should be per texture(?)
     and not per root signature xd
	 */
    {
        D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

        if (FAILED(gfxDevice._device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE,
            &featureData, sizeof(featureData))))
        {
            featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
        }
        // leftover of the descriptor table based heap, replaced by ResourceDescriptorHeap
        //D3D12_DESCRIPTOR_RANGE1 texture2DRange{};
        //texture2DRange.BaseShaderRegister = 0;
        //texture2DRange.NumDescriptors = MAX_TEXTURES;
        //texture2DRange.OffsetInDescriptorsFromTableStart = 0;
        //texture2DRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        //texture2DRange.RegisterSpace = 0;
        //texture2DRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;

        /*CD3DX12_DESCRIPTOR_RANGE cbvTable;
        cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);*/

        D3D12_ROOT_PARAMETER1 rootParameters[1] {};

        rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        rootParameters[0].Constants.RegisterSpace = 0;
        rootParameters[0].Constants.ShaderRegister = 0;
        rootParameters[0].Constants.Num32BitValues = sizeof(ConstBuffer)/4;

        // replaced by ResourceDescriptorHeap
        //rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        //rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        //rootParameters[1].DescriptorTable.NumDescriptorRanges = 1;
        //rootParameters[1].DescriptorTable.pDescriptorRanges = &texture2DRange;

        D3D12_STATIC_SAMPLER_DESC sampler = {};
        sampler.Filter = D3D12_FILTER_ANISOTROPIC;
        sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
        sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
        sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
        sampler.MipLODBias = 0;
        sampler.MaxAnisotropy = 8;
        sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_GREATER;
        sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
        sampler.MinLOD = 0.0f;
        sampler.MaxLOD = D3D12_FLOAT32_MAX;
        sampler.ShaderRegister = 0;
        sampler.RegisterSpace = 0;
        sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
        rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters,
            1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
            | D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED |
            D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED);

        ComPtr<ID3DBlob> signature;
        ComPtr<ID3DBlob> error;
        DX_ASSERT(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc,
            featureData.HighestVersion, &signature, &error));
        DX_ASSERT(gfxDevice._device->CreateRootSignature(0,
            signature->GetBufferPointer(), signature->GetBufferSize(),
            IID_PPV_ARGS(&pipeline._rootSignature)));
    }
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
            D3D12_SHADER_BYTECODE bytecode;
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

    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    psoDesc.RasterizerState.FrontCounterClockwise = true;
    psoDesc.DepthStencilState.DepthEnable = pipelineDesc._enableDepthTest;
    psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    psoDesc.DepthStencilState.StencilEnable = pipelineDesc._enableStencilTest;
    psoDesc.DSVFormat = swapChain._depthStencil->GetDesc().Format;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = swapChain._renderTargets[0]->GetDesc().Format;
    psoDesc.SampleDesc.Count = 1;
    DX_ASSERT(gfxDevice._device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipeline._pipelineState)));

    return pipeline;
}
