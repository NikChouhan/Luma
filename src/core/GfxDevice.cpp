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
            if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0,
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
            if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0,
                _uuidof(ID3D12Device), nullptr)))
            {
                break;
            }
        }
    }
    *ppAdapter = adapter.Detach();
}

GfxDevice CreateDevice(GfxDeviceDesc desc)
{
    GfxDevice gfxDevice{};

    u32 dxgiFactoryFlags = 0;

#if defined(DEBUG)
    ComPtr<ID3D12Debug6> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
    {
        debugController->EnableDebugLayer();
        dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
    }
#endif
    DX_ASSERT(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&gfxDevice._factory)));

    ComPtr<IDXGIAdapter1> hwAdapter;
    GetHardwareAdapter(gfxDevice._factory.Get(), &hwAdapter, true);

    DX_ASSERT(D3D12CreateDevice(hwAdapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&gfxDevice._device)));

    ComPtr<ID3D12DebugDevice2> debugDevice;
    DX_ASSERT(gfxDevice._device->QueryInterface(IID_PPV_ARGS(&debugDevice)));

    // create d3d12 memory allocator
    D3D12MA::ALLOCATOR_DESC allocatorDesc{};
    allocatorDesc.pDevice = gfxDevice._device.Get();
    allocatorDesc.pAdapter = hwAdapter.Get();

    DX_ASSERT(D3D12MA::CreateAllocator(&allocatorDesc, &gfxDevice._allocator));

    // describe and create command queue
    D3D12_COMMAND_QUEUE_DESC queueDesc{};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    DX_ASSERT(gfxDevice._device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&gfxDevice._commandQueue)));

    for (u32 i = 0; i < frameCount; i++)
    {
        DX_ASSERT(
        gfxDevice._device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&gfxDevice.
            _commandAllocators[i])));
    }
    
    return gfxDevice;
}

void DestroyDevice(GfxDevice& gfxDevice)
{
		
}

ComPtr<ID3D12GraphicsCommandList1> CreateCommandList(GfxDevice& gfxDevice)
{
    ComPtr<ID3D12GraphicsCommandList1> commandList;

    DX_ASSERT(gfxDevice._device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, gfxDevice._commandAllocators->Get(),
        nullptr,
        IID_PPV_ARGS(&commandList)));

    return commandList;
}

void ImmediateSubmit(GfxDevice& gfxDevice, FrameSync& frameSync, LAMBDA(ComPtr<ID3D12GraphicsCommandList1>) callback)
{
	ComPtr<ID3D12GraphicsCommandList1> commandList = CreateCommandList(gfxDevice);

    callback(commandList);
    DX_ASSERT(commandList->Close());

    ID3D12CommandList* ppCommandLists[] = { commandList.Get() };
    gfxDevice._commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
    WaitForGPU(gfxDevice, frameSync);
}
