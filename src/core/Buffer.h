#pragma once

#include "Common.h"
#include "GfxDevice.h"

using namespace DirectX;

enum class BufferType
{
	VERTEX,
    INDEX,
    CONSTANT
};

struct ConstBuffer
{
    u32 _materialIndex;
    u32 _padding[3];
    XMMATRIX _worldViewProj;
    XMMATRIX _worldMatrix;
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
    D3D12_VERTEX_BUFFER_VIEW _vertexBufferView;
    D3D12_INDEX_BUFFER_VIEW _indexBufferView;
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