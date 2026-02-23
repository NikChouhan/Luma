#include "VulkanBackend.h"
#include <SDL2/SDL_vulkan.h>

using namespace VulkanInternal;

void VulkanBackend::Init(SDL_Window* window, u32 width, u32 height)
{
	if(volkInitialize() == VK_SUCCESS)
		printl(Log::LogLevel::Error, "[Core] Volk works!");
	else
	{
		printl(Log::LogLevel::Error, "[VULKAN] Volk fails!");
		return;
	}

	uint32_t extensionCount;
	SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, nullptr);
	std::vector<const char*> extensionNames(extensionCount);
	SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, extensionNames.data());

	// instance
	vkb::InstanceBuilder builder;
	auto inst_ret = builder.set_app_name("Luma")
		.request_validation_layers()
		.use_default_debug_messenger()
		.enable_extensions(extensionCount, extensionNames.data())
		.build();

	if (!inst_ret)
		printl(Log::LogLevel::Error, "[Core] Vulkan instance creation failed!");
	vkb::Instance vkb_inst = inst_ret.value();

	g_instance = vkb_inst.instance;
	// volk instance load
	volkLoadInstance(vkb_inst.instance);

	// surface
	VkSurfaceKHR surface = VK_NULL_HANDLE;
	if (SDL_FALSE == SDL_Vulkan_CreateSurface(window, vkb_inst.instance, &surface))
	{
		printl(Log::LogLevel::Error,"[Core] Failed to create surface, SDL Error: %s", SDL_GetError());
	}

	// physical device
	vkb::PhysicalDeviceSelector selector{ vkb_inst };

	constexpr char extensions[] = {
		"VK_EXT_mutable_descriptor_type"
	};
	// descriptor heap extension
	VkPhysicalDeviceDescriptorHeapFeaturesEXT heapFeatures;
	heapFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_HEAP_FEATURES_EXT;
	heapFeatures.descriptorHeap = true;
	heapFeatures.descriptorHeapCaptureReplay = true;

	VkPhysicalDeviceDescriptorHeapPropertiesEXT heapProps = {
	.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_HEAP_PROPERTIES_EXT
	};

	g_samplerHeapSize = heapProps.maxSamplerHeapSize;
	g_resourceHeapSize = heapProps.maxResourceHeapSize;

	// other features
	VkPhysicalDeviceScalarBlockLayoutFeatures scalarBlockLayoutFeatures{};
	scalarBlockLayoutFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SCALAR_BLOCK_LAYOUT_FEATURES;
	scalarBlockLayoutFeatures.scalarBlockLayout = true;

	VkPhysicalDeviceDescriptorIndexingFeatures bindless{};
	bindless.shaderSampledImageArrayNonUniformIndexing = true;
	bindless.descriptorBindingPartiallyBound = true;
	bindless.runtimeDescriptorArray = true;
	bindless.descriptorBindingVariableDescriptorCount = true;
	bindless.descriptorBindingSampledImageUpdateAfterBind = true;

	VkPhysicalDeviceFeatures features{};
	features.fragmentStoresAndAtomics = true;
	features.shaderInt64 = true;
	features.shaderInt16 = true;
	features.samplerAnisotropy = true;

	VkPhysicalDeviceVulkan12Features vulkan12Features{};
	vulkan12Features.bufferDeviceAddress = true;
	vulkan12Features.timelineSemaphore = true;

	VkPhysicalDeviceVulkan13Features vulkan13Features{};
	vulkan13Features.synchronization2 = true;
	vulkan13Features.dynamicRendering = true;

	vulkan13Features.pNext = &bindless;
	bindless.pNext = &scalarBlockLayoutFeatures;

	auto phy_ret = selector.set_surface(g_surface)
		.set_minimum_version(1, 4)
		.require_dedicated_transfer_queue()
		.add_required_extension(extensions)
		.set_required_features(features)
		.set_required_features_12(vulkan12Features)
		.set_required_features_13(vulkan13Features)
		.add_required_extension_features(heapFeatures)
		.select();

	if (!phy_ret)
		printl(Log::LogLevel::Error, "[Core] Vulkan physical device selection failed!");

	// logical device
	vkb::DeviceBuilder deviceBuilder{ phy_ret.value() };
	auto dev_ret = deviceBuilder.build();

	// load all entrypoints directly from driver
	volkLoadDevice(dev_ret.value().device);

	g_physicalDevice = dev_ret.value().physical_device;
	g_device = dev_ret.value().device;

	if (!dev_ret)
		printl(Log::LogLevel::Error, "[Core] Vulkan logical device creation failed!");
	vkb::Device vkb_device = dev_ret.value();

	// graphics queue (will be using the same queue for present as well, kinda stupid not to)
	auto graphicsQueueRet = vkb_device.get_queue(vkb::QueueType::graphics);
	if (!graphicsQueueRet)
		printl(Log::LogLevel::Error, "[Core] Vulkan queue creation failed!");

	g_graphicsQueue = graphicsQueueRet.value();

	// VMA stuff
	{
		VmaAllocatorCreateInfo allocatorCreateInfo = {};
		allocatorCreateInfo.physicalDevice = g_physicalDevice;
		allocatorCreateInfo.device = g_device;
		allocatorCreateInfo.instance = g_instance;
		allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_4;
		allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

		VmaVulkanFunctions vulkanFunctions;
		RHI_ASSERT(vmaImportVulkanFunctionsFromVolk(&allocatorCreateInfo, &vulkanFunctions));

		allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;
		RHI_ASSERT(vmaCreateAllocator(&allocatorCreateInfo, &g_allocator));
	}

	// create swapchain
	vkb::SwapchainBuilder swapchainBuilder{ vkb_device };
	auto swap_ret = swapchainBuilder.set_old_swapchain(g_vkbSwapchain).build();

	if (!swap_ret)
		printl(Log::LogLevel::Error, "[Core] Swapchain init failed with error, {} {}", swap_ret.error().message(), swap_ret.vk_result());

	vkb::destroy_swapchain(g_vkbSwapchain);
	g_vkbSwapchain = swap_ret.value();

	g_swapchain = g_vkbSwapchain.swapchain;

	// frame sync
	g_imageAvailableSemaphores.resize(frameCount);
	g_renderFinishedSemaphores.resize(g_vkbSwapchain.image_count);
	g_inFlightFences.resize(frameCount);
	g_imageInFlight.resize(g_vkbSwapchain.image_count, VK_NULL_HANDLE);

	VkSemaphoreCreateInfo semaphoreCreateInfo{};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceCreateInfo{};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < frameCount; i++)
	{
		// TODO: Replace with RHI_ASSERT
		if (g_disp.createSemaphore(&semaphoreCreateInfo, nullptr, &g_imageAvailableSemaphores[i]) != VK_SUCCESS
		|| g_disp.createFence(&fenceCreateInfo, nullptr, &g_inFlightFences[i]) != VK_SUCCESS)
		{
			printl(Log::LogLevel::Error, "[Core] Failed to create sync objects!");
			return;
		}
	}

	// depth buffer stuff
	{
		VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;

		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = width;
		imageInfo.extent.height = height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.format = depthFormat;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocInfo{};
		allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

		RHI_ASSERT(vmaCreateImage(g_allocator, &imageInfo, &allocInfo,
			&g_depthImage, &g_depthAllocation, nullptr));

		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = g_depthImage;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = depthFormat;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		vkCreateImageView(g_device, &viewInfo, nullptr, &g_depthImageView);
	}

	// bindless stuff
	{
		
	}
}
