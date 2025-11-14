#include "RootSignature.h"

#include "Buffer.h"
#include "Pipeline.h"

RootSign CreateRootSignature(const GfxDevice& gfxDevice, RootSignDesc desc)
{
	RootSign rootSign{};

    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
    featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

    if (FAILED(gfxDevice._device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE,
        &featureData, sizeof(featureData))))
    {
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    if (desc._type == RootSignDesc::RASTER)
    {

        D3D12_ROOT_PARAMETER1 rootParameters[1]{};

        rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        rootParameters[0].Constants.RegisterSpace = 0;
        rootParameters[0].Constants.ShaderRegister = 0;
        rootParameters[0].Constants.Num32BitValues = sizeof(ConstBuffer) / 4;

        D3D12_TEXTURE_ADDRESS_MODE mode = D3D12_TEXTURE_ADDRESS_MODE_WRAP;

        D3D12_STATIC_SAMPLER_DESC samplers[2] = {};
        samplers[0].Filter = D3D12_FILTER_ANISOTROPIC;
        samplers[0].AddressU = mode;
        samplers[0].AddressV = mode;
        samplers[0].AddressW = mode;
        samplers[0].MipLODBias = 0;
        samplers[0].MaxAnisotropy = 16;
        samplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
        samplers[0].BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
        samplers[0].MinLOD = 0.0f;
        samplers[0].MaxLOD = D3D12_FLOAT32_MAX;
        samplers[0].ShaderRegister = 0;
        samplers[0].RegisterSpace = 0;
        samplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        // normal map sampling
        samplers[1].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        samplers[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        samplers[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        samplers[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        samplers[1].MipLODBias = 0.0f;
        samplers[1].MaxAnisotropy = 16;
        samplers[1].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
        samplers[1].BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
        samplers[1].MinLOD = 0.0f;
        samplers[1].MaxLOD = D3D12_FLOAT32_MAX;
        samplers[1].ShaderRegister = 1;
        samplers[1].RegisterSpace = 0;
        samplers[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
        rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters,
            _countof(samplers), samplers, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
            | D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED /*|
            D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED*/ /* for samplers set with heaps. I
            am baking it inside the root descriptors, so I don't need it*/);

        ComPtr<ID3DBlob> signature;
        ComPtr<ID3DBlob> error;
        DX_ASSERT(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc,
            featureData.HighestVersion, &signature, &error));
        DX_ASSERT(gfxDevice._device->CreateRootSignature(0,
            signature->GetBufferPointer(), signature->GetBufferSize(),
            IID_PPV_ARGS(&rootSign._rootSignature)));
    }
    else if (desc._type == RootSignDesc::DEPTH_PRE_PASS)
    {
        D3D12_ROOT_PARAMETER1 rootParameters[1]{};

        rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        rootParameters[0].Constants.RegisterSpace = 0;
        rootParameters[0].Constants.ShaderRegister = 0;
        rootParameters[0].Constants.Num32BitValues = sizeof(DepthPPBuffer) / 4;

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
        rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters,
            0,nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        ComPtr<ID3DBlob> signature;
        ComPtr<ID3DBlob> error;
        DX_ASSERT(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc,
            featureData.HighestVersion, &signature, &error));
        DX_ASSERT(gfxDevice._device->CreateRootSignature(0,
            signature->GetBufferPointer(), signature->GetBufferSize(),
            IID_PPV_ARGS(&rootSign._rootSignature)));
    }
    else if (desc._type == RootSignDesc::SHADER_EFFECT)
    {
        // set the descriptors for the ray tracing pipeline
        D3D12_ROOT_PARAMETER1 rootParameters[1]{};

        rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        rootParameters[0].Constants.RegisterSpace = 0;
        rootParameters[0].Constants.ShaderRegister = 0;
        rootParameters[0].Constants.Num32BitValues = sizeof(ShaderEffects) / 4;

        D3D12_STATIC_SAMPLER_DESC samplers[1] = {};
        samplers[0].Filter = D3D12_FILTER_ANISOTROPIC;
        samplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
        samplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
        samplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
        samplers[0].MipLODBias = 0;
        samplers[0].MaxAnisotropy = 16;
        samplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
        samplers[0].BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
        samplers[0].MinLOD = 0.0f;
        samplers[0].MaxLOD = D3D12_FLOAT32_MAX;
        samplers[0].ShaderRegister = 0;
        samplers[0].RegisterSpace = 0;
        samplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
        rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters,
            1, samplers,
            D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED);

        ComPtr<ID3DBlob> signature;
        ComPtr<ID3DBlob> error;
        DX_ASSERT(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc,
            featureData.HighestVersion, &signature, &error));
        DX_ASSERT(gfxDevice._device->CreateRootSignature(0,
            signature->GetBufferPointer(), signature->GetBufferSize(),
            IID_PPV_ARGS(&rootSign._rootSignature)));
    }

    return rootSign;
}
