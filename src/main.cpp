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

bool isOpen = true;
bool keys[256] = {};
int lastMouseX = 0;
int lastMouseY = 0;
int currentMouseX = 0;
int currentMouseY = 0;

static bool isMouseCaptured = false;

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static void HandleCamera(Camera& camera, f32 deltaTime);

struct AppData
{
	GfxDevice* gfxDevice;
	FrameSync* frameSync;
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

	AppData appData{ &gfxDevice, &frameSync };


	HWND hwnd = CreateWindowExW(
		0, className, L"Luma", WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		nullptr,
		nullptr,
		hInstance,
		&appData);

	if (hwnd == nullptr)
		return 0;

	ShowWindow(hwnd, cmdShow);

	Log::Init();

	Camera camera = CreatePerspectiveCamera(
	{
	._angle = 1.3,
	._aspectRatio = 16.f/9.f,
	._near = 0.1f,
	._far = 10000.f});

	// swapchain
	Swapchain swapchain = CreatSwapChain(gfxDevice, frameSync,
		{
			._aspectRatio = 16./9.,
			._height = 1080,
			._width = 1920,
			._vsyncEnable = true,
			._hwnd = hwnd
		});
	ComPtr<ID3D12GraphicsCommandList10> commandList = CreateCommandList(gfxDevice);
	DX_ASSERT(commandList->Close());
	//std::string modelPath = "../../../../assets/models/bistro2/bistro2.gltf";
	std::string modelPath = "../../../../assets/models/sponza2/sponza2.gltf";
	//std::string modelPath = "../../../../assets/models/haunted_house/haunted_house.gltf";
	Model model = LoadModel(gfxDevice, frameSync, commandList.Get(),
		{
		._path = modelPath});

	DXCRes dxcRes = ShaderCompiler();

	// compute pass for shader effects
	wchar_t backdropCSPath[] = L"../../../../shaders/shaders/space_backdrop.hlsl";
	Shader backdropCS = CreateShader(gfxDevice, dxcRes,
		{
		._shaderPath = backdropCSPath,
		._pEntryPoint = L"CSMain",
		._pTarget = L"cs_6_7",
		._type = Type::COMPUTE });
	Pipeline backdropComputePipeline = CreatePipeline(gfxDevice, swapchain,
		{
		._pipelineType = PipelineType::COMPUTE,
		._shaders = {backdropCS},
		._enableDepthTest = FALSE,
		._enableStencilTest = FALSE,
		._isDepthPrePass = FALSE });

	// depth pipeline
	wchar_t depthPPVertexShaderPath[] = L"../../../../shaders/shaders/depth_pass.hlsl";
	Shader depthPrePassVS= CreateShader(gfxDevice, dxcRes,
		{
		._shaderPath = depthPPVertexShaderPath,
		._pEntryPoint = L"DepthVS",
		._pTarget = L"vs_6_7",
		._type = Type::VERTEX });

	Pipeline depthPrePass = CreatePipeline(gfxDevice, swapchain,
		{
		._pipelineType = PipelineType::GRAPHICS,
		._shaders = {depthPrePassVS} ,
		._enableDepthTest = TRUE,
		._enableStencilTest = FALSE,
		._isDepthPrePass = TRUE });

	// inline ray tracing pass
	wchar_t inlineRayTracingVertexShaderPath[] = L"../../../../shaders/shaders/model.hlsl";
	Shader inlineRayTracingVertexShader = CreateShader(gfxDevice, dxcRes,
		{
		._shaderPath = inlineRayTracingVertexShaderPath,
		._pEntryPoint = L"VSMain",
		._pTarget = L"vs_6_7",
		._type = Type::VERTEX });
	wchar_t inlineRayTracingPixelShaderPath[] = L"../../../../shaders/shaders/model.hlsl";
	Shader inlineRayTracingPixelShader = CreateShader(gfxDevice, dxcRes,
		{
		._shaderPath = inlineRayTracingPixelShaderPath,
		._pEntryPoint = L"PSMain",
		._pTarget = L"ps_6_7",
		._type = Type::PIXEL });
	Pipeline rasterPipeline = CreatePipeline(gfxDevice, swapchain,
		{
		._pipelineType = PipelineType::GRAPHICS,
		._shaders = {inlineRayTracingVertexShader, inlineRayTracingPixelShader},
		._enableDepthTest = TRUE,
		._enableStencilTest = FALSE,
		._isDepthPrePass = FALSE });

	WaitForGPU(gfxDevice, frameSync);

	auto on_render = [&]()
	{
		WaitForGPU(gfxDevice, frameSync);
		DX_ASSERT(gfxDevice._commandAllocators[frameSync._frameIndex]->Reset());
		DX_ASSERT(commandList->Reset(gfxDevice._commandAllocators[frameSync._frameIndex].Get(), nullptr));

		// 
		SubmitPasses(commandList, gfxDevice, swapchain, frameSync, 
			camera, backdropComputePipeline, depthPrePass, rasterPipeline, model);
		// present the Frame
		DX_ASSERT(swapchain._swapchain->Present(0, DXGI_PRESENT_ALLOW_TEARING));
		MoveToNextFrame(gfxDevice, swapchain, frameSync);
	};
	LARGE_INTEGER perfFrequency;
	QueryPerformanceFrequency(&perfFrequency);

	LARGE_INTEGER lastFrameTime;
	QueryPerformanceCounter(&lastFrameTime);

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

				/*printl(Log::LogLevel::Info, "[Camera] Camera position: x: {}, y: {}, z: {}", 
				   camera._pos.x, camera._pos.y, camera._pos.z);*/
				timeSinceLastUpdate = 0.f;
			}
			camera._time += deltaTime / 10.;
		}
	}
	return 0;
}

LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	AppData* appData = nullptr;

	if (uMsg == WM_CREATE)
	{
		CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
		appData = reinterpret_cast<AppData*>(pCreate->lpCreateParams);
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