#include "Resources.h"

ResourceManager::ResourceManager(const GfxDevice& lgfxDevice, FrameSync& lframeSync)
	: gfxDevice_(lgfxDevice), frameSync_(lframeSync){}

ResourceManager::~ResourceManager()
= default;

ResourceHandle ResourceManager::CreateResource(ResourceCreateInfo desc, const std::string name)
{
	if (!name.empty())
	{
		auto it = resourceNameMap_.find(name);
		if (it != resourceNameMap_.end() && IsResourceHandleValid(it->second))
			return it->second;
	}
	ResourceHandle handle = AllocateResourceHandle();
	u32 index = handle.index;

	Resource resource = std::visit(ResourceCreator{ gfxDevice_, frameSync_ }, desc);

	if (index >= resources_.size())
	{
		resources_.resize(index + 1);
	}

	resources_.at(index) = ManagedResource{
		.resource = resource,
		.handle = handle,
		.name = name
	};

	resourceNameMap_[name] = handle;

	return handle;
}

Resource* ResourceManager::GetResource(ResourceHandle handle)
{
	if (!IsResourceHandleValid(handle)) {
		return nullptr;
	}

	u32 index = GetResourceIndex(handle);
	return &resources_.at(index).resource;
}

const Resource* ResourceManager::GetResource(ResourceHandle handle) const
{
	if (!IsResourceHandleValid(handle)) 
	{
		return nullptr;
	}

	u32 index = GetResourceIndex(handle);
	return &resources_.at(index).resource;
}

ResourceHandle ResourceManager::GetResourceHandleByName(const std::string& name)
{
	if (!name.empty())
	{
		auto it = resourceNameMap_.find(name);
		if (it != resourceNameMap_.end() && IsResourceHandleValid(it->second))
			return it->second;
	}
	return g_invalidResourceHandle;
}

void ResourceManager::ReleaseResource(ResourceHandle handle)
{
	if (!IsResourceHandleValid(handle))
		return;
	u32 index = handle.index;

	generations_.at(index)++;

	freeList_.push_back(index);

	resources_.at(index) = ManagedResource{};
}

ResourceHandle ResourceManager::AllocateResourceHandle()
{
    u32 index;

    if (!freeList_.empty()) 
	{
        index = freeList_.back();
        freeList_.pop_back();
    }
    else 
	{
        index = static_cast<u32>(generations_.size());
        generations_.push_back(0);
    }

    return ResourceHandle{
        .index = index,
        .generation = generations_[index]
    };
}

u32 ResourceManager::GetResourceIndex(ResourceHandle handle) const
{
	return handle.index;
}

bool ResourceManager::IsResourceHandleValid(ResourceHandle handle) const
{
	return handle.index < generations_.size() && generations_[handle.index] == handle.generation;
}

void ResourceManager::CreateBindlessHeap()
{
	D3D12_DESCRIPTOR_HEAP_DESC srvTextureHeap{};
	srvTextureHeap.NumDescriptors = MAX_TEXTURES;
	srvTextureHeap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvTextureHeap.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	(gfxDevice_.device_->CreateDescriptorHeap(&srvTextureHeap, IID_PPV_ARGS(&bindlessHeap_)));
}