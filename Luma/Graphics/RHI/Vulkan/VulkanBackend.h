#pragma once
#include <array>
#include <vector>
#include <span>
#include <variant>
#include <optional>
#include <SDL_video.h>

#include "VMA/VmaUsage.h"

#include "Core/Common.h"
#include "Graphics/Globals.h"
#include "Graphics/RHI/RHITypes.h"

#include <vk-bootstrap/VkBootstrap.h>


namespace VulkanInternal
{
    // View definitions adapted for Vulkan
    struct VertexBufferView {
        VkBuffer buffer;
        u64 offset;
        u64 size; // size in bytes
        u32 stride;
    };

    struct IndexBufferView {
        VkBuffer buffer;
        u64 offset;
        u64 size;
        VkIndexType indexType; // VK_INDEX_TYPE_UINT16 or VK_INDEX_TYPE_UINT32
    };

    struct ConstantBufferView {
        VkBuffer buffer;
        u64 offset;
        u64 size;
    };

    struct StorageBufferView {
        VkBuffer buffer;
        u64 offset;
        u64 size;
    };

    struct TexelBufferView {
        VkBuffer buffer;
        u64 offset;
        u64 size;
        VkFormat format; 
    };

    using BufferView = std::variant<
        std::monostate,        // No view (raw buffer)
        VertexBufferView,
        IndexBufferView,
        ConstantBufferView,
        StorageBufferView,
        TexelBufferView
    >;

    struct BufferResource
    {
        std::optional<VertexBufferView> vertexView;
        std::optional<IndexBufferView> indexView;
        std::optional<ConstantBufferView> constantView;
        std::optional<TexelBufferView> srvView;
        std::optional<StorageBufferView> uavView;

        [[nodiscard]] const VertexBufferView* AsVertexBuffer() const
        {
            return vertexView ? &(*vertexView) : nullptr;
        }
        [[nodiscard]] const IndexBufferView* AsIndexBuffer() const
        {
            return indexView ? &(*indexView) : nullptr;
        }
        [[nodiscard]] const ConstantBufferView* AsConstantBuffer() const
        {
            return constantView ? &(*constantView) : nullptr;
        }
        [[nodiscard]] const TexelBufferView* AsShaderResourceView() const
        {
            return srvView ? &(*srvView) : nullptr;
        }
        [[nodiscard]] const StorageBufferView* AsUnorderedAccessView() const
        {
            return uavView ? &(*uavView) : nullptr;
        }

        VkBuffer buffer = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;
        u64 size = 0;
        VkDeviceAddress gpuAddress = 0;
        void* mappedData = nullptr;
    };

    // textures
    struct TextureResource
    {
        VkImage image = VK_NULL_HANDLE;
        VkImageView view = VK_NULL_HANDLE; // Default view
        VmaAllocation allocation = VK_NULL_HANDLE;

        u32 bindlessSampledIndex = 0xFFFFFFFF;
        u32 bindlessStorageIndex = 0xFFFFFFFF;

        RHIFormat format = RHIFormat::UNKNOWN;
    };

    // shaders
    struct ShaderResource
    {
        VkShaderModule module = VK_NULL_HANDLE;
        RHIShaderStage stage;
        std::string entryPoint;
    };

    // pipelines
    struct PipelineResource
    {
        VkPipeline pipeline = VK_NULL_HANDLE;
        VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
        VkPipelineBindPoint bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    };

    // Global Vulkan Context

    // using vkb objects for ease
    inline vkb::Instance g_vkbInstance;
    inline vkb::InstanceDispatchTable g_instanceDispatchTable;
    inline vkb::PhysicalDevice g_vkbPhysicalDevice;
    inline vkb::Device g_vkbDevice;
    inline vkb::DispatchTable g_disp;

    // raw vulkan handles
    inline VkInstance g_instance;
    inline VkPhysicalDevice g_physicalDevice;
    inline VkDevice g_device;

    inline VkQueue g_graphicsQueue = VK_NULL_HANDLE;
    inline u32 g_graphicsQueueFamily = 0;
    inline VmaAllocator g_allocator = VK_NULL_HANDLE;

    // Debug/Validation
    inline VkDebugUtilsMessengerEXT g_debugMessenger = VK_NULL_HANDLE;

    // Swapchain
    inline vkb::Swapchain g_vkbSwapchain;
    inline VkSurfaceKHR g_surface = VK_NULL_HANDLE;
    inline VkFormat g_swapchainFormat = VK_FORMAT_B8G8R8A8_UNORM;
    inline VkExtent2D g_swapchainExtent = { 0, 0 };
    inline std::vector<VkImage> g_swapchainImages;
    inline std::vector<VkImageView> g_swapchainImageViews;
    inline u32 g_currentSwapchainImageIndex = 0;

    // depth
    inline VkImage g_depthImage;
    inline VmaAllocation g_depthAllocation;
    inline VkImageView g_depthImageView;

    // raw vk swapchain
    inline VkSwapchainKHR g_swapchain;

    // Frame Sync
    inline u32 g_frameIndex = 0;
    inline std::vector<VkSemaphore> g_imageAvailableSemaphores;
    inline std::vector<VkSemaphore> g_renderFinishedSemaphores;
    inline std::vector<VkFence> g_inFlightFences;
    inline std::vector<VkFence> g_imageInFlight;

    // Bindless System
    inline VkDescriptorPool g_descriptorPool = VK_NULL_HANDLE;
    inline VkDescriptorSetLayout g_bindlessSetLayout = VK_NULL_HANDLE;
    inline VkDescriptorSet g_bindlessSet = VK_NULL_HANDLE;

    constexpr u32 kMaxBindlessTextures = 10000;
    constexpr u32 kMaxBindlessBuffers = 10000;

    // resource pools
    inline std::vector<BufferResource> g_buffers;
    inline std::vector<TextureResource> g_textures;
    inline std::vector<ShaderResource> g_shaders;
    inline std::vector<PipelineResource> g_pipelines;

    // sizes for samplers and resources
    inline VkDeviceSize g_resourceHeapSize;
    inline VkDeviceSize g_samplerHeapSize;

    // reuse lists
    inline std::vector<u32> g_freeBuffers;
    inline std::vector<u32> g_freeTextures;
    inline std::vector<u32> g_freeShaders;
    inline std::vector<u32> g_freePipelines;

    // generations
    inline std::vector<u8> g_generationsBuffers;
    inline std::vector<u8> g_generationsTextures;
    inline std::vector<u8> g_generationsShaders;
    inline std::vector<u8> g_generationsPipelines;

    inline PipelineHandle g_currentPipeline = {};

    struct ImmediateContext
    {
        VkCommandPool commandPool = VK_NULL_HANDLE;
        VkCommandBuffer commandBuffer = VK_NULL_HANDLE;

        VkSemaphore timelineSemaphore = VK_NULL_HANDLE;
        u64 fenceValue = 0;
    };
    inline ImmediateContext g_immediateContext;
}

struct VulkanBackend
{
    static void Init(SDL_Window* window, u32 width, u32 height);
    static void Shutdown();

    static void BeginFrame();
    static void EndFrame();
    static void WaitIdle();

    // Resource Creation
    static BufferHandle CreateBuffer(const RHIBufferDesc& desc, const void* initialData);
    static TextureHandle CreateTexture(const RHITextureDesc& desc, const void* initialData);
    static ShaderHandle CreateShader(const RHIShaderDesc& shaderDesc);
    static PipelineHandle CreateGraphicsPipeline(const RHIGraphicsPipelineDesc& desc);
    static PipelineHandle CreateComputePipeline(const RHIComputePipelineDesc& desc);

    // Getters
    static VulkanInternal::BufferResource* GetBuffer(BufferHandle handle);
    static VulkanInternal::TextureResource* GetTexture(TextureHandle handle);
    static VulkanInternal::ShaderResource* GetShader(ShaderHandle handle);
    static VulkanInternal::PipelineResource* GetPipeline(PipelineHandle handle);

    // Resource Destruction
    static void DestroyBuffer(BufferHandle handle);
    static void DestroyTexture(TextureHandle handle);
    static void DestroyShader(ShaderHandle handle);
    static void DestroyPipeline(PipelineHandle handle);

    // Bindless Accessors
    static i32 GetBindlessReadIndex(TextureHandle handle);
    static i32 GetBindlessWriteIndex(TextureHandle handle);
    static i32 GetBindlessReadIndex(BufferHandle handle);
    static i32 GetBindlessWriteIndex(BufferHandle handle);

    struct VulkanBackendCommandList
    {
        VkCommandPool commandPools[frameCount];
        VkCommandBuffer cmdBuffers[frameCount];
        u32 currentFrameIndex = 0;

        void Begin();
        void End();
        void Submit();

        void TextureBarrier(TextureHandle handle, RHIResourceState before, RHIResourceState after);
        void BufferBarrier(BufferHandle handle, RHIResourceState before, RHIResourceState after);

        void SetPipeline(PipelineHandle handle);
        void SetGraphicsPushConstants(const void* data, u32 size, u32 offset);
        void SetComputePushConstants(const void* data, u32 size, u32 offset);

        void BeginRendering(const std::vector<TextureHandle> colorTargets, TextureHandle depthTarget);
        void EndRendering();

        void SetViewport(const RHIViewPort& viewPort);
        void SetScissor(const RHIScissor& scissor);

        void Draw(u32 vertexCount, u32 instanceCount, u32 firstVertex, u32 firstInstance);
        void DrawIndexed(u32 indexCount, u32 instanceCount, u32 firstIndex, i32 vertexOffset, u32 firstInstance);
        void Dispatch(u32 x, u32 y, u32 z);

        void CopyBufferToTexture(BufferHandle src, TextureHandle dst);
        void CopyBuffer(BufferHandle src, BufferHandle dst, u64 size, u64 srcOffset, u64 dstOffset);

        void BindVertexBuffer(BufferHandle handle);
        void BindIndexBuffer(BufferHandle handle);

        bool isImmediate = false;
    };

    static VulkanBackendCommandList* CreateCommandList(bool isImmediate);
    static void DestroyCommandList(VulkanBackendCommandList* cl);

    // Helpers
    static VkFormat ConvertFormat(RHIFormat format);
    static VkImageLayout ConvertResourceState(RHIResourceState state);
    static VkCompareOp ConvertDepthFunc(RHIDepthFunc func);
    static VkPrimitiveTopology ConvertTopology(RHITopology topology);
};