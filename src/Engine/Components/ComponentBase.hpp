#pragma once

namespace Engine
{
	class Scene;
	class SceneObject;
}

#include <functional>

namespace Engine
{
	class ComponentBase
	{
	public:
		static constexpr bool isSingleton = false;

		ComponentBase(SceneObject& owningObject, size_t indexInSceneObject, size_t indexInScene) noexcept;
		ComponentBase(const ComponentBase& right) = delete;
		ComponentBase(ComponentBase&& right) noexcept = delete;

		virtual ~ComponentBase();

		ComponentBase& operator=(const ComponentBase& right) = delete;
		ComponentBase& operator=(ComponentBase&& right) = delete;

		[[nodiscard]] SceneObject& GetSceneObject();
		[[nodiscard]] const SceneObject& GetSceneObject() const;
		[[nodiscard]] size_t GetIndexInScene() const;

	private:
		std::reference_wrapper<SceneObject> sceneObjectRef;
		size_t indexInScene;
		size_t indexInSceneObject;

		friend Scene;
	};
}