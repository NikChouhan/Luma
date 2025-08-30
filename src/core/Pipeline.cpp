#include "Pipeline.h"

Pipeline CreatePipeline(GfxDevice& gfxDevice, PipelineDesc pipelineDesc)
{
    Pipeline pipeline{};
    // create an empty root signature
    {
        CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
        rootSignatureDesc.Init(0, nullptr, 0, nullptr, 
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        ComPtr<ID3DBlob> signature;
        ComPtr<ID3DBlob> error;
        DX_ASSERT(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1,
            &signature, &error));
        DX_ASSERT(
            gfxDevice._device->CreateRootSignature(0, signature->GetBufferPointer(), 
                signature->GetBufferSize(),
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
            .SemanticName = "COLOR", .SemanticIndex = 0, .Format = DXGI_FORMAT_R32G32B32A32_FLOAT, .InputSlot = 0,
            .AlignedByteOffset = 12, .InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
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
            psoDesc.VS = CD3DX12_SHADER_BYTECODE(shader._pBlob.Get());
        }
        else if (shader._type == Type::PIXEL)
        {
            psoDesc.PS = CD3DX12_SHADER_BYTECODE(shader._pBlob.Get());
        }
    }
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.StencilEnable = FALSE;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.SampleDesc.Count = 1;
    DX_ASSERT(gfxDevice._device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipeline._pipelineState)));

    return pipeline;
}