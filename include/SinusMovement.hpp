#pragma once

#include "DEngine/Components/ScriptBase.hpp"

#include "DMath/Vector/Vector.hpp"

namespace Assignment02
{
	class SinusMovement : public Engine::Components::ScriptBase
	{
	public:
		using ParentType = ScriptBase;

		explicit SinusMovement(Engine::SceneObject& owningObject);

		Math::Vector3D startPos{};

	protected:
		void SceneStart() override;

		void Tick() override;
	};
}