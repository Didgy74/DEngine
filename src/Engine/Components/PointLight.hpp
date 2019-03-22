#pragma once

#include "Components.hpp"
#include "../Enum.hpp"

#include "DMath/Vector/Vector.hpp"
#include "DMath/Matrix/Matrix.hpp"


namespace Engine
{
	namespace Components
	{
		class PointLight : public ComponentBase
		{
		public:
			using ParentType = ComponentBase;

			explicit PointLight(SceneObject& owningObject);
			~PointLight();

			float intensity;
			Math::Vector3D positionOffset;

			[[nodiscard]] Math::Matrix<4, 3> GetModel_Reduced(Space space) const;
			[[nodiscard]] Math::Matrix4x4 GetModel(Space space) const;
		};
	}
}