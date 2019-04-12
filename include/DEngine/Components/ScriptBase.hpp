#pragma once

#include <functional>

namespace Engine
{
	class SceneObject;
	class Scene;

	namespace Components
	{
		class ScriptBase
		{
		public:
			explicit ScriptBase(SceneObject& owningObject);

			ScriptBase(const ScriptBase&) = delete;
			ScriptBase(ScriptBase&&) = delete;
			ScriptBase& operator=(const ScriptBase&) = delete;
			ScriptBase& operator=(ScriptBase&&) = delete;

			virtual ~ScriptBase() = 0;

			SceneObject& GetSceneObject();
			const SceneObject& GetSceneObject() const;

		protected:
			virtual void SceneStart() = 0;
			
			virtual void Tick() = 0;

		private:
			std::reference_wrapper<SceneObject> sceneObjectRef;

			friend Scene;
		};
	}
}