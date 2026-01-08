#include "Camera.h"

void InitViewMatrix(Camera& camera)
{
	camera.view = DirectX::XMMatrixLookAtLH(camera.pos, camera.target, camera.up);
}

Camera CreatePerspectiveCamera(PersCameraDesc cameraDesc)
{
	Camera camera = {};

	camera.pos = SM::Vector3(2., 1., -0.3993596);
	//camera._pos = SM::Vector3(0., 0., -20.);

	camera.target = SM::Vector3(-5.9500113, 1., -0.468321);

	camera.up = SM::Vector3(0.f, 1.f, 0.f);
		
	camera.angle = cameraDesc.angle;
	camera.aspectRatio = cameraDesc.aspectRatio;
	camera.nearPlane = cameraDesc.nearPlane;
	camera.farPlane = cameraDesc.farPlane;

	camera.projection = DirectX::XMMatrixPerspectiveFovLH(camera.angle,
		camera.aspectRatio, camera.farPlane, camera.nearPlane);

	InitViewMatrix(camera);

	return camera;
}

void Translate(Camera& camera, SM::Vector3 direction)
{
	camera.pos += direction;
	camera.target += direction;
	InitViewMatrix(camera);
}
