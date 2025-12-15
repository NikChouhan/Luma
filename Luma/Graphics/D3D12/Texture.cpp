// Texture.cpp
#include "Texture.h"
#include "Graphics/FrameSync.h"
#include <D3D12MemAlloc.h>

#include "Graphics/Globals.h"


static void UploadTextureData(const GfxDevice& gfxDevice, FrameSync& frameSync, 
                              const ComPtr<ID3D12Resource>& resource, 
                              const Texture2DDesc& desc,
                              const TextureUsage usage)
{
    auto heapProps = CD3DX12_HEAP_PROPERTIES((usage == TextureUsage::UPLOAD) ? D3D12_HEAP_TYPE_UPLOAD : D3D12_HEAP_TYPE_GPU_UPLOAD);

    if (usage == TextureUsage::UPLOAD)
    {
        const UINT64 uploadBufferSize = GetRequiredIntermediateSize(resource.Get(),
            0, 1);
        ComPtr<ID3D12Resource> textureUploadHeapResource;
        auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);

        DX_ASSERT(gfxDevice.device_->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &bufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&textureUploadHeapResource)));


        D3D12_SUBRESOURCE_DATA textureData = {};
        textureData.pData = desc.initialData;
        textureData.RowPitch = desc.width * desc.texPixelSize;
        textureData.SlicePitch = textureData.RowPitch * desc.height;

        ImmediateSubmit(gfxDevice, &frameSync.immediateContext_, [&]()
            {
                UpdateSubresources(frameSync.immediateContext_.commandList.Get(), resource.Get(),
                    textureUploadHeapResource.Get(), 0, 0, 1, &textureData);
                auto pBarrier = CD3DX12_RESOURCE_BARRIER::Transition(resource.Get(),
                    D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
                frameSync.immediateContext_.commandList->ResourceBarrier(1, &pBarrier);
            });
    }
    else if (usage == TextureUsage::GPU_UPLOAD)
    {
	    
    }
}

// TODO: Have a function to upload multiple textures at once (i don't need it rn, but may in the future)

static ComPtr<ID3D12Resource> CreateTextureResource(
    const GfxDevice& gfxDevice,
    FrameSync& frameSync,
    const Texture2DDesc& desc,
    const TextureUsage& usage,
    const D3D12_RESOURCE_FLAGS resourceFlags)
{
    ComPtr<ID3D12Resource> resource;

	D3D12_RESOURCE_DESC resourceDesc = {};
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resourceDesc.Width = desc.width;
    resourceDesc.Height = desc.height;
    resourceDesc.DepthOrArraySize = desc.arraySize;
    resourceDesc.MipLevels = desc.mipLevels;
    resourceDesc.Format = desc.format;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.SampleDesc.Quality = 0;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    resourceDesc.Flags = resourceFlags;

    D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON;
    D3D12_CLEAR_VALUE* pClearValue = nullptr;
    D3D12_CLEAR_VALUE clearValue = {};

    if (resourceFlags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) 
    {
        initialState = D3D12_RESOURCE_STATE_RENDER_TARGET;
        clearValue.Format = desc.format;
        clearValue.Color[0] = 0.0f;
        clearValue.Color[1] = 0.0f;
        clearValue.Color[2] = 0.0f;
        clearValue.Color[3] = 1.0f;
        pClearValue = &clearValue;
    }
    D3D12_HEAP_TYPE heapType{};
    switch (usage)
    {
    case TextureUsage::UPLOAD:
        heapType = D3D12_HEAP_TYPE_UPLOAD;
        initialState = D3D12_RESOURCE_STATE_GENERIC_READ;
        break;

    case TextureUsage::DEFAULT:
        heapType = D3D12_HEAP_TYPE_DEFAULT;
        initialState = D3D12_RESOURCE_STATE_COMMON;
        break;
    case TextureUsage::READBACK:
        heapType = D3D12_HEAP_TYPE_READBACK;
        initialState = D3D12_RESOURCE_STATE_COPY_DEST;
        break;

    case TextureUsage::GPU_UPLOAD:
        heapType = D3D12_HEAP_TYPE_GPU_UPLOAD;
        initialState = D3D12_RESOURCE_STATE_COMMON;
        break;
    }

	D3D12MA::ALLOCATION_DESC allocDesc = {};
    allocDesc.HeapType = heapType;
    allocDesc.Flags = D3D12MA::ALLOCATION_FLAG_NONE;

    D3D12MA::Allocation* allocation;
    DX_ASSERT(gfxDevice.allocator_->CreateResource(
        &allocDesc,
        &resourceDesc,
        initialState,
        pClearValue,
        &allocation,
        IID_PPV_ARGS(&resource)));

    assert(desc.initialData);
    if (usage == TextureUsage::UPLOAD || usage == TextureUsage::GPU_UPLOAD) 
    {
        UploadTextureData(gfxDevice, frameSync, resource, desc, usage);
    }

    // allocation->Release();

    return resource;
}

static u32 CreateTextureSRV(
    const GfxDevice& gfxDevice,
    ID3D12DescriptorHeap* heap,
    u32* nextIndex,
    const ComPtr<ID3D12Resource>& resource,
    const Texture2DDesc& desc)
{
    u32 index = (*nextIndex)++;

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = desc.format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels = desc.mipLevels;
    srvDesc.Texture2D.MostDetailedMip = 0;

    CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(heap->GetCPUDescriptorHandleForHeapStart());
    u32 descriptorSize = gfxDevice.device_->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    cpuHandle.Offset(index, descriptorSize);

    gfxDevice.device_->CreateShaderResourceView(resource.Get(), &srvDesc, cpuHandle);

    return index;
}

static u32 CreateTextureRTV(
    const GfxDevice& gfxDevice,
    ID3D12DescriptorHeap* rtvHeap,  // Note: Separate RTV heap!
    u32* nextRTVIndex,
    const ComPtr<ID3D12Resource>& resource,
    const Texture2DDesc& desc)
{
    u32 index = (*nextRTVIndex)++;

    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.Format = desc.format;
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    rtvDesc.Texture2D.MipSlice = 0;

    CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart());
    u32 descriptorSize = gfxDevice.device_->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    cpuHandle.Offset(index, descriptorSize);

    gfxDevice.device_->CreateRenderTargetView(resource.Get(), &rtvDesc, cpuHandle);

    return index;
}


static u32 CreateTextureUAV(
    const GfxDevice& gfxDevice,
    ID3D12DescriptorHeap* heap,
    u32* nextIndex,
    const ComPtr<ID3D12Resource>& resource,
    const Texture2DDesc& desc,
    u32 mipLevel)
{
    u32 index = (*nextIndex)++;

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = desc.format;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    uavDesc.Texture2D.MipSlice = mipLevel;

    CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(heap->GetCPUDescriptorHandleForHeapStart());
    u32 descriptorSize = gfxDevice.device_->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    cpuHandle.Offset(index, descriptorSize);

    gfxDevice.device_->CreateUnorderedAccessView(resource.Get(), nullptr, &uavDesc, cpuHandle);

    return index;
}

Texture CreateTexture(const GfxDevice& gfxDevice, FrameSync& frameSync, const TextureCreateInfo& createInfo)
{
    const auto& desc = createInfo.desc;
    Texture texture = {};

    D3D12_RESOURCE_FLAGS resourceFlags = D3D12_RESOURCE_FLAG_NONE;
    if (HasFlag(desc.viewFlags, TextureViewFlags::RTV)) 
    {
        resourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    }
    if (HasFlag(desc.viewFlags, TextureViewFlags::UAV)) 
    {
        resourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }
    if (!HasFlag(desc.viewFlags, TextureViewFlags::SRV))
    {
        resourceFlags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
    }

	texture.resource = CreateTextureResource(gfxDevice, frameSync, desc, createInfo.usage, resourceFlags);

    if (createInfo.debugName) 
    {
        DX_ASSERT(texture.resource->SetName(createInfo.debugName));
    }

    if (!createInfo.heap) 
    {
        return texture;
    }
	if (HasFlag(desc.viewFlags, TextureViewFlags::SRV)) 
    {
        texture.srvIndex = CreateTextureSRV(
            gfxDevice,
            createInfo.heap,
            &GlobalStorage::bindlessHeapIndex.nextIndex,
            texture.resource,
            desc);
    }

    /* RTV needs separate heap! (currently only RTVs I have are swapchain images
     * TODO: when i need i will create and pass a separate heap all for RTVs
     */
    //if (HasFlag(desc.viewFlags, TextureViewFlags::RTV)) 
    //{
    //    texture.rtvIndex = CreateTextureRTV(gfxDevice, rtvHeap, nextRTVIndex, texture.resource, desc);
    //}

    // DSV will be part of the swapchain and not be implemented here

    if (HasFlag(desc.viewFlags, TextureViewFlags::UAV)) 
    {
        if (desc.createMipUAVs) 
        {
            // create UAV for each mip level
            for (u32 mip = 0; mip < desc.mipLevels; ++mip) 
            {
                u32 uavIdx = CreateTextureUAV(
                    gfxDevice,
                    createInfo.heap,
                    &GlobalStorage::bindlessHeapIndex.nextIndex,
                    texture.resource,
                    desc,
                    mip);
                texture.mipUAVIndices.push_back(uavIdx);
            }
            texture.uavIndex = texture.mipUAVIndices[0];
        }
        else 
        {
            texture.uavIndex = CreateTextureUAV(
                gfxDevice,
                createInfo.heap,
                &GlobalStorage::bindlessHeapIndex.nextIndex,
                texture.resource,
                desc,
                0);
        }
    }
    return texture;
}
