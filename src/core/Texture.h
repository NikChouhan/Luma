#pragma once
#include "GfxDevice.h"

enum class TextureType;
struct GfxDevice;

enum class TextureResourceType : u8
{
	SAMPLE,
	UAV
};
struct Texture
{
	ComPtr<ID3D12Resource> _resource{};
};

struct TextureDesc
{
	u32 _texWidth = 256;
	u32 _texHeight = 256;
	u32 _texPixelSize = 4;
	unsigned char* _pContents = nullptr;
	TextureResourceType _textureType{};
	TextureType _type;
};

Texture CreateTexture(GfxDevice& gfxDevice, FrameSync& frameSync, TextureDesc desc);
