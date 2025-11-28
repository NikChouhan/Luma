#pragma once

#include <optional>

#include "GfxDevice.h"

enum class TextureUsage : u8
{
	UPLOAD,
    DEFAULT,
    GPU_UPLOAD,
    READBACK
};

enum class TextureViewFlags : u8
{
    NONE = 0,
    SRV = 1 << 0,
    UAV = 1 << 1,
	RTV = 1 << 2
};
// allows having multiple Texture views for a resource along with the heap indexes for each
inline TextureViewFlags operator|(TextureViewFlags a, TextureViewFlags b)
{
    return static_cast<TextureViewFlags>(static_cast<u32>(a) | static_cast<u32>(b));
}

inline bool HasFlag(TextureViewFlags flags, TextureViewFlags check)
{
    return (static_cast<u32>(flags) & static_cast<u32>(check)) != 0;
}

struct Texture2DDesc
{
    u32 width = 1920;
    u32 height = 1080;
    u32 texPixelSize = 4;
    u32 mipLevels = 1;
    u32 arraySize = 1;
    DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
    TextureViewFlags viewFlags = TextureViewFlags::SRV;

    // create UAV per mip level (for mip generation)
    bool createMipUAVs = false;

    const void* initialData = nullptr;
};

struct Texture
{
    ComPtr<ID3D12Resource> resource;

    std::optional<u32> srvIndex;
    std::optional<u32> rtvIndex;
    std::optional<u32> uavIndex;

    // if createMipUAVs is true
    std::vector<u32> mipUAVIndices;
};

struct TextureCreateInfo
{
    Texture2DDesc desc;
    const wchar_t* debugName = nullptr;

    TextureUsage usage; // mostly for ReBar on/off
    ID3D12DescriptorHeap* heap = nullptr;
    u32* nextHeapIndex = nullptr;
};

// never use GPU_UPLOAD. I haven't implemented it yet
Texture CreateTexture(const GfxDevice& gfxDevice, FrameSync& frameSync, const TextureCreateInfo& createInfo);