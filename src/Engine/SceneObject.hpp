#pragma once

namespace Engine
{
	class Scene;
	class SceneObject;
	class ComponentBase;
	class SingletonComponentBase;
}

#include "Scene.hpp"
#include "Transform.hpp"

#include <vector>
#include <functional>
#include <unordered_map>
#include <typeindex>

namespace Engine
{
	void Destroy(SceneObject& sceneObject);

	class SceneObject
	{
	public:
		SceneObject(Scene& owningScene, size_t indexInScene);
		SceneObject(const SceneObject& right) = delete;
		SceneObject(SceneObject&& right) noexcept = delete;
		~SceneObject();

		SceneObject& operator=(const SceneObject& right) = delete;
		SceneObject& operator=(SceneObject&& right) = delete;

		[[nodiscard]] Scene& GetScene();

		[[nodiscard]] SceneObject* GetParent() const;

		template<typename ComponentType>
		ComponentType& AddComponent();

		template<typename ComponentType>
		ComponentType* GetComponent() const;

		Transform transform;
		
	private:
		std::reference_wrapper<Scene> sceneRef;
		SceneObject* parent;
		std::vector<SceneObject*> children;
		std::unordered_map<std::type_index, std::vector<std::reference_wrapper<ComponentBase>>> components;
		std::unordered_map<std::type_index, std::reference_wrapper<SingletonComponentBase>> singletonComponents;
	};

	template<typename ComponentType>
	ComponentType& SceneObject::AddComponent()
	{
		if constexpr (ComponentType::isSingleton == true)
		{
			auto iterator = singletonComponents.find(typeid(ComponentType));
			if (iterator != singletonComponents.end())
			{
				// We found an existing component. Return it.
				return static_cast<ComponentType&>(iterator->second.get());
			}
			else
			{
				// We found no existing component, make one.
				ComponentType& newComponent = GetScene().AddSingletonComponentFromSceneObject<ComponentType>(*this);
				singletonComponents.insert({typeid(ComponentType), newComponent});
				return newComponent;
			}
		}
		else
		{
			std::vector<std::reference_wrapper<ComponentBase>>& compVector = components[typeid(ComponentType)];
			ComponentType& newComponent = GetScene().AddComponentFromSceneObject<ComponentType>(*this, compVector.size());
			compVector.emplace_back(newComponent);
			return newComponent;
		}

	}

	template<typename ComponentType>
	ComponentType* SceneObject::GetComponent() const
	{
		if constexpr (ComponentType::isSingleton == true)
		{
			auto iterator = singletonComponents.find(typeid(ComponentType));
			if (iterator == singletonComponents.end())
				return nullptr;
			else
			{
				auto& ref = iterator->second.get();
				return static_cast<ComponentType*>(&ref);
			}
		}
		else
		{
			auto iterator = components.find(typeid(ComponentType));
			if (iterator == components.end())
				return nullptr;
			else
			{
				auto& ref = iterator->second.front().get();
				return static_cast<ComponentType*>(&ref);
			}
		}


	}
}