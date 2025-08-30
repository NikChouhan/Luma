#include "GfxDevice.h"
#include "Swapchain.h"
#include "Pipeline.h"
#include "FrameSync.h"
#include "Buffer.h"

struct FramePresent
{
	ComPtr<ID3D12GraphicsCommandList> commandList;
	Swapchain swapchain;
	FrameSync frameSync;
	Pipeline pipeline;
	Buffer vertexBuffer;
};

static bool isOpen = true;

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PWSTR pCmdLine, int cmdShow)
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
	// create graphics device
	const GfxDeviceDesc gfxDeviceDesc = { ._width = 1920, ._height = 1080, ._aspectRatio = 16. / 9., ._hwnd = hwnd };
	GfxDevice gfxDevice= CreateDevice(gfxDeviceDesc);
	// swapchain
	Swapchain swapchain = CreatSwapChain(gfxDevice,
		{._vsyncEnable = true});

	// vertex buffer for triangle
	Vertex triangleVertices[] =
	{
		{.position = { 0.0f, 0.25f * gfxDeviceDesc._aspectRatio, 0.0f },
			.color = { 1.0f, 0.0f, 0.0f, 1.0f } },
		{.position = { 0.25f, -0.25f * gfxDeviceDesc._aspectRatio, 0.0f },
			.color = { 0.0f, 1.0f, 0.0f, 1.0f } },
		{.position = { -0.25f, -0.25f * gfxDeviceDesc._aspectRatio, 0.0f },
			.color = { 0.0f, 0.0f, 1.0f, 1.0f } }
	};
	Buffer vertexBuffer = CreateBuffer(gfxDevice,
		{
		._bufferSize = sizeof(triangleVertices),
		._pContents = triangleVertices,});
	// shaders
	constexpr wchar_t shaderPath[] = L"../../../../shaders/triangle.hlsl";
	Shader vertexShader = CreateShader(gfxDevice,
		{
		._shaderPath = shaderPath,
		._pEntryPoint = "VSMain",
		._pTarget = "vs_5_0",
		._type = Type::VERTEX});
	Shader pixelShader = CreateShader(gfxDevice,
		{
		._shaderPath = shaderPath,
		._pEntryPoint = "PSMain",
		._pTarget = "ps_5_0",
		._type = Type::PIXEL});
	// PSO
	Pipeline pipeline = CreatePipeline(gfxDevice,
		{
		._shaders = {vertexShader, pixelShader},
		._enableDepthTest = false,
		._enableStencilTest = false});
	FrameSync frameSync = CreateFrameSyncResources(gfxDevice);
	// wait for the assets to be uploaded before rendering the frame
	WaitForGPU(gfxDevice, frameSync);

	// create command list
	ComPtr<ID3D12GraphicsCommandList> commandList{};
	DX_ASSERT(gfxDevice._device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
		gfxDevice._commandAllocators[frameSync._frameIndex].Get(),
		pipeline._pipelineState.Get(), IID_PPV_ARGS(&commandList)));
	DX_ASSERT(commandList->Close());
	
	auto onRender = [&]()
	{
		SubmitandPresent(commandList, gfxDevice, swapchain,
			frameSync, pipeline, vertexBuffer);
		MoveToNextFrame(gfxDevice, swapchain, frameSync);
	};

	MSG msg{};
	if (msg.message == WM_QUIT)
	{
		DestroyDevice(gfxDevice);
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