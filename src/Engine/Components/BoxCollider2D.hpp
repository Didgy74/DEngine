#pragma once

#include "Collider2D.hpp"
#include "../Enum.hpp"

#include "Math/Vector/Vector.hpp"
#include "Math/Matrix/Matrix.hpp"

namespace Engine
{
	class BoxCollider2D : public Collider2D
	{
	public:
		using ParentType = Collider2D;

		BoxCollider2D(SceneObject& owningObject, size_t indexInSceneObject, size_t indexInScene);
		~BoxCollider2D();

		Math::Matrix3x3 GetModel2D(Space space) const;
		Math::Matrix<3, 2> GetModel2D_Reduced(Space space) const;
		Math::Matrix2x2 GetRotationModel2D(Space space) const;
		Math::Matrix3x3 GetRotationModel2D_Homo(Space space) const;
		
		Math::Vector2D position;
		float rotation;
		Math::Vector2D size;
	};
}