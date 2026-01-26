#pragma once
#include <cstdint>

enum class ResourceState : u32
{
    Common = 0,
    VertexAndConstantBuffer = 0x1,
    IndexBuffer = 0x2,
    RenderTarget = 0x4,
    UnorderedAccess = 0x8,
    DepthWrite = 0x10,
    DepthRead = 0x20,
    NonPixelShaderResource = 0x40,
    PixelShaderResource = 0x80,
    CopyDest = 0x400,
    CopySource = 0x800,
    Present = 0x2000, // D3D12 specific, Common for Vulkan
    
    // Helpers
    GenericRead = VertexAndConstantBuffer | IndexBuffer | NonPixelShaderResource | PixelShaderResource | CopySource,
    Unknown = 0xFFFFFFFF
};

inline bool IsReadState(ResourceState state) 
{
    return (static_cast<u32>(state) & (static_cast<u32>(ResourceState::GenericRead) | static_cast<u32>(ResourceState::DepthRead))) != 0;
}

inline bool IsWriteState(ResourceState state) 
{
    return !IsReadState(state);
}