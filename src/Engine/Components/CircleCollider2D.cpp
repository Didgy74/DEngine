#include "CircleCollider2D.hpp"

#include "Math/LinearTransform2D.hpp"

#include "../Scene.hpp"
#include "../SceneObject.hpp"

Engine::CircleCollider2D::CircleCollider2D(SceneObject& owningObject, size_t indexInSceneObject, size_t indexInScene) :
	ParentType(owningObject, indexInSceneObject, indexInScene),
	radius(0.5f),
	position{0, 0}
{
}

Engine::CircleCollider2D::~CircleCollider2D()
{
}

Math::Matrix<3, 2> Engine::CircleCollider2D::GetModel2D_Reduced(Space space) const
{
	const auto& localModel = Math::LinTran2D::Translate_Reduced(position);
	if (space == Space::Local)
		return localModel;
	else
	{
		const auto& worldSpaceModel = GetSceneObject().transform.GetModel2D_Reduced(Space::World);
		return Math::LinTran2D::Multiply(worldSpaceModel, localModel);
	}
}
