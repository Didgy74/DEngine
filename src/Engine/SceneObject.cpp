#include "SceneObject.hpp"

#include "Engine.hpp"

#include "Scene.hpp"

#include "Math/LinearTransform2D.hpp"

Engine::SceneObject::SceneObject(Scene& owningScene, size_t indexInScene) : 
	transform(*this),
	parent(nullptr),
	sceneRef(owningScene)
{
}

Engine::SceneObject::~SceneObject()
{
	
}

Engine::Scene& Engine::SceneObject::GetScene() 
{
	return sceneRef.get();
}

Engine::SceneObject* Engine::SceneObject::GetParent() const { return parent; }

void Engine::Destroy(SceneObject& sceneObject)
{
	Core::Destroy(sceneObject);
}
