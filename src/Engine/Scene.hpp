#pragma once

namespace Engine
{
	class Core;
	class Scene;
	class SceneObject;
}

#include "Physics2D.hpp"
#include "Time/Time.hpp"
#include "ComponentReference.hpp"

#include <vector>
#include <any>
#include <functional>
#include <memory>
#include <typeindex>
#include <unordered_map>

namespace Engine
{
	class Scene
	{
	public:
		explicit Scene(size_t indexInEngine);

		Scene(const Scene &right) = delete;

		Scene(Scene &&right) noexcept = delete;

		~Scene();

		Scene& operator=(const Scene &right) = delete;

		Scene& operator=(Scene &&right) = delete;

		SceneObject &NewSceneObject();
		const SceneObject& GetSceneObject(size_t index) const;
		SceneObject& GetSceneObject(size_t index);
		size_t GetSceneObjectCount() const;

		template<typename ComponentType>
		size_t GetComponentCount() const;
		template<typename ComponentType>
		ComponentType* GetComponent(const CompRef<ComponentType>& ref);
		template<typename ComponentType>
		const std::vector<ComponentType>* GetAllComponents() const;


		[[nodiscard]] size_t GetIndexInEngine() const;

		void Clear();

	private:
		template<typename T>
		struct Table
		{
			std::vector<size_t> id;
			std::vector<T> data;
		};

		template<typename ComponentType>
		[[nodiscard]] std::pair<size_t, std::reference_wrapper<ComponentType>> AddComponent(SceneObject &owningObject);
		std::unordered_map<std::type_index, std::any> components;
		size_t componentGUIDCounter;

		Time::SceneData &GetTimeData();

		void RemoveSceneObject(SceneObject &owningObject);

		size_t indexInEngine;
		std::vector<SceneObject*> sceneObjects;
		Time::SceneData timeData;
		Physics2D::SceneData physics2DData;

		friend class Engine::Core;
		friend class Engine::SceneObject;
	};
}

template<typename ComponentType>
std::pair<size_t, std::reference_wrapper<ComponentType>> Engine::Scene::AddComponent(SceneObject& owningObject)
{
	using ContainerType = Table<ComponentType>;

	auto iterator = components.find(typeid(ComponentType));
	if (iterator == components.end())
	{
		// No component-vector for this type, make one.
		auto iteratorOpt = components.insert({typeid(ComponentType), std::make_any<ContainerType>()});
		assert(iteratorOpt.second);
		iterator = iteratorOpt.first;
	}

	auto& table = std::any_cast<ContainerType&>(iterator->second);
	table.data.emplace_back(owningObject);
	table.id.emplace_back(componentGUIDCounter);
	componentGUIDCounter++;
	return { table.id.back(), table.data.back() };
}

template<typename ComponentType>
size_t Engine::Scene::GetComponentCount() const
{
	auto iterator = components.find(typeid(ComponentType));
	if (iterator == components.end())
		return 0;

	using ContainerType = Table<ComponentType>;
	const auto& container = std::any_cast<ContainerType&>(iterator->second);
	return container.data.size();
}

template<typename ComponentType>
ComponentType* Engine::Scene::GetComponent(const CompRef<ComponentType> &ref)
{
	auto iterator = components.find(typeid(ComponentType));
	if (iterator == components.end())
		return nullptr;

	using ContainerType = Table<ComponentType>;
	auto& table = std::any_cast<ContainerType&>(iterator->second);

	auto idIterator = std::find(table.id.begin(), table.id.end(), ref.GetGUID());
	if (idIterator == table.id.end())
		return nullptr;

	auto index = std::distance(table.id.begin(), idIterator);

	return &table.data[index];
}

template<typename ComponentType>
const std::vector<ComponentType>* Engine::Scene::GetAllComponents() const
{
	auto iterator = components.find(typeid(ComponentType));
	if (iterator == components.end())
		return nullptr;

	using ContainerType = Table<ComponentType>;
	const auto& table = std::any_cast<const ContainerType&>(iterator->second);
	return &table.data;
}
