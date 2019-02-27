#pragma once

#include "SingletonComponentBase.hpp"

#include "Math/Vector/Vector.hpp"

namespace Engine
{
	class RigidBody2D : public SingletonComponentBase
	{
	public:
		using ParentType = SingletonComponentBase;

		RigidBody2D(SceneObject& owningObject, size_t indexInScene);
		~RigidBody2D() override;

		Math::Vector2D position;
		Math::Vector2D velocity;
		float torque;

		float GetMass() const;
		void SetMass(float newMass);
		float GetInverseMass() const;
		void SetInverseMass(float newInverseMass);

	private:
		float inverseMass;
	};
}