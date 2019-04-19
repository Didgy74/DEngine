#pragma once

#include "DEngine/Components/ScriptBase.hpp"

#include "DMath/Vector/Vector.hpp"

namespace Assignment02
{
	class InputRotate : public Engine::Components::ScriptBase
	{
	public:
		using ParentType = ScriptBase;

		explicit InputRotate(Engine::SceneObject& owningObject);

	protected:
		void SceneStart() override;

		void Tick() override;
	};
}