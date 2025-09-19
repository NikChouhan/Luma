#include "Buffer.h"

#include <D3D12MemAlloc.h>

Buffer CreateBuffer(GfxDevice& gfxDevice, BufferDesc desc)
{
    Buffer buffer{};

    auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(desc._bufferSize);
    D3D12MA::CALLOCATION_DESC allocDesc = D3D12MA::CALLOCATION_DESC{
        D3D12_HEAP_TYPE_GPU_UPLOAD,
    D3D12MA::ALLOCATION_FLAG_STRATEGY_MIN_TIME};

    D3D12MA::Allocation* bufferAllocation{};
    DX_ASSERT(gfxDevice._allocator->CreateResource(&allocDesc, &bufferDesc, 
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr, &bufferAllocation, IID_NULL, nullptr));
    buffer._resource = bufferAllocation->GetResource();
    bufferAllocation->Release();

    u8* pDataBegin;
    CD3DX12_RANGE readRange(0, 0);
    DX_ASSERT(buffer._resource->Map(0, &readRange, reinterpret_cast<void**>(&pDataBegin)));
    memcpy(pDataBegin, desc._pContents, desc._bufferSize);
    // don't want to unmap the pointer for future use

    // init the buffer view
    if (desc._bufferType == BufferType::VERTEX)
    {
        buffer._vertexBufferView.BufferLocation = buffer._resource->GetGPUVirtualAddress();
        buffer._vertexBufferView.StrideInBytes = sizeof(Vertex);
        buffer._vertexBufferView.SizeInBytes = desc._bufferSize;
    }
    else if (desc._bufferType == BufferType::INDEX)
    {
        buffer._indexBufferView.BufferLocation = buffer._resource->GetGPUVirtualAddress();
        buffer._indexBufferView.Format = DXGI_FORMAT_R32_UINT;
        buffer._indexBufferView.SizeInBytes = desc._bufferSize;
    }
    
    return buffer;
}