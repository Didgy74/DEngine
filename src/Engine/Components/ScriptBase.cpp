#include "ScriptBase.hpp"

namespace Engine
{
	namespace Components
	{
		ScriptBase::ScriptBase(SceneObject& owningObject) :
			sceneObjectRef(owningObject)
		{

		}

		ScriptBase::~ScriptBase()
		{

		}

		void ScriptBase::Start() {}

		void ScriptBase::Tick() {}

		SceneObject& ScriptBase::GetSceneObject() { return sceneObjectRef.get(); }

		const SceneObject& ScriptBase::GetSceneObject() const { return sceneObjectRef.get(); }
	}
}