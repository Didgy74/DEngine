#pragma once

namespace Engine
{
	class Core;
	class Scene;
	class SceneObject;
	namespace Components
	{
		class ScriptBase;
	}
}

#include "Physics2D.hpp"
#include "Time/Time.hpp"
#include "ComponentReference.hpp"
#include "Components/ScriptBase.hpp"

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

		template<typename T>
		size_t GetComponentCount() const;
		template<typename T>
		T* GetComponent(const CompRef<T>& ref);
		template<typename T>
		const std::vector<T>* GetAllComponents() const;


		[[nodiscard]] size_t GetIndexInEngine() const;

		const Time::SceneData& GetTimeData() const;

		void Clear();

	private:
		template<typename T>
		struct Table
		{
			std::vector<size_t> id;
			std::vector<T> data;
		};

		template<typename T>
		[[nodiscard]] auto AddComponent(SceneObject &owningObject);
		std::unordered_map<std::type_index, std::any> components;
		size_t componentGUIDCounter;


		std::vector<Components::ScriptBase*> scriptComponents;
		void Scripts_SceneStart();
		void ScriptTick();

		Time::SceneData& GetTimeData();

		void RemoveSceneObject(SceneObject &owningObject);

		size_t indexInEngine;
		std::vector<std::unique_ptr<SceneObject>> sceneObjects;
		Time::SceneData timeData;
		Physics2D::SceneData physics2DData;

		friend class Engine::Core;
		friend class Engine::SceneObject;
	};
}

template<typename T>
auto Engine::Scene::AddComponent(SceneObject& owningObject)
{
	if constexpr (std::is_base_of<Components::ScriptBase, T>())
	{
		using ReturnType = std::reference_wrapper<T>;
		
		scriptComponents.emplace_back(new T(owningObject));
		T& ptr = static_cast<T&>(*scriptComponents.back());
		return ReturnType(ptr);
	}
	else
	{
		using ReturnType = std::pair<size_t, T&>;
		using ContainerType = Table<T>;

		auto iterator = components.find(typeid(T));
		if (iterator == components.end())
		{
			// No component-vector for this type, make one.
			auto iteratorOpt = components.insert({ typeid(T), std::make_any<ContainerType>() });
			assert(iteratorOpt.second);
			iterator = iteratorOpt.first;
		}

		ContainerType& table = std::any_cast<ContainerType&>(iterator->second);
		table.data.emplace_back(owningObject);
		table.id.emplace_back(componentGUIDCounter);
		componentGUIDCounter++;

		return ReturnType{ table.id.back(), table.data.back() };
	}
}

template<typename T>
size_t Engine::Scene::GetComponentCount() const
{
	auto iterator = components.find(typeid(T));
	if (iterator == components.end())
		return 0;

	using ContainerType = Table<T>;
	const auto& container = std::any_cast<ContainerType&>(iterator->second);
	return container.data.size();
}

template<typename T>
T* Engine::Scene::GetComponent(const CompRef<T> &ref)
{
	auto iterator = components.find(typeid(T));
	if (iterator == components.end())
		return nullptr;

	using ContainerType = Table<T>;
	auto& table = std::any_cast<ContainerType&>(iterator->second);

	auto idIterator = std::find(table.id.begin(), table.id.end(), ref.GetGUID());
	if (idIterator == table.id.end())
		return nullptr;

	auto index = std::distance(table.id.begin(), idIterator);

	return &table.data[index];
}

template<typename T>
const std::vector<T>* Engine::Scene::GetAllComponents() const
{
	auto iterator = components.find(typeid(T));
	if (iterator == components.end())
		return nullptr;

	using ContainerType = Table<T>;
	const auto& table = std::any_cast<const ContainerType&>(iterator->second);
	return &table.data;
}
