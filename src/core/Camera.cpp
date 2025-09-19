#include "Camera.h"

static void InitViewMatrix(Camera& camera)
{
	camera._view = DirectX::XMMatrixLookAtLH(camera._pos, camera._target, camera._up);
}

Camera CreatePerspectiveCamera(PersCameraDesc cameraDesc)
{
	Camera camera = {};
	//camera._target = SM::Vector3(0., 0., 1.f);
	//camera._up = camera._target.Cross(SM::Vector3(1.,0.,0.));
	//camera._up.Normalize();

	camera._pos = SM::Vector3(0.f, 10.f, -20.f); //

	camera._target = SM::Vector3(0.f, 0.f, 0.f);

	camera._up = SM::Vector3(0.f, 1.f, 0.f);
		
	camera._angle = cameraDesc._angle;
	camera._aspectRatio = cameraDesc._aspectRatio;
	camera._near = cameraDesc._near;
	camera._far = cameraDesc._far;

	camera._projection = DirectX::XMMatrixPerspectiveFovLH(camera._angle,
		camera._aspectRatio, camera._near, camera._far);

	InitViewMatrix(camera);

	return camera;
}


