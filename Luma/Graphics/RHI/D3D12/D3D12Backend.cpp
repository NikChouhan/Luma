#include "D3D12Backend.h"

#include <dxgi1_6.h>
#include <d3dx12/d3dx12.h>

#include "Core/Common.h"
#include "Graphics/RHI/RHI.h"
#include <SDL_syswm.h>

using namespace D3D12Internal;

// forward decs
static D3D12Internal::BufferResource CreateBufferResource(const RHIBufferDesc& desc);
static D3D12Internal::TextureResource CreateTextureResource(
    D3D12MA::Allocator* allocator,
    const D3D12_RESOURCE_FLAGS resourceFlags,
    const RHITextureDesc& desc,
    const void* initialData);
static u32 CreateTextureUAV(
    u32* nextIndex,
    const ComPtr<ID3D12Resource>& resource,
    const RHITextureDesc& desc,
    u32 mipLevel);
static u32 CreateTextureRTV(
    ID3D12DescriptorHeap* rtvHeap,  // Note: Separate RTV heap!
    u32* nextRTVIndex,
    const ComPtr<ID3D12Resource>& resource,
    const RHITextureDesc& desc);
static u32 CreateTextureSRV(
    u32* nextIndex,
    const ComPtr<ID3D12Resource>& resource,
    const RHITextureDesc& desc);


struct BufferViewCreator;

static BufferHandle AssignBufferHandle();
static TextureHandle AssignTextureHandle();
static ShaderHandle AssignShaderHandle();
static PipelineHandle AssignPipelineHandle();

static bool IsBufferHandleValid(BufferHandle handle);
static bool IsTextureHandleValid(TextureHandle handle);
static bool IsShaderHandleValid(ShaderHandle handle);
static bool IsPipelineHandleValid(PipelineHandle handle);

static void GetHardwareAdapter(
    IDXGIFactory1* pFactory,
    IDXGIAdapter1** ppAdapter,
    bool requestHighPerformanceAdapter);
static void IsDirectXRayTracingSuppported(ID3D12Device14* device);
static ComPtr<ID3D12RootSignature> CreateRootSignatureFromBlob(IDxcBlob* shaderBlob);
static std::vector<D3D12_INPUT_ELEMENT_DESC> CreateInputElementDesc(const std::vector<RHIInputBindingDesc>& bindingDesc,
    const std::vector<RHIInputAttributeDesc>& attributeDesc,
    std::vector<std::string>& outOwnedSemanticNames);

namespace D3D12Internal
{
    struct BufferSizeVisitor
    {
        u32 operator()(const RHIVertexBufferCreateInfo& createInfo) const
        {
            return createInfo.vertexCount * createInfo.vertexStride;
        }
        u32 operator()(const RHIIndexBufferCreateInfo& createInfo) const
        {
            u32 indexSize = (createInfo.format == RHIFormat::R16_UINT) ? 2 : 4;
            return createInfo.indexCount * indexSize;
        }
        u32 operator()(const RHIConstantBufferCreateInfo& createInfo) const
        {
            // Align to 256 bytes for CBVs
            return (createInfo.sizeInBytes + 255) & ~255; // last bits of 255 in binary are flipped to 0 with ~ op,
            // which when &ed with something > 255 give multiple of 256
        }
        u32 operator()(const RHIStructuredBufferCreateInfo& createInfo) const
        {
            return createInfo.elementCount * createInfo.elementStride;
        }
        u32 operator()(const RHIRawBufferCreateInfo& createInfo) const
        {
            return createInfo.sizeInBytes;
        }
    };
    u32 GetBufferSize(const RHIBufferCreateInfo& createInfo)
    {
        return std::visit(BufferSizeVisitor{}, createInfo);
    }
    struct BufferUpdateVisitor
    {
        void* mappedData;

        void operator()(const RHIVertexBufferCreateInfo& createInfo) const
        {
            u32 size = GetBufferSize(createInfo);
            memcpy(mappedData, createInfo.vertices, size);
        }
        void operator()(const RHIIndexBufferCreateInfo& desc) const
        {
            u32 size = GetBufferSize(desc);
            memcpy(mappedData, desc.indices, size);
        }
        void operator()(const RHIConstantBufferCreateInfo& desc) const
        {
            if (desc.data)
            {
                u32 size = GetBufferSize(desc);
                memcpy(mappedData, desc.data, size);
            }
        }
        void operator()(const RHIStructuredBufferCreateInfo& desc) const
        {
            if (desc.data)
            {
                u32 size = GetBufferSize(desc);
                memcpy(mappedData, desc.data, size);
            }
        }
        void operator()(const RHIRawBufferCreateInfo& desc) const
        {
            if (desc.data)
            {
                u32 size = GetBufferSize(desc);
                memcpy(mappedData, desc.data, size);
            }
        }
    };

    struct BufferFormatVisitor
    {
        DXGI_FORMAT operator()(const RHIVertexBufferCreateInfo& createInfo) const
        {
            return DXGI_FORMAT_UNKNOWN;
        }
        DXGI_FORMAT operator()(const RHIIndexBufferCreateInfo& createInfo) const
        {
            DXGI_FORMAT format{};
            switch (createInfo.format)
            {
            case RHIFormat::R32_UINT: format = DXGI_FORMAT_R32_UINT;
                break;
            case RHIFormat::R16_UINT: format = DXGI_FORMAT_R16_UINT;
                break;
            default: format = DXGI_FORMAT_R32_UINT;
                break;
            }
            return format;
        }
        DXGI_FORMAT operator()(const RHIConstantBufferCreateInfo& createInfo) const
        {
            return DXGI_FORMAT_UNKNOWN;
        }
        DXGI_FORMAT operator()(const RHIStructuredBufferCreateInfo& createInfo) const
        {
            return DXGI_FORMAT_UNKNOWN;
        }
        DXGI_FORMAT operator()(const RHIRawBufferCreateInfo& createInfo) const
        {
            return DXGI_FORMAT_R32_TYPELESS;
        }
    };

    DXGI_FORMAT GetBufferFormat(const RHIBufferCreateInfo& createInfo)
    {
        return std::visit(BufferFormatVisitor{}, createInfo);
    }
}

struct BufferViewCreator
{
    D3D12Internal::BufferResource& buffer;
    ID3D12Device* device;
    ID3D12DescriptorHeap* heap;
    RHIResourceView view;

    void operator()(const RHIVertexBufferCreateInfo& createInfo) const
    {
        D3D12_VERTEX_BUFFER_VIEW vbv{};
        vbv.BufferLocation = buffer.resource->GetGPUVirtualAddress();
        vbv.StrideInBytes = createInfo.vertexStride;
        vbv.SizeInBytes = D3D12Internal::GetBufferSize(createInfo);

        buffer.vertexView = D3D12Internal::VertexBufferView{ .view = vbv, .stride = createInfo.vertexStride };
    }

    void operator()(const RHIIndexBufferCreateInfo& createInfo) const
    {
        D3D12_INDEX_BUFFER_VIEW ibv{};
        ibv.BufferLocation = buffer.resource->GetGPUVirtualAddress();
        ibv.Format = D3D12Internal::GetBufferFormat(createInfo);
        ibv.SizeInBytes = D3D12Internal::GetBufferSize(createInfo);

        buffer.indexView = D3D12Internal::IndexBufferView{ .view = ibv, .indexFormat = ibv.Format };
    }

    void operator()(const RHIConstantBufferCreateInfo& createInfo) const
    {
        std::optional<u32> heapIdx = std::nullopt;
        if (createInfo.createView && heap)
        {
            heapIdx = (GlobalStorage::bindlessHeapIndex.nextIndex)++;
            u32 descriptorSize = device->GetDescriptorHandleIncrementSize(
                D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
            cbvDesc.BufferLocation = buffer.resource->GetGPUVirtualAddress();
            cbvDesc.SizeInBytes = D3D12Internal::GetBufferSize(createInfo);

            CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(heap->GetCPUDescriptorHandleForHeapStart());
            cpuHandle.Offset(*heapIdx, descriptorSize);
            device->CreateConstantBufferView(&cbvDesc, cpuHandle);
        }

        buffer.constantView = D3D12Internal::ConstantBufferView{ .gpuAddress = buffer.resource->GetGPUVirtualAddress(),
            .sizeInBytes = D3D12Internal::GetBufferSize(createInfo),
            .heapIndex = *heapIdx };
    }
    void operator()(const RHIStructuredBufferCreateInfo& createInfo) const
    {
        if (heap)
        {
            u32 descriptorSize = device->GetDescriptorHandleIncrementSize(
                D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

            D3D12_CPU_DESCRIPTOR_HANDLE baseHandle =
                heap->GetCPUDescriptorHandleForHeapStart();

            // always create SRVs
            std::optional<u32> srvIndex = std::nullopt;
            srvIndex = (GlobalStorage::bindlessHeapIndex.nextIndex)++;

            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Format = DXGI_FORMAT_UNKNOWN;
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srvDesc.Buffer.FirstElement = 0;
            srvDesc.Buffer.NumElements = createInfo.elementCount;
            srvDesc.Buffer.StructureByteStride = createInfo.elementStride;

            CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(baseHandle);
            srvHandle.Offset(*srvIndex, descriptorSize);

            device->CreateShaderResourceView(
                buffer.resource.Get(), &srvDesc, srvHandle);

            buffer.srvView = D3D12Internal::ShaderResourceView{
            buffer.resource->GetGPUVirtualAddress(),
            D3D12Internal::GetBufferSize(createInfo),
            DXGI_FORMAT_UNKNOWN,
            srvIndex
            };

            // Create UAV if requested
            if (HasFlag(view, RHIResourceView::STORE))
            {
                std::optional<u32> uavIndex = std::nullopt;
                uavIndex = (GlobalStorage::bindlessHeapIndex.nextIndex)++;

                D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
                uavDesc.Format = DXGI_FORMAT_UNKNOWN;
                uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
                uavDesc.Buffer.FirstElement = 0;
                uavDesc.Buffer.NumElements = createInfo.elementCount;
                uavDesc.Buffer.StructureByteStride = createInfo.elementStride;

                CD3DX12_CPU_DESCRIPTOR_HANDLE uavHandle(baseHandle);
                uavHandle.Offset(*uavIndex, descriptorSize);

                device->CreateUnorderedAccessView(
                    buffer.resource.Get(), nullptr, &uavDesc, uavHandle);

                buffer.uavView = D3D12Internal::UnorderedAccessView{
                buffer.resource->GetGPUVirtualAddress(),
                D3D12Internal::GetBufferSize(createInfo),
                DXGI_FORMAT_UNKNOWN,
                uavIndex
                };
            }
        }
    }

    void operator()(const RHIRawBufferCreateInfo& createInfo) const
    {
        if (heap)
        {
            u32 descriptorSize = device->GetDescriptorHandleIncrementSize(
                D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

            D3D12_CPU_DESCRIPTOR_HANDLE baseHandle =
                heap->GetCPUDescriptorHandleForHeapStart();

            {
                std::optional<u32> srvIndex = std::nullopt;
                srvIndex = (GlobalStorage::bindlessHeapIndex.nextIndex)++;

                u32 elementSize = 4;  // For R32 formats
                u32 numElements = createInfo.sizeInBytes / elementSize;

                D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
                srvDesc.Format = D3D12Internal::GetBufferFormat(createInfo);
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
                srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                srvDesc.Buffer.FirstElement = 0;
                srvDesc.Buffer.NumElements = numElements;
                srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;

                CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(baseHandle);
                srvHandle.Offset(*srvIndex, descriptorSize);

                device->CreateShaderResourceView(
                    buffer.resource.Get(), &srvDesc, srvHandle);

                buffer.srvView = D3D12Internal::ShaderResourceView{
                buffer.resource->GetGPUVirtualAddress(),
                createInfo.sizeInBytes,
                srvDesc.Format,
                srvIndex
                };
            }

            if (HasFlag(view, RHIResourceView::STORE))
            {
                std::optional<u32> uavIndex = std::nullopt;
                uavIndex = (GlobalStorage::bindlessHeapIndex.nextIndex)++;

                u32 elementSize = 4;
                u32 numElements = createInfo.sizeInBytes / elementSize;

                D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
                uavDesc.Format = D3D12Internal::GetBufferFormat(createInfo);
                uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
                uavDesc.Buffer.FirstElement = 0;
                uavDesc.Buffer.NumElements = numElements;
                uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;

                CD3DX12_CPU_DESCRIPTOR_HANDLE uavHandle(baseHandle);
                uavHandle.Offset(*uavIndex, descriptorSize);

                device->CreateUnorderedAccessView(
                    buffer.resource.Get(), nullptr, &uavDesc, uavHandle);

                buffer.uavView = D3D12Internal::UnorderedAccessView{
               buffer.resource->GetGPUVirtualAddress(),
               createInfo.sizeInBytes,
               uavDesc.Format,
               uavIndex
                };
            }
        }
    }
};

void D3D12Backend::Init(SDL_Window* window, u32 width, u32 height)
{
    SDL_SysWMinfo wmInfo; SDL_GetWindowWMInfo(window, &wmInfo); 
    g_hwnd = wmInfo.info.win.window;
    g_width = width;
    g_height = height;
    g_viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0., 1.);
    g_scissorRect = CD3DX12_RECT(0, 0, static_cast<LONG>(width), static_cast<LONG>(height));

    u32 dxgiFactoryFlags = 0;

#ifdef DEBUG
    ComPtr<ID3D12Debug6> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
    {
        debugController->EnableDebugLayer();
        dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
    }
#endif
    RHI_ASSERT(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&D3D12Internal::g_factory)));

    ComPtr<IDXGIAdapter1> hwAdapter;
    GetHardwareAdapter(g_factory.Get(), &hwAdapter, true);

    RHI_ASSERT(D3D12CreateDevice(hwAdapter.Get(), D3D_FEATURE_LEVEL_12_2, IID_PPV_ARGS(&D3D12Internal::g_device)));
    // ray tracing stuff
    IsDirectXRayTracingSuppported(g_device.Get());
#ifdef DEBUG
    ComPtr<ID3D12DebugDevice2> debugDevice;
    RHI_ASSERT(D3D12Internal::g_device->QueryInterface(IID_PPV_ARGS(&debugDevice)));
#endif

    // create d3d12 memory allocator
    D3D12MA::ALLOCATOR_DESC allocatorDesc{};
    allocatorDesc.pDevice = g_device.Get();
    allocatorDesc.pAdapter = hwAdapter.Get();

    RHI_ASSERT(D3D12MA::CreateAllocator(&allocatorDesc, &D3D12Internal::g_allocator));

    // describe and create command queue
    D3D12_COMMAND_QUEUE_DESC queueDesc{};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    RHI_ASSERT(D3D12Internal::g_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&D3D12Internal::g_commandQueue)));

    for (u32 i = 0; i < frameCount; i++)
    {
        RHI_ASSERT(D3D12Internal::g_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&D3D12Internal::g_commandAllocators[i])));
    }

    CD3DX12FeatureSupport features;
    features.Init(g_device.Get());
    D3D_SHADER_MODEL shaderModel = features.HighestShaderModel();   // shader_model_6_7 for me
    
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
    // swapchain
    {
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.BufferCount = frameCount;

        swapChainDesc.Width = width;
        swapChainDesc.Height = height;
        swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
        swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.SampleDesc.Count = 1;

        ComPtr<IDXGISwapChain1> swapChain;
        RHI_ASSERT(g_factory->CreateSwapChainForHwnd(
            g_commandQueue.Get(),        // Swap chain needs the queue so that it can force a flush on it.
            g_hwnd,
            &swapChainDesc,
            nullptr,
            nullptr,
            &swapChain
        ));

        RHI_ASSERT(g_factory->MakeWindowAssociation(g_hwnd, DXGI_MWA_NO_ALT_ENTER));
        RHI_ASSERT(swapChain.As(&g_swapchain));
    	
    	// swapchain heap
        // know that you can place other RTV resource here as well, and use with bindless
        // note: changing heap mid way is not good, prefer using OmSetRenderTargets not bindless
        heapDesc.NumDescriptors = 8;
        heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        g_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&g_rtvHeap));
        g_rtvDescriptorSize = g_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        // create rtvs for backbuffer
        {
            D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
            rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

            CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(g_rtvHeap->GetCPUDescriptorHandleForHeapStart());
            for (u32 i = 0; i < frameCount; ++i)
            {
                RHI_ASSERT(g_swapchain->GetBuffer(i, IID_PPV_ARGS(&g_renderTargets[i])));
                g_device->CreateRenderTargetView(g_renderTargets[i].Get(), &rtvDesc, rtvHandle);
                rtvHandle.Offset(1, g_rtvDescriptorSize);
            }
        }
    }

    // depth stuff
    {
        // depth buffer DSV heap
        D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc{};
        dsvHeapDesc.NumDescriptors = 1;
        dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        RHI_ASSERT(g_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&g_dsvHeap)));

        D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc{};
        depthStencilViewDesc.Format = DXGI_FORMAT_D32_FLOAT;
        depthStencilViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        depthStencilViewDesc.Flags = D3D12_DSV_FLAG_NONE;

        D3D12_CLEAR_VALUE depthOptimisedClearValue{};
        depthOptimisedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
        depthOptimisedClearValue.DepthStencil.Depth = 0.f;
        depthOptimisedClearValue.DepthStencil.Stencil = 0;

        const CD3DX12_HEAP_PROPERTIES depthStencilHeapProps(D3D12_HEAP_TYPE_DEFAULT);
        const CD3DX12_RESOURCE_DESC depthStencilTextureDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT,
            u64(width), u64(height),
            1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

        RHI_ASSERT(g_device->CreateCommittedResource(
            &depthStencilHeapProps,
            D3D12_HEAP_FLAG_NONE,
            &depthStencilTextureDesc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &depthOptimisedClearValue,
            IID_PPV_ARGS(&g_depthStencil)));

        g_device->CreateDepthStencilView(g_depthStencil.Get(), &depthStencilViewDesc,
            g_dsvHeap->GetCPUDescriptorHandleForHeapStart());
    }

    // bindless heap
    {
        heapDesc.NumDescriptors = 1'000'000;
        heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        g_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&D3D12Internal::g_bindlessHeap));
        g_bindlessDescriptorSize = g_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }
    // framesync stuff
    {
        RHI_ASSERT(g_device->CreateFence(g_fenceValues[g_frameIndex],
            D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_fence)));
        g_fenceValues[g_frameIndex]++;

        // create event handle to use for frame sync
        g_fenceEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
        if (g_fenceEvent == nullptr)
        {
            RHI_ASSERT(HRESULT_FROM_WIN32(GetLastError()));
        }
    }
    // shaders dxc init
    {
        RHI_ASSERT(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(g_dxcRes.pUtils.GetAddressOf())));
        RHI_ASSERT(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(g_dxcRes.pCompiler.GetAddressOf())));
        RHI_ASSERT(g_dxcRes.pUtils->CreateDefaultIncludeHandler(g_dxcRes.pIncludeHandler.GetAddressOf()));
    }

    // immediate context
    {
        RHI_ASSERT(g_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
            IID_PPV_ARGS(&g_immediateContext.cmdAllocator)));

        RHI_ASSERT(g_device->CreateCommandList(
            0,
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            g_immediateContext.cmdAllocator.Get(),
            nullptr,
            IID_PPV_ARGS(&g_immediateContext.commandList)));

        RHI_ASSERT(g_immediateContext.commandList->Close());

        RHI_ASSERT(g_device->CreateFence(0,
            D3D12_FENCE_FLAG_NONE,
            IID_PPV_ARGS(&g_immediateContext.fence)));
        g_immediateContext.fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

        if (!g_immediateContext.fenceEvent)
        {
            printl(Log::LogLevel::Error, "Failed to create fence event, for Immediate context!");
            abort();
        }
    }

    // reserve space for all the resources, shaders, pipelines
    g_buffers.resize(1'000);
    g_textures.resize(1'000);
    g_shaders.resize(400);
    g_pipelines.resize(200);
}

void D3D12Backend::Shutdown()
{
    // TODO: delete resources
}

void D3D12Backend::BeginFrame()
{
    WaitIdle();
}

void D3D12Backend::EndFrame()
{
    MoveToNextFrame();
}


void D3D12Backend::WaitIdle()
{

    /* commandQueue->Signal processes the GPU side command to set the fence to the fence value at the particular index
     * then fence->SetEventOnCompletion sets the event the same (set the fence value at frame index)
     * but on the CPU. The next command (WaitForSingleObjectEx) waits for the fence event CPU side to complete in the GPU side.
     * We basically linked the GPU and the CPU side with an event and a fence. Then increase the fence value for the next frame
    */

    /* This is more imp info:
    * what Signal command does is it pushes the value of the fenceValue (for the particular frame index) at the end of the
    * command queue. This ensures that all the commands submitted prior execute and then the value of the fence is set to
    * the submitted fence value
    */

    // schedule signal command in the queue
    RHI_ASSERT(g_commandQueue->Signal(g_fence.Get(), g_fenceValues[g_frameIndex]));

    // wait until the fence has been processed
    /* the fence->SetEventOnCompletion fn checks the value of the fence with the fenceValue and stalls the CPU main thread
    * until the value is reached
    */
    RHI_ASSERT(g_fence->SetEventOnCompletion(g_fenceValues[g_frameIndex], g_fenceEvent));
    WaitForSingleObjectEx(g_fenceEvent, INFINITE, FALSE);

    // increment the fence value for the current frame
    g_fenceValues[g_frameIndex]++;
}

BufferHandle D3D12Backend::CreateBuffer(const RHIBufferDesc& desc, const void* initialData)
{


    // does buffer creation
    BufferResource resource = CreateBufferResource(desc);
    // buffer view
    std::visit(BufferViewCreator{
	    .buffer = resource,
	    .device = g_device.Get(),
	    .heap = g_bindlessHeap.Get(),
	    .view = desc.view }, desc.createInfo);

	BufferHandle handle = AssignBufferHandle();

    g_buffers.at(handle.index) = resource;

    return handle;
}

TextureHandle D3D12Backend::CreateTexture(const RHITextureDesc& desc, const void* initialData)
{
    // resource flags
    D3D12_RESOURCE_FLAGS resourceFlags = D3D12_RESOURCE_FLAG_NONE;
    if (HasFlag(desc.view, RHIResourceView::RENDER_TARGET))
    {
        resourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    }
    if (HasFlag(desc.view, RHIResourceView::STORE))
    {
        resourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }

    // texture creation
    TextureResource textureRes = CreateTextureResource(g_allocator.Get(), resourceFlags, desc, initialData);

    // texture views
    if (HasFlag(desc.view, RHIResourceView::LOAD))
    {
        textureRes.bindlessSRVIndex = CreateTextureSRV(
            &GlobalStorage::bindlessHeapIndex.nextIndex,
            textureRes.resource,
            desc);
    }
    if (HasFlag(desc.view, RHIResourceView::STORE))
    {
        // TODO: impl a nice version later when needed
        //if (desc.createPerMipViews)
        //{
        //    // create UAV for each mip level
        //    for (u32 mip = 0; mip < desc.mips; ++mip)
        //    {
        //        u32 uavIdx = CreateTextureUAV(
        //        &GlobalStorage::bindlessHeapIndex.nextIndex,
        //            textureRes.resource,
        //            desc,
        //            mip);
        //        textureRes.mipUAVIndices.push_back(uavIdx);
        //    }
        //    texture.uavIndex = texture.mipUAVIndices[0];
        //}
        //else
        {
            textureRes.bindlessUAVIndex = CreateTextureUAV(
                &GlobalStorage::bindlessHeapIndex.nextIndex,
                textureRes.resource,
                desc,
                0);
        }
    }
    TextureHandle handle = AssignTextureHandle();

    g_textures[handle.index] = textureRes;

    return handle;
}

ShaderHandle D3D12Backend::CreateShader(const RHIShaderDesc& shaderDesc)
{


	ShaderResource shader;
	shader.type = shaderDesc.stage;

    const wchar_t* path = shaderDesc.path;
    const wchar_t* filename = wcsrchr(path, L'/');
    if (filename != nullptr) filename++;
    else filename = path;

    std::wstring binName = std::wstring(filename) + L".bin";
    std::wstring pdbName = std::wstring(filename) + L".pdb";

    std::wstring shaderPathOnly;
    if (filename != nullptr && filename != path)
    {
        size_t pathLength = (filename - path);
        shaderPathOnly = std::wstring(path, pathLength);
    }
    else
    {
        shaderPathOnly = L"";
    }
    const wchar_t* emp = L"";
    LPCWSTR pszArgs[] =
    {
        shaderDesc.path,
        L"-E", shaderDesc.entryPoint,
        L"-T", shaderDesc.target,
        L"-Zi",
        //L"-Zss",
        L"-Zsb",
        L"-Fo", binName.c_str(),
        L"-Fd", emp
    };
    DxcBuffer source;
    ComPtr<IDxcBlobEncoding> pBlobEnc;
    ComPtr<IDxcResult> compileResult;
    // open source file
    RHI_ASSERT(g_dxcRes.pUtils->LoadFile(shaderDesc.path, nullptr, pBlobEnc.GetAddressOf()));
    source.Ptr = pBlobEnc->GetBufferPointer();
    source.Size = pBlobEnc->GetBufferSize();
    source.Encoding = DXC_CP_ACP;

    // compile it with the arguments
    RHI_ASSERT(g_dxcRes.pCompiler->Compile(&source,
        pszArgs,
        _countof(pszArgs),
        g_dxcRes.pIncludeHandler.Get(),
        IID_PPV_ARGS(&compileResult)));

    ComPtr<IDxcBlobUtf16> pShaderName = nullptr;

    HRESULT resultCode;
    compileResult->GetStatus(&resultCode);
    if (FAILED(resultCode))
    {
        ComPtr<IDxcBlobEncoding> pError;
        if (SUCCEEDED(compileResult->GetErrorBuffer(&pError)) && pError)
        {
            OutputDebugStringA("\n--- SHADER COMPILATION ERROR ---\n");
            printl(Log::LogLevel::Error, "Errors: {}", (const char*)pError->GetBufferPointer());
            OutputDebugStringA("\n--------------------------------\n");
        }
        RHI_ASSERT(false);
    }

    RHI_ASSERT(compileResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shader.pBlob), &pShaderName));
    if (shader.pBlob != nullptr)
    {
        FILE* fp = NULL;

        _wfopen_s(&fp, pShaderName->GetStringPointer(), L"wb");
        fwrite(shader.pBlob->GetBufferPointer(), shader.pBlob->GetBufferSize(), 1, fp);
        fclose(fp);
    }

    ComPtr<IDxcBlob> pPDB = nullptr;
    ComPtr<IDxcBlobUtf16> pPDBName = nullptr;
    compileResult->GetOutput(DXC_OUT_PDB, IID_PPV_ARGS(&pPDB), &pPDBName);
    {
        FILE* fp = NULL;

        // Note that if you don't specify -Fd, a pdb name will be automatically generated.
        // Use this file name to save the pdb so that PIX can find it quickly.
        _wfopen_s(&fp, pPDBName->GetStringPointer(), L"wb");
        fwrite(pPDB->GetBufferPointer(), pPDB->GetBufferSize(), 1, fp);
        fclose(fp);
    }
    // TODO: Shader res though pushed back isnt cached to get with a handle
    g_shaders.push_back(shader);

    ShaderHandle handle = AssignShaderHandle();

    g_shaders.at(handle.index) = shader;

    return handle;
}

PipelineHandle D3D12Backend::CreateGraphicsPipeline(const RHIGraphicsPipelineDesc& desc)
{

    PipelineResource pipeline;

    ShaderResource* vs = GetShader(desc.vs);
    ShaderResource* ps = GetShader(desc.ps);
    assert(vs && "Vertex Shader is mandatory");

    /* TODO: Currently I am only passing VS blob cuz my shaders will have a common root signature
     * declared at the top. Both will share it
     * Guess I was wrong. If Pixel shader has the [RootSignature[<define>]] part at the top of its
     * definition, vs blob cant have access to it. Rather define it at the top of both definitions
     * OR
     * Always define it above ps def, and once its reflected to c++ code, it will be used
     * nonetheless, and if ps doesn't exist (depth pre pass for ex) define it above vs def
    */
    if (ps) pipeline.rootSignature = CreateRootSignatureFromBlob(ps->pBlob.Get());
    else pipeline.rootSignature = CreateRootSignatureFromBlob(vs->pBlob.Get());

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.pRootSignature = pipeline.rootSignature.Get();

    psoDesc.VS = CD3DX12_SHADER_BYTECODE({ vs->pBlob->GetBufferPointer(), vs->pBlob->GetBufferSize() });
    if (ps) psoDesc.PS = CD3DX12_SHADER_BYTECODE(ps->pBlob->GetBufferPointer(), ps->pBlob->GetBufferSize());

    std::vector<std::string> ownedSemanticNames;
    std::vector<D3D12_INPUT_ELEMENT_DESC> inputElementDescs;

    if (desc.inputBindings.empty()) {
        inputElementDescs = CreateInputElementDesc(kBindingdescs,
            kInputAttributes, ownedSemanticNames);
    }
    else inputElementDescs = CreateInputElementDesc(desc.inputBindings,
        desc.inputAttributes, ownedSemanticNames);
    psoDesc.InputLayout = { .pInputElementDescs = inputElementDescs.data(), .NumElements = u32(inputElementDescs.size()) };

    // this is the opaque blend state i.e default
    auto& blend = psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    if (desc.blend == RHIBlendMode::ALPHA_BLEND) {
        blend.RenderTarget[0].BlendEnable = TRUE;
        blend.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
        blend.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
        blend.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    }
    // TODO: handle other blend states, don't need it for now so leaving it empty

    // Rasterizer State
    auto& rast = psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    switch (desc.rasterMode)
    {
    case RHIRasterMode::BACK:
        rast.CullMode = D3D12_CULL_MODE_BACK;
        break;
    case RHIRasterMode::FRONT:
        rast.CullMode = D3D12_CULL_MODE_FRONT;
        break;
    case RHIRasterMode::NONE:
        rast.CullMode = D3D12_CULL_MODE_NONE;
        break;
    case RHIRasterMode::WIREFRAME:
        rast.FillMode = D3D12_FILL_MODE_WIREFRAME;
        break;
    }

    rast.FrontCounterClockwise = TRUE;

    // Depth Stencil
    auto& depth = psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC1(D3D12_DEFAULT);
    if (desc.depthMode == RHIDepthMode::NONE) {
        depth.DepthEnable = FALSE;
    }
    else if (desc.depthMode == RHIDepthMode::READ)
    {
        depth.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    }

    switch (desc.depthFunc)
    {
    case RHIDepthFunc::NONE:
        depth.DepthFunc = D3D12_COMPARISON_FUNC_NONE;
        break;
    case RHIDepthFunc::NEVER:
        depth.DepthFunc = D3D12_COMPARISON_FUNC_NEVER;
        break;
    case RHIDepthFunc::LESS:
        depth.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
        break;
    case RHIDepthFunc::EQUAL:
        depth.DepthFunc = D3D12_COMPARISON_FUNC_EQUAL;
        break;
    case RHIDepthFunc::LEQUAL:
        depth.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
        break;
    case RHIDepthFunc::GREATER:
        depth.DepthFunc = D3D12_COMPARISON_FUNC_GREATER;
        break;
    case RHIDepthFunc::NEQUAL:
        depth.DepthFunc = D3D12_COMPARISON_FUNC_NOT_EQUAL;
        break;
    case RHIDepthFunc::GEQUAL:
        depth.DepthFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
        break;
    case RHIDepthFunc::ALWAYS:
        depth.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        break;
    }

    // Topology
    switch (desc.topology)
    {
    case RHITopology::TRIANGLE_LIST :    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; pipeline.topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST; break;
    case RHITopology::LINE_LIST     :    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;     pipeline.topology = D3D_PRIMITIVE_TOPOLOGY_LINELIST;     break;
    case RHITopology::POINT_LIST    :    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;    pipeline.topology = D3D_PRIMITIVE_TOPOLOGY_POINTLIST;    break;
    }

    // Formats
    if (desc.colorFormats.size() == 0)
    {
        psoDesc.NumRenderTargets = 0;
    }
    else
    {
        psoDesc.NumRenderTargets = desc.colorFormats.size();

        for (u32 i = 0; i< desc.colorFormats.size(); i++)
        {
            psoDesc.RTVFormats[i] = ConvertFormat(desc.colorFormats[i]);
        }
    }
    psoDesc.DSVFormat = ConvertFormat(desc.depthFormat);
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.SampleDesc.Count = 1;

    RHI_ASSERT(g_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipeline.pso)));

    PipelineHandle handle = AssignPipelineHandle();
	g_pipelines[handle.index] = pipeline;

    return handle;
}

PipelineHandle D3D12Backend::CreateComputePipeline(const RHIComputePipelineDesc& desc)
{
    ShaderResource* cs = GetShader(desc.cs);
    ComPtr<ID3D12RootSignature> rootSignature = CreateRootSignatureFromBlob(cs->pBlob.Get());

    PipelineResource pipeline;
    pipeline.isCompute = true;

    D3D12_COMPUTE_PIPELINE_STATE_DESC csoDesc{};
    csoDesc.pRootSignature = rootSignature.Get();

    D3D12_SHADER_BYTECODE cShaderBytecode{};
    cShaderBytecode.BytecodeLength = cs->pBlob->GetBufferSize();
    cShaderBytecode.pShaderBytecode = cs->pBlob->GetBufferPointer();
    csoDesc.CS = cShaderBytecode;

    csoDesc.NodeMask = 0;

    RHI_ASSERT(g_device->CreateComputePipelineState(&csoDesc, IID_PPV_ARGS(&pipeline.pso)));

    PipelineHandle handle = AssignPipelineHandle();

    g_pipelines[handle.index] = pipeline;

    return handle;
}

D3D12Internal::BufferResource* D3D12Backend::GetBuffer(BufferHandle handle) 
{
    if (!IsBufferHandleValid(handle)) return nullptr;
    return &g_buffers[handle.index];
}
D3D12Internal::TextureResource* D3D12Backend::GetTexture(TextureHandle handle) 
{
    if (!IsTextureHandleValid(handle)) return nullptr;
    return &g_textures[handle.index];
}
D3D12Internal::ShaderResource* D3D12Backend::GetShader(ShaderHandle handle) 
{
    if (!IsShaderHandleValid(handle)) return nullptr;
    return &g_shaders[handle.index];
}
D3D12Internal::PipelineResource* D3D12Backend::GetPipeline(PipelineHandle handle) 
{
    if (!IsPipelineHandleValid(handle)) return nullptr;
    return &g_pipelines[handle.index];
}

void D3D12Backend::DestroyBuffer(BufferHandle handle)
{

}

void D3D12Backend::DestroyTexture(TextureHandle handle)
{
}

void D3D12Backend::DestroyShader(ShaderHandle handle)
{

}

void D3D12Backend::DestroyPipeline(PipelineHandle handle)
{
	using namespace D3D12Internal;
}

i32 D3D12Backend::GetBindlessReadIndex(TextureHandle handle)
{

    if (!handle.IsValid())
        return -1;
    TextureResource* texture = GetTexture(handle);
    return (i32)texture->bindlessSRVIndex;
}

i32 D3D12Backend::GetBindlessWriteIndex(TextureHandle handle)
{

    if (!handle.IsValid())
        return -1;
    TextureResource* texture = GetTexture(handle);
    return (i32)texture->bindlessUAVIndex;
}

i32 D3D12Backend::GetBindlessReadIndex(BufferHandle handle)
{

    if (!handle.IsValid())
        return -1;
    BufferResource* buffer = GetBuffer(handle);
    return (i32)buffer->AsShaderResourceView()->heapIndex.value();
}

i32 D3D12Backend::GetBindlessWriteIndex(BufferHandle handle)
{

    if (!handle.IsValid())
        return -1;
    BufferResource* buffer = GetBuffer(handle);
    return (i32)buffer->AsUnorderedAccessView()->heapIndex.value();
}


void D3D12Backend::D3D12BackendCommandList::Begin()
{
    if (isImmediate)
    {
        auto allocator = g_immediateContext.cmdAllocator;
        RHI_ASSERT(allocator->Reset());
        RHI_ASSERT(g_immediateContext.commandList->Reset(allocator.Get(), nullptr));
    }
    else
    {
        auto allocator = g_commandAllocators[g_frameIndex];
        RHI_ASSERT(allocator->Reset());
        RHI_ASSERT(cmdLists[g_frameIndex]->Reset(allocator.Get(), nullptr));

        ID3D12DescriptorHeap* heaps[] = { g_bindlessHeap.Get() };
        cmdLists[g_frameIndex]->SetDescriptorHeaps(1, heaps);

        TextureBarrier(g_invalidTextureHandle, RHIResourceState::PRESENT, RHIResourceState::RENDER_TARGET);

        g_dsvHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(g_dsvHeap->GetCPUDescriptorHandleForHeapStart());
        g_rtvHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(g_rtvHeap->GetCPUDescriptorHandleForHeapStart(), g_frameIndex, g_rtvDescriptorSize);

        const float clearColor[] = { 0.4f, 0.2f, 0.7f, 1.0f };
        cmdLists[g_frameIndex]->ClearRenderTargetView(g_rtvHandle, clearColor, 0, nullptr);
        cmdLists[g_frameIndex]->ClearDepthStencilView(g_dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 0.0f, 0, 0, nullptr);
    }
}

void D3D12Backend::D3D12BackendCommandList::End()
{
    if (isImmediate)
    {
        RHI_ASSERT(g_immediateContext.commandList->Close());
    }
    else  RHI_ASSERT(cmdLists[g_frameIndex]->Close());
}

void D3D12Backend::D3D12BackendCommandList::Submit()
{
    if (isImmediate)
    {
        ID3D12CommandList* ppCommandLists[] = { g_immediateContext.commandList.Get() };
        g_commandQueue->ExecuteCommandLists(1, ppCommandLists);

        const u64 currentFenceValue = ++g_immediateContext.fenceValue;
        RHI_ASSERT(g_commandQueue->Signal(g_immediateContext.fence.Get(), currentFenceValue));

        if (g_immediateContext.fence->GetCompletedValue() < currentFenceValue)
        {
            RHI_ASSERT(g_immediateContext.fence->SetEventOnCompletion(currentFenceValue, g_immediateContext.fenceEvent));
            WaitForSingleObject(g_immediateContext.fenceEvent, INFINITE);
        }
    }
    else
    {
        ID3D12CommandList* ppCommandLists[] = { cmdLists[g_frameIndex].Get() };
        g_commandQueue->ExecuteCommandLists(1, ppCommandLists);

        RHI_ASSERT(g_swapchain->Present(0, DXGI_PRESENT_ALLOW_TEARING));
    }
}

void D3D12Backend::D3D12BackendCommandList::TextureBarrier(TextureHandle handle, RHIResourceState before,
                                                           RHIResourceState after)
{
    auto stateBefore = ConvertResourceState(before);
    auto stateAfter= ConvertResourceState(after);
    if (handle == g_invalidTextureHandle)
    {
	    // the resource is either backbuffer render target or depth stencil
        // they will be handled with the global vars

        // handle backbuffer
        if (before == RHIResourceState::RENDER_TARGET || before == RHIResourceState::PRESENT)
        {
            auto backBuffer = g_renderTargets[g_frameIndex];
            auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
                backBuffer.Get(),
                stateBefore,
                stateAfter
            );
            cmdLists[g_frameIndex]->ResourceBarrier(1, &barrier);
        }

        if (before == RHIResourceState::DEPTH_WRITE || before == RHIResourceState::DEPTH_READ)
        {
            auto depthStencil = g_depthStencil;
            auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
                depthStencil.Get(),
                stateBefore,
                stateAfter);
            cmdLists[g_frameIndex]->ResourceBarrier(1, &barrier);
        }
    }
    else
    {
        TextureResource* resource = GetTexture(handle);
        auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            resource->resource.Get(),
            stateBefore,
            stateAfter);
        cmdLists[g_frameIndex]->ResourceBarrier(1, &barrier);
    }
}

void D3D12Backend::D3D12BackendCommandList::BufferBarrier(BufferHandle handle, RHIResourceState before,
	RHIResourceState after)
{

	auto stateBefore = ConvertResourceState(before);
    auto stateAfter = ConvertResourceState(after);

    BufferResource* resource = GetBuffer(handle);
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        resource->resource.Get(),
        stateBefore,
        stateAfter);
    cmdLists[g_frameIndex]->ResourceBarrier(1, &barrier);
}


void D3D12Backend::D3D12BackendCommandList::SetPipeline(PipelineHandle handle)
{

    if (!handle.IsValid())
        return;
    PipelineResource* pipeline = GetPipeline(handle);
    cmdLists[g_frameIndex]->SetPipelineState(pipeline->pso.Get());
    if(pipeline->isCompute == true) cmdLists[g_frameIndex]->SetComputeRootSignature(pipeline->rootSignature.Get());
    cmdLists[g_frameIndex]->SetGraphicsRootSignature(pipeline->rootSignature.Get());

    cmdLists[g_frameIndex]->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void D3D12Backend::D3D12BackendCommandList::SetGraphicsPushConstants(const void* data, u32 size, u32 offset)
{
    cmdLists[g_frameIndex]->SetGraphicsRoot32BitConstants(0, size, data, offset);
}

void D3D12Backend::D3D12BackendCommandList::SetComputePushConstants(const void* data, u32 size, u32 offset)
{
    cmdLists[g_frameIndex]->SetComputeRoot32BitConstants(0, size, data, offset);
}

static CD3DX12_CPU_DESCRIPTOR_HANDLE GetRTVHandle(TextureHandle textureHandle)
{
    TextureResource* resource = D3D12Backend::GetTexture(textureHandle);
    u32 rtvIndex = resource->rtvIndex;

    return CD3DX12_CPU_DESCRIPTOR_HANDLE (
        g_rtvHeap->GetCPUDescriptorHandleForHeapStart(),
        rtvIndex,
        g_rtvDescriptorSize
    );
}

static CD3DX12_CPU_DESCRIPTOR_HANDLE GetDSVHandle(TextureHandle textureHandle)
{
    return CD3DX12_CPU_DESCRIPTOR_HANDLE (g_dsvHeap->GetCPUDescriptorHandleForHeapStart());
}

void D3D12Backend::D3D12BackendCommandList::BeginRendering(const std::vector<TextureHandle> colorTargets,
                                                           TextureHandle depthTarget)
{
    if (colorTargets.empty() && depthTarget == g_invalidTextureHandle)
    {
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(g_rtvHeap->GetCPUDescriptorHandleForHeapStart(),
            g_frameIndex, g_rtvDescriptorSize);
        CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle{ g_dsvHeap->GetCPUDescriptorHandleForHeapStart() };
        cmdLists[g_frameIndex]->OMSetRenderTargets(
            0, nullptr,
            FALSE,
            &dsvHandle
        );
    }
    else if (colorTargets[0] == g_invalidTextureHandle && depthTarget == g_invalidTextureHandle)
    {
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(g_rtvHeap->GetCPUDescriptorHandleForHeapStart(),
            g_frameIndex, g_rtvDescriptorSize);
        CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle{ g_dsvHeap->GetCPUDescriptorHandleForHeapStart() };
        cmdLists[g_frameIndex]->OMSetRenderTargets(
            1,
            &rtvHandle,
            FALSE,
            &dsvHandle
        );
    }
    else
    {
        std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtvHandles;

        for (auto colorTarget : colorTargets)
        {
            auto* tex = GetTexture(colorTarget);
            if (!tex) continue;

            CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle = GetRTVHandle(colorTarget);
            rtvHandles.push_back(rtvHandle);
        }

        D3D12_CPU_DESCRIPTOR_HANDLE* dsvHandle = nullptr;
        D3D12_CPU_DESCRIPTOR_HANDLE dsv;

        if (depthTarget.IsValid())
        {
            dsv = GetDSVHandle(depthTarget);
            dsvHandle = &dsv;
        }

        cmdLists[g_frameIndex]->OMSetRenderTargets(
            static_cast<UINT>(rtvHandles.size()),
            rtvHandles.empty() ? nullptr : rtvHandles.data(),
            FALSE,
            dsvHandle
        );
    }
}

void D3D12Backend::D3D12BackendCommandList::EndRendering()
{
    // dont ever convert to write states before the end of frame
	// Impilict resource decay mentions that read states decay to common state always
	// but not write states, doing this will stop the decay and cause issues furhter
    TextureBarrier(g_invalidTextureHandle, RHIResourceState::RENDER_TARGET, RHIResourceState::PRESENT);
}

void D3D12Backend::D3D12BackendCommandList::SetViewport(const RHIViewPort& viewPort)
{
    if (viewPort.width <= 0.0f || viewPort.height <= 0.0f)
    {
        cmdLists[g_frameIndex]->RSSetViewports(1, &g_viewport);
    }
    else
    {
        CD3DX12_VIEWPORT vp(viewPort.x, viewPort.y, viewPort.width, viewPort.height,
            viewPort.minDepth, viewPort.maxDepth);
        cmdLists[g_frameIndex]->RSSetViewports(1, &vp);
    }
}

void D3D12Backend::D3D12BackendCommandList::SetScissor(const RHIScissor& scissor)
{

	if (scissor.width <= 0 || scissor.height <= 0)
    {
        cmdLists[g_frameIndex]->RSSetScissorRects(1, &g_scissorRect);
    }
    else
    {
        CD3DX12_RECT rect(scissor.x, scissor.y,
            scissor.x + scissor.width, scissor.y + scissor.height);
        cmdLists[g_frameIndex]->RSSetScissorRects(1, &rect);
    }
}

void D3D12Backend::D3D12BackendCommandList::Draw(u32 vertexCount, u32 instanceCount, u32 firstVertex, u32 firstInstance)
{
    cmdLists[g_frameIndex]->DrawInstanced(vertexCount, instanceCount, firstVertex, firstInstance);
}

void D3D12Backend::D3D12BackendCommandList::DrawIndexed(u32 indexCount, u32 instanceCount, u32 firstIndex,
	i32 vertexOffset, u32 firstInstance)
{
    cmdLists[g_frameIndex]->DrawIndexedInstanced(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void D3D12Backend::D3D12BackendCommandList::Dispatch(u32 x, u32 y, u32 z)
{
    cmdLists[g_frameIndex]->Dispatch(x, y, z);
}

void D3D12Backend::D3D12BackendCommandList::BindVertexBuffer(BufferHandle handle)
{

    BufferResource* bufferResource = GetBuffer(handle);
    cmdLists[g_frameIndex]->IASetVertexBuffers(0, 1, &bufferResource->AsVertexBuffer()->view);
}

void D3D12Backend::D3D12BackendCommandList::BindIndexBuffer(BufferHandle handle)
{

    BufferResource* bufferResource = GetBuffer(handle);
    cmdLists[g_frameIndex]->IASetIndexBuffer(&bufferResource->AsIndexBuffer()->view);
}

D3D12Backend::D3D12BackendCommandList* D3D12Backend::CreateCommandList(bool isImmediate)
{
    D3D12BackendCommandList* commandList = new D3D12BackendCommandList();

    if (isImmediate)
    {
        // use the immediate command list
        commandList->isImmediate = true;

        RHI_ASSERT(g_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
            IID_PPV_ARGS(&g_immediateContext.cmdAllocator)));

        RHI_ASSERT(g_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
            g_immediateContext.cmdAllocator.Get(),
            nullptr,
            IID_PPV_ARGS(&g_immediateContext.commandList)));

        RHI_ASSERT(g_immediateContext.commandList->Close());
    }

    else
    {
        for (u32 i = 0; i < frameCount; i++)
        {
            RHI_ASSERT(g_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                IID_PPV_ARGS(&commandList->commandAllocators[i])));

            RHI_ASSERT(g_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
                commandList->commandAllocators[i].Get(),
                nullptr,
                IID_PPV_ARGS(&commandList->cmdLists[i])));

            RHI_ASSERT(commandList->cmdLists[i]->Close());
        }
    }
    return commandList;
}

void D3D12Backend::DestroyCommandList(D3D12BackendCommandList* cmdList)
{
    delete cmdList;
}

void D3D12Backend::MoveToNextFrame()
{

    /*
     *
     * perform the other tasks whatever
     *
    */
    // do framesync stuff before the move to next frame
    const u64 currentFenceValue = g_fenceValues[g_frameIndex];
    RHI_ASSERT(g_commandQueue->Signal(g_fence.Get(), currentFenceValue));

    // update the frame index
    g_frameIndex = g_swapchain->GetCurrentBackBufferIndex();

    // CPU side code
    // if the next frame is not ready to be rendered yet, wait until its ready
    if (g_fence->GetCompletedValue() < g_fenceValues[g_frameIndex])
    {
        RHI_ASSERT(g_fence->SetEventOnCompletion(g_fenceValues[g_frameIndex], g_fenceEvent));
        WaitForSingleObjectEx(g_fenceEvent, INFINITE, FALSE);
    }

    // set the fence value for the next frame
    g_fenceValues[g_frameIndex] = currentFenceValue + 1;
}

DXGI_FORMAT D3D12Backend::ConvertFormat(RHIFormat format)
{
    switch (format)
    {
    case RHIFormat::R8G8B8A8_UNORM:
        return DXGI_FORMAT_R8G8B8A8_UNORM;
    case RHIFormat::R8G8B8A8_UNORM_SRGB:
        return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    case RHIFormat::R16G16B16A16_FLOAT:
        return DXGI_FORMAT_R16G16B16A16_FLOAT;
    case RHIFormat::R32G32B32A32_FLOAT:
        return DXGI_FORMAT_R32G32B32A32_FLOAT;
    case RHIFormat::D32_FLOAT:
        return DXGI_FORMAT_D32_FLOAT;
    case RHIFormat::R16_UINT:
        return DXGI_FORMAT_R16_UINT;
    case RHIFormat::R32_UINT:
        return DXGI_FORMAT_R32_UINT;
    case RHIFormat::R32_TYPELESS:
        return DXGI_FORMAT_R32_TYPELESS;
    case RHIFormat::UNKNOWN:
        return DXGI_FORMAT_UNKNOWN;
        break;
	case RHIFormat::R32G32B32_FLOAT:
        return DXGI_FORMAT_R32G32B32_FLOAT;
		break;
	case RHIFormat::R32G32_FLOAT:
        return DXGI_FORMAT_R32G32_FLOAT;
		break;
    default:
        return DXGI_FORMAT_UNKNOWN;
    }
}

D3D12_RESOURCE_STATES D3D12Backend::ConvertResourceState(RHIResourceState state)
{
	switch (state)
	{
	case RHIResourceState::COMMON:
        return D3D12_RESOURCE_STATE_COMMON;
		break;
	case RHIResourceState::VERTEX_BUFFER:
        return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		break;
	case RHIResourceState::INDEX_BUFFER:
        return D3D12_RESOURCE_STATE_INDEX_BUFFER;
		break;
	case RHIResourceState::RENDER_TARGET:
        return D3D12_RESOURCE_STATE_RENDER_TARGET;
		break;
	case RHIResourceState::DEPTH_WRITE:
        return D3D12_RESOURCE_STATE_DEPTH_WRITE;
		break;
	case RHIResourceState::DEPTH_READ:
        return D3D12_RESOURCE_STATE_DEPTH_READ;
		break;
	case RHIResourceState::UNORDERED_ACCESS:
        return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		break;
	case RHIResourceState::SHADER_RESOURCE:
        return D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
		break;
	case RHIResourceState::COPY_DEST:
        return D3D12_RESOURCE_STATE_COPY_DEST;
		break;
	case RHIResourceState::COPY_SOURCE:
        return D3D12_RESOURCE_STATE_COPY_SOURCE;
		break;
	case RHIResourceState::PRESENT:
        return D3D12_RESOURCE_STATE_PRESENT;
		break;
	default:
		return D3D12_RESOURCE_STATE_COMMON;
        break;
	}
}

D3D12_COMPARISON_FUNC D3D12Backend::ConvertDepthFunc(RHIDepthFunc func)
{
	switch (func)
	{
	case RHIDepthFunc::NONE:
        return D3D12_COMPARISON_FUNC_NONE;
		break;
	case RHIDepthFunc::NEVER:
        return D3D12_COMPARISON_FUNC_NEVER;
		break;
	case RHIDepthFunc::LESS:
        return D3D12_COMPARISON_FUNC_LESS;
		break;
	case RHIDepthFunc::EQUAL:
        return D3D12_COMPARISON_FUNC_EQUAL;
		break;
	case RHIDepthFunc::LEQUAL:
        return D3D12_COMPARISON_FUNC_LESS_EQUAL;
		break;
	case RHIDepthFunc::GREATER:
        return D3D12_COMPARISON_FUNC_GREATER;
		break;
	case RHIDepthFunc::NEQUAL:
        return D3D12_COMPARISON_FUNC_NOT_EQUAL;
		break;
	case RHIDepthFunc::GEQUAL:
        return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
		break;
	case RHIDepthFunc::ALWAYS:
        return D3D12_COMPARISON_FUNC_ALWAYS;
		break;
    default:
        return D3D12_COMPARISON_FUNC_GREATER;
	}
}

D3D12_PRIMITIVE_TOPOLOGY D3D12Backend::ConvertTopology(RHITopology topology)
{
	switch (topology)
	{
	case RHITopology::TRIANGLE_LIST:
        return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		break;
	case RHITopology::LINE_LIST:
        return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
		break;
	case RHITopology::POINT_LIST:
        return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
		break;
    default:
        return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	}
}

static void GetHardwareAdapter(
    IDXGIFactory1* pFactory,
    IDXGIAdapter1** ppAdapter,
    bool requestHighPerformanceAdapter)
{
    *ppAdapter = nullptr;

    ComPtr<IDXGIAdapter1> adapter;

    ComPtr<IDXGIFactory6> factory6;
    if (SUCCEEDED(pFactory->QueryInterface(IID_PPV_ARGS(&factory6))))
    {
        for (
            UINT adapterIndex = 0;
            SUCCEEDED(factory6->EnumAdapterByGpuPreference(
                adapterIndex,
                requestHighPerformanceAdapter == true ? DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE :
                DXGI_GPU_PREFERENCE_UNSPECIFIED,
                IID_PPV_ARGS(&adapter)));
                ++adapterIndex)
        {
            DXGI_ADAPTER_DESC1 desc;
            RHI_ASSERT(adapter->GetDesc1(&desc));

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                // Don't select the Basic Render Driver adapter.
                continue;
            }

            // Check to see whether the adapter supports Direct3D 12, but don't create the
            // actual device yet.
            if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0,
                _uuidof(ID3D12Device), nullptr)))
            {
                break;
            }
        }
    }

    if (adapter.Get() == nullptr)
    {
        for (UINT adapterIndex = 0; SUCCEEDED(pFactory->EnumAdapters1(adapterIndex, &adapter)); ++adapterIndex)
        {
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                // Don't select the Basic Render Driver adapter.
                continue;
            }

            // Check to see whether the adapter supports Direct3D 12, but don't create the
            // actual device yet.
            if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0,
                _uuidof(ID3D12Device), nullptr)))
            {
                break;
            }
        }
    }
    *ppAdapter = adapter.Detach();
}
static void IsDirectXRayTracingSuppported(ID3D12Device14* device)
{
    D3D12_FEATURE_DATA_D3D12_OPTIONS5 featureSupportData = {};

    RHI_ASSERT(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &featureSupportData,
        sizeof(featureSupportData)));
    if (featureSupportData.RaytracingTier < D3D12_RAYTRACING_TIER_1_0)
        throw std::runtime_error("Raytracing not supported on device");
}


static BufferHandle AssignBufferHandle()
{
    BufferHandle handle;
    if (!g_freeBuffers.empty())
    {
        handle.index = g_freeBuffers.back();
        g_freeBuffers.pop_back();
        handle.generation = g_generationsBuffers[handle.index];
    }
    else
    {
        handle.index = (u32)g_generationsBuffers.size();
        g_generationsBuffers.push_back(0);
    }
    return handle;
}

static TextureHandle AssignTextureHandle()
{
    u32 index;
    if (!g_freeTextures.empty())
    {
        index = g_freeTextures.back();
        g_freeTextures.pop_back();
    }
    else
    {
        index = (u32)g_generationsTextures.size();
        g_generationsTextures.push_back(0);
    }
    return TextureHandle{
    .index = index,
    .generation = g_generationsTextures[index] };
}

static ShaderHandle AssignShaderHandle()
{
    u32 index;
    if (!g_freeShaders.empty())
    {
        index = g_freeShaders.back();
        g_freeShaders.pop_back();
    }
    else
    {
        index = (u32)g_generationsShaders.size();
        g_generationsShaders.push_back(0);
    }
    return ShaderHandle{
    .index = index,
    .generation = g_generationsShaders[index] };
}

static PipelineHandle AssignPipelineHandle()
{
    u32 index;
    if (!g_freePipelines.empty())
    {
        index = g_freePipelines.back();
        g_freePipelines.pop_back();
    }
    else
    {
        index = (u32)g_generationsPipelines.size();
        g_generationsPipelines.push_back(0);
    }
    return PipelineHandle{
    .index = index,
    .generation = g_generationsPipelines[index] };
}

static bool IsBufferHandleValid(BufferHandle handle) 
{

    return handle.index < g_generationsBuffers.size() &&
        g_generationsBuffers[handle.index] == handle.generation;
}
static bool IsTextureHandleValid(TextureHandle handle) 
{

    return handle.index < g_generationsTextures.size() &&
        g_generationsTextures[handle.index] == handle.generation;
}
static bool IsShaderHandleValid(ShaderHandle handle) 
{

    return handle.index < g_generationsShaders.size() &&
        g_generationsShaders[handle.index] == handle.generation;
}
static bool IsPipelineHandleValid(PipelineHandle handle) 
{

    return handle.index < g_generationsPipelines.size() &&
        g_generationsPipelines[handle.index] == handle.generation;
}

static BufferResource CreateBufferResource(const RHIBufferDesc& desc)
{
    BufferResource bufferRes{};
    u32 bufferSize = GetBufferSize(desc.createInfo);
    D3D12_HEAP_TYPE heapType{};
    D3D12_RESOURCE_STATES initialState{};

    switch (desc.usage)
    {
    case RHIMemoryeUsage::UPLOAD:
        heapType = D3D12_HEAP_TYPE_UPLOAD;
        initialState = D3D12_RESOURCE_STATE_GENERIC_READ;
        break;

    case RHIMemoryeUsage::DEFAULT:
        heapType = D3D12_HEAP_TYPE_DEFAULT;
        initialState = D3D12_RESOURCE_STATE_COMMON;
        break;
    case RHIMemoryeUsage::READBACK:
        heapType = D3D12_HEAP_TYPE_READBACK;
        initialState = D3D12_RESOURCE_STATE_COPY_DEST;
        break;

    case RHIMemoryeUsage::GPU_UPLOAD:
        heapType = D3D12_HEAP_TYPE_GPU_UPLOAD;
        initialState = D3D12_RESOURCE_STATE_COMMON;
        break;
    }
    D3D12_RESOURCE_FLAGS resourceFlags{};

    if (HasFlag(desc.view, RHIResourceView::LOAD))
    {
        // srvs are always created
    }
    if (HasFlag(desc.view, RHIResourceView::STORE))
    {
        resourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }

    auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize, resourceFlags);
    D3D12MA::ALLOCATION_DESC allocDesc{};
    allocDesc.HeapType = heapType;
    allocDesc.Flags = D3D12MA::ALLOCATION_FLAG_NONE;

    auto allocator = g_allocator.Get();
    RHI_ASSERT(allocator->CreateResource(
        &allocDesc,
        &bufferDesc,
        initialState,
        nullptr,
        &bufferRes.allocation,
        IID_NULL,
        nullptr));

    bufferRes.resource = bufferRes.allocation->GetResource();
    RHI_ASSERT(bufferRes.resource->SetName(desc.debugName));

    if (heapType == D3D12_HEAP_TYPE_UPLOAD || heapType == D3D12_HEAP_TYPE_GPU_UPLOAD)
    {
        void* pMappedData;
        CD3DX12_RANGE readRange(0, 0);
        RHI_ASSERT(bufferRes.resource->Map(0, &readRange, &pMappedData));
        std::visit(BufferUpdateVisitor{ pMappedData }, desc.createInfo);

        // prolly wont unmap buffers
        //if (!createInfo.keepMapped) {
        //    buffer.resource->Unmap(0, nullptr);
        //}
    }

    return bufferRes;
}

static void UploadTextureData(const ComPtr<ID3D12Resource>& resource,
    const RHITextureDesc& desc,
    const RHIMemoryeUsage usage,
    const void* initialData)
{
    auto heapProps = CD3DX12_HEAP_PROPERTIES((usage == RHIMemoryeUsage::UPLOAD) ? D3D12_HEAP_TYPE_UPLOAD : D3D12_HEAP_TYPE_GPU_UPLOAD);

    if (usage == RHIMemoryeUsage::UPLOAD)
    {
        const UINT subresourceCount = desc.arraySize * desc.mips;
        const UINT64 uploadBufferSize = GetRequiredIntermediateSize(resource.Get(),
            0, subresourceCount);
        ComPtr<ID3D12Resource> textureUploadHeapResource;
        auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);

        auto device = D3D12Internal::g_device.Get();
        RHI_ASSERT(device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &bufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&textureUploadHeapResource)));


        if (initialData)
        {
            std::vector<D3D12_SUBRESOURCE_DATA> subresources(subresourceCount);
            const uint8_t* pData = static_cast<const uint8_t*>(initialData);

            UINT64 sliceSize = desc.width * desc.height * 4; // TODO: remove the hard coded impl

            for (u32 i = 0; i < desc.arraySize; ++i)
            {
                // Point to the specific offset for this face in the combinedData buffer
                subresources[i].pData = pData + (i * sliceSize);
                subresources[i].RowPitch = desc.width * 4;
                subresources[i].SlicePitch = sliceSize;
            }

            /* TODO: Need to find a way to do this less messy way
            */
            RHI::ImmediateSubmit( [&]()
                {
                    UpdateSubresources(g_immediateContext.commandList.Get(),
                        resource.Get(),
                        textureUploadHeapResource.Get(),
                        0, 0, subresourceCount, subresources.data());
                    auto pBarrier = CD3DX12_RESOURCE_BARRIER::Transition(resource.Get(),
                        D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
                    g_immediateContext.commandList->ResourceBarrier(1, &pBarrier);
                });
        }
    }
    else if (usage == RHIMemoryeUsage::GPU_UPLOAD)
    {

    }
}

static TextureResource CreateTextureResource(D3D12MA::Allocator* allocator,
    const D3D12_RESOURCE_FLAGS resourceFlags, 
    const RHITextureDesc& desc, 
    const void* initialData)
{

    TextureResource textureRes{};

    CD3DX12_RESOURCE_DESC resourceDesc{};
    if (desc.depth == 0)
    {
        resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(
            D3D12Backend::ConvertFormat(desc.format),
            desc.width, desc.height,
            desc.arraySize, desc.mips,
            1, 0,
            resourceFlags);
    }
    else if (desc.depth > 0)
    {
        resourceDesc = CD3DX12_RESOURCE_DESC::Tex3D(
            D3D12Backend::ConvertFormat(desc.format),
            desc.width, desc.height, desc.depth,
            desc.mips,
            resourceFlags);
    }

    D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON;
    D3D12_CLEAR_VALUE* pClearValue = nullptr;
    D3D12_CLEAR_VALUE clearValue = {};

    if (resourceFlags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET)
    {
        initialState = D3D12_RESOURCE_STATE_RENDER_TARGET;
        clearValue.Format = D3D12Backend::ConvertFormat(desc.format);
        clearValue.Color[0] = 0.0f;
        clearValue.Color[1] = 0.0f;
        clearValue.Color[2] = 0.0f;
        clearValue.Color[3] = 1.0f;
        pClearValue = &clearValue;
    }

    D3D12MA::ALLOCATION_DESC allocDesc = {};
    allocDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
    allocDesc.Flags = D3D12MA::ALLOCATION_FLAG_NONE;

    ComPtr<ID3D12Resource> resource;
    RHI_ASSERT(allocator->CreateResource(
        &allocDesc,
        &resourceDesc,
        initialState,
        pClearValue,
        &textureRes.allocation,
        IID_PPV_ARGS(&resource)));

    textureRes.resource = textureRes.allocation->GetResource();
    if (initialData && (desc.usage == RHIMemoryeUsage::UPLOAD || desc.usage == RHIMemoryeUsage::GPU_UPLOAD))
    {
        UploadTextureData(resource, desc, desc.usage, initialData);
    }

    if (desc.debugName)
    {
        RHI_ASSERT(textureRes.resource->SetName(desc.debugName));
    }
    // allocation->Release();

    return textureRes;
}
static u32 CreateTextureSRV(
    u32* nextIndex,
    const ComPtr<ID3D12Resource>& resource,
    const RHITextureDesc& desc)
{
    u32 index = (*nextIndex)++;
    CD3DX12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};

    auto format = D3D12Backend::ConvertFormat(desc.format);
    srvDesc.Format = format;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    if (desc.depth > 0)
    {
        // 3D Texture
        srvDesc = CD3DX12_SHADER_RESOURCE_VIEW_DESC::Tex3D(format);
    }
    else if (desc.arraySize == 6)
    {
        // TODO:temp fix; ArraySize 6 implies Cubemap for now
        srvDesc = CD3DX12_SHADER_RESOURCE_VIEW_DESC::TexCube(format);
    }
    else if (desc.arraySize > 1)
    {
        // standard texture array
        srvDesc = CD3DX12_SHADER_RESOURCE_VIEW_DESC::Tex2DArray(format);
    }
    else
    {
        srvDesc = CD3DX12_SHADER_RESOURCE_VIEW_DESC::Tex2D(format);
    }

    CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(g_bindlessHeap->GetCPUDescriptorHandleForHeapStart());
    u32 descriptorSize = g_device->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    cpuHandle.Offset(index, descriptorSize);

    g_device->CreateShaderResourceView(resource.Get(), &srvDesc, cpuHandle);

    return index;
}

static u32 CreateTextureRTV(
    ID3D12DescriptorHeap* rtvHeap,  // Note: Separate RTV heap!
    u32* nextRTVIndex,
    const ComPtr<ID3D12Resource>& resource,
    const RHITextureDesc& desc)
{

    u32 index = (*nextRTVIndex)++;

    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.Format = D3D12Backend::ConvertFormat(desc.format);
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    rtvDesc.Texture2D.MipSlice = 0;

    CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart());
    u32 descriptorSize = g_device->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    cpuHandle.Offset(index, descriptorSize);

    g_device->CreateRenderTargetView(resource.Get(), &rtvDesc, cpuHandle);

    return index;
}


static u32 CreateTextureUAV(
    u32* nextIndex,
    const ComPtr<ID3D12Resource>& resource,
    const RHITextureDesc& desc,
    u32 mipLevel)
{

    u32 index = (*nextIndex)++;

    CD3DX12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    auto format = D3D12Backend::ConvertFormat(desc.format);
    uavDesc.Format = format;

    if (desc.depth > 0)
    {
        // 3D Texture
        uavDesc = CD3DX12_UNORDERED_ACCESS_VIEW_DESC::Tex3D(format);
    }
    else if (desc.arraySize > 1)
    {
        uavDesc = CD3DX12_UNORDERED_ACCESS_VIEW_DESC::Tex2DArray(format);
    }
    else
    {
        uavDesc = CD3DX12_UNORDERED_ACCESS_VIEW_DESC::Tex2D(format);
    }

    CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(g_bindlessHeap->GetCPUDescriptorHandleForHeapStart());
    u32 descriptorSize = g_device->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    cpuHandle.Offset(index, descriptorSize);

    g_device->CreateUnorderedAccessView(resource.Get(), nullptr, &uavDesc, cpuHandle);

    return index;
}

static ComPtr<ID3D12RootSignature> CreateRootSignatureFromBlob(IDxcBlob* shaderBlob)
{

    if (!shaderBlob) return nullptr;

    ComPtr<ID3D12RootSignature> rootSignature;

    // The D3D12 runtime can parse the Root Signature directly from the compiled shader blob
    // i am using only dx12 shader macro based root signatures, pretty much for now
    HRESULT hr = g_device->CreateRootSignature(
        0,                                      // NodeMask (0 for single GPU)
        shaderBlob->GetBufferPointer(),         // Pointer to the shader bytecode
        shaderBlob->GetBufferSize(),            // Size of the shader bytecode
        IID_PPV_ARGS(&rootSignature)
    );

    if (FAILED(hr))
    {
        RHI_ASSERT(hr);
        return nullptr;
    }
    return rootSignature;
}

static std::vector<D3D12_INPUT_ELEMENT_DESC> CreateInputElementDesc(const std::vector<RHIInputBindingDesc>& bindingDescs,
    const std::vector<RHIInputAttributeDesc>& attributeDescs,
    std::vector<std::string>& outOwnedSemanticNames)
{
    std::vector<D3D12_INPUT_ELEMENT_DESC> inputElements;
    outOwnedSemanticNames.clear();

    for (const auto& attr : attributeDescs) {

    	const RHIInputBindingDesc* pBinding = nullptr;
        for (const auto& bind : bindingDescs) {
            if (bind.binding == attr.binding) {
                pBinding = &bind;
                break;
            }
        }
        if (!pBinding) {
            printl(Log::LogLevel::Error, "Binding missing!");
            continue;
        }

        D3D12_INPUT_ELEMENT_DESC elem = {};

        if (attr.semanticName.empty()) {
            outOwnedSemanticNames.push_back("ATTRIBUTE" + std::to_string(attr.location));
            elem.SemanticName = outOwnedSemanticNames.back().c_str();
        }
        else {
            elem.SemanticName = attr.semanticName.c_str();
        }

        elem.SemanticIndex = attr.semanticIndex;
        elem.Format = D3D12Backend::ConvertFormat(attr.format);
        elem.InputSlot = attr.binding;
        elem.AlignedByteOffset = attr.offset;
        elem.InputSlotClass = (pBinding->inputRate == RHIInputRate::PerVertex)
            ? D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA
            : D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
        elem.InstanceDataStepRate = pBinding->instanceStepRate;

        inputElements.push_back(elem);
    }

    return inputElements;
}