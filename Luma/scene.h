#pragma once
#include <d3d12.h>
#include <imgui.h>
#include <string>
#include <wrl/client.h>
#include "StandardTypes.h"
#include "Inspector.h"
#include "Model.h"
#include "Watcher.h"

#include <json.hpp>
using json = nlohmann::json;


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
	Model* model{};
};

struct Scene
{
	std::string modelPath{};
	ComPtr<ID3D12GraphicsCommandList10> commandList;
	DXCRes dxcRes = {};
	ResourceManager* resourceManager;
	Inspector inspector = {};
	Model model = {};
	Watcher watcher{};

	json renderGraphData{};

	void ParseRenderGraph();
	void Render(GfxDevice& gfxDevice, FrameSync& frameSync, Swapchain& swapchain, Camera& camera);
};

Scene CreateScene(GfxDevice& gfxDevice, 
	FrameSync& frameSync, 
	Swapchain& swapchain, 
	Camera& camera, 
	SceneDesc sceneDesc);
