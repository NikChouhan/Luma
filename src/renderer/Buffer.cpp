#include "Buffer.h"

#include <D3D12MemAlloc.h>

struct BufferViewCreator
{
    const Buffer& buffer;
    const GfxDevice& gfxDevice;
    const BufferCreateInfo& createInfo;

    BufferView operator()(const VertexBufferDesc& desc) const
    {
        D3D12_VERTEX_BUFFER_VIEW vbv{};
        vbv.BufferLocation = buffer.resource->GetGPUVirtualAddress();
        vbv.StrideInBytes = desc.vertexStride;
        vbv.SizeInBytes = GetBufferSize(desc);

        return VertexBufferView{ .view = vbv, .stride = desc.vertexStride };
    }

    BufferView operator()(const IndexBufferDesc& desc) const
    {
        D3D12_INDEX_BUFFER_VIEW ibv{};
        ibv.BufferLocation = buffer.resource->GetGPUVirtualAddress();
        ibv.Format = desc.indexFormat;
        ibv.SizeInBytes = GetBufferSize(desc);

        return IndexBufferView{ .view = ibv, .indexFormat = desc.indexFormat };
    }

    BufferView operator()(const ConstantBufferDesc& desc) const
    {
    	std::optional<u32> heapIdx = std::nullopt;
		if (desc.createView && createInfo.bindlessHeap && createInfo.nextHeapIndex)
		{
            heapIdx = (*createInfo.nextHeapIndex)++;
            u32 descriptorSize = gfxDevice.device->GetDescriptorHandleIncrementSize(
                D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
            cbvDesc.BufferLocation = buffer.resource->GetGPUVirtualAddress();
            cbvDesc.SizeInBytes = GetBufferSize(desc);

            CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(createInfo.bindlessHeap->GetCPUDescriptorHandleForHeapStart());
            cpuHandle.Offset(*heapIdx, descriptorSize);
            gfxDevice.device->CreateConstantBufferView(&cbvDesc, cpuHandle);
		}

        return ConstantBufferView{ .gpuAddress = buffer.resource->GetGPUVirtualAddress(),
            .sizeInBytes = GetBufferSize(desc),
            .heapIndex = *heapIdx };
    }
    BufferView operator()(const StructuredBufferDesc& desc) const
    {
        if (createInfo.bindlessHeap && createInfo.nextHeapIndex) {
            u32 descriptorSize = gfxDevice.device->GetDescriptorHandleIncrementSize(
                D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

            D3D12_CPU_DESCRIPTOR_HANDLE baseHandle =
                createInfo.bindlessHeap->GetCPUDescriptorHandleForHeapStart();

            // Create SRV if requested
            if (desc.createSRV) 
            {
                std::optional<u32> srvIndex = std::nullopt;
                srvIndex = (*createInfo.nextHeapIndex)++;

                D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
                srvDesc.Format = DXGI_FORMAT_UNKNOWN;
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
                srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                srvDesc.Buffer.FirstElement = 0;
                srvDesc.Buffer.NumElements = desc.elementCount;
                srvDesc.Buffer.StructureByteStride = desc.elementStride;

                CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(baseHandle);
                srvHandle.Offset(*srvIndex, descriptorSize);

                gfxDevice.device->CreateShaderResourceView(
                    buffer.resource.Get(), &srvDesc, srvHandle);

                return ShaderResourceView{
                buffer.resource->GetGPUVirtualAddress(),
                GetBufferSize(desc),
                DXGI_FORMAT_UNKNOWN,
                srvIndex
                };
            }

            // Create UAV if requested
            if (desc.createUAV) 
            {
                std::optional<u32> uavIndex = std::nullopt;
                uavIndex = (*createInfo.nextHeapIndex)++;

                D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
                uavDesc.Format = DXGI_FORMAT_UNKNOWN;
                uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
                uavDesc.Buffer.FirstElement = 0;
                uavDesc.Buffer.NumElements = desc.elementCount;
                uavDesc.Buffer.StructureByteStride = desc.elementStride;

                CD3DX12_CPU_DESCRIPTOR_HANDLE uavHandle(baseHandle);
                uavHandle.Offset(*uavIndex, descriptorSize);

                gfxDevice.device->CreateUnorderedAccessView(
                    buffer.resource.Get(), nullptr, &uavDesc, uavHandle);

                return UnorderedAccessView{
                buffer.resource->GetGPUVirtualAddress(),
                GetBufferSize(desc),
                DXGI_FORMAT_UNKNOWN,
                uavIndex
                };
            }
        }
        return std::monostate{};
    }

    BufferView operator()(const RawBufferDesc& desc) const
    {
        if (createInfo.bindlessHeap && createInfo.nextHeapIndex) 
        {
            u32 descriptorSize = gfxDevice.device->GetDescriptorHandleIncrementSize(
                D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

            D3D12_CPU_DESCRIPTOR_HANDLE baseHandle =
                createInfo.bindlessHeap->GetCPUDescriptorHandleForHeapStart();

            if (desc.createSRV) 
            {
                std::optional<u32> srvIndex = std::nullopt;
                srvIndex = (*createInfo.nextHeapIndex)++;

                u32 elementSize = 4;  // For R32 formats
                u32 numElements = desc.sizeInBytes / elementSize;

                D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
                srvDesc.Format = desc.format;
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
                srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                srvDesc.Buffer.FirstElement = 0;
                srvDesc.Buffer.NumElements = numElements;

                CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(baseHandle);
                srvHandle.Offset(*srvIndex, descriptorSize);

                gfxDevice.device->CreateShaderResourceView(
                    buffer.resource.Get(), &srvDesc, srvHandle);

                return ShaderResourceView{
			    buffer.resource->GetGPUVirtualAddress(),
			    desc.sizeInBytes,
			    desc.format,
			    srvIndex
                };
            }

            if (desc.createUAV) 
            {
                std::optional<u32> uavIndex = std::nullopt;
                uavIndex = (*createInfo.nextHeapIndex)++;

                u32 elementSize = 4;
                u32 numElements = desc.sizeInBytes / elementSize;

                D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
                uavDesc.Format = desc.format;
                uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
                uavDesc.Buffer.FirstElement = 0;
                uavDesc.Buffer.NumElements = numElements;

                CD3DX12_CPU_DESCRIPTOR_HANDLE uavHandle(baseHandle);
                uavHandle.Offset(*uavIndex, descriptorSize);

                gfxDevice.device->CreateUnorderedAccessView(
                    buffer.resource.Get(), nullptr, &uavDesc, uavHandle);

                return UnorderedAccessView{
               buffer.resource->GetGPUVirtualAddress(),
               desc.sizeInBytes,
               desc.format,
               uavIndex
                };
            }
        }
        return std::monostate{};
    }
};


Buffer CreateBuffer(const GfxDevice& gfxDevice, const BufferCreateInfo& createInfo)
{
    Buffer buffer = {};

    u32 bufferSize = GetBufferSize(createInfo.desc);
    D3D12_HEAP_TYPE heapType{};
    D3D12_RESOURCE_STATES initialState{};

    switch (createInfo.usage)
    {
    case BufferUsage::UPLOAD :
        heapType = D3D12_HEAP_TYPE_UPLOAD;
        initialState = D3D12_RESOURCE_STATE_GENERIC_READ;
        break;

    case BufferUsage::DEFAULT:
        heapType = D3D12_HEAP_TYPE_DEFAULT;
        initialState = D3D12_RESOURCE_STATE_COMMON;
        break;
    case BufferUsage::READBACK:
        heapType = D3D12_HEAP_TYPE_READBACK;
        initialState = D3D12_RESOURCE_STATE_COPY_DEST;
        break;

    case BufferUsage::GPU_UPLOAD:
        heapType = D3D12_HEAP_TYPE_GPU_UPLOAD;
        initialState = D3D12_RESOURCE_STATE_COMMON;
        break;
    }

    auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
    D3D12MA::ALLOCATION_DESC allocDesc{};
    allocDesc.HeapType = heapType;
    allocDesc.Flags = D3D12MA::ALLOCATION_FLAG_NONE;

    DX_ASSERT(gfxDevice.allocator->CreateResource(
        &allocDesc,
        &bufferDesc,
        initialState,
        nullptr,
        &buffer.allocation,
        IID_NULL,
        nullptr));

    buffer.resource = buffer.allocation->GetResource();
    DX_ASSERT(buffer.resource->SetName(createInfo.debugName));

    if (heapType == D3D12_HEAP_TYPE_UPLOAD || heapType == D3D12_HEAP_TYPE_GPU_UPLOAD) 
    {
        void* pMappedData;
        CD3DX12_RANGE readRange(0, 0);
        DX_ASSERT(buffer.resource->Map(0, &readRange, &pMappedData));
        std::visit(BufferUpdateVisitor{ pMappedData }, createInfo.desc);

        if (!createInfo.keepMapped) {
            buffer.resource->Unmap(0, nullptr);
        }
    }

    buffer.view = std::visit(BufferViewCreator{
        .buffer = buffer,
    	.gfxDevice = gfxDevice,
    	.createInfo = createInfo}, createInfo.desc);

    return buffer;
}

void UpdateBuffer(Buffer& buffer, const BufferDesc& desc)
{
    void* pMappedData;
    CD3DX12_RANGE readRange(0, 0);
    DX_ASSERT(buffer.resource->Map(0, &readRange, &pMappedData));

    std::visit(BufferUpdateVisitor{ pMappedData }, desc);

    buffer.resource->Unmap(0, nullptr);
}
