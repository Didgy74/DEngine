#pragma once

#include "ComponentBase.hpp"
#include "../Enum.hpp"

#include "Math/Vector/Vector.hpp"
#include "Math/Matrix/Matrix.hpp"

namespace Engine
{
	class BoxCollider2D : public ComponentBase
	{
	public:
		using ParentType = ComponentBase;

		explicit BoxCollider2D(SceneObject& owningObject);
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