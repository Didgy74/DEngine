#include "DEngine/Math/Vector.hpp"

#include "DEngine/Math/Common.hpp"

DEngine::f32 DEngine::Math::Vector<2, DEngine::f32>::Magnitude() const
{
	return Sqrt(MagnitudeSqrd());
}

DEngine::Math::Vector<2, DEngine::f32> DEngine::Math::Vector<2, DEngine::f32>::Normalized() const
{
	f32 mag = Magnitude();
	return Vector<2, f32>{ x / mag, y / mag };
}

void DEngine::Math::Vector<2, DEngine::f32>::Normalize()
{
	f32 magnitude = Magnitude();
	x /= magnitude;
	y /= magnitude;
}

DEngine::f32 DEngine::Math::Vector<3, DEngine::f32>::Magnitude() const
{
	return Sqrt(MagnitudeSqrd());
}

DEngine::Math::Vector<3, DEngine::f32> DEngine::Math::Vector<3, DEngine::f32>::Normalized() const
{
	f32 mag = Magnitude();
	return Vector<3, f32>{ x / mag, y / mag, z / mag };
}

void DEngine::Math::Vector<3, DEngine::f32>::Normalize()
{
	f32 magnitude = Magnitude();
	x /= magnitude;
	y /= magnitude;
	z /= magnitude;
}

DEngine::f32 DEngine::Math::Vector<4, DEngine::f32>::Magnitude() const
{
	return Sqrt(MagnitudeSqrd());
}

DEngine::Math::Vector<4, DEngine::f32> DEngine::Math::Vector<4, DEngine::f32>::Normalized() const
{
	f32 magnitude = Magnitude();
	return Vector<4, f32>{ x / magnitude, y / magnitude, z / magnitude, w / magnitude };
}

void DEngine::Math::Vector<4, DEngine::f32>::Normalize()
{
	f32 magnitude = Magnitude();
	x /= magnitude;
	y /= magnitude;
	z /= magnitude;
	w /= magnitude;
}
