#pragma once

#include "ComponentBase.hpp"
#include "../Enum.hpp"

#include "Math/Vector/Vector.hpp"

namespace Engine
{
	class CircleCollider2D : public ComponentBase
	{
	public:
		using ParentType = ComponentBase;

		explicit CircleCollider2D(SceneObject& owningObject);
		~CircleCollider2D();

		Math::Matrix<3, 2> GetModel2D_Reduced(Space space) const;

		Math::Vector2D position;
		float radius;
	};
}