#include "Scene.hpp"

#include <algorithm>
#include <cassert>

#include "SceneObject.hpp"

Engine::Scene::Scene(size_t indexInEngine) :
	indexInEngine(indexInEngine),
	timeData(),
	physics2DData(),
	componentGUIDCounter(0)
{
}

Engine::Scene::~Scene()
{
}

Engine::SceneObject& Engine::Scene::NewSceneObject()
{
	sceneObjects.emplace_back(new SceneObject(*this, sceneObjects.size()));
	return *sceneObjects.back();
}

size_t Engine::Scene::GetIndexInEngine() const { return indexInEngine; }

Engine::Time::SceneData& Engine::Scene::GetTimeData() { return timeData; }

void Engine::Scene::RemoveSceneObject(SceneObject& owningObject)
{

}

const Engine::SceneObject& Engine::Scene::GetSceneObject(size_t index) const { return *sceneObjects[index]; }

Engine::SceneObject& Engine::Scene::GetSceneObject(size_t index) { return *sceneObjects[index]; }

size_t Engine::Scene::GetSceneObjectCount() const { return sceneObjects.size(); }

void Engine::Scene::Clear()
{
	components.clear();
}


