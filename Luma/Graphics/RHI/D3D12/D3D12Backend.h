#pragma once
#include <array>
#include <d3d12.h>
#include <span>
#include <D3D12MemAlloc.h>
#include <dxcapi.h>
#include <optional>
#include <SDL_video.h>
#include <wrl/client.h>

#include "Core/Common.h"
#include "Graphics/Globals.h"
#include "Graphics/RHI/RHITypes.h"

using Microsoft::WRL::ComPtr;

namespace D3D12Internal
{
    // buffers
    struct VertexBufferView {
        D3D12_VERTEX_BUFFER_VIEW view;
        u32 stride;
    };

    struct IndexBufferView {
        D3D12_INDEX_BUFFER_VIEW view;
        DXGI_FORMAT indexFormat;  // R16 or R32
    };

    struct ConstantBufferView {
        D3D12_GPU_VIRTUAL_ADDRESS gpuAddress;
        u32 sizeInBytes;
        std::optional<u32> heapIndex;
    };
    struct UnorderedAccessView {
        D3D12_GPU_VIRTUAL_ADDRESS gpuAddress;
        u32 sizeInBytes;
        DXGI_FORMAT format;
        std::optional<u32> heapIndex;
    };

    struct ShaderResourceView {
        D3D12_GPU_VIRTUAL_ADDRESS gpuAddress;
        u32 sizeInBytes;
        DXGI_FORMAT format;
        std::optional<u32> heapIndex;
    };

    using BufferView = std::variant<
        std::monostate,        // No view (raw buffer)
        VertexBufferView,
        IndexBufferView,
        ConstantBufferView,
        UnorderedAccessView,
        ShaderResourceView
    >;

    struct BufferResource
    {
        std::optional<VertexBufferView> vertexView;
        std::optional<IndexBufferView> indexView;
        std::optional<ConstantBufferView> constantView;
        std::optional<ShaderResourceView> srvView;
        std::optional<UnorderedAccessView> uavView;

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
        [[nodiscard]] const ShaderResourceView* AsShaderResourceView() const
        {
            return srvView ? &(*srvView) : nullptr;
        }
        [[nodiscard]] const UnorderedAccessView* AsUnorderedAccessView() const
        {
            return uavView ? &(*uavView) : nullptr;
        }

        ComPtr<ID3D12Resource> resource;
        D3D12MA::Allocation* allocation = nullptr;
        u64 size = 0;
        u32 stride = 0;
        D3D12_GPU_VIRTUAL_ADDRESS gpuAddress = 0;
    };

    // textures
    struct TextureResource
    {
        ComPtr<ID3D12Resource> resource;
        D3D12MA::Allocation* allocation = nullptr;
        u32 bindlessSRVIndex = 0xFFFFFFFF;
        u32 bindlessUAVIndex = 0xFFFFFFFF;

        u32 rtvIndex;
    };

    // shaders
    struct DXCRes
    {
        ComPtr<IDxcUtils> pUtils;
        ComPtr<IDxcCompiler3> pCompiler;
        ComPtr<IDxcIncludeHandler> pIncludeHandler;
    };
    struct ShaderResource
    {
        ComPtr<IDxcBlob> pBlob;
        RHIShaderStage type;
    };

    // pipelines
    struct PipelineResource
    {
        ComPtr<ID3D12PipelineState> pso;
        ComPtr<ID3D12RootSignature> rootSignature;
        D3D12_PRIMITIVE_TOPOLOGY topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        bool isCompute = false;
    };

    // internal windows stuff
    inline ComPtr<IDXGIFactory2> g_factory;
    inline HWND g_hwnd;

    // global d3d12 stuff
    inline ComPtr<ID3D12Device14> g_device;
    inline ComPtr<D3D12MA::Allocator> g_allocator;
    inline ComPtr<ID3D12CommandQueue> g_commandQueue;
    inline ComPtr<ID3D12CommandAllocator> g_commandAllocators[frameCount];
    inline ComPtr<ID3D12DescriptorHeap> g_bindlessHeap;

    // framesync stuff
    inline u32 g_frameIndex = 0;
    inline HANDLE g_fenceEvent = INVALID_HANDLE_VALUE;
    inline ComPtr<ID3D12Fence> g_fence;
    inline std::array<u64, frameCount> g_fenceValues{ 0, 0 };

    // swapchain
    inline u32 g_width;
    inline u32 g_height;
    inline CD3DX12_VIEWPORT g_viewport;
    inline CD3DX12_RECT g_scissorRect;
    inline ComPtr<IDXGISwapChain4> g_swapchain;

    inline std::array<ComPtr<ID3D12Resource>, frameCount> g_renderTargets;
    inline ComPtr<ID3D12Resource> g_depthStencil;
    inline ComPtr<ID3D12DescriptorHeap> g_rtvHeap;
    inline ComPtr<ID3D12DescriptorHeap> g_dsvHeap;
    inline u32 g_rtvDescriptorSize = 0;
    inline u32 g_dsvDescriptorSize = 0;

    inline CD3DX12_CPU_DESCRIPTOR_HANDLE g_rtvHandle;
    inline CD3DX12_CPU_DESCRIPTOR_HANDLE g_dsvHandle;

    inline u32 g_bindlessDescriptorSize = 0;

    inline u32 g_nextBindlessIndex = 0;
    inline u32 g_nextRTVIndex = 0;
    inline u32 g_nextDSVIndex = 0;

    // immediate context stuff
    struct ImmediateContext
    {
        ComPtr<ID3D12CommandAllocator> cmdAllocator;
        ComPtr<ID3D12GraphicsCommandList1> commandList;
        ComPtr<ID3D12Fence> fence;
        u64 fenceValue = 0;
        HANDLE fenceEvent = nullptr;
    };

    inline ImmediateContext g_immediateContext ;

    enum class BindlessCategory : u8 
	{
        TextureLoad,  // ShaderRead
        TextureStore, // ShaderWrite
        BufferLoad,   // ShaderRead buffers, CB included
        BufferStore,  // ShaderWrite buffers
        //Sampler,      // samplers
        //TextureRTV    // RTV texture
    };
    // bindless slots per views start index
    // allocate atleast 1.5M heap size (1M will do for now)
    // TODO: actually use this shit dont write and forget
    constexpr u32 kMaxPerCategory = 250000;

    constexpr u32 kSampledTextureStart = 0;
    constexpr u32 kStorageTextureStart = kSampledTextureStart + kMaxPerCategory;
    constexpr u32 kSampledBufferStart = kStorageTextureStart + kMaxPerCategory;
    constexpr u32 kStorageBufferStart = kSampledBufferStart + kMaxPerCategory;
    //constexpr u32 kSamplerStart = kStorageBufferStart + kMaxPerCategory;
    //constexpr u32 kTextureRTVStart = kSamplerStart + kMaxPerCategory;

    // resource pools with handle-based indexing
    inline std::vector<BufferResource> g_buffers;
    inline std::vector<TextureResource> g_textures;

    // generations and free lists for handle reuse
    inline std::vector<u32> g_freeBuffers;
    inline std::vector<u32> g_freeTextures;

    inline std::vector<u8> g_generationsBuffers;
    inline std::vector<u8> g_generationsTextures;

    // shaders stuff
    inline DXCRes g_dxcRes{};
    inline std::vector<ShaderResource> g_shaders;
    inline std::vector<u32> g_freeShaders;
    inline std::vector<u8> g_generationsShaders;

    // pipelines stuff
    inline std::vector<PipelineResource> g_pipelines;
    inline std::vector<u32> g_freePipelines;
    inline std::vector<u8> g_generationsPipelines;

    // current bound pipeline
    inline PipelineHandle g_currentPipeline = {};
}

struct D3D12Backend
{
    static void Init(SDL_Window* window, u32 width, u32 height);
    static void Shutdown();

    static void BeginFrame();
    static void EndFrame();
    static void WaitIdle();

    static BufferHandle CreateBuffer(const RHIBufferDesc& desc, const void* initialData);
    static TextureHandle CreateTexture(const RHITextureDesc& desc, const void* initialData);
    static ShaderHandle CreateShader(const RHIShaderDesc& shaderDesc);
    static PipelineHandle CreateGraphicsPipeline(const RHIGraphicsPipelineDesc& desc);
    static PipelineHandle CreateComputePipeline(const RHIComputePipelineDesc& desc);

    // getters
    static D3D12Internal::BufferResource* GetBuffer(BufferHandle handle);
    static D3D12Internal::TextureResource* GetTexture(TextureHandle handle);
    static D3D12Internal::ShaderResource* GetShader(ShaderHandle handle);
    static D3D12Internal::PipelineResource* GetPipeline(PipelineHandle handle);

    // resource destruction
    static void DestroyBuffer(BufferHandle handle);
    static void DestroyTexture(TextureHandle handle);
    static void DestroyShader(ShaderHandle handle);
    static void DestroyPipeline(PipelineHandle handle);

    static i32 GetBindlessReadIndex(TextureHandle handle);
    static i32 GetBindlessWriteIndex(TextureHandle handle);

    static i32 GetBindlessReadIndex(BufferHandle handle);
    static i32 GetBindlessWriteIndex(BufferHandle handle);

    struct D3D12BackendCommandList
    {
        ComPtr<ID3D12GraphicsCommandList4> cmdLists[frameCount];
        ComPtr<ID3D12CommandAllocator> commandAllocators[frameCount];

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
    static D3D12BackendCommandList* CreateCommandList(bool isImmediate);
    static void DestroyCommandList(D3D12BackendCommandList* cl);

    static void MoveToNextFrame();
    static DXGI_FORMAT ConvertFormat(RHIFormat format);
    static D3D12_RESOURCE_STATES ConvertResourceState(RHIResourceState state);
    static D3D12_COMPARISON_FUNC ConvertDepthFunc(RHIDepthFunc func);
    static D3D12_PRIMITIVE_TOPOLOGY ConvertTopology(RHITopology topology);
};