#include <DEngine/Physics.hpp>

#include <DEngine/Scene.hpp>

#include <DEngine/Math/LinearTransform2D.hpp>
#include <DEngine/Std/Trait.hpp>
#include <DEngine/Std/Utility.hpp>
#include <DEngine/detail/Assert.hpp>

// Collider polygons use counter-clockwise winding order.

using namespace DEngine;
using namespace DEngine::Physics;

namespace DEngine::Physics
{
	f32 Vec2DCross(Math::Vec2 const& a, Math::Vec2 const& b)
	{
		return a.x * b.y - a.y * b.x;
	}

	void Resolve_Circle_Circle(
		Std::Span<decltype(Scene::rigidbodies)::value_type> rigidbodies,
		Std::Span<decltype(Scene::transforms)::value_type const> transforms,
		Std::Span<decltype(Scene::circleColliders)::value_type const> circleColliders);
	void Resolve_Circle_OBB(
		Std::Span<decltype(Scene::rigidbodies)::value_type> rigidbodies,
		Std::Span<decltype(Scene::transforms)::value_type const> transforms,
		Std::Span<decltype(Scene::circleColliders)::value_type const> circleColliders,
		Std::Span<decltype(Scene::boxColliders)::value_type const> boxColliders);
	void Resolve_OBB_OBB(
		Std::Span<decltype(Scene::rigidbodies)::value_type> rigidbodies,
		Std::Span<decltype(Scene::transforms)::value_type const> transforms,
		Std::Span<decltype(Scene::boxColliders)::value_type const> boxColliders);

	bool Collide_OBB_OBB_SAT(
		Transform leftTransform,
		BoxCollider2D boxCollider,
		Transform rightTransform,
		BoxCollider2D rightCollider);

	struct Collide_OBB_OBB_Return
	{
		Math::Vec2 contactPoint;
		Math::Vec2 contactNormal;
	};
	Std::Opt<Collide_OBB_OBB_Return> Collide_OBB_OBB_Diag_Internal(
		Transform leftTransform,
		BoxCollider2D boxCollider,
		Transform rightTransform,
		BoxCollider2D rightCollider);
	 void Collide_OBB_OBB_Diag(
		Transform leftTransform,
		Rigidbody2D* leftRb,
		BoxCollider2D boxCollider,
		Transform rightTransform,
		Rigidbody2D* rightRb,
		BoxCollider2D rightCollider);

	// ContactNormal is left-to-right
	static void CollisionTest(
		Math::Vec2 leftCenter,
		Rigidbody2D* leftRb,
		Math::Vec2 rightCenter,
		Rigidbody2D* rightRb,
		Math::Vec2 contactPoint, 
		Math::Vec2 contactNormal)
	{
		// Atleast one rigidbody must be valid.
		DENGINE_DETAIL_ASSERT(leftRb || rightRb);
		// Assert that the contact-normal has length equals 1
		DENGINE_DETAIL_ASSERT(Math::Abs(contactNormal.MagnitudeSqrd() - 1.f) < 0.01f);

		Math::Vec2 leftVelocity{};
		f32 leftAngularVelocity = 0.f;
		f32 leftInverseMass = 0.f;
		if (leftRb)
		{
			leftVelocity = leftRb->velocity;
			leftAngularVelocity = leftRb->angularVelocity;
			leftInverseMass = leftRb->inverseMass;
		}

		Math::Vec2 rightVelocity{};
		f32 rightAngularVelocity = 0.f;
		f32 rightInverseMass = 0.f;
		if (rightRb)
		{
			rightVelocity = rightRb->velocity;
			rightAngularVelocity = rightRb->angularVelocity;
			rightInverseMass = rightRb->inverseMass;
		}

		// Do nothing if both rigidbodies have infinite mass
		// TODO: Add proper tolerances
		if (leftInverseMass == 0.f && rightInverseMass == 0.f)
			return;

		Math::Vec2 leftContactRadiusVec = contactPoint - leftCenter;
		Math::Vec2 rightContactRadiusVec = contactPoint - rightCenter;

		// Calculate relative velocity
		Math::Vec2 leftContactRadiusVecRotated = { -leftContactRadiusVec.y, leftContactRadiusVec.x };
		leftContactRadiusVecRotated *= leftAngularVelocity;
		Math::Vec2 rightContactRadiusVecRotated = { -rightContactRadiusVec.y, rightContactRadiusVec.x };
		rightContactRadiusVecRotated *= rightAngularVelocity;

		Math::Vec2 radiusV = rightVelocity + rightContactRadiusVecRotated - leftVelocity - leftContactRadiusVecRotated;

		// Relative velocity along the contactNormal. If we the velocity is separating, we have nothing to do.
		f32 separatingVelocity = Math::Vec2::Dot(radiusV, contactNormal);
		if (separatingVelocity > 0)
			return;

		f32 raCrossN = Vec2DCross(leftContactRadiusVec, contactNormal);
		f32 rbCrossN = Vec2DCross(rightContactRadiusVec, contactNormal);

		//f32 inverseMassSum = bodyA->inverseMass + bodyB->inverseMass + (raCrossN * raCrossN) * bodyA->inverseInertia + (rbCrossN * rbCrossN) * bodyB->inverseInertia;
		f32 inverseMassSum = leftInverseMass + rightInverseMass + (raCrossN * raCrossN) + (rbCrossN * rbCrossN);

		// Calculate impulse scalar value
		f32 impulse = -(1.0f + 1.f) * separatingVelocity;
		impulse /= inverseMassSum;

		// Apply impulse to each physics body
		Math::Vec2 impulseV = contactNormal * impulse;

		if (leftRb)
		{
			leftRb->velocity += leftInverseMass * -impulseV;
			leftRb->angularVelocity += Vec2DCross(leftContactRadiusVec, -impulseV);
		}

		if (rightRb)
		{
			rightRb->velocity += rightInverseMass * impulseV;
			rightRb->angularVelocity += Vec2DCross(rightContactRadiusVec, impulseV);
		}
	}
}

void DEngine::Physics::Update(Scene& scene, f32 deltaTime)
{
	Resolve_Circle_Circle(
		{ scene.rigidbodies.data(), scene.rigidbodies.size() },
		{ scene.transforms.data(), scene.transforms.size() },
		{ scene.circleColliders.data(), scene.circleColliders.size() });

	Resolve_Circle_OBB(
		{ scene.rigidbodies.data(), scene.rigidbodies.size() },
		{ scene.transforms.data(), scene.transforms.size() },
		{ scene.circleColliders.data(), scene.circleColliders.size() },
		{ scene.boxColliders.data(), scene.boxColliders.size() });

	Resolve_OBB_OBB(
		{ scene.rigidbodies.data(), scene.rigidbodies.size() },
		{ scene.transforms.data(), scene.transforms.size() },
		{ scene.boxColliders.data(), scene.boxColliders.size() });

	for (uSize i = 0; i < scene.rigidbodies.size(); i += 1)
	{
		Entity const& entity = scene.rigidbodies[i].a;
		Rigidbody2D& rb = scene.rigidbodies[i].b;

		rb.velocity *= Math::Pow(rb.linearDamp, deltaTime);
		//rb.angularVelocity *= Math::Pow(rb.linearDamp, deltaTime);
		rb.velocity += rb.acceleration * deltaTime;
		rb.angularVelocity += rb.torque * deltaTime;
		
		rb.acceleration = {};
		rb.torque = 0;

		auto leftPosIt = Std::FindIf(
			scene.transforms.begin(),
			scene.transforms.end(),
			[entity](decltype(Scene::transforms)::value_type const& val) -> bool { return val.a == entity; });
		if (leftPosIt == scene.transforms.end())
			continue;
		auto& transform = leftPosIt->b;
		transform.position += rb.velocity.AsVec3() * deltaTime;
		transform.rotation += rb.angularVelocity * deltaTime;
	}
}

void DEngine::Physics::Resolve_Circle_Circle(
	Std::Span<decltype(Scene::rigidbodies)::value_type> rigidbodies,
	Std::Span<decltype(Scene::transforms)::value_type const> transforms,
	Std::Span<decltype(Scene::circleColliders)::value_type const> circleColliders)
{
	for (uSize leftIndex = 0; leftIndex < circleColliders.Size(); leftIndex += 1)
	{
		Entity const& leftEntity = circleColliders[leftIndex].a;
		CircleCollider2D const& leftCollider = circleColliders[leftIndex].b;

		// First check if this entity has a transform
		auto const leftTransIt = Std::FindIf(
			transforms.AsRange(),
			[leftEntity](decltype(transforms)::ValueType const& val) -> bool { return val.a == leftEntity; });
		if (leftTransIt == transforms.end())
			continue;
		auto& leftTransform = leftTransIt->b;

		// Check if this entity has a rigidbody
		auto const leftRbIt = Std::FindIf(
			rigidbodies.AsRange(),
			[leftEntity](decltype(rigidbodies)::ValueType const& val) -> bool { return val.a == leftEntity; });
		Rigidbody2D* leftRb = leftRbIt != rigidbodies.end() ? &leftRbIt->b : nullptr;

		for (uSize rightIndex = leftIndex + 1; rightIndex < circleColliders.Size(); rightIndex += 1)
		{
			Entity const& rightEntity = circleColliders[rightIndex].a;
			CircleCollider2D const& rightCollider = circleColliders[rightIndex].b;

			// First check if this entity has a position
			auto const rightTransIt = Std::FindIf(
				transforms.AsRange(),
				[rightEntity](decltype(transforms)::ValueType const& val) -> bool { return val.a == rightEntity; });
			if (rightTransIt == transforms.end())
				continue;
			auto& rightTransform = rightTransIt->b;

			// Check if this entity has a rigidbody
			auto const rightRbIt = Std::FindIf(
				rigidbodies.AsRange(),
				[rightEntity](decltype(rigidbodies)::ValueType const& val) -> bool { return val.a == rightEntity; });
			Rigidbody2D* rightRb = rightRbIt != rigidbodies.end() ? &rightRbIt->b : nullptr;

			if (leftRb == nullptr && rightRb == nullptr)
				continue;

			f32 distance = (rightTransform.position.AsVec2() - leftTransform.position.AsVec2()).Magnitude();
			if (distance <= (leftCollider.radius * leftTransform.scale.x) + (rightCollider.radius))
			{
				Math::Vec2 relativePosVec = rightTransform.position.AsVec2() - leftTransform.position.AsVec2();
				Math::Vec2 contactPoint = leftTransform.position.AsVec2() + (relativePosVec.Normalized() * leftCollider.radius);
				Math::Vec2 contactNormal = relativePosVec.Normalized();
				CollisionTest(
					leftTransform.position.AsVec2(),
					leftRb,
					rightTransform.position.AsVec2(),
					rightRb,
					contactPoint,
					contactNormal);
			}
		}
	}
}

void DEngine::Physics::Resolve_Circle_OBB(
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
			[leftEntity](decltype(transforms)::ValueType const& val) -> bool { return val.a == leftEntity; });
		if (leftTransIt == transforms.end())
			continue;
		auto& leftTransform = leftTransIt->b;

		// Check if this entity has a rigidbody
		auto const leftRbIt = Std::FindIf(
			rigidbodies.AsRange(),
			[leftEntity](decltype(rigidbodies)::ValueType const& val) -> bool { return val.a == leftEntity; });
		Rigidbody2D* leftRb = leftRbIt != rigidbodies.end() ? &leftRbIt->b : nullptr;

		for (uSize rightIndex = 0; rightIndex < boxColliders.Size(); rightIndex += 1)
		{
			Entity const& rightEntity = boxColliders[rightIndex].a;
			BoxCollider2D const& rightCollider = boxColliders[rightIndex].b;

			// First check if this entity has a position
			auto const rightTransIt = Std::FindIf(
				transforms.AsRange(),
				[rightEntity](decltype(transforms)::ValueType const& val) -> bool { return val.a == rightEntity; });
			if (rightTransIt == transforms.end())
				continue;
			auto& rightTransform = rightTransIt->b;

			// Check if this entity has a rigidbody
			auto const rightRbIt = Std::FindIf(
				rigidbodies.AsRange(),
				[rightEntity](decltype(rigidbodies)::ValueType const& val) -> bool { return val.a == rightEntity; });
			Rigidbody2D* rightRb = rightRbIt != rigidbodies.end() ? &rightRbIt->b : nullptr;

			if (leftRb == nullptr && rightRb == nullptr)
				continue;

			// Get closest point on box to circle
			{
				Math::Vec2 d = leftTransform.position.AsVec2() - rightTransform.position.AsVec2();

				// 0 is right
				// 1 is up
				Math::Vec2 rightUnitVectors[2] = {
					Math::LinTran2D::Rotate(rightTransform.rotation) * Math::Vec2::Right(),
					Math::LinTran2D::Rotate(rightTransform.rotation) * Math::Vec2::Up(),
				};

				// Start the center of right, move towards left in direction of right's unit vectors.
				Math::Vec2 nearestPoint = rightTransform.position.AsVec2();
				for (uSize dimension = 0; dimension < 2; dimension += 1)
				{
					f32 dist = Math::Vec2::Dot(d, rightUnitVectors[dimension]);
					f32 halfExtent = rightCollider.size[dimension] * rightTransform.scale[dimension] / 2.f;
					if (dist > halfExtent)
						dist = halfExtent;
					else if (dist < -halfExtent)
						dist = -halfExtent;
					nearestPoint += dist * rightUnitVectors[dimension];
				}

				f32 distToNearestPoint = (leftTransform.position.AsVec2() - nearestPoint).Magnitude();
				if (distToNearestPoint <= leftCollider.radius * leftTransform.scale.x)
				{
					// We have a collision
					CollisionTest(
						leftTransform.position.AsVec2(),
						leftRb,
						rightTransform.position.AsVec2(),
						rightRb,
						nearestPoint,
						(nearestPoint - leftTransform.position.AsVec2()).Normalized());
				}
			}
		}
	}
}


#include <iostream>

void DEngine::Physics::Resolve_OBB_OBB(
	Std::Span<decltype(Scene::rigidbodies)::value_type> rigidbodies,
	Std::Span<decltype(Scene::transforms)::value_type const> transforms,
	Std::Span<decltype(Scene::boxColliders)::value_type const> boxColliders)
{
	for (uSize leftIndex = 0; leftIndex < boxColliders.Size(); leftIndex += 1)
	{
		Entity const& leftEntity = boxColliders[leftIndex].a;
		BoxCollider2D const& leftCollider = boxColliders[leftIndex].b;

		// First check if this entity has a transform
		auto const leftTransIt = Std::FindIf(
			transforms.AsRange(),
			[leftEntity](decltype(transforms)::ValueType const& val) -> bool { return val.a == leftEntity; });
		if (leftTransIt == transforms.end())
			continue;
		auto& leftTransform = leftTransIt->b;

		// Check if this entity has a rigidbody
		auto const leftRbIt = Std::FindIf(
			rigidbodies.AsRange(),
			[leftEntity](decltype(rigidbodies)::ValueType const& val) -> bool { return val.a == leftEntity; });
		Rigidbody2D* leftRb = leftRbIt != rigidbodies.end() ? &leftRbIt->b : nullptr;

		for (uSize rightIndex = leftIndex + 1; rightIndex < boxColliders.Size(); rightIndex += 1)
		{
			Entity const& rightEntity = boxColliders[rightIndex].a;
			BoxCollider2D const& rightCollider = boxColliders[rightIndex].b;

			// First check if this entity has a position
			auto const rightTransIt = Std::FindIf(
				transforms.AsRange(),
				[rightEntity](decltype(transforms)::ValueType const& val) -> bool { return val.a == rightEntity; });
			if (rightTransIt == transforms.end())
				continue;
			auto& rightTransform = rightTransIt->b;

			// Check if this entity has a rigidbody
			auto const rightRbIt = Std::FindIf(
				rigidbodies.AsRange(),
				[rightEntity](decltype(rigidbodies)::ValueType const& val) -> bool { return val.a == rightEntity; });
			Rigidbody2D* rightRb = rightRbIt != rigidbodies.end() ? &rightRbIt->b : nullptr;

			if (leftRb == nullptr && rightRb == nullptr)
				continue;

			Collide_OBB_OBB_Diag(
				leftTransform,
				leftRb,
				leftCollider,
				rightTransform,
				rightRb,
				rightCollider);
		}
	}
}

Std::Opt<Physics::Collide_OBB_OBB_Return> DEngine::Physics::Collide_OBB_OBB_Diag_Internal(
	Transform leftTransform,
	BoxCollider2D leftCollider,
	Transform rightTransform,
	BoxCollider2D rightCollider)
{
	constexpr uSize count = 4;

	Math::Vec2 const leftHalfExtent = { leftCollider.size.x / 2.f, leftCollider.size.y / 2.f };
	Math::Vec2 leftCorners[count] = {
		{ -leftHalfExtent.x, leftHalfExtent.y },
		{ -leftHalfExtent.x, -leftHalfExtent.y },
		{ leftHalfExtent.x, -leftHalfExtent.y },
		{ leftHalfExtent.x, leftHalfExtent.y } };
	
	Math::Vec2 const rightHalfExtent = { rightCollider.size.x / 2.f, rightCollider.size.y / 2.f };
	Math::Vec2 const rightCorners[count] = {
		{ -rightHalfExtent.x, rightHalfExtent.y },
		{ -rightHalfExtent.x, -rightHalfExtent.y },
		{ rightHalfExtent.x, -rightHalfExtent.y },
		{ rightHalfExtent.x, rightHalfExtent.y } };
	
	Math::Mat2 const leftRotationMat = Math::LinTran2D::Rotate(leftTransform.rotation);
	Math::Mat2 const rightRotationMat = Math::LinTran2D::Rotate(rightTransform.rotation);
	for (uSize diagonalIndex = 0; diagonalIndex < count; diagonalIndex += 1)
	{
		Math::Vec2 const leftCorner = leftRotationMat * Math::Vec2{ 
			leftCorners[diagonalIndex].x * leftTransform.scale.x, 
			leftCorners[diagonalIndex].y * leftTransform.scale.y };
		
		Math::Vec2 const leftDiagStart = leftTransform.position.AsVec2();
		Math::Vec2 const leftDiagEnd = leftDiagStart + leftCorner;

		for (uSize edgeIndex = 0; edgeIndex < count; edgeIndex += 1)
		{
			Math::Vec2 const rightCornerA = { 
				rightCorners[edgeIndex].x * rightTransform.scale.x, 
				rightCorners[edgeIndex].y * rightTransform.scale.y };
			Math::Vec2 const rightCornerB = {
				rightCorners[(edgeIndex + 1) % count].x * rightTransform.scale.x,
				rightCorners[(edgeIndex + 1) % count].y * rightTransform.scale.y };
			
			Math::Vec2 const rightEdge = rightRotationMat * (rightCornerB - rightCornerA);

			Math::Vec2 const rightEdgeStart = rightTransform.position.AsVec2() + rightRotationMat * rightCornerA;
			Math::Vec2 const rightEdgeEnd = rightEdgeStart + rightEdge;

			f32 const& x1 = leftDiagStart.x;
			f32 const& y1 = leftDiagStart.y;
			f32 const& x2 = leftDiagEnd.x;
			f32 const& y2 = leftDiagEnd.y;
			f32 const& x3 = rightEdgeStart.x;
			f32 const& y3 = rightEdgeStart.y;
			f32 const& x4 = rightEdgeEnd.x;
			f32 const& y4 = rightEdgeEnd.y;

			// Check if lines are parallell
			f32 denominator = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);
			if (Math::Abs(denominator) > 0.0001f)
			{
				// Lines are not paralell

				// Parametric line intersection
				f32 t = ((x1 - x3) * (y3 - y4) - (y1 - y3) * (x3 - x4)) / denominator;
				// Negating U fixes a bug somehow
				f32 u = ((x1 - x2) * (y1 - y3) - (y1 - y2) * (x1 - x3)) / denominator;
				u = -u;

				if (t >= 0.f && t <= 1.f && u >= 0.f && u <= 1.f)
				{
					Physics::Collide_OBB_OBB_Return returnVal;
					returnVal.contactPoint = leftDiagEnd;
					Math::Vec2 rightEdgeNormalized = rightEdge.Normalized();
					returnVal.contactNormal = -Math::Vec2{ rightEdgeNormalized.y, -rightEdgeNormalized.x };
					return Std::Opt<Collide_OBB_OBB_Return>{ returnVal };
				}
			}
		}
	}

	return Std::nullOpt;
}

void Physics::Collide_OBB_OBB_Diag(
	Transform leftTransform,
	Rigidbody2D* leftRb,
	BoxCollider2D leftCollider,
	Transform rightTransform,
	Rigidbody2D* rightRb,
	BoxCollider2D rightCollider)
{
	Std::Opt<Physics::Collide_OBB_OBB_Return> a = Physics::Collide_OBB_OBB_Diag_Internal(
		leftTransform,
		leftCollider,
		rightTransform,
		rightCollider);

	if (!a.HasValue())
	{
		a = Physics::Collide_OBB_OBB_Diag_Internal(
			rightTransform,
			rightCollider,
			leftTransform,
			leftCollider);
		if (a.HasValue())
			a.Value().contactNormal = -a.Value().contactNormal;
	}


	if (a.HasValue())
	{
		Physics::Collide_OBB_OBB_Return temp = a.Value();
		CollisionTest(
			leftTransform.position.AsVec2(),
			leftRb,
			rightTransform.position.AsVec2(),
			rightRb,
			temp.contactPoint,
			temp.contactNormal);
	}
}