#include "CircleCollider2D.hpp"

#include "DMath/LinearTransform2D.hpp"

#include "../Scene.hpp"
#include "../SceneObject.hpp"

namespace Engine
{
	namespace Components
	{
		CircleCollider2D::CircleCollider2D(SceneObject& owningObject) :
			ParentType(owningObject),
			radius(0.5f),
			positionOffset{ 0, 0 }
		{
		}

		CircleCollider2D::~CircleCollider2D()
		{
		}

		Math::Matrix<3, 2> CircleCollider2D::GetModel2D_Reduced(Space space) const
		{
			const auto& localModel = Math::LinTran2D::Translate_Reduced(positionOffset);
			if (space == Space::Local)
				return localModel;
			else
			{
				const auto& worldSpaceModel = GetSceneObject().transform.GetModel2D_Reduced(Space::World);
				return Math::LinTran2D::Multiply(worldSpaceModel, localModel);
			}
		}
	}
}


