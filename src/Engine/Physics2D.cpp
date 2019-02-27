#include "Physics2D.hpp"

#include "Scene.hpp"
#include "SceneObject.hpp"
#include "Components/CircleCollider2D.hpp"
#include "Components/BoxCollider2D.hpp"
#include "Components/RigidBody2D.hpp"
#include "Time.hpp"

#include "Math/LinearTransform2D.hpp"

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
			RigidBody2D* left;
			RigidBody2D* right;
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
	// Build circle array
	const auto& circlesPtr = scene.GetComponents<CircleCollider2D>();
	if (!circlesPtr)
		data->circles.clear();
	else
	{
		const auto& circles = *circlesPtr;
		data->circles.resize(circles.size());

		for (size_t i = 0; i < data->circles.size(); i++)
		{
			const auto& circle = *circles[i];
			auto model = circle.GetModel2D_Reduced(Space::World);
			data->circles[i].position = Math::LinTran2D::GetTranslation(model);
			data->circles[i].radius = Math::Vector2D{ model[0][0], model[0][1] }.Magnitude() * 0.5f;
		}
	}

	// Box array
	return;
	const auto& boxesPtr = scene.GetComponents<BoxCollider2D>();
	if (!boxesPtr)
		data->boxes.clear();
	else
	{
		const auto& boxes = *boxesPtr;
		data->boxes.resize(boxes.size());

		for (size_t i = 0; i < data->boxes.size(); i++)
		{
			//const auto& box = *boxes[i];

			//auto model = box.GetModel2D_Reduced(Space::World);

			//data->boxes[i].position = Math::LinTran2D::GetTranslation(model);
			data->boxes[i].position = {};
		}
	}
}

void Engine::Physics2D::TestCircles(const Scene& scene)
{
	for (size_t i = 0; i < data->circles.size(); i++)
	{
		const auto& left = data->circles[i];
		for (size_t j = i + 1; j < data->circles.size(); j++)
		{
			const auto& right = data->circles[j];
			float distance = (left.position - right.position).Magnitude();
			if (distance <= (left.radius + right.radius))
			{
				auto& sceneCircles = *scene.GetComponents<CircleCollider2D>();
				RigidBody2D* rbLeft = sceneCircles[i]->GetSceneObject().GetComponent<RigidBody2D>();
				RigidBody2D* rbRight = sceneCircles[j]->GetSceneObject().GetComponent<RigidBody2D>();

				Math::Vector2D normal = (left.position - right.position).GetNormalized();

				float penetrationDepth = (left.radius + right.radius) - distance;

				data->collisions.push_back({normal, rbLeft, rbRight, penetrationDepth});
			}
		}
	}
}

void Engine::Physics2D::TestBoxCollider2D(const Scene& scene)
{
	// Check collisions
	for (size_t i = 0; i < data->boxes.size(); i++)
	{
		for (size_t j = i + 1; j < data->boxes.size(); j++)
		{
			for (size_t s = 0; s < 2; s++)
			{
				const auto& left = s == 0 ? data->boxes[i] : data->boxes[j];
				const auto& right = s == 0 ? data->boxes[j] : data->boxes[i];

				// Test left against right
				for (size_t k = 0; k < 4; k++)
				{
					bool collision = true;
					/*Math::Vector2D normal = right.upVector;
					const Math::Vector2D& vertex = left.first[k];
					for (size_t m = 0; m < 4; m++)
					{
						const Plane& plane = { normal, right.first[m] };
						float dist = PointPlaneDistance(vertex, plane);
						if (dist > 0.f)
						{
							collision = false;
							break;
						}
						normal.Rotate(true);
					}*/

					collision = false;
					if (collision)
					{
						//std::cout << "Collision" << count++ << std::endl;
					}
				}
			}
		}
	}
}

void Engine::Physics2D::TestBoxCircle(const Scene& scene)
{
	if (data->circles.size() <= 0 || data->boxes.size() <= 0)
		return;

	const auto& circles = data->circles;
	const auto& boxes = data->boxes;

	for (size_t i = 0; i < circles.size(); i++)
	{
		const auto& circle = circles[i];
		
		for (size_t j = 0; j < boxes.size(); j++)
		{
			const auto& box = boxes[j];

			Math::Vector2D d = circle.position - box.position;

			Math::Vector2D q = box.position;

			for (size_t dim = 0; dim < 2; dim++)
			{
				float dist = Math::Vector2D::Dot(d, box.orientationVectors[dim]);

				//dist = Math::Clamp(dist, -box.extents[dim], box.extents[dim]);

				q += dist * box.orientationVectors[dim];
			}

			Math::Vector2D vectorBetween = q - circle.position;

			float distance = vectorBetween.Magnitude();
			if (distance <= circle.radius)
			{
				auto& sceneBoxes = *scene.GetComponents<BoxCollider2D>();
				RigidBody2D* rbLeft = sceneBoxes[j]->GetSceneObject().GetComponent<RigidBody2D>();

				auto& sceneCircles = *scene.GetComponents<CircleCollider2D>();
				RigidBody2D* rbRight = sceneCircles[i]->GetSceneObject().GetComponent<RigidBody2D>();

				float penetrationDepth = circle.radius - distance;

				data->collisions.push_back({vectorBetween.GetNormalized(), rbLeft, rbRight, penetrationDepth});
			}
		}
	}
}

void Engine::Physics2D::UpdateRigidbodies(const Scene& scene)
{
	const auto& rb2dPtr = scene.GetComponents<Engine::RigidBody2D>();
	if (!rb2dPtr)
		return;
	const auto& rigidbodies = *rb2dPtr;

	for (const auto& rbPtr : rigidbodies)
	{
		auto& rb = *rbPtr;
		//rb.position += rb.velocity * Time::FixedDeltaTime();
	}
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
	auto rigidbodiesPtr = scene.GetComponents<RigidBody2D>();
	if (rigidbodiesPtr == nullptr)
		return;

	auto& rigidbodies = *rigidbodiesPtr;

	for (auto& rbPtr : rigidbodies)
	{
		assert(rbPtr != nullptr);

		auto& rb = *rbPtr;

		rb.GetSceneObject().transform.localPosition = rb.position.AsVec3();
	}
}

float Engine::Physics2D::PointPlaneDistance(const Math::Vector2D& point, const Plane& plane) 
{
	return Math::Vector2D::Dot(plane.normal, point) - Math::Vector2D::Dot(plane.normal, plane.position); 
}
