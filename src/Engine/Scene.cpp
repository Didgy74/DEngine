#include "Scene.hpp"

#include <algorithm>
#include <cassert>

#include "SceneObject.hpp"

namespace Engine
{
	Scene::Scene(size_t indexInEngine) :
			indexInEngine(indexInEngine),
			timeData(),
			physics2DData(),
			componentGUIDCounter(0)
	{
	}

	Scene::~Scene()
	{
	}

	SceneObject& Scene::NewSceneObject()
	{
		sceneObjects.emplace_back(std::make_unique<SceneObject>(*this, sceneObjects.size()));
		return *sceneObjects.back();
	}

	size_t Scene::GetIndexInEngine() const { return indexInEngine; }

	Time::SceneData& Scene::GetTimeData() { return timeData; }

	void Scene::RemoveSceneObject(SceneObject& owningObject)
	{

	}

	const SceneObject& Scene::GetSceneObject(size_t index) const { return *sceneObjects[index]; }

	SceneObject& Scene::GetSceneObject(size_t index) { return *sceneObjects[index]; }

	size_t Scene::GetSceneObjectCount() const { return sceneObjects.size(); }

	void Scene::Clear()
	{
		components.clear();
	}

	void Scene::ScriptStart()
	{
		for (auto& ptr : scriptComponents)
		{
			auto& ref = *ptr;
			ref.Start();
		}
	}

	void Scene::ScriptTick()
	{
		for (auto& ptr : scriptComponents)
		{
			auto& ref = *ptr;
			ref.Tick();
		}
	}
}




