#pragma once

#include <wrl/client.h>
#include "StandardTypes.h"
#include "Core/Camera.h"
#include "Renderer/Model.h"

struct RenderObject
{
	Model* model = nullptr;
	SM::Matrix transform = SM::Matrix::Identity;
	u32 id;
};

struct DirectionalLight
{
	SM::Vector3 direction_{ 0.,0,0. };
	SM::Vector3 color_{1.,1.,1.};
	float intensity_ = 1.f;
};

struct Scene
{
	Scene(const GfxDevice& gfxDevice, ResourceManager& resourceManager);
	~Scene() = default;

	void Load();
	void Update(float deltaTime);

	[[nodiscard]] const Camera& GetCamera() const { return camera_;}
	Camera& GetCamera() { return  camera_;}

	[[nodiscard]] const std::vector<RenderObject>& GetRenderObjects() const { return renderObjects_; }
	[[nodiscard]] const DirectionalLight& GetSun() const { return sun_; }

private:
	const GfxDevice& gfxDevice_;
	ResourceManager& resourceManager_;

	Camera camera_;
	DirectionalLight sun_;

	std::vector<std::unique_ptr<Model>> loadedModels_;
	std::vector<RenderObject> renderObjects_;
};