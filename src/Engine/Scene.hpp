#pragma once

namespace Engine
{
	class Core;
	class Scene;
	class SceneObject;
}

#include "Physics2D.hpp"
#include "Time.hpp"

#include <vector>
#include <functional>
#include <typeindex>
#include <unordered_map>

namespace Engine
{
	class Scene
	{
	public:
		Scene(size_t indexInEngine);
		Scene(const Scene& right) = delete;
		Scene(Scene&& right) noexcept = delete;
		~Scene();

		Scene& operator=(const Scene& right) = delete;
		Scene& operator=(Scene&& right) = delete;

		SceneObject& NewSceneObject();
		const SceneObject& GetSceneObject(size_t index) const;
		SceneObject& GetSceneObject(size_t index);
		size_t GetSceneObjectCount() const;

		template<typename ComponentType>
		const std::vector<ComponentType*>* GetComponents() const;
		template<typename ComponentType>
		const ComponentType& GetComponent(size_t index) const;
		template<typename ComponentType>
		ComponentType& GetComponent(size_t index);
		template<typename ComponentType>
		size_t GetComponentCount() const;

		[[nodiscard]] size_t GetIndexInEngine() const;

		[[nodiscard]] bool RenderSceneValid() const;

	private:
		Time::SceneData& GetTimeData();

		void InvalidateRenderScene();

		template<typename ComponentType>
		[[nodiscard]] ComponentType& AddComponentFromSceneObject(SceneObject& owningObject, size_t indexInSceneObject);
		template<typename ComponentType>
		[[nodiscard]] ComponentType& AddSingletonComponentFromSceneObject(SceneObject& owningObject);
		void RemoveSceneObject(SceneObject& owningObject);

		size_t indexInEngine;
		std::vector<SceneObject*> sceneObjects;
		std::unordered_map<std::type_index, void*> components;
		bool renderSceneValid;

		Time::SceneData timeData;
		Physics2D::SceneData physics2DData;

		friend class Engine::Core;
		friend class Engine::SceneObject;
		friend class RenderComponent;
	};

	template<typename ComponentType>
	const std::vector<ComponentType*>* Scene::GetComponents() const
	{
		return nullptr;
	}

	template<typename ComponentType>
	ComponentType& Scene::AddComponentFromSceneObject(SceneObject& owningObject, size_t indexInSceneObject)
	{
		void*& ptr = components[typeid(ComponentType)];
		if (!ptr)
			ptr = new std::vector<ComponentType*>();

		std::vector<ComponentType*>& vector = *static_cast<std::vector<ComponentType*>*>(ptr);
		auto* newObj = new ComponentType(owningObject, indexInSceneObject, vector.size());
		vector.emplace_back(newObj);
		return *newObj;
	}

	template<typename ComponentType>
	ComponentType& Scene::AddSingletonComponentFromSceneObject(SceneObject& owningObject)
	{
		//std::vector<SingletonComponentBase*>& vector = singletonComponents[typeid(ComponentType)];
		//ComponentType* newObj = new ComponentType(owningObject, vector.size());
		ComponentType* newObj = new ComponentType(owningObject, 0);
		//vector.push_back(newObj);
		return *newObj;
	}
}