#include "DEngine/Components/RigidBody2D.hpp"

#include "DEngine/SceneObject.hpp"

namespace Engine
{
	namespace Components
	{
		RigidBody2D::RigidBody2D(SceneObject& owningObject) :
			ParentType(owningObject)
		{
			position = owningObject.GetPosition(Space::World).AsVec2();
		}

		RigidBody2D::~RigidBody2D()
		{
		}

		float RigidBody2D::GetMass() const { return 1.f / inverseMass; }

		float RigidBody2D::GetInverseMass() const { return inverseMass; }

		void RigidBody2D::SetInverseMass(float newInverseMass)
		{
			inverseMass = newInverseMass;
		}

		void RigidBody2D::SetMass(float newMass)
		{
			inverseMass = 1.f / newMass;
		}
	}
}


