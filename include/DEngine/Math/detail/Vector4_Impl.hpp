#pragma once

namespace DEngine::Math
{
	constexpr Vector<2, f32> Vector<4, f32>::AsVec2() const
	{
		return Vector<2, f32>{ x, y };
	}
	constexpr Vector<3, f32> Vector<4, f32>::AsVec3() const
	{
		return Vector<3, f32>{ x, y, z };
	}

	constexpr f32& Vector<4, f32>::At(uSize index)
	{
		switch (index)
		{
		case 0:
			return x;
		case 1:
			return y;
		case 2:
			return z;
		case 3:
			return w;
		default:
			return x;
		}
	}
	constexpr f32 const& Vector<4, f32>::At(uSize index) const
	{
		switch (index)
		{
		case 0:
			return x;
		case 1:
			return y;
		case 2:
			return z;
		case 3:
			return w;
		default:
			return x;
		}
	}

	constexpr f32 Vector<4, f32>::Dot(Vector<4, f32> const& lhs, Vector<4, f32> const& rhs)
	{
		return (lhs.x * rhs.x) + (lhs.y * rhs.y) + (lhs.z * rhs.z) + (lhs.w * rhs.w);
	}

	constexpr f32* Vector<4, f32>::Data()
	{
		return &x;
	}
	constexpr f32 const* Vector<4, f32>::Data() const
	{
		return &x;
	}

	constexpr f32 Vector<4, f32>::MagnitudeSqrd() const
	{
		return (x * x) + (y * y) + (z * z) + (w * w);
	}

	constexpr Vector<4, f32> Vector<4, f32>::SingleValue(f32 const& input)
	{
		return { input, input, input, input };
	}
	constexpr Vector<4, f32> Vector<4, f32>::Zero()
	{
		return Vector<4, f32>{ 0.f, 0.f, 0.f, 0.f };
	}
	constexpr Vector<4, f32> Vector<4, f32>::One()
	{
		return { 1.f, 1.f, 1.f, 1.f };
	}

	constexpr Vector<4, f32>& Vector<4, f32>::operator+=(Vector<4, f32> const& rhs)
	{
		x += rhs.x;
		y += rhs.y;
		z += rhs.z;
		w += rhs.w;
		return *this;
	}
	constexpr Vector<4, f32>& Vector<4, f32>::operator-=(Vector<4, f32> const& rhs)
	{
		x -= rhs.x;
		y -= rhs.y;
		z -= rhs.z;
		w -= rhs.w;
		return *this;
	}
	constexpr Vector<4, f32>& Vector<4, f32>::operator*=(f32 const& rhs)
	{
		x *= rhs;
		y *= rhs;
		z *= rhs;
		return *this;
	}
	constexpr Vector<4, f32> Vector<4, f32>::operator+(Vector<4, f32> const& rhs) const
	{
		return { x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w };
	}
	constexpr Vector<4, f32> Vector<4, f32>::operator-(Vector<4, f32> const& rhs) const
	{
		return { x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w };
	}
	constexpr Vector<4, f32> Vector<4, f32>::operator-() const
	{
		return { -x, -y, -z, -w };
	}
	constexpr bool Vector<4, f32>::operator==(Vector<4, f32> const& rhs) const
	{
		return x == rhs.x && y == rhs.y && z == rhs.z && rhs.w;
	}
	constexpr bool Vector<4, f32>::operator!=(Vector<4, f32> const& rhs) const
	{
		return x != rhs.x || y != rhs.y || z != rhs.z || w != rhs.w;
	}

	constexpr Vector<4, f32> operator*(Vector<4, f32> const& lhs, f32 const& rhs)
	{
		return { lhs.x * rhs, lhs.y * rhs, lhs.z * rhs, lhs.w * rhs };
	}

	constexpr Vector<4, f32> operator*(f32 const& lhs, Vector<4, f32> const& rhs)
	{
		return { lhs * rhs.x, lhs * rhs.y, lhs * rhs.z, lhs * rhs.w };
	}
}