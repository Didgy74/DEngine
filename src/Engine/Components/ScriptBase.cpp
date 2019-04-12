#include "DEngine/Components/ScriptBase.hpp"

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

		void ScriptBase::SceneStart() {}

		void ScriptBase::Tick() {}

		SceneObject& ScriptBase::GetSceneObject() { return sceneObjectRef.get(); }

		const SceneObject& ScriptBase::GetSceneObject() const { return sceneObjectRef.get(); }
	}
}