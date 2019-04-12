#include "DEngine/Components/CircleCollider2D.hpp"

#include "DMath/LinearTransform2D.hpp"

#include "DEngine/Scene.hpp"
#include "DEngine/SceneObject.hpp"

namespace Engine
{
	namespace Components
	{
		CircleCollider2D::CircleCollider2D(SceneObject& owningObject) :
			ParentType(owningObject)
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
				const auto& worldSpaceModel = GetSceneObject().GetModel2D_Reduced(Space::World);
				return Math::LinTran2D::Multiply_Reduced(worldSpaceModel, localModel);
			}
		}
	}
}


