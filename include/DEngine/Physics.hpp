#pragma once

#include <DEngine/FixedWidthTypes.hpp>
#include <DEngine/Math/Vector.hpp>

namespace DEngine
{
	class Scene;
}

namespace DEngine::Physics
{
	struct Rigidbody2D
	{
		static constexpr f32 defaultLinearDamp = 0.95f;

		Math::Vec2 velocity{};
		Math::Vec2 acceleration{};
		f32 angularVelocity = 0;
		f32 torque = 0;

		f32 inverseMass = 1.f;

		// Range [0, 1]
		// Factor of velocity retention per second.
		f32 linearDamp = defaultLinearDamp;

		// Range [0, 1]
		f32 restitution = 1.f;
	};

	struct CircleCollider2D
	{
		f32 radius = 0.5f;
	};

	struct BoxCollider2D
	{
		Math::Vec2 size{ 1.f, 1.f };
	};

	struct Instance
	{
		
	};

	void Update(Scene& scene, f32 deltaTime);
}