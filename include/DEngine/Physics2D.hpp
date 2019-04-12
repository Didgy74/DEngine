#pragma once

#include "DMath/Vector/Vector.hpp"

namespace Engine
{
	class Scene;

	namespace Physics2D
	{
		constexpr float gravity = 9.81f;
		
		constexpr Math::Vector2D defaultGravity = Math::Vector2D::Down() * gravity;

		class SceneData
		{
		public:
			Math::Vector2D gravityVector = defaultGravity;
		};

		namespace Core
		{
			void Initialize();
			void FixedTick(const Scene& scene);
		}
	}
}