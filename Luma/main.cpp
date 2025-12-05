#include <pch.h>

#include <windowsx.h>
#include <fstream>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "Graphics/GfxDevice.h"
#include "Graphics/D3D12/Swapchain.h"
#include "Graphics/D3D12/Pipeline.h"
#include "Graphics/FrameSync.h"
#include "Graphics/D3D12/Buffer.h"
#include "Graphics/D3D12/Texture.h"
#include "Core/Log.h"

#include "Graphics/Globals.h"

#include "Core/Camera.h"
#include "Renderer/Model.h"
#include "Graphics/D3D12/RootSignature.h"
#include "Renderer/Inspector.h"

#include "scene.h"
#include "Core/Timer.h"


extern "C++" IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static bool isOpen = true;
static bool keys[256] = {};
static int lastMouseX = 0;
static int lastMouseY = 0;
static int currentMouseX = 0;
static int currentMouseY = 0;

static bool isMouseCaptured = false;

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static void HandleCamera(Camera& camera, f32 deltaTime);

struct AppData
{
	GfxDevice* gfxDevice;
	FrameSync* frameSync;
	Swapchain* swapchain;
	Model* model;
};

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

	// create graphics device
	constexpr GfxDeviceDesc gfxDeviceDesc{};
	GfxDevice gfxDevice = CreateDevice(gfxDeviceDesc);
	FrameSync frameSync = CreateFrameSyncResources(gfxDevice);
	AppData appData{.gfxDevice = &gfxDevice, .frameSync = &frameSync };

	HWND hwnd = CreateWindowExW(
		0, className, L"Luma", WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		nullptr,
		nullptr,
		hInstance,
		&appData);
	if (hwnd == nullptr)
		return 0;

	Log::Init();

	Camera camera = CreatePerspectiveCamera(
	{
	._angle = 1.3,
	._aspectRatio = 16.f/9.f,
	._near = 0.1f,
	._far = 10000.f});

	// swapchain
	Swapchain swapchain = CreateSwapChain(gfxDevice, frameSync,
		{
			._height = 720,
			._width = 1280,
			._vsyncEnable = true,
			._hwnd = hwnd
		});

	appData.swapchain = &swapchain;

	ComPtr<ID3D12GraphicsCommandList10> commandList = CreateCommandList(gfxDevice);


	std::string modelPath = "../../../../assets/models/sponza2/sponza2.gltf";
	//std:: string modelPath = "../../../../assets/models/bistro3/scene.gltf";

	Model model = LoadModel(gfxDevice, frameSync, swapchain,commandList.Get(),
		{
		._path = modelPath });

	appData.model = &model;
	ShowWindow(hwnd, cmdShow);

	Scene scene = CreateScene(gfxDevice,
		frameSync,
		swapchain,
		camera,
		{
		.model = &model});

	Timer timer = CreateTimer();

	SetupLightSettingsHandler();

	MSG msg{};

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

			f32 deltaTime = static_cast<f32>(currentFrameTime.QuadPart - timer._lastFrameTime.QuadPart) / static_cast<f32>(timer._perfFrequency.QuadPart);

			timer._lastFrameTime = currentFrameTime;

			HandleCamera(camera, deltaTime);
			scene.Render(gfxDevice, frameSync, swapchain, camera);

			camera._time += deltaTime / 1.;
		}
	}
	WaitForGPU(gfxDevice, frameSync);

	// Cleanup
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
	return 0;
}

LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hwnd, uMsg, wParam, lParam)) 
	{
		return true;
	}
	AppData* appData = nullptr;
	if (uMsg == WM_CREATE)
	{
		CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
		appData = static_cast<AppData*>(pCreate->lpCreateParams);
		SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(appData));
	}
	else
	{
		appData = reinterpret_cast<AppData*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
	}

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

		if (wParam == VK_ESCAPE)
		{
			WaitForGPU(*appData->gfxDevice, *appData->frameSync);
			DestroyDevice(*appData->gfxDevice);
			PostQuitMessage(0);
		}
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
	case WM_SIZE:
	{
		if (appData)
		{
			UINT width = LOWORD(lParam);
			UINT height = HIWORD(lParam);

			// Ignore zero size (minimized)
			if (width == 0 || height == 0)
				return 0;
			if (width == static_cast<UINT>(appData->swapchain->_width) &&
				height == static_cast<UINT>(appData->swapchain->_height))
				return 0;

			appData->swapchain->_width = width;
			appData->swapchain->_height = height;

			// If we're actively resizing, just flag it
			if (appData->swapchain->_isResizing)
			{
				appData->swapchain->_needsResize = true;
			}
			else
			{
				appData->swapchain->ResizeSwapChain(width, height, appData->model);
			}
		}
	return 0;
	}
	case WM_ENTERSIZEMOVE:
	{
		WaitForGPU(*appData->gfxDevice, *appData->frameSync);
		// started dragging/resizing
		if (appData)
		{
			appData->swapchain->_isResizing = true;
		}
		return 0;
	}

	case WM_EXITSIZEMOVE:
	{
		// finished dragging/resizing
		if (appData)
		{
			appData->swapchain->_isResizing = false;

			// Now perform the actual resize if needed
			if (appData->swapchain->_needsResize)
			{
				WaitForGPU(*appData->gfxDevice, *appData->frameSync);
				appData->swapchain->ResizeSwapChain(appData->swapchain->_width, appData->swapchain->_height, appData->model);
				appData->swapchain->_needsResize = false;
			}
		}
		return 0;
	}
	//case WM_GETMINMAXINFO:
	//{
	//	// Optional: Set minimum window size
	//	MINMAXINFO* minMaxInfo = reinterpret_cast<MINMAXINFO*>(lParam);
	//	minMaxInfo->ptMinTrackSize.x = 320;
	//	minMaxInfo->ptMinTrackSize.y = 240;
	//	return 0;
	//}
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

			camera._yaw = yaw;
			camera._pitch = pitch;
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