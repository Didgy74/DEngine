#include "RigidBody2D.hpp"

#include "../Scene.hpp"
#include "../SceneObject.hpp"

Engine::RigidBody2D::RigidBody2D(SceneObject& owningObject) :
	ParentType(owningObject),
	velocity(),
	inverseMass(1.f),
	torque()
{
	position = owningObject.transform.GetPosition(Space::World).AsVec2();
}

Engine::RigidBody2D::~RigidBody2D()
{
}

float Engine::RigidBody2D::GetMass() const { return 1.f / inverseMass; }

float Engine::RigidBody2D::GetInverseMass() const { return inverseMass; }

void Engine::RigidBody2D::SetInverseMass(float newInverseMass)
{
	inverseMass = newInverseMass;
}

void Engine::RigidBody2D::SetMass(float newMass)
{
	inverseMass = 1.f / newMass;
}
