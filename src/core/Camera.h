#pragma once
#include "GfxDevice.h"

struct Camera
{
	SM::Vector3 _pos{};
	SM::Vector3 _target{};
	SM::Vector3 _up{};

	f32 _angle{};
	f32 _aspectRatio{};
	f32 _near{};
	f32 _far{};


	SM::Matrix _ortho{};
	SM::Matrix _projection{};
	SM::Matrix _view{};
};

struct OrtCameraDesc
{
	
};

struct PersCameraDesc
{
	f32 _angle{};
	f32 _aspectRatio{};
	f32 _near{};
	f32 _far{};
};

Camera CreatePerspectiveCamera(PersCameraDesc cameraDesc);
Camera CreateOrthographicCamera(OrtCameraDesc);
