#pragma once

#include "Components.hpp"
#include "../Enum.hpp"

#include "DMath/Vector/Vector.hpp"

#include "../Renderer/Renderer.hpp"

namespace Engine
{
	namespace Components
	{
		class Camera : public ComponentBase
		{
		public:
			using ParentType = ComponentBase;

			enum class ProjectionMode
			{
				Perspective,
				Orthgraphic
			};

			static constexpr float defaultFovY = 50.f;
			static constexpr float defaultOrtographicWidth = 10.f;
			static constexpr float defaultZNear = 0.1f;
			static constexpr float defaultZFar = 100.f;
			static constexpr ProjectionMode defaultProjectionMode = ProjectionMode::Perspective;

			explicit Camera(SceneObject& owningObject);
			~Camera();

			Math::Vector3D positionOffset;
			Math::Vector3D forward;
			Math::Vector3D up;

			float fov;
			float orthographicWidth;
			float zNear;
			float zFar;
			ProjectionMode projectionMode;

			void LookAt(const Math::Vector3D& newTarget);

			[[nodiscard]] Math::Matrix<4, 3, float> GetModel_Reduced(Space space) const;
			Renderer::CameraInfo GetRendererCameraInfo() const;
		};
	}
}