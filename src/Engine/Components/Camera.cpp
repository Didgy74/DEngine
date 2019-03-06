#include "Camera.hpp"

#include "Math/LinearTransform3D.hpp"

#include "../Renderer/Renderer.hpp"

Engine::Camera::Camera(SceneObject& owningObject) :
	ParentType(owningObject),
	position(),
	fov(defaultFovY),
	forward(Math::Vector3D::Back()),
	up(Math::Vector3D::Up()),
	zNear(defaultZNear),
	zFar(defaultZFar),
	activeProjectionMode(ProjectionMode::Perspective),
	orthographicWidth(defaultOrtographicWidth)
{
}

Engine::Camera::~Camera()
{
}

void Engine::Camera::LookAt(const Math::Vector3D& newTarget)
{
	forward = (newTarget - position).GetNormalized();
}

Engine::Renderer::CameraInfo Engine::Camera::GetCameraInfo() const
{
	Renderer::CameraInfo cameraInfo;

	cameraInfo.fovY = fov;
	cameraInfo.orthoWidth = orthographicWidth;
	cameraInfo.zNear = zNear;
	cameraInfo.zFar = zFar;

	if (activeProjectionMode == ProjectionMode::Perspective)
		cameraInfo.projectMode = Renderer::CameraInfo::ProjectMode::Perspective;

	cameraInfo.transform = Math::LinTran3D::LookAtRH(position, position + forward, up);

	return cameraInfo;
}
