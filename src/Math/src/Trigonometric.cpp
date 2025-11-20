module;

#include <cmath>

module DEngine.Math.Trigonometric;

import DEngine.FixedWidthTypes;

using namespace DEngine;

f32 Math::Sin(f32 radians) noexcept
{
	return sinf(radians);
}

f32 Math::Cos(f32 radians) noexcept
{
	return cosf(radians);
}

f32 Math::ArcCos(f32 in) noexcept
{
	return acosf(in);
}

f32 Math::ArcSin(f32 in) noexcept
{
	return asinf(in);
}

f32 Math::Tan(f32 radians) noexcept
{
	return tanf(radians);
}

f32 Math::ArcTan2(f32 a, f32 b) noexcept
{
	return atan2f(a, b);
}
