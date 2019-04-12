#pragma once

#include <functional>

namespace Engine
{
	class SceneObject;

	namespace Components
	{
		template<typename T>
		constexpr bool IsSingleton() { return false; }

		class ComponentBase
		{
		public:
			explicit ComponentBase(SceneObject& owningObject);

			[[nodiscard]] SceneObject& GetSceneObject();
			[[nodiscard]] const SceneObject& GetSceneObject() const;

			[[nodiscard]] size_t GetID() const;

		private:
			std::reference_wrapper<SceneObject> sceneObjectRef;
		};
	}
}