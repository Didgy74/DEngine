#include "DEngine/Components/Components.hpp"

namespace Engine
{
	namespace Components
	{
		ComponentBase::ComponentBase(SceneObject& owningObject) :
			sceneObjectRef(owningObject)
		{

		}

		SceneObject& ComponentBase::GetSceneObject() { return sceneObjectRef.get(); }

		const SceneObject& ComponentBase::GetSceneObject() const { return sceneObjectRef.get(); }
	}
}