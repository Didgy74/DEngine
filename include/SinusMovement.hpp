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
		float movementMultiplier = 7.5f;
		float timeMultiplier = 25.f;

	protected:
		void SceneStart() override;

		void Tick() override;
	};
}