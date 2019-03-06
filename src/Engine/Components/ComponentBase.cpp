#include "ComponentBase.hpp"

#include "../Scene.hpp"
#include "../SceneObject.hpp"

Engine::ComponentBase::ComponentBase(SceneObject& owningObject) :
	sceneObjectRef(owningObject)
{
}

Engine::ComponentBase::~ComponentBase()
{

}

Engine::SceneObject& Engine::ComponentBase::GetSceneObject() { return sceneObjectRef.get(); }

const Engine::SceneObject& Engine::ComponentBase::GetSceneObject() const { return sceneObjectRef.get(); }