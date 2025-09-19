#include "GfxDevice.h"
#include "Swapchain.h"
#include "Pipeline.h"
#include "FrameSync.h"
#include "Buffer.h"
#include "Texture.h"
#include "Log.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "Camera.h"
#include "Model.h"

//struct FramePresent
//{
//	ComPtr<ID3D12GraphicsCommandList> commandList;
//	Swapchain swapchain;
//	FrameSync frameSync;
//	Pipeline pipeline;
//	Buffer vertexBuffer;
//};

static bool isOpen = true;

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

static int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PWSTR pCmdLine, int cmdShow)
{
	constexpr wchar_t className[] = L"D3D12 Triangle";

	WNDCLASSW wc{};
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = className;

	RegisterClassW(&wc);

	HWND hwnd = CreateWindowExW(
		0, className, L"D3D12 Triangle", WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		nullptr,
		nullptr,
		hInstance,
	nullptr);

	if (hwnd == nullptr)
		return 0;

	ShowWindow(hwnd, cmdShow);

	Luma::Log::Init();

	Camera camera = CreatePerspectiveCamera(
	{
	._angle = 1.4,
	._aspectRatio = 16.f/9.f,
	._near = 0.1f,
	._far = 1000.f});

	// create graphics device
	constexpr GfxDeviceDesc gfxDeviceDesc{};
	GfxDevice gfxDevice= CreateDevice(gfxDeviceDesc);
	FrameSync frameSync = CreateFrameSyncResources(gfxDevice);
	// swapchain
	Swapchain swapchain = CreatSwapChain(gfxDevice, frameSync,
		{
			._aspectRatio = 16./9.,
			._height = 1080,
			._width = 1920,
			._vsyncEnable = true,
			._hwnd = hwnd
		});

	Model model = LoadModel(gfxDevice, frameSync,
		{
		._path = "../../../../assets/models/sponza2/sponza2.gltf"});


	DXCRes dxcRes = ShaderCompiler();
	wchar_t shaderPath[] = L"../../../../shaders/shaders/triangle.hlsl";
	Shader vertexShader = CreateShader(gfxDevice, dxcRes,
		{
		._shaderPath = shaderPath,
		._pEntryPoint = L"VSMain",
		._pTarget = L"vs_6_6",
		._type = Type::VERTEX});

	Shader pixelShader = CreateShader(gfxDevice, dxcRes,
		{
		._shaderPath = shaderPath,
		._pEntryPoint = L"PSMain",
		._pTarget = L"ps_6_6",
		._type = Type::PIXEL});
	// PSO
	Pipeline pipeline = CreatePipeline(gfxDevice,
		{
		._shaders = {vertexShader, pixelShader},
		._enableDepthTest = TRUE,
		._enableStencilTest = FALSE});
	// wait for the assets to be uploaded before rendering the frame
	WaitForGPU(gfxDevice, frameSync);

	ComPtr<ID3D12GraphicsCommandList1> commandList = CreateCommandList(gfxDevice);
	DX_ASSERT(commandList->Close());
	
	auto onRender = [&]()
	{
		SubmitandPresent(commandList, gfxDevice, swapchain,
			frameSync, camera, pipeline, model);
		MoveToNextFrame(gfxDevice, swapchain, frameSync);
	};

	auto updateCamera = [&]()
		{

		};

	MSG msg{};
	if (msg.message == WM_QUIT)
	{
		WaitForGPU(gfxDevice, frameSync);
		DestroyDevice(gfxDevice);
		PostQuitMessage(0);
	}
	while (WM_QUIT != msg.message)
	{
		if (PeekMessageW(&msg, nullptr, 0,0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
		else
		{
			onRender();
			updateCamera();
		}
	}
	return 0;
}

LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_DESTROY:
		isOpen = false;
		PostQuitMessage(0);
		return 0;
	default:
		return DefWindowProcW(hwnd, uMsg, wParam, lParam);
	}
}