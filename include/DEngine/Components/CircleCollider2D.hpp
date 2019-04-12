#pragma once

#include "DEngine/Components/Components.hpp"
#include "DEngine/Enum.hpp"

#include "DMath/Vector/Vector.hpp"

namespace Engine
{
	namespace Components
	{
		class CircleCollider2D : public ComponentBase
		{
		public:
			using ParentType = ComponentBase;

			explicit CircleCollider2D(SceneObject& owningObject);
			~CircleCollider2D();

			Math::Matrix<3, 2> GetModel2D_Reduced(Space space) const;

			Math::Vector2D positionOffset{};
			float radius{ 0.5 };
		};
	}
}