#include "Camera.hpp"

#include "DMath/LinearTransform3D.hpp"

#include "../Renderer/Renderer.hpp"

namespace Engine
{
	namespace Components
	{
		Camera::Camera(SceneObject& owningObject) :
			ParentType(owningObject),
			positionOffset(),
			fov(defaultFovY),
			forward(Math::Vector3D::Back()),
			up(Math::Vector3D::Up()),
			zNear(defaultZNear),
			zFar(defaultZFar),
			projectionMode(ProjectionMode::Perspective),
			orthographicWidth(defaultOrtographicWidth)
		{
		}

		Camera::~Camera()
		{
		}

		void Camera::LookAt(const Math::Vector3D& newTarget)
		{
			forward = (newTarget - positionOffset).GetNormalized();
		}

		Renderer::CameraInfo Camera::GetCameraInfo() const
		{
			Renderer::CameraInfo cameraInfo;

			cameraInfo.fovY = fov;
			cameraInfo.orthoWidth = orthographicWidth;
			cameraInfo.zNear = zNear;
			cameraInfo.zFar = zFar;

			if (projectionMode == ProjectionMode::Perspective)
				cameraInfo.projectMode = Renderer::CameraInfo::ProjectMode::Perspective;
			else if (projectionMode == ProjectionMode::Orthgraphic)
				cameraInfo.projectMode = Renderer::CameraInfo::ProjectMode::Orthographic;

			cameraInfo.transform = Math::LinTran3D::LookAt_RH(positionOffset, positionOffset + forward, up);

			return cameraInfo;
		}
	}
}


