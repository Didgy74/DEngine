#include "ComponentBase.hpp"

#include "../Scene.hpp"
#include "../SceneObject.hpp"

Engine::ComponentBase::ComponentBase(SceneObject& owningObject, size_t indexInSceneObject, size_t indexInScene) noexcept :
	sceneObjectRef(owningObject),
	indexInScene(indexInScene),
	indexInSceneObject(indexInSceneObject)
{
}

Engine::ComponentBase::~ComponentBase()
{

}

Engine::SceneObject& Engine::ComponentBase::GetSceneObject() { return sceneObjectRef.get(); }

const Engine::SceneObject& Engine::ComponentBase::GetSceneObject() const { return sceneObjectRef.get(); }

size_t Engine::ComponentBase::GetIndexInScene() const { return indexInScene; }