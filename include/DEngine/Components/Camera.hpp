#pragma once

#include "DEngine/Components/Components.hpp"
#include "DEngine/Enum.hpp"

#include "DMath/Vector/Vector.hpp"
#include "DMath/Matrix/Matrix.hpp"
#include "DMath/UnitQuaternion.hpp"

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

			Math::Vector3D positionOffset{};
			Math::UnitQuaternion<> rotation{};
			float fov = defaultFovY;
			float orthographicWidth = defaultOrtographicWidth;
			float zNear = defaultZNear;
			float zFar = defaultZFar;
			ProjectionMode projectionMode = ProjectionMode::Perspective;

			void LookAt(const Math::Vector3D& newTarget, Space space);

			[[nodiscard]] Math::Matrix<4, 3, float> GetModel_Reduced(Space space) const;
			[[nodiscard]] Math::Matrix<4, 4, float> GetModel(Space space) const;
			[[nodiscard]] Math::Matrix<4, 4, float> GetViewModel(Space space) const;

			[[nodiscard]] Math::Vector<3, float> GetPosition(Space space) const;
		};
	}
}