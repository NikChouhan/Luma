#include "Camera.h"

void InitViewMatrix(Camera& camera)
{
	camera._view = DirectX::XMMatrixLookToLH(camera._pos, camera._target, camera._up);
}

Camera CreatePerspectiveCamera(PersCameraDesc cameraDesc)
{
	Camera camera = {};
	//camera._target = SM::Vector3(0., 0., 1.f);
	//camera._up = camera._target.Cross(SM::Vector3(1.,0.,0.));
	//camera._up.Normalize();

	camera._pos = SM::Vector3(-1.9467199, 2.5605016, -0.3993596);

	SM::Vector3 lookAtPoint = SM::Vector3(-5.9500113, 1.7869439, -0.468321);
	camera._target = lookAtPoint - camera._pos;

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

