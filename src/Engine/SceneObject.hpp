#pragma once

namespace Engine
{
	class Scene;
	class SceneObject;
}

#include "Scene.hpp"
#include "Transform.hpp"
#include "ComponentReference.hpp"
#include "Components/Components.hpp"

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

		template<typename T>
		std::pair<std::reference_wrapper<T>, CompRef<T>> AddComponent();

		Transform transform;
		
	private:
		std::reference_wrapper<Scene> sceneRef;
		SceneObject* parent;
		std::vector<SceneObject*> children;
		std::unordered_map<std::type_index, std::vector<size_t>> components;
		std::unordered_map<std::type_index, size_t> singletonComponents;
	};

	template<typename T>
	std::pair<std::reference_wrapper<T>, CompRef<T>> SceneObject::AddComponent()
	{
		if constexpr (Components::IsSingleton<T>() == false)
		{
			auto iterator = components.find(typeid(T));
			if (iterator == components.end())
			{
				// No vector for this component type found, make one
				auto iteratorOpt = components.insert({typeid(T), {}});
				assert(iteratorOpt.second);
				iterator = iteratorOpt.first;
			}

			auto& vector = iterator->second;

			if constexpr (std::is_base_of<Components::ComponentBase, T>())
			{
				auto guidRefPair = GetScene().AddComponent<T>(*this);
				vector.emplace_back(guidRefPair.first);

				return { guidRefPair.second, CompRef<T>(GetScene(), guidRefPair.first) };
			}
		}
	}
}