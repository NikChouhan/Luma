#pragma once

#include <variant>

#include "Graphics/D3D12/Buffer.h"
#include "Graphics/FrameSync.h"
#include "Graphics/D3D12/Texture.h"

// the texture type only supports CBVs, SRVs, UAVs
// VBs, IBs, shaders are more like part of Pipelines,
// and will be handled likewise

struct ResourceHandle {
	u32 index : 24;      // Array index
	u32 generation : 8;  // Generation counter
};

static constexpr ResourceHandle g_invalidResourceHandle = { 0xFFFFFF, 0xFF };

// ============================================================================
// Render graph texture/buffer resources
// ============================================================================

// resource type and usage need to be shifted to respective Descriptors.
// The plan is to read the json, find the 'texture'/'buffer' type, usage, name, etc
// and then pass them to resourceManager.CreateResource(desc, "name").
// Same with Pipelines and Shaders handled in PipelineCache.h/cpp


using ResourceCreateInfo = std::variant<TextureCreateInfo, BufferCreateInfo>;
using Resource = std::variant<Texture, Buffer>;

struct ResourceCreator
{
	GfxDevice& gfxDevice;
	FrameSync& frameSync;
	Resource operator() (const TextureCreateInfo& desc) const
	{
		return CreateTexture(gfxDevice, frameSync, desc);
	}
	Resource operator() (const BufferCreateInfo& desc) const
	{
		return CreateBuffer(gfxDevice, desc);
	}
};

struct ManagedResource
{
	Resource resource;
	ResourceHandle handle;
	std::string name;  // for frame graph
};

struct ResourceManager
{
	ResourceManager(const GfxDevice& lgfxDevice, FrameSync& lframeSync);
	// single copy of resource manager should be present
	ResourceManager(const ResourceManager& resourceManager) = delete;
	ResourceManager operator=(const ResourceManager& resourceManager) = delete;
	~ResourceManager();

	[[nodiscard]] ResourceHandle CreateResource(ResourceCreateInfo desc, const std::string& name);

	Resource* GetResource(ResourceHandle handle);
	const Resource* GetResource(ResourceHandle handle) const;
	ResourceHandle GetResourceHandleByName(const std::string& name);
	void ReleaseResource(ResourceHandle handle);

	ComPtr<ID3D12DescriptorHeap> GetBindlessHeap() { return bindlessHeap_; }

private:
	GfxDevice gfxDevice_;
	FrameSync frameSync_;
	ComPtr<ID3D12DescriptorHeap> bindlessHeap_;

	std::vector<ManagedResource> resources_;
	std::vector<u8> generations_;
	std::vector<u32> freeList_;
	std::unordered_map<std::string, ResourceHandle> resourceNameMap_;

	[[nodiscard]] ResourceHandle AllocateResourceHandle();
	[[nodiscard]] u32 GetResourceIndex(ResourceHandle handle) const;
	[[nodiscard]] bool IsResourceHandleValid(ResourceHandle handle) const;

	void ResourceManager::CreateBindlessHeap();
};
