#include "Physics2D.hpp"

#include "Scene.hpp"
#include "SceneObject.hpp"
#include "Components/CircleCollider2D.hpp"
#include "Components/BoxCollider2D.hpp"
#include "Components/RigidBody2D.hpp"
#include "Time/Time.hpp"

#include "DMath/LinearTransform2D.hpp"

#include <vector>
#include <memory>

#include <iostream>

namespace Engine
{
	namespace Physics2D
	{
		struct Plane
		{
			Math::Vector2D normal;
			Math::Vector2D position;
		};

		struct Circle2D
		{
			Math::Vector2D position;
			float radius;
		};

		struct Box2D
		{
			Math::Vector2D position;
			std::array<Math::Vector2D, 2> orientationVectors;
		};

		struct Collision
		{
			Math::Vector2D normal;
			Components::RigidBody2D* left;
			Components::RigidBody2D* right;
			float penetrationDepth;
		};

		struct Data
		{
			std::vector<Circle2D> circles;
			std::vector<Box2D> boxes;
			std::vector<Collision> collisions;
		};

		static std::unique_ptr<Data> data;

		void BuildArrays(const Scene& scene);
		void TestCircles(const Scene& scene);
		void TestBoxCollider2D(const Scene& scene);
		void TestBoxCircle(const Scene& scene);
		void UpdateRigidbodies(const Scene& scene);
		void ResolveCollisions();
		void ApplyRigidBodies(const Scene& scene, Data& data);

		float PointPlaneDistance(const Math::Vector2D& point, const Plane& plane);
	}
}

void Engine::Physics2D::Core::Initialize()
{
	data = std::make_unique<Data>();

	data->collisions.reserve(50);
	data->boxes.reserve(50);
	data->circles.reserve(50);
}

void Engine::Physics2D::Core::FixedTick(const Scene& scene)
{
	Data& data = *Physics2D::data;

	UpdateRigidbodies(scene);
	ApplyRigidBodies(scene, data);
	
	BuildArrays(scene);

	TestCircles(scene);
	TestBoxCollider2D(scene);
	TestBoxCircle(scene);

	ResolveCollisions();

	ApplyRigidBodies(scene, data);
}

void Engine::Physics2D::BuildArrays(const Scene& scene)
{
}

void Engine::Physics2D::TestCircles(const Scene& scene)
{

}

void Engine::Physics2D::TestBoxCollider2D(const Scene& scene)
{
}

void Engine::Physics2D::TestBoxCircle(const Scene& scene)
{
}

void Engine::Physics2D::UpdateRigidbodies(const Scene& scene)
{
}

void Engine::Physics2D::ResolveCollisions()
{
	// Resolve collisions

	for (auto& collision : data->collisions)
	{
		// Resolve velocity
		// Check if there is atleast one rigidbody involved. If it is, it will be the left.
		if (collision.left)
		{
			Math::Vector2D relativeVelocityVector = collision.left->velocity;
			if (collision.right)
				relativeVelocityVector -= collision.right->velocity;
			float separatingVelocity = Math::Vector2D::Dot(relativeVelocityVector, collision.normal);
			if (separatingVelocity < 0)
			{
				float totalInverseMass = collision.left->GetInverseMass();
				if (collision.right)
					totalInverseMass += collision.right->GetInverseMass();

				if (totalInverseMass > 0.f)
				{
					float restitution = 1.0f;
					float newSepVelocity = -separatingVelocity * restitution;
					float deltaVelocity = newSepVelocity - separatingVelocity;
					float impulse = deltaVelocity / totalInverseMass;

					Math::Vector2D impulsePerIMass = collision.normal * impulse;

					collision.left->velocity += impulsePerIMass * collision.left->GetInverseMass();
					if (collision.right)
						collision.right->velocity += impulsePerIMass * -collision.right->GetInverseMass();
				}
			}
		}
		


		// Resolve interpenetration
		if (collision.penetrationDepth > 0.f)
		{
			float totalInverseMass = 0.f;
			if (collision.left)
				totalInverseMass += collision.left->GetInverseMass();
			if (collision.right)
				totalInverseMass += collision.right->GetInverseMass();

			Math::Vector2D movePerInverseMass = collision.normal * (-collision.penetrationDepth / totalInverseMass);


		}
		


	}

	data->collisions.clear();
}

void Engine::Physics2D::ApplyRigidBodies(const Scene& scene, Data& data)
{

}

float Engine::Physics2D::PointPlaneDistance(const Math::Vector2D& point, const Plane& plane) 
{
	return Math::Vector2D::Dot(plane.normal, point) - Math::Vector2D::Dot(plane.normal, plane.position); 
}
