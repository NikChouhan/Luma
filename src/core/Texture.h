#pragma once
#include "GfxDevice.h"

struct GfxDevice;
struct Texture
{
	ComPtr<ID3D12Resource> _resource{};
	ComPtr<ID3D12DescriptorHeap> _srvHeap;
};

struct TextureDesc
{
	u32 _texWidth = 256;
	u32 _texHeight = 256;
	u32 _texPixelSize = 4;
	unsigned char* _pContents = nullptr;
};

Texture CreateTexture(GfxDevice& gfxDevice, FrameSync& frameSync, TextureDesc desc);
