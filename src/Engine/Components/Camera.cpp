#include "Camera.hpp"

#include "Math/LinearTransform3D.hpp"

Engine::Camera::Camera(SceneObject& owningObject, size_t indexInSceneObject, size_t indexInScene) noexcept :
	ParentType(owningObject, indexInSceneObject, indexInScene),
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

Engine::Camera::operator Engine::Renderer::CameraInfo() const
{
	Renderer::CameraInfo cameraInfo;

	cameraInfo.transform = Math::LinTran3D::LookAtRH(position, position + forward, up);
	cameraInfo.fovY = fov;
	
	if (activeProjectionMode == ProjectionMode::Perspective)
		cameraInfo.projectMode = Renderer::CameraInfo::ProjectMode::Projection;
	else
		cameraInfo.projectMode = Renderer::CameraInfo::ProjectMode::Orthogonal;

	return cameraInfo;
}
