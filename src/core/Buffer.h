#pragma once

#include "Common.h"
#include "GfxDevice.h"

typedef u32 Index;
using namespace DirectX;

namespace D3D12MA
{
	class Allocation;
}


enum class BufferType
{
	VERTEX,
    INDEX,
    UAV
};

struct ConstBuffer
{
    XMMATRIX _worldViewProj;

    XMMATRIX _worldMatrix;

    u32 _albedoIndex;
    u32 _normalIndex;
    u32 _metallicRoughnessIndex;
    u32 _emissiveIndex;

    u32 _accelerationStructureIndex;
    SM::Vector3 _dirLightDir;

    float _dirLightIntensity;
    float _dirLightColor[3];

    float _pointLightIntensity;
    SM::Vector3 _cameraPos;

    float _pointLightRadius;
    float _pointLightColor[3];
};

struct DepthPPBuffer
{
    XMMATRIX _worldViewProj;
    XMMATRIX _worldMatrix;
};

struct ShaderEffects
{
    float _resolution[2];
    float _time;
    float _cameraYaw;

    float _cameraPitch;
    u32 _uavIndex;
    u32 _padding1;
    u32 _padding2;

    alignas(16) SM::Vector3 _cameraPos;
};

struct Vertex
{
    XMFLOAT3 _position;
    XMFLOAT2 _texCoord;
    XMFLOAT3 _normal;
};

struct Buffer
{
    ComPtr<ID3D12Resource> _resource;
    D3D12MA::Allocation* _allocation;
    union
	{
        D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view;
        D3D12_INDEX_BUFFER_VIEW index_buffer_view;
    };
};

struct BufferDesc
{
    u32 _bufferSize = 0;
    /* buffer may or may not have contents. Imagine just a pointer to memory
       in a shader reflection system where a change in GPU code affects CPU side memory
    */
    BufferType _bufferType;
    void* _pContents = nullptr; 
};

Buffer CreateBuffer(GfxDevice& gfxDevice, BufferDesc desc);
void DestroyBuffer(GfxDevice& gfxDevice, Buffer& buffer);

inline void AllocateUAVBuffer(ID3D12Device* pDevice, UINT64 bufferSize, ID3D12Resource** ppResource, D3D12_RESOURCE_STATES initialResourceState = D3D12_RESOURCE_STATE_COMMON, const wchar_t* resourceName = nullptr)
{
    auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    DX_ASSERT(pDevice->CreateCommittedResource(
        &uploadHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        initialResourceState,
        nullptr,
        IID_PPV_ARGS(ppResource)));
    if (resourceName)
    {
        (*ppResource)->SetName(resourceName);
    }
}

inline void AllocateUploadBuffer(ID3D12Device* pDevice, void* pData, UINT64 datasize, ID3D12Resource** ppResource, const wchar_t* resourceName = nullptr)
{
    auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(datasize);
    DX_ASSERT(pDevice->CreateCommittedResource(
        &uploadHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(ppResource)));
    if (resourceName)
    {
        (*ppResource)->SetName(resourceName);
    }
    void* pMappedData;
    (*ppResource)->Map(0, nullptr, &pMappedData);
    memcpy(pMappedData, pData, datasize);
    (*ppResource)->Unmap(0, nullptr);
}