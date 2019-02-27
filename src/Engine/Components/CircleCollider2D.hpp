#pragma once

#include "Collider2D.hpp"
#include "../Enum.hpp"

#include "Math/Vector/Vector.hpp"

namespace Engine
{
	class CircleCollider2D : public Collider2D
	{
	public:
		using ParentType = Collider2D;

		CircleCollider2D(SceneObject& owningObject, size_t indexInSceneObject, size_t indexInScene);
		~CircleCollider2D();

		Math::Matrix<3, 2> GetModel2D_Reduced(Space space) const;

		Math::Vector2D position;
		float radius;
	};
}