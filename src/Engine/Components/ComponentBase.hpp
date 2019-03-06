#pragma once

namespace Engine
{
	class SceneObject;
}

#include <functional>

namespace Engine
{
	class ComponentBase
	{
	public:
		static constexpr bool isSingleton = false;

		explicit ComponentBase(SceneObject& owningObject);

		virtual ~ComponentBase();

		[[nodiscard]] SceneObject& GetSceneObject();
		[[nodiscard]] const SceneObject& GetSceneObject() const;

	private:
		std::reference_wrapper<SceneObject> sceneObjectRef;
	};
}