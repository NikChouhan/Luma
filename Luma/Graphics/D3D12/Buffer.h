#pragma once

#include <optional>
#include <variant>

#include "Core/Common.h"
#include "Graphics/GfxDevice.h"
#include "Graphics/Globals.h"

typedef u32 Index;

namespace D3D12MA
{
	class Allocation;
}

struct Vertex
{
	DirectX::XMFLOAT3 position_;
    DirectX::XMFLOAT2 texCoord_;
    DirectX::XMFLOAT3 normal_;
};

struct VertexBufferView {
    D3D12_VERTEX_BUFFER_VIEW view;
    u32 stride;
};

struct IndexBufferView {
    D3D12_INDEX_BUFFER_VIEW view;
    DXGI_FORMAT indexFormat;  // R16 or R32
};

struct ConstantBufferView {
    D3D12_GPU_VIRTUAL_ADDRESS gpuAddress;
    u32 sizeInBytes;
    std::optional<u32> heapIndex;
};
struct UnorderedAccessView {
    D3D12_GPU_VIRTUAL_ADDRESS gpuAddress;
    u32 sizeInBytes;
    DXGI_FORMAT format;
    std::optional<u32> heapIndex;
};

struct ShaderResourceView {
    D3D12_GPU_VIRTUAL_ADDRESS gpuAddress;
    u32 sizeInBytes;
    DXGI_FORMAT format;
    std::optional<u32> heapIndex;
};

using BufferView = std::variant<
	std::monostate,        // No view (raw buffer)
	VertexBufferView,
	IndexBufferView,
	ConstantBufferView,
    UnorderedAccessView,
    ShaderResourceView
>;

struct Buffer
{
    ComPtr<ID3D12Resource> resource;
    D3D12MA::Allocation* allocation = nullptr;
    BufferView view;
    u32 heapIndex = 0;

    [[nodiscard]] const VertexBufferView* AsVertexBuffer() const
    {
        return std::get_if<VertexBufferView>(&view);
    }
    [[nodiscard]] const IndexBufferView* AsIndexBuffer() const
    {
        return std::get_if<IndexBufferView>(&view);
    }
    [[nodiscard]] const ConstantBufferView* AsConstantBuffer() const
    {
        return std::get_if<ConstantBufferView>(&view);
    }
	[[nodiscard]] const UnorderedAccessView* AsUnorderedAccessView() const
    {
        return std::get_if<UnorderedAccessView>(&view);
    }
    [[nodiscard]] const ShaderResourceView* AsShaderResourceView() const
    {
        return std::get_if<ShaderResourceView>(&view);
    }
};

struct VertexBufferDesc
{
    const void* vertices;
    u32 vertexCount;
    u32 vertexStride;
};

struct IndexBufferDesc
{
    const void* indices;
    u32 indexCount;
    DXGI_FORMAT indexFormat = DXGI_FORMAT_R32_UINT;
};

struct ConstantBufferDesc
{
    const void* data;
    u32 sizeInBytes;
    bool createView = true; // view created and set in descriptor heap
};

struct StructuredBufferDesc
{
    const void* data;
    u32 elementCount;
    u32 elementStride;
    bool createSRV = false;
    bool createUAV = false;
};

struct RawBufferDesc
{
    const void* data;
    u32 sizeInBytes;

    bool createSRV = false;
    bool createUAV = false;
    DXGI_FORMAT format = DXGI_FORMAT_R32_TYPELESS;
};

using BufferDesc = std::variant<
    VertexBufferDesc,
    IndexBufferDesc,
    ConstantBufferDesc,
	StructuredBufferDesc,
    RawBufferDesc
>;

enum class BufferUsage : u8 {
    UPLOAD,      // CPU write, GPU read (default for VB/IB/CB)
    DEFAULT,     // GPU only
    READBACK,    // GPU write, CPU read
    GPU_UPLOAD   // CPU available (and writeable, don't read ever, its slow af), GPU located memory
};

struct BufferCreateInfo {
    BufferDesc desc;
    BufferUsage usage = BufferUsage::UPLOAD;
    bool keepMapped = false;
    const wchar_t* debugName = nullptr;

    ID3D12DescriptorHeap* bindlessHeap = nullptr;
};
Buffer CreateBuffer(const GfxDevice& gfxDevice, const BufferCreateInfo& createInfo);

inline Buffer CreateVertexBuffer(const GfxDevice& gfxDevice,
    const void* vertices,
    u32 vertexCount,
    u32 vertexStride,
    const wchar_t* debugName = nullptr)
{
    return CreateBuffer(gfxDevice, {
        .desc = VertexBufferDesc{.vertices = vertices, .vertexCount = vertexCount, .vertexStride = vertexStride},
        .debugName = debugName
        });
}

inline Buffer CreateIndexBuffer(const GfxDevice& gfxDevice,
    const void* indices,
    u32 indexCount,
    DXGI_FORMAT format = DXGI_FORMAT_R32_UINT,
    const wchar_t* debugName = nullptr)
{
    return CreateBuffer(gfxDevice, {
        .desc = IndexBufferDesc{.indices = indices, .indexCount = indexCount, .indexFormat = format},
        .debugName = debugName
        });
}

inline Buffer CreateConstantBuffer(const GfxDevice& gfxDevice,
    const void* data,
    u32 sizeInBytes,
    const wchar_t* debugName = nullptr)
{
    return CreateBuffer(gfxDevice, {
        .desc = ConstantBufferDesc{.data = data, .sizeInBytes = sizeInBytes},
        .keepMapped = true, // cbs almost 
        .debugName = debugName
        });
}
// buffer size
struct BufferSizeVisitor
{
    u32 operator()(const VertexBufferDesc& desc) const
    {
        return desc.vertexCount * desc.vertexStride;
    }
    u32 operator()(const IndexBufferDesc& desc) const
    {
        u32 indexSize = (desc.indexFormat == DXGI_FORMAT_R16_UINT) ? 2 : 4;
        return desc.indexCount * indexSize;
    }
    u32 operator()(const ConstantBufferDesc& desc) const
    {
        // Align to 256 bytes for CBVs
        return (desc.sizeInBytes + 255) & ~255; // last bits of 255 in binary are flipped to 0 with ~ op,
        // which when &ed with something > 255 give multiple of 256
    }
    u32 operator()(const StructuredBufferDesc& desc) const
    {
        return desc.elementCount * desc.elementStride;
    }
    u32 operator()(const RawBufferDesc& desc) const
    {
        return desc.sizeInBytes;
    }
};

inline u32 GetBufferSize(const BufferDesc& desc)
{
    return std::visit(BufferSizeVisitor{}, desc);
}

// buffer update for mapped buffers (included in .h cuz it can be called anywhere)

struct BufferUpdateVisitor
{
    void* mappedData;

    void operator()(const VertexBufferDesc& desc) const
    {
        u32 size = GetBufferSize(desc);
        memcpy(mappedData, desc.vertices, size);
    }
    void operator()(const IndexBufferDesc& desc) const
    {
        u32 size = GetBufferSize(desc);
        memcpy(mappedData, desc.indices, size);
    }
    void operator()(const ConstantBufferDesc& desc) const
    {
        if (desc.data)
        {
            u32 size = GetBufferSize(desc);
            memcpy(mappedData, desc.data, size);
        }
    }
    void operator()(const StructuredBufferDesc& desc) const
    {
        if (desc.data)
        {
            u32 size = GetBufferSize(desc);
            memcpy(mappedData, desc.data, size);
        }
    }
    void operator()(const RawBufferDesc& desc) const
    {
        if (desc.data)
        {
            u32 size = GetBufferSize(desc);
            memcpy(mappedData, desc.data, size);
        }
    }
};

void UpdateBuffer(Buffer& buffer, const BufferDesc& desc);