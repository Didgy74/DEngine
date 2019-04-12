#include "DEngine/Scene.hpp"

#include <algorithm>
#include <cassert>

#include "DEngine/SceneObject.hpp"

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

	const Time::SceneData& Scene::GetTimeData() const { return timeData; }

	Time::SceneData& Scene::GetTimeData() { return timeData; }

	void Scene::RemoveSceneObject(SceneObject& owningObject)
	{

	}

	const SceneObject& Scene::GetSceneObject(size_t index) const { return *sceneObjects[index]; }

	SceneObject& Scene::GetSceneObject(size_t index) { return *sceneObjects[index]; }

	size_t Scene::GetSceneObjectCount() const { return sceneObjects.size(); }

	void Scene::Clear()
	{
		sceneObjects.clear();
		components.clear();
	}

	void Scene::Scripts_SceneStart()
	{
		for (auto& ptr : scriptComponents)
		{
			auto& ref = *ptr;
			ref.SceneStart();
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




