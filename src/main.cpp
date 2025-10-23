#include <pch.h>

#include <windowsx.h>
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
#include "RootSignature.h"

//struct FramePresent
//{
//	ComPtr<ID3D12GraphicsCommandList> commandList;
//	Swapchain swapchain;
//	FrameSync frameSync;
//	Pipeline pipeline;
//	Buffer vertexBuffer;
//};

// initially I was going with the all ray traced ("path traced") image
// to be denoised later but it's kinda stupid. I am happy with the
// texture mapping through raster while lights/shadows/AO done with
// Ray tracing. I would have liked the compute shader way (hw agnostic) but
// DXR is fine for now. Custom BVH, etc will be too time costly too early

static bool isOpen = true;
static bool keys[256] = {};
static int lastMouseX = 0;
static int lastMouseY = 0;
static int currentMouseX = 0;
static int currentMouseY = 0;

static bool isMouseCaptured = false;

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static void HandleCamera(Camera& camera, f32 deltaTime);

int WINAPI wWinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR lpCmdLine,
	_In_ int cmdShow
)
{
	constexpr wchar_t className[] = L"Luma";

	WNDCLASSW wc{};
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = className;

	RegisterClassW(&wc);

	HWND hwnd = CreateWindowExW(
		0, className, L"Luma", WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		nullptr,
		nullptr,
		hInstance,
	nullptr);

	if (hwnd == nullptr)
		return 0;

	ShowWindow(hwnd, cmdShow);

	Log::Init();

	Camera camera = CreatePerspectiveCamera(
	{
	._angle = 1.1,
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

	//std::string modelPath = "../../../../assets/models/bistro2/bistro2.gltf";
	std::string modelPath = "../../../../assets/models/sponza2/sponza2.gltf";
	Model model = LoadModel(gfxDevice, frameSync,
		{
		._path = modelPath});

	DXCRes dxcRes = ShaderCompiler();

	// depth pipeline
	wchar_t depthPPShaderPath[] = L"../../../../shaders/shaders/depth_pass.hlsl";
	Shader depthPrePassShader = CreateShader(gfxDevice, dxcRes,
		{
		._shaderPath = depthPPShaderPath,
		._pEntryPoint = L"DepthVS",
		._pTarget = L"vs_6_7",
		._type = Type::VERTEX });
	Pipeline depthPrePass = CreatePipeline(gfxDevice, swapchain, 
	{
	._shaders = {depthPrePassShader},
	._enableDepthTest = TRUE,
	._enableStencilTest = FALSE,
	._enableRasterizer = FALSE,
	._isDepthPrePass = true });

	// 	raster pipeline
	wchar_t shaderPath[] = L"../../../../shaders/shaders/model.hlsl";

	Shader vertexShader = CreateShader(gfxDevice, dxcRes,
		{
		._shaderPath = shaderPath,
		._pEntryPoint = L"VSMain",
		._pTarget = L"vs_6_7",
		._type = Type::VERTEX});

	Shader pixelShader = CreateShader(gfxDevice, dxcRes,
		{
		._shaderPath = shaderPath,
		._pEntryPoint = L"PSMain",
		._pTarget = L"ps_6_7",
		._type = Type::PIXEL});

	RootSign rootSignMain = CreateRootSignature(gfxDevice, { ._isDepthPrePass = false });
	Pipeline graphicsPipeline = CreatePipeline(gfxDevice, swapchain,
		{
		._shaders = {vertexShader, pixelShader},
		._enableDepthTest = TRUE,
		._enableStencilTest = FALSE,
		._enableRasterizer = TRUE,
		._isDepthPrePass = false });

	// wait for the assets to be uploaded before rendering the frame
	WaitForGPU(gfxDevice, frameSync);

	ComPtr<ID3D12GraphicsCommandList10> commandList = CreateCommandList(gfxDevice);
	DX_ASSERT(commandList->Close());
	
	auto on_render = [&]()
	{
		// depth prepass
		SubmitPass(commandList, gfxDevice, swapchain, frameSync, 
			camera, depthPrePass, graphicsPipeline, model);

		// present the Frame
		DX_ASSERT(swapchain._swapchain->Present(0, DXGI_PRESENT_ALLOW_TEARING));
		MoveToNextFrame(gfxDevice, swapchain, frameSync);
	};
	// high resolution time
	LARGE_INTEGER perfFrequency;
	QueryPerformanceFrequency(&perfFrequency);

	LARGE_INTEGER lastFrameTime;
	QueryPerformanceCounter(&lastFrameTime);

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
			LARGE_INTEGER currentFrameTime;
			QueryPerformanceCounter(&currentFrameTime);

			f32 deltaTime = static_cast<f32>(currentFrameTime.QuadPart - lastFrameTime.QuadPart) / static_cast<f32>(perfFrequency.QuadPart);

			lastFrameTime = currentFrameTime;

			HandleCamera(camera, deltaTime);

			on_render();

			static f32 timeSinceLastUpdate = 0.f;
			timeSinceLastUpdate += deltaTime;
			if (timeSinceLastUpdate >= 1.0f)
			{
				wchar_t titleBuffer[64];
				swprintf_s(titleBuffer, L"Luma frametime: %.3f ms", deltaTime * 1000.0f);
				SetWindowTextW(hwnd, titleBuffer);

				printl(Log::LogLevel::Info, "[Camera] Camera position: x: {}, y: {}, z: {}", 
				   camera._pos.x, camera._pos.y, camera._pos.z);

				timeSinceLastUpdate = 0.f;
			}
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
	case WM_KEYDOWN:
		if (wParam < 256) keys[wParam] = true;
		return 0;
	case WM_KEYUP:
		if (wParam < 256) keys[wParam] = false;
		if (wParam == 'M')
		{
			isMouseCaptured = !isMouseCaptured;
			ShowCursor(!isMouseCaptured);

			RECT rect;
			GetClientRect(hwnd, &rect);
			ClientToScreen(hwnd, (POINT*)&rect.left);
			ClientToScreen(hwnd, (POINT*)&rect.right);

			if (isMouseCaptured)
			{
				ClipCursor(&rect);
			}
			else
			{
				ClipCursor(NULL);
			}

			POINT center = { rect.right / 2, rect.bottom / 2 };
			SetCursorPos(center.x, center.y);
			lastMouseX = currentMouseX = center.x;
			lastMouseY = currentMouseY = center.y;
		}
		return 0;
	case WM_MOUSEMOVE:
		lastMouseX = currentMouseX;
		lastMouseY = currentMouseY;
		currentMouseX = GET_X_LPARAM(lParam);
		currentMouseY = GET_Y_LPARAM(lParam);
		return 0;
	//case WM_LBUTTONDOWN:
	//	return 0;
	//case WM_RBUTTONDOWN:
	//	return 0;
	//case WM_MOUSEWHEEL:
	//	return 0;
	default:
		return DefWindowProcW(hwnd, uMsg, wParam, lParam);
	}
}


void HandleCamera(Camera& camera, f32 deltaTime)
{
	if (isMouseCaptured)
	{
		constexpr float lookSensitivity = 0.1f;
		int deltaX = currentMouseX - lastMouseX;
		int deltaY = currentMouseY - lastMouseY;

		if (deltaX != 0 || deltaY != 0)
		{
			float yaw = static_cast<float>(deltaX) * lookSensitivity * (XM_PI / 180.0f);
			float pitch = static_cast<float>(deltaY) * -lookSensitivity * (XM_PI / 180.0f);

			SM::Vector3 forward = camera._target - camera._pos;
			SM::Vector3 right = forward.Cross(camera._up);
			right.Normalize();

			SM::Matrix yawRotation = SM::Matrix::CreateFromAxisAngle(SM::Vector3::UnitY, yaw);
			SM::Matrix pitchRotation = SM::Matrix::CreateFromAxisAngle(right, pitch);

			forward = XMVector3TransformNormal(forward, yawRotation);
			forward = XMVector3TransformNormal(forward, pitchRotation);

			camera._target = camera._pos + forward;

			right = forward.Cross(SM::Vector3::UnitY);
			camera._up = right.Cross(forward);

			lastMouseX = currentMouseX;
			lastMouseY = currentMouseY;

			InitViewMatrix(camera);
		}
	}

	constexpr float moveSpeed = 8.f;

	SM::Vector3 forward = camera._target - camera._pos;
	forward.Normalize();

	SM::Vector3 up = camera._up;
	up.Normalize();

	SM::Vector3 right = forward.Cross(up);
	right.Normalize();

	SM::Vector3 movement(0.0f, 0.0f, 0.0f);
	if (keys['W']) movement += forward * moveSpeed * deltaTime;
	if (keys['A']) movement += right * moveSpeed * deltaTime;
	if (keys['S']) movement -= forward * moveSpeed * deltaTime;
	if (keys['D']) movement -= right * moveSpeed * deltaTime;
	if (keys['Q']) movement -= up * moveSpeed * deltaTime;
	if (keys['E']) movement += up * moveSpeed * deltaTime;
	
	Translate(camera, movement);
}