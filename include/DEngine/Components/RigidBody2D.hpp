#pragma once

#include "DEngine/Components/Components.hpp"

#include "DMath/Vector/Vector.hpp"

namespace Engine
{
	namespace Components
	{
		template<>
		constexpr bool IsSingleton<class RigidBody2D>() { return true; }

		class RigidBody2D : public ComponentBase
		{
		public:
			using ParentType = ComponentBase;

			explicit RigidBody2D(SceneObject& owningObject);
			~RigidBody2D();

			Math::Vector2D position{};
			Math::Vector2D velocity{};
			float torque{};

			float GetMass() const;
			void SetMass(float newMass);
			float GetInverseMass() const;
			void SetInverseMass(float newInverseMass);

		private:
			float inverseMass{ 1.f };
		};
	}

	
}