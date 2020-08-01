#include "DEngine/Physics.hpp"

#include "DEngine/Scene.hpp"


namespace DEngine::Physics
{
	void Resolve_Circle_Circle(
		Std::Span<decltype(Scene::rigidbodies)::ValueType> rigidbodies,
		Std::Span<decltype(Scene::transforms)::ValueType const> transforms,
		Std::Span<decltype(Scene::circleColliders)::ValueType const> circleColliders,
		Std::Span<decltype(Scene::boxColliders)::ValueType const> boxColliders);
	void Resolve_Circle_OBB(
		Std::Span<Transform> transforms,
		Std::Span<CircleCollider2D const> circleColliders,
		Std::Span<BoxCollider2D const> boxColliders);

}

void DEngine::Physics::Update(Scene& scene, f32 deltaTime)
{
	Resolve_Circle_Circle(
		scene.rigidbodies,
		scene.transforms,
		scene.circleColliders,
		scene.boxColliders);

	for (uSize i = 0; i < scene.rigidbodies.Size(); i += 1)
	{
		Entity const& entity = scene.rigidbodies[i].a;
		Rigidbody2D& rb = scene.rigidbodies[i].b;

		// First check if this entity has a position
		auto leftPosIndex = scene.transforms.FindIf(
			[entity](Std::Pair<Entity, Transform> const& val) -> bool { return val.a == entity; });
		if (!leftPosIndex.HasValue())
			continue;
		auto& transform = scene.transforms[leftPosIndex.Value()].b;

		rb.velocity *= Math::Pow(rb.linearDamp, deltaTime);
		rb.velocity += rb.acceleration * deltaTime;
		rb.acceleration.x = 0.f;
		rb.acceleration.y = 0.f;

		transform.position += rb.velocity.AsVec3() * deltaTime;
	}
}

void DEngine::Physics::Resolve_Circle_Circle(
	Std::Span<decltype(Scene::rigidbodies)::ValueType> rigidbodies,
	Std::Span<decltype(Scene::transforms)::ValueType const> transforms,
	Std::Span<decltype(Scene::circleColliders)::ValueType const> circleColliders,
	Std::Span<decltype(Scene::boxColliders)::ValueType const> boxColliders)
{
	for (uSize leftIndex = 0; leftIndex < circleColliders.Size(); leftIndex += 1)
	{
		Entity const& leftEntity = circleColliders[leftIndex].a;
		CircleCollider2D const& leftCollider = circleColliders[leftIndex].b;

		// First check if this entity has a position
		auto leftPosIndex = transforms.FindIf([leftEntity](Std::Pair<Entity, Transform> const& val) -> bool {
			return val.a == leftEntity;
			});

		if (!leftPosIndex.HasValue())
			continue;
		auto& leftTransform = transforms[leftPosIndex.Value()].b;

		// Check if this entity has a rigidbody
		auto leftRbIndex = rigidbodies.FindIf([&leftEntity](Std::Pair<Entity, Rigidbody2D> const& val) -> bool {
			return val.a == leftEntity;
			});
		Rigidbody2D* leftRb = leftRbIndex.HasValue() ? &rigidbodies[leftRbIndex.Value()].b : nullptr;

		for (uSize rightIndex = leftIndex + 1; rightIndex < circleColliders.Size(); rightIndex += 1)
		{
			Entity const& rightEntity = circleColliders[rightIndex].a;
			CircleCollider2D const& rightCollider = circleColliders[rightIndex].b;

			// First check if this entity has a position
			auto rightPosIndex = transforms.FindIf([rightEntity](Std::Pair<Entity, Transform> const& val) -> bool {
				return val.a == rightEntity;
				});
			if (!rightPosIndex.HasValue())
				continue;
			auto& rightTransform = transforms[rightPosIndex.Value()].b;

			// Check if this entity has a rigidbody
			auto rightRbIndex = rigidbodies.FindIf([&rightEntity](Std::Pair<Entity, Rigidbody2D> const& val) -> bool {
				return val.a == rightEntity;
				});
			Rigidbody2D* rightRb = rightRbIndex.HasValue() ? &rigidbodies[rightRbIndex.Value()].b : nullptr;
			if (leftRb == nullptr && rightRb == nullptr)
				continue;

			Math::Vec2 relativePosVec = (rightTransform.position - leftTransform.position).AsVec2();

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