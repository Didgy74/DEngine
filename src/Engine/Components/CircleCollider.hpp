#pragma once

#include "Components.hpp"
#include "../Enum.hpp"

#include "DMath/Vector/Vector.hpp"

namespace Engine
{
	namespace Components
	{
		class CircleCollider : ComponentBase
		{
		public:
			using ParentType = ComponentBase;

			explicit CircleCollider(SceneObject& owningObject);
			~CircleCollider();

			Math::Matrix<4, 3, float> GetModel_Reduced(Space space) const;

			Math::Vector<3, float> positionOffset;
			float radius;
		};
	}
}