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
#include "ComponentReference.hpp"

#include <vector>
#include <functional>
#include <unordered_map>
#include <typeindex>
#include <type_traits>

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
		std::pair<std::reference_wrapper<ComponentType>, CompRef<ComponentType>> AddComponent();

		Transform transform;
		
	private:
		std::reference_wrapper<Scene> sceneRef;
		SceneObject* parent;
		std::vector<SceneObject*> children;
		std::unordered_map<std::type_index, std::vector<size_t>> components;
		std::unordered_map<std::type_index, size_t> singletonComponents;
	};

	template<typename ComponentType>
	std::pair<std::reference_wrapper<ComponentType>, CompRef<ComponentType>> SceneObject::AddComponent()
	{
		static_assert(std::is_base_of<ComponentBase, ComponentType>() || std::is_base_of<SingletonComponentBase, ComponentType>(),
		              "Component added to SceneObject must inherit from ComponentBase or SingletonComponentBase");
		if constexpr (ComponentType::isSingleton == false)
		{
			auto iterator = components.find(typeid(ComponentType));
			if (iterator == components.end())
			{
				// No vector for this component type found, make one
				auto iteratorOpt = components.insert({typeid(ComponentType), {}});
				assert(iteratorOpt.second);
				iterator = iteratorOpt.first;
			}

			auto& vector = iterator->second;

			auto guidRefPair = GetScene().AddComponent<ComponentType>(*this);

			vector.emplace_back(guidRefPair.first);

			return { guidRefPair.second, CompRef<ComponentType>(GetScene(), guidRefPair.first) };
		}
	}
}