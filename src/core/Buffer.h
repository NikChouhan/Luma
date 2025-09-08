#pragma once

#include "GfxDevice.h"

using namespace DirectX;
struct Vertex
{
    XMFLOAT3 position;
    XMFLOAT2 texCoord;
};

struct Buffer
{
    ComPtr<ID3D12Resource> _resource;
    D3D12_VERTEX_BUFFER_VIEW _bufferView;
};

struct BufferDesc
{
    u32 _bufferSize = 0;
    /* buffer may or may not have contents. Imagine just a pointer to memory
       in a shader reflection system where a change in GPU code affects CPU side memory
    */
    void* _pContents = nullptr; 
};

Buffer CreateBuffer(GfxDevice& gfxDevice, BufferDesc desc);
void DestroyBuffer(GfxDevice& gfxDevice, Buffer& buffer);