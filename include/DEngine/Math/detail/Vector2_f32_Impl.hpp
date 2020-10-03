#pragma once

namespace DEngine::Math
{
	constexpr Vector<3, f32> Vector<2, f32>::AsVec3(f32 zValue) const
	{
		return Vector<3, f32>{ x, y, zValue };
	}

	constexpr Vector<4, f32> Vector<2, f32>::AsVec4(f32 zValue, f32 wValue) const
	{
		return Vector<4, f32>{ x, y, zValue, wValue };
	}

	constexpr f32 Vector<2, f32>::MagnitudeSqrd() const
	{
		return (x * x) + (y * y);
	}

	constexpr f32* Vector<2, f32>::Data()
	{
		return &x;
	}

	constexpr f32 const* Vector<2, f32>::Data() const
	{
		return &x;
	}

	constexpr f32 Vector<2, f32>::Dot(Vector<2, f32> const& lhs, Vector<2, f32> const& rhs)
	{
		return (lhs.x * rhs.x) + (lhs.y * rhs.y);
	}

	constexpr Vector<2, f32> Vector<2, f32>::SingleValue(const f32& input)
	{
		return Vector<2, f32>{ input, input };
	}
	constexpr Vector<2, f32> Vector<2, f32>::Zero()
	{
		return Vector<2, f32>{ f32(0), f32(0) };
	}
	constexpr Vector<2, f32> Vector<2, f32>::One()
	{
		return Vector<2, f32>{ f32(1), f32(1) };
	}
	constexpr Vector<2, f32> Vector<2, f32>::Up()
	{
		return Vector<2, f32>{f32(0), f32(1)};
	}
	constexpr Vector<2, f32> Vector<2, f32>::Down()
	{
		return Vector<2, f32>{f32(0), f32(-1)};
	}
	constexpr Vector<2, f32> Vector<2, f32>::Left()
	{
		return Vector<2, f32>{f32(-1), f32(0)};
	}
	constexpr Vector<2, f32> Vector<2, f32>::Right()
	{
		return Vector<2, f32>{f32(1), f32(0)};
	}

	constexpr Vector<2, f32>& Vector<2, f32>::operator+=(const Vector<2, f32>& rhs)
	{
		x += rhs.x;
		y += rhs.y;
		return *this;
	}
	constexpr Vector<2, f32>& Vector<2, f32>::operator-=(const Vector<2, f32>& rhs)
	{
		x -= rhs.x;
		y -= rhs.y;
		return *this;
	}
	constexpr Vector<2, f32>& Vector<2, f32>::operator*=(const f32& rhs)
	{
		x *= rhs;
		y *= rhs;
		return *this;
	}
	[[nodiscard]] constexpr Vector<2, f32> Vector<2, f32>::operator+(const Vector<2, f32>& rhs) const
	{
		return Vector<2, f32>{ x + rhs.x, y + rhs.y };
	}
	[[nodiscard]] constexpr Vector<2, f32> Vector<2, f32>::operator-(const Vector<2, f32>& rhs) const
	{
		return Vector<2, f32>{ x - rhs.x, y - rhs.y };
	}
	[[nodiscard]] constexpr Vector<2, f32> Vector<2, f32>::operator-() const
	{
		return Vector<2, f32>{ -x, -y };
	}
	[[nodiscard]] constexpr bool Vector<2, f32>::operator==(const Vector<2, f32>& rhs) const
	{
		return x == rhs.x && y == rhs.y;
	}
	[[nodiscard]] constexpr bool Vector<2, f32>::operator!=(const Vector<2, f32>& rhs) const
	{
		return x != rhs.x || y != rhs.y;
	}

	constexpr Vector<2, f32> operator*(Vector<2, f32> const& lhs, f32 const& rhs)
	{
		return Vector<2, f32>{ lhs.x * rhs, lhs.y * rhs };
	}

	constexpr Vector<2, f32> operator*(f32 const& lhs, Vector<2, f32> const& rhs)
	{
		return Vector<2, f32>{ lhs * rhs.x, lhs * rhs.y };
	}
}