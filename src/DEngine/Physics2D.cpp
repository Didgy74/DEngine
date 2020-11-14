#include <DEngine/Physics.hpp>

#include <DEngine/Scene.hpp>

#include <DEngine/Std/Utility.hpp>

namespace DEngine::Physics
{
	void Resolve_Circle_Circle(
		Std::Span<decltype(Scene::rigidbodies)::value_type> rigidbodies,
		Std::Span<decltype(Scene::transforms)::value_type const> transforms,
		Std::Span<decltype(Scene::circleColliders)::value_type const> circleColliders,
		Std::Span<decltype(Scene::boxColliders)::value_type const> boxColliders);
	void Resolve_Circle_OBB(
		Std::Span<Transform> transforms,
		Std::Span<CircleCollider2D const> circleColliders,
		Std::Span<BoxCollider2D const> boxColliders);

}

void DEngine::Physics::Update(Scene& scene, f32 deltaTime)
{
	Resolve_Circle_Circle(
		{ scene.rigidbodies.data(), scene.rigidbodies.size() },
		{ scene.transforms.data(), scene.transforms.size() },
		{ scene.circleColliders.data(), scene.circleColliders.size() },
		{ scene.boxColliders.data(), scene.circleColliders.size() });

	for (uSize i = 0; i < scene.rigidbodies.size(); i += 1)
	{
		Entity const& entity = scene.rigidbodies[i].a;
		Rigidbody2D& rb = scene.rigidbodies[i].b;

		auto leftPosIt = Std::FindIf(
			scene.transforms.begin(),
			scene.transforms.end(),
			[entity](decltype(Scene::transforms)::value_type const& val) -> bool { return val.a == entity; });
		if (leftPosIt == scene.transforms.end())
			continue;
		auto& transform = leftPosIt->b;

		rb.velocity *= Math::Pow(rb.linearDamp, deltaTime);
		rb.velocity += rb.acceleration * deltaTime;
		rb.acceleration.x = 0.f;
		rb.acceleration.y = 0.f;

		transform.position += rb.velocity.AsVec3() * deltaTime;
	}
}

void DEngine::Physics::Resolve_Circle_Circle(
	Std::Span<decltype(Scene::rigidbodies)::value_type> rigidbodies,
	Std::Span<decltype(Scene::transforms)::value_type const> transforms,
	Std::Span<decltype(Scene::circleColliders)::value_type const> circleColliders,
	Std::Span<decltype(Scene::boxColliders)::value_type const> boxColliders)
{
	for (uSize leftIndex = 0; leftIndex < circleColliders.Size(); leftIndex += 1)
	{
		Entity const& leftEntity = circleColliders[leftIndex].a;
		CircleCollider2D const& leftCollider = circleColliders[leftIndex].b;

		// First check if this entity has a transform
		auto const leftTransIt = Std::FindIf(
			transforms.AsRange(),
			[leftEntity](decltype(transforms)::ValueType const& val) -> bool {
				return val.a == leftEntity; });
		if (leftTransIt == transforms.end())
			continue;
		auto& leftTransform = leftTransIt->b;

		// Check if this entity has a rigidbody
		auto const leftRbIt = Std::FindIf(
			rigidbodies.AsRange(),
			[leftEntity](decltype(rigidbodies)::ValueType const& val) -> bool {
				return val.a == leftEntity; });
		Rigidbody2D* leftRb = leftRbIt != rigidbodies.end() ? &leftRbIt->b : nullptr;

		for (uSize rightIndex = leftIndex + 1; rightIndex < circleColliders.Size(); rightIndex += 1)
		{
			Entity const& rightEntity = circleColliders[rightIndex].a;
			CircleCollider2D const& rightCollider = circleColliders[rightIndex].b;

			// First check if this entity has a position
			auto const rightTransIt = Std::FindIf(
				transforms.AsRange(),
				[rightEntity](decltype(transforms)::ValueType const& val) -> bool {
					return val.a == rightEntity; });
			if (rightTransIt == transforms.end())
				continue;
			auto& rightTransform = rightTransIt->b;

			// Check if this entity has a rigidbody
			auto const rightRbIt = Std::FindIf(
				rigidbodies.AsRange(),
				[rightEntity](decltype(rigidbodies)::ValueType const& val) -> bool {
					return val.a == rightEntity; });
			Rigidbody2D* rightRb = rightRbIt ? &rightRbIt->b : nullptr;

			if (leftRb == nullptr && rightRb == nullptr)
				continue;

			Math::Vec2 relativePosVec = rightTransform.position.AsVec2() - leftTransform.position.AsVec2();

			f32 distSqrd = relativePosVec.MagnitudeSqrd();
			f32 sumRadiusSqrd = Math::Sqrd(rightCollider.radius + leftCollider.radius);

			if (distSqrd <= sumRadiusSqrd)
			{
				f32 magnitude = Math::Sqrt(distSqrd);
				Math::Vec2 contactNormal = { relativePosVec.x / magnitude, relativePosVec.y / magnitude };

				f32 separatingVelocity = Math::Vec2::Dot(rightRb->velocity - leftRb->velocity, contactNormal);

				if (separatingVelocity < 0)
				{
					// We are closing in, so we need to resolve it.
					f32 newSepVelocity = -separatingVelocity * Math::Lerp(leftRb->restitution, rightRb->restitution, 0.5f);

					f32 deltaVelocity = newSepVelocity * separatingVelocity;

					f32 totalInverseMass = leftRb->inverseMass + rightRb->inverseMass;

					f32 impulse = deltaVelocity / totalInverseMass;

					Math::Vec2 impulsePerIMass = contactNormal * impulse;

					leftRb->velocity += impulsePerIMass * leftRb->inverseMass;
					rightRb->velocity += -impulsePerIMass * rightRb->inverseMass;

				}
			}
		}
	}

}