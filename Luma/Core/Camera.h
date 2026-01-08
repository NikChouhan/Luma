#pragma once
#include "Core/Common.h"

struct Camera
{
	SM::Vector3 pos{};
	SM::Vector3 target{};
	SM::Vector3 up{};

	f32 angle{};
	f32 aspectRatio{};
	f32 nearPlane{};
	f32 farPlane{};

	SM::Matrix ortho{};
	SM::Matrix projection{};
	SM::Matrix view{};

	float yawAngle;
	float pitchAngle;

	float time;
};

struct OrtCameraDesc
{
	
};

struct PersCameraDesc
{
	f32 angle{};
	f32 aspectRatio{};
	f32 nearPlane{};
	f32 farPlane{};
};

Camera CreatePerspectiveCamera(PersCameraDesc cameraDesc);
Camera CreateOrthographicCamera(OrtCameraDesc);

void InitViewMatrix(Camera& camera);
void Translate(Camera& camera, SM::Vector3 direction);
