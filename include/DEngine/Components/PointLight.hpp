#pragma once

#include "DEngine/Components/Components.hpp"
#include "DEngine/Enum.hpp"

#include "DMath/Vector/Vector.hpp"
#include "DMath/Matrix/Matrix.hpp"

#include "Utility/Color.hpp"

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

			float intensity{ 1 };
			Math::Vector3D color{ 1, 1, 1 };
			Math::Vector3D positionOffset{};

			[[nodiscard]] Math::Matrix<4, 3> GetModel_Reduced(Space space) const;
			[[nodiscard]] Math::Matrix4x4 GetModel(Space space) const;
			[[nodiscard]] Math::Vector<3, float> GetPosition(Space space) const;
		};
	}
}