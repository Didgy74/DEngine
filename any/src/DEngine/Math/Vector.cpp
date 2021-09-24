#include <DEngine/Math/Vector.hpp>

#include <DEngine/Math/Common.hpp>
#include <DEngine/Math/Constant.hpp>
#include <DEngine/Math/Trigonometric.hpp>

#include <DEngine/Math/impl/Assert.hpp>

using namespace DEngine;
using namespace DEngine::Math;

f32 Vector<2, f32>::Magnitude() const
{
	return Sqrt(MagnitudeSqrd());
}

Vector<2, f32> Vector<2, f32>::GetNormalized() const
{
	f32 mag = Magnitude();
	return Vector<2, f32>{ x / mag, y / mag };
}

void Vector<2, f32>::Normalize()
{
	f32 magnitude = Magnitude();
	x /= magnitude;
	y /= magnitude;
}

// Returns this vector rotated radians amount around the Z axis.
Vector<2, f32> Vector<2, f32>::GetRotated(f32 radians) const noexcept
{
	f32 const cs = Cos(radians);
	f32 const sn = Sin(radians);
	return { x * cs - y * sn, x * sn + y * cs };
}

f32 Vector<2, f32>::SignedAngle(Vector<2, f32> const& a, Vector<2, f32> const& b) noexcept
{
	DENGINE_IMPL_ASSERT((a.MagnitudeSqrd() - 1.f) <= 0.00001f);
	DENGINE_IMPL_ASSERT((b.MagnitudeSqrd() - 1.f) <= 0.00001f);

	f32 temp = ArcTan2(a.y, a.x) - ArcTan2(b.y, b.x);
	if (temp > pi)
		temp -= 2 * pi;
	else if (temp <= -pi)
		temp += 2 * pi;
	return temp;
}

f32 Vector<3, f32>::Magnitude() const
{
	return Sqrt(MagnitudeSqrd());
}

Vector<3, f32> Vector<3, f32>::GetNormalized() const
{
	f32 mag = Magnitude();
	return Vector<3, f32>{ x / mag, y / mag, z / mag };
}

void Vector<3, f32>::Normalize()
{
	f32 magnitude = Magnitude();
	x /= magnitude;
	y /= magnitude;
	z /= magnitude;
}

f32 Vector<4, f32>::Magnitude() const
{
	return Sqrt(MagnitudeSqrd());
}

Vector<4, f32> Vector<4, f32>::GetNormalized() const
{
	f32 magnitude = Magnitude();
	return Vector<4, f32>{ x / magnitude, y / magnitude, z / magnitude, w / magnitude };
}

void Vector<4, f32>::Normalize()
{
	f32 magnitude = Magnitude();
	x /= magnitude;
	y /= magnitude;
	z /= magnitude;
	w /= magnitude;
}
