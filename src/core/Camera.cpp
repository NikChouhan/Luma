#include "Camera.h"

void InitViewMatrix(Camera& camera)
{
	camera._view = DirectX::XMMatrixLookAtLH(camera._pos, camera._target, camera._up);
}

Camera CreatePerspectiveCamera(PersCameraDesc cameraDesc)
{
	Camera camera = {};

	camera._pos = SM::Vector3(2., 1., -0.3993596);
	//camera._pos = SM::Vector3(0., 0., -20.);

	camera._target = SM::Vector3(-5.9500113, 1., -0.468321);

	camera._up = SM::Vector3(0.f, 1.f, 0.f);
		
	camera._angle = cameraDesc._angle;
	camera._aspectRatio = cameraDesc._aspectRatio;
	camera._near = cameraDesc._near;
	camera._far = cameraDesc._far;

	camera._projection = DirectX::XMMatrixPerspectiveFovLH(camera._angle,
		camera._aspectRatio, camera._far, camera._near);

	InitViewMatrix(camera);

	return camera;
}

void Translate(Camera& camera, SM::Vector3 direction)
{
	camera._pos += direction;
	camera._target += direction;
	InitViewMatrix(camera);
}

