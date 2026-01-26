#include "Inspector.h"

#include "Core/Common.h"
#include "Graphics/RHI/D3D12/D3D12Backend.h"
#include "Graphics/RHI/D3D12/DescriptorHeap.h"

static constexpr int APP_SRV_HEAP_SIZE = 64;

static DescriptorHeapAllocator* g_ImGuiHeapAllocator = nullptr;

using namespace D3D12Internal;
void Inspector::CreateImguiHeap()
{
    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    desc.NumDescriptors = APP_SRV_HEAP_SIZE;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    RHI_ASSERT(g_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(_imguiHeap.GetAddressOf())));
    g_ImGuiHeapAllocator->Create(g_device.Get(), _imguiHeap.Get());
}

void Inspector::CreateInspector()
{
    g_ImGuiHeapAllocator = new DescriptorHeapAllocator;
    CreateImguiHeap();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    io = &ImGui::GetIO();
	(void)io;
    io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io->ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(g_hwnd);

    ImGui_ImplDX12_InitInfo initInfo = {};
    initInfo.Device = g_device.Get();
    initInfo.CommandQueue = g_commandQueue.Get();
    initInfo.NumFramesInFlight = frameCount;
    initInfo.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    initInfo.DSVFormat = DXGI_FORMAT_UNKNOWN;
    // Allocating SRV descriptors (for textures) is up to the application, so we provide callbacks.
    // (current version of the backend will only allocate one descriptor, future versions will need to allocate more)
    initInfo.SrvDescriptorHeap = _imguiHeap.Get();
    initInfo.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_handle, 
        D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_handle) { return g_ImGuiHeapAllocator->Alloc(out_cpu_handle, out_gpu_handle); };
    initInfo.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle,
        D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle) { return g_ImGuiHeapAllocator->Free(cpu_handle, gpu_handle); };
    ImGui_ImplDX12_Init(&initInfo);
}
