#include "CircleCollider.hpp"

namespace Engine
{
	namespace Components
	{
		CircleCollider::CircleCollider(SceneObject& owningObject) :
			ParentType(owningObject),
			positionOffset(),
			radius(1)
		{
		}

		CircleCollider::~CircleCollider()
		{

		}
		Math::Matrix<4, 3, float> CircleCollider::GetModel_Reduced(Space space) const
		{
			return Math::Matrix<4, 3, float>();
		}
	}
}