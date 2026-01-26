#include "scene.h"

#include "Graphics/Globals.h"

static void HandleCamera(Camera& camera, f32 deltaTime);

Scene::Scene() {}

void Scene::Load()
{
	camera_ = CreatePerspectiveCamera(
		{
			.angle = 1.3,
			.aspectRatio = 16.f / 9.f,
			.nearPlane = 0.1f,
			.farPlane = 10000.f
		});

	printl(Log::LogLevel::Info, "[Scene] Loading World ...");

	auto sponza = std::make_unique<Model>();
	//sponza->Load("../../../../assets/models/bistroWithEmissive/Untitled.gltf");
	sponza->Load("../../../../assets/models/sponza2/sponza2.gltf");
	//sponza->Load("../../../../assets/models/Sponza/glTF/Sponza.gltf");
	//sponza->Load("../../../../assets/models/sponzaJeremiah/sponza.gltf");
	//sponza->Load("../../../../assets/models/sponzaBanner/sponza.gltf");

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

	printl(Log::LogLevel::Info, "Camera pos-> x:{}, y: {}, z: {}", camera_.pos.x, camera_.pos.y, camera_.pos.z);
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

			SM::Vector3 forward = camera.target - camera.pos;
			SM::Vector3 right = forward.Cross(camera.up);
			right.Normalize();

			SM::Matrix yawRotation = SM::Matrix::CreateFromAxisAngle(SM::Vector3::UnitY, yaw);
			SM::Matrix pitchRotation = SM::Matrix::CreateFromAxisAngle(right, pitch);

			forward = XMVector3TransformNormal(forward, yawRotation);
			forward = XMVector3TransformNormal(forward, pitchRotation);

			camera.target = camera.pos + forward;

			right = forward.Cross(SM::Vector3::UnitY);
			camera.up = right.Cross(forward);

			IGS::lastMouseX = IGS::currentMouseX;
			IGS::lastMouseY = IGS::currentMouseY;

			InitViewMatrix(camera);

			camera.yawAngle = yaw;
			camera.pitchAngle = pitch;
		}
	}

	constexpr float moveSpeed = 8.f;

	SM::Vector3 forward = camera.target - camera.pos;
	forward.Normalize();

	SM::Vector3 up = camera.up;
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