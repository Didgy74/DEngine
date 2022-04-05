#include <DEngine/Math/Common.hpp>
#include <DEngine/Math/Constant.hpp>
#include <DEngine/Math/LinearTransform3D.hpp>
#include <DEngine/Math/Trigonometric.hpp>

#include <cmath>

using namespace DEngine;

f32 Math::Ceil(f32 input)
{
	return ceilf(input);
}

f64 Math::Ceil(f64 input)
{
	return ceill(input);
}

f32 Math::Round(f32 input)
{
	return roundf(input);
}

f32 Math::Sqrt(f32 input)
{
	return sqrtf(input);
}

f64 Math::Sqrt(f64 input)
{
	return sqrtl(input);
}

f32 Math::Pow(f32 coefficient, f32 exponent)
{
	return powf(coefficient, exponent);
}

f64 Math::Pow(f64 coefficient, f64 exponent)
{
	return powl(coefficient, exponent);
}

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

f32 Math::Tan(f32 radians) noexcept
{
	return tanf(radians);
}

f32 Math::ArcTan2(f32 a, f32 b) noexcept
{
	return atan2f(a, b);
}