#pragma once

#include <DEngine/FixedWidthTypes.hpp>

namespace DEngine::Math
{	
	[[nodiscard]] f32 Sin(f32 radians) noexcept;
	[[nodiscard]] f32 Cos(f32 radians) noexcept;
	[[nodiscard]] f32 ArcCos(f32 in) noexcept;
	[[nodiscard]] f32 Tan(f32 radians) noexcept;
	[[nodiscard]] f32 ArcTan2(f32 a, f32 b) noexcept;
} 
