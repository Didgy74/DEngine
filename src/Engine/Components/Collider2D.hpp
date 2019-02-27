#pragma once

#include "ComponentBase.hpp"

namespace Engine
{
	class Collider2D : public ComponentBase
	{
	public:
		using ParentType = ComponentBase;

		Collider2D(SceneObject& owningObject, size_t indexInSceneObject, size_t indexInScene);
		~Collider2D();

	private:
		bool isStatic;
	};
}