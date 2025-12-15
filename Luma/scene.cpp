#include "scene.h"

static void HandleCamera(Camera& camera, f32 deltaTime);

Scene::Scene(const GfxDevice& gfxDevice, ResourceManager& resourceManager)
	: gfxDevice_(gfxDevice), resourceManager_(resourceManager) {}

void Scene::Load()
{
	camera_ = CreatePerspectiveCamera(
		{
			._angle = 1.3,
			._aspectRatio = 16.f / 9.f,
			._near = 0.1f,
			._far = 10000.f
		});

	printl(Log::LogLevel::Info, "[Scene] Loading World ...");

	auto sponza = std::make_unique<Model>(gfxDevice_, &resourceManager_);
	sponza->Load("../../../../assets/model/sponza2/sponza2.gltf");

	renderObjects_.push_back(RenderObject{
	.model = sponza.get(),
	.transform = SM::Matrix::CreateScale(1.f),
		});

	loadedModels_.push_back(std::move(sponza));
	printl(Log::LogLevel::Info, "[Scene] Loading complete. Objects: {}", renderObjects_.size());
}

void Scene::Update(float deltaTime)
{
	HandleCamera(camera_, deltaTime);
}

void HandleCamera(Camera& camera, f32 deltaTime)
{
	if (IGS::isMouseCaptured)
	{
		int deltaX = IGS::currentMouseX - IGS::lastMouseX;
		int deltaY = IGS::currentMouseY - IGS::lastMouseY;

		if (deltaX != 0 || deltaY != 0)
		{
			constexpr float lookSensitivity = 0.1f;
			float yaw = static_cast<float>(deltaX) * lookSensitivity * (DirectX::XM_PI / 180.0f);
			float pitch = static_cast<float>(deltaY) * -lookSensitivity * (DirectX::XM_PI / 180.0f);

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

			IGS::lastMouseX = IGS::currentMouseX;
			IGS::lastMouseY = IGS::currentMouseY;

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
	if (IGS::keys['W']) movement += forward * moveSpeed * deltaTime;
	if (IGS::keys['A']) movement += right * moveSpeed * deltaTime;
	if (IGS::keys['S']) movement -= forward * moveSpeed * deltaTime;
	if (IGS::keys['D']) movement -= right * moveSpeed * deltaTime;
	if (IGS::keys['Q']) movement -= up * moveSpeed * deltaTime;
	if (IGS::keys['E']) movement += up * moveSpeed * deltaTime;

	Translate(camera, movement);
}