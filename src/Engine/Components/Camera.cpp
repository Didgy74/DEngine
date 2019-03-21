#include "Camera.hpp"

#include "DMath/LinearTransform3D.hpp"

#include "../Renderer/Renderer.hpp"

namespace Engine
{
	namespace Components
	{
		Camera::Camera(SceneObject& owningObject) :
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

		Camera::~Camera()
		{
		}

		void Camera::LookAt(const Math::Vector3D& newTarget)
		{
			forward = (newTarget - position).GetNormalized();
		}

		Renderer::CameraInfo Camera::GetCameraInfo() const
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
	}
}


