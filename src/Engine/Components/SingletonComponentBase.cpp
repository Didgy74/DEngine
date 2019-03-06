#include "SingletonComponentBase.hpp"

#include "../Scene.hpp"

Engine::SingletonComponentBase::SingletonComponentBase(SceneObject& owner) :
    sceneObjectRef(owner)
{

}

Engine::SingletonComponentBase::~SingletonComponentBase()
{

}

Engine::SceneObject &Engine::SingletonComponentBase::GetSceneObject()
{
    return sceneObjectRef.get();
}

const Engine::SceneObject &Engine::SingletonComponentBase::GetSceneObject() const
{
    return sceneObjectRef.get();
}
