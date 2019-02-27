#include "SingletonComponentBase.hpp"

#include "../Scene.hpp"

Engine::SingletonComponentBase::SingletonComponentBase(SceneObject& owner, size_t indexInScene) noexcept :
    sceneObjectRef(owner),
    indexInScene(indexInScene)
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

size_t Engine::SingletonComponentBase::GetIndexInScene() const
{
    return indexInScene;
}
