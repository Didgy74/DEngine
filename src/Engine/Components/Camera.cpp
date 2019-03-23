#include "Camera.hpp"

#include "../SceneObject.hpp"

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

		Math::Matrix<4, 3, float> Camera::GetModel_Reduced(Space space) const
		{
			using namespace Math::LinTran3D;
			const auto& localModel = Translate_Reduced(positionOffset);
			if (space == Space::Local)
				return localModel;
			else
				return Multiply(GetSceneObject().transform.GetModel_Reduced(Space::World), localModel);
		}

		Renderer::CameraInfo Camera::GetRendererCameraInfo() const
		{
			Renderer::CameraInfo cameraInfo;

			cameraInfo.fovY = fov;
			cameraInfo.orthoWidth = orthographicWidth;
			cameraInfo.zNear = zNear;
			cameraInfo.zFar = zFar;

			if (projectionMode == ProjectionMode::Perspective)
				cameraInfo.projectMode = Renderer::CameraInfo::ProjectionMode::Perspective;
			else if (projectionMode == ProjectionMode::Orthgraphic)
				cameraInfo.projectMode = Renderer::CameraInfo::ProjectionMode::Orthographic;

			cameraInfo.transform = Math::LinTran3D::LookAt_RH(positionOffset, positionOffset + forward, up);

			auto test = GetModel_Reduced(Space::World);
			cameraInfo.worldSpacePos = Math::LinTran3D::GetTranslation(test);

			return cameraInfo;
		}
	}
}


