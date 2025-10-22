#include "Buffer.h"

#include <D3D12MemAlloc.h>

Buffer CreateBuffer(GfxDevice& gfxDevice, BufferDesc desc)
{
    // REBAR path, stupid doesn't work
    Buffer buffer{};

    auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(desc._bufferSize);
    D3D12MA::CALLOCATION_DESC allocDesc = D3D12MA::CALLOCATION_DESC{
        D3D12_HEAP_TYPE_GPU_UPLOAD,
    D3D12MA::ALLOCATION_FLAG_COMMITTED };

    D3D12MA::Allocation* bufferAllocation{};
    DX_ASSERT(gfxDevice._allocator->CreateResource(&allocDesc, &bufferDesc, 
        D3D12_RESOURCE_STATE_COMMON,
        nullptr, &bufferAllocation, IID_NULL, nullptr));
    buffer._resource = bufferAllocation->GetResource();
    bufferAllocation->Release();

    u8* pDataBegin;
    CD3DX12_RANGE readRange(0, 0);
    DX_ASSERT(buffer._resource->Map(0, &readRange, reinterpret_cast<void**>(&pDataBegin)));
    memcpy(pDataBegin, desc._pContents, desc._bufferSize);

    buffer._resource->Unmap(0, nullptr);
    // don't want to unmap the pointer for future use

    // init the buffer view
    if (desc._bufferType == BufferType::VERTEX)
    {
        buffer.vertex_buffer_view.BufferLocation = buffer._resource->GetGPUVirtualAddress();
        buffer.vertex_buffer_view.StrideInBytes = sizeof(Vertex);
        buffer.vertex_buffer_view.SizeInBytes = desc._bufferSize;
    }
    else if (desc._bufferType == BufferType::INDEX)
    {
        buffer.index_buffer_view.BufferLocation = buffer._resource->GetGPUVirtualAddress();
        buffer.index_buffer_view.Format = DXGI_FORMAT_R32_UINT;
        buffer.index_buffer_view.SizeInBytes = desc._bufferSize;
    }
    
    return buffer;

    // non REBAR path (will be pre-checked at start later)

    /*Buffer buffer{};
    auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(desc._bufferSize);

    D3D12MA::ALLOCATION_DESC allocDesc = {};
    allocDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
    allocDesc.Flags = D3D12MA::ALLOCATION_FLAG_NONE;

    DX_ASSERT(gfxDevice._allocator->CreateResource(
        &allocDesc,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        &buffer._allocation,
        IID_NULL,
        nullptr));

    buffer._resource = buffer._allocation->GetResource();

    u8* pDataBegin;
    CD3DX12_RANGE readRange(0, 0);
    DX_ASSERT(buffer._resource->Map(0, nullptr, reinterpret_cast<void**>(&pDataBegin)));
    memcpy(pDataBegin, desc._pContents, desc._bufferSize);
    buffer._resource->Unmap(0, nullptr);

    if (desc._bufferType == BufferType::VERTEX)
    {
        buffer.vertex_buffer_view.BufferLocation = buffer._resource->GetGPUVirtualAddress();
        buffer.vertex_buffer_view.StrideInBytes = sizeof(Vertex);
        buffer.vertex_buffer_view.SizeInBytes = desc._bufferSize;
    }
    else if (desc._bufferType == BufferType::INDEX)
    {
        buffer.index_buffer_view.BufferLocation = buffer._resource->GetGPUVirtualAddress();
        buffer.index_buffer_view.Format = DXGI_FORMAT_R32_UINT;
        buffer.index_buffer_view.SizeInBytes = desc._bufferSize;
    }

    return buffer;*/
}