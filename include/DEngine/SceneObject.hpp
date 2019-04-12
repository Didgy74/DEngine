#pragma once

namespace Engine
{
	class Scene;
	class SceneObject;
	namespace Components
	{
		class ScriptBase;
	}
}

#include "Scene.hpp"
#include "ComponentReference.hpp"
#include "Components/Components.hpp"
#include "Enum.hpp"

#include <vector>
#include <functional>
#include <unordered_map>
#include <typeindex>
#include <type_traits>

#include "DMath/Vector/Vector.hpp"
#include "DMath/Matrix/Matrix.hpp"
#include "DMath/UnitQuaternion.hpp"

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
		decltype(auto) AddComponent();

		template<typename T>
		auto GetComponents();
		template<typename T>
		auto GetComponents() const;
		
		Math::Vector3D localPosition{};
		Math::UnitQuaternion<> localRotation{};
		Math::Vector3D localScale{ 1, 1, 1 };

		Math::Vector3D GetPosition(Space space) const;

		Math::Matrix<4, 3> GetModel_Reduced(Space space) const;
		Math::Matrix<3, 2> GetModel2D_Reduced(Space space) const;

		Math::Matrix2x2 GetRotationModel2D(Space space) const;
		Math::Matrix<3, 3, float> GetRotationModel(Space space) const;

		Math::Vector<3, float> GetForwardVector(Space space) const;
		Math::Vector<3, float> GetUpVector(Space space) const;
		Math::Vector<3, float> GetRightVector(Space space) const;

	private:
		std::reference_wrapper<Scene> sceneRef;
		SceneObject* parent = nullptr;
		std::vector<SceneObject*> children;
		std::unordered_map<std::type_index, std::vector<size_t>> components;
		std::unordered_map<std::type_index, size_t> singletonComponents;
	};

	template<typename T>
	decltype(auto) SceneObject::AddComponent()
	{
		Scene& scene = GetScene();

		if constexpr (Components::IsSingleton<T>() == false)
		{
			if constexpr (std::is_base_of<Components::ScriptBase, T>())
			{
				std::reference_wrapper<T> test = scene.AddComponent<T>(*this);
				return test.get();
			}
			else
			{
				using ReturnType = std::pair<CompRef<T>, std::reference_wrapper<T>>;

				auto iterator = components.find(typeid(T));
				if (iterator == components.end())
				{
					// No vector for this component type found, make one
					auto iteratorOpt = components.insert({ typeid(T), {} });
					assert(iteratorOpt.second);
					iterator = iteratorOpt.first;
				}

				auto& vector = iterator->second;

				if constexpr (std::is_base_of<Components::ComponentBase, T>())
				{
					auto guidRefPair = scene.AddComponent<T>(*this);
					vector.emplace_back(guidRefPair.first);

					return ReturnType{ CompRef<T>(scene, guidRefPair.first), guidRefPair.second };
				}
			}

			
		}
	}
}