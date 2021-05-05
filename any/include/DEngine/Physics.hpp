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

		enum class Type : u8
		{
			Dynamic,
			Static
		};
		Type type = Type::Dynamic;

		void* b2BodyPtr = nullptr;
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