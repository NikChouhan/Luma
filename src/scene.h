#pragma once
#include <d3d12.h>
#include <imgui.h>
#include <string>
#include <wrl/client.h>

#include "Resources.h"
#include "StandardTypes.h"
#include "Inspector.h"
#include "Model.h"

struct Inspector;
using namespace Microsoft::WRL;

struct Camera;
struct Swapchain;
struct FrameSync;
struct GfxDevice;
struct Device;

enum class SceneType : u8
{
	SPONZA,
	BISTRO
};

struct SceneDesc
{
	SceneType _sceneType;
};

struct Scene
{
	std::string _modelPath{};
	ComPtr<ID3D12GraphicsCommandList10> _commandList;
	DXCRes dxcRes = {};
	Resources resources = {};
	Inspector inspector = {};
	Model model = {};

	static void HotReload(GfxDevice& gfxDevice, Swapchain& swapchain, DXCRes& dxcRes, Resources& resources);
	void Render(GfxDevice& gfxDevice, FrameSync& frameSync, Swapchain& swapchain, Camera& camera);
	void HotReloadShaders(GfxDevice& gfxDevice, DXCRes* dxcRes, Swapchain& swapchain);
};

Scene CreateScene(GfxDevice& gfxDevice, 
	FrameSync& frameSync, 
	Swapchain& swapchain, 
	Camera& camera, 
	SceneDesc sceneDesc);
