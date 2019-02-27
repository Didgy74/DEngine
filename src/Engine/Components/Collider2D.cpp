#include "Collider2D.hpp"

Engine::Collider2D::Collider2D(SceneObject& owningObject, size_t indexInSceneObject, size_t indexInScene) :
	ParentType(owningObject, indexInSceneObject, indexInScene),
	isStatic(false)
{
}

Engine::Collider2D::~Collider2D()
{
}
