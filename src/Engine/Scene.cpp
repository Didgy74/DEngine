#include "Scene.hpp"

#include <algorithm>
#include <cassert>

#include "SceneObject.hpp"

Engine::Scene::Scene(size_t indexInEngine) :
	indexInEngine(indexInEngine),
	renderSceneValid(true),
	timeData(),
	physics2DData()
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

bool Engine::Scene::RenderSceneValid() const { return renderSceneValid; }

Engine::Time::SceneData& Engine::Scene::GetTimeData() { return timeData; }

void Engine::Scene::InvalidateRenderScene()
{
	renderSceneValid = false;
}

void Engine::Scene::RemoveSceneObject(SceneObject& owningObject)
{

}
