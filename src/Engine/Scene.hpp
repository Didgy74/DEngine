#pragma once

namespace Engine
{
	class Core;
	class Scene;
	class SceneObject;
	class ComponentBase;
	class SingletonComponentBase;
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
		std::unordered_map<std::type_index, std::vector<ComponentBase*>> components;
		std::unordered_map<std::type_index, std::vector<SingletonComponentBase*>> singletonComponents;
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
		if constexpr (ComponentType::isSingleton == true)
		{
            auto iterator = singletonComponents.find(typeid(ComponentType));
            if (iterator == singletonComponents.end())
                return nullptr;
            else
                return reinterpret_cast<const std::vector<ComponentType*>*>(&iterator->second);
		}
		else
		{
			auto iterator = components.find(typeid(ComponentType));
			if (iterator == components.end())
				return nullptr;
			else
				return reinterpret_cast<const std::vector<ComponentType*>*>(&iterator->second);
		}
	}

	template<typename ComponentType>
	ComponentType& Scene::AddComponentFromSceneObject(SceneObject& owningObject, size_t indexInSceneObject)
	{
		std::vector<ComponentBase*>& vector = components[typeid(ComponentType)];
		ComponentType* newObj = new ComponentType(owningObject, indexInSceneObject, vector.size());
		vector.push_back(newObj);
		return *newObj;
	}

	template<typename ComponentType>
	ComponentType& Scene::AddSingletonComponentFromSceneObject(SceneObject& owningObject)
	{
		std::vector<SingletonComponentBase*>& vector = singletonComponents[typeid(ComponentType)];
		ComponentType* newObj = new ComponentType(owningObject, vector.size());
		vector.push_back(newObj);
		return *newObj;
	}
}