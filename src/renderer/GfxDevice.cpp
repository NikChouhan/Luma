#include "GfxDevice.h"
#include <D3D12MemAlloc.h>

#include "FrameSync.h"

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
            DX_ASSERT(adapter->GetDesc1(&desc));

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

	DX_ASSERT(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &featureSupportData,
            sizeof(featureSupportData)));
    if (featureSupportData.RaytracingTier < D3D12_RAYTRACING_TIER_1_0)
        throw std::runtime_error("Raytracing not supported on device");
}


GfxDevice CreateDevice(GfxDeviceDesc desc)
{
    GfxDevice gfxDevice{};

    u32 dxgiFactoryFlags = 0;
    ComPtr<IDXGIFactory2> factory;


#if defined(DEBUG)
    ComPtr<ID3D12Debug6> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
    {
        debugController->EnableDebugLayer();
        dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
    }
#endif
    DX_ASSERT(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));

    ComPtr<IDXGIAdapter1> hwAdapter;
    GetHardwareAdapter(factory.Get(), &hwAdapter, true);

    DX_ASSERT(D3D12CreateDevice(hwAdapter.Get(), D3D_FEATURE_LEVEL_12_2, IID_PPV_ARGS(&gfxDevice.device)));
    // ray tracing stuff
    IsDirectXRayTracingSuppported(gfxDevice.device.Get());
#ifdef DEBUG
    ComPtr<ID3D12DebugDevice2> debugDevice;
    DX_ASSERT(gfxDevice.device->QueryInterface(IID_PPV_ARGS(&debugDevice)));
#endif

    // create d3d12 memory allocator
    D3D12MA::ALLOCATOR_DESC allocatorDesc{};
    allocatorDesc.pDevice = gfxDevice.device.Get();
    allocatorDesc.pAdapter = hwAdapter.Get();

    DX_ASSERT(D3D12MA::CreateAllocator(&allocatorDesc, &gfxDevice.allocator));

    // describe and create command queue
    D3D12_COMMAND_QUEUE_DESC queueDesc{};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    DX_ASSERT(gfxDevice.device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&gfxDevice.commandQueue)));

    for (u32 i = 0; i < frameCount; i++)
    {
        DX_ASSERT(
        gfxDevice.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&gfxDevice.
            commandAllocators[i])));
    }

    CD3DX12FeatureSupport features;
    features.Init(gfxDevice.device.Get());
    D3D_SHADER_MODEL shaderModel = features.HighestShaderModel();   // shader_model_6_7 for me

    
    return gfxDevice;
}

void DestroyDevice(GfxDevice& gfxDevice)
{

}

ComPtr<ID3D12GraphicsCommandList10> CreateCommandList(const GfxDevice& gfxDevice)
{
    ComPtr<ID3D12GraphicsCommandList10> commandList;

    DX_ASSERT(gfxDevice.device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, gfxDevice.commandAllocators->Get(),
        nullptr,
        IID_PPV_ARGS(&commandList)));

    return commandList;
}

void ImmediateSubmit(const GfxDevice& gfxDevice, ImmediateContext* immediateCtx, LAMBDA() callback)
{
    DX_ASSERT(immediateCtx->cmdAllocator->Reset());
    DX_ASSERT(immediateCtx->commandList->Reset(
        immediateCtx->cmdAllocator.Get(),
        nullptr));

    callback();
    DX_ASSERT(immediateCtx->commandList->Close());

    ID3D12CommandList* ppCommandLists[] = { immediateCtx->commandList.Get() };
    gfxDevice.commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    const u64 currentFenceValue = ++immediateCtx->fenceValue;
    DX_ASSERT(gfxDevice.commandQueue->Signal(immediateCtx->fence.Get(), currentFenceValue));

    if (immediateCtx->fence->GetCompletedValue() < currentFenceValue)
    {
        DX_ASSERT(immediateCtx->fence->SetEventOnCompletion(currentFenceValue, immediateCtx->fenceEvent));
        WaitForSingleObject(immediateCtx->fenceEvent, INFINITE);
    }
}
