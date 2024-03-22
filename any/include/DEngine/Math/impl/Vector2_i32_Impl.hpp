#pragma once

#include <DEngine/Math/impl/Assert.hpp>

namespace DEngine::Math
{
	constexpr Vector<3, i32> Vector<2, i32>::AsVec3(i32 zValue) const
	{
		return Vector<3, i32>{ x, y, zValue };
	}

	constexpr Vector<4, i32> Vector<2, i32>::AsVec4(i32 zValue, i32 wValue) const
	{
		return Vector<4, i32>{ x, y, zValue, wValue };
	}

	constexpr f32 Vector<2, i32>::MagnitudeSqrd() const
	{
		return f32((x * x) + (y * y));
	}

	constexpr i32* Vector<2, i32>::Data()
	{
		return &x;
	}

	constexpr i32 const* Vector<2, i32>::Data() const
	{
		return &x;
	}

	constexpr i32 Vector<2, i32>::Dot(Vector<2, i32> const& lhs, Vector<2, i32> const& rhs)
	{
		return (lhs.x * rhs.x) + (lhs.y * rhs.y);
	}

	constexpr Vector<2, i32> Vector<2, i32>::SingleValue(i32 const& input)
	{
		return Vector<2, i32>{ input, input };
	}
	constexpr Vector<2, i32> Vector<2, i32>::Zero()
	{
		return Vector<2, i32>{ i32(0), i32(0) };
	}
	constexpr Vector<2, i32> Vector<2, i32>::One()
	{
		return Vector<2, i32>{ i32(1), i32(1) };
	}
	constexpr Vector<2, i32> Vector<2, i32>::Up()
	{
		return Vector<2, i32>{i32(0), i32(1)};
	}
	constexpr Vector<2, i32> Vector<2, i32>::Down()
	{
		return Vector<2, i32>{i32(0), i32(-1)};
	}
	constexpr Vector<2, i32> Vector<2, i32>::Left()
	{
		return Vector<2, i32>{i32(-1), i32(0)};
	}
	constexpr Vector<2, i32> Vector<2, i32>::Right()
	{
		return Vector<2, i32>{i32(1), i32(0)};
	}

	constexpr i32& Vector<2, i32>::operator[](uSize i) noexcept
	{
		DENGINE_IMPL_MATH_ASSERT_MSG(
			i < 2,
			"Attempted to index into a Vec<2, i32> with an index out of bounds.");
		return (&x)[i];
	}
	constexpr i32 Vector<2, i32>::operator[](uSize i) const noexcept
	{
		DENGINE_IMPL_MATH_ASSERT_MSG(
			i < 2,
			"Attempted to index into a Vec<2, i32> with an index out of bounds.");
		return (&x)[i];
	}

	constexpr Vector<2, i32>& Vector<2, i32>::operator+=(Vector<2, i32> const& rhs)
	{
		x += rhs.x;
		y += rhs.y;
		return *this;
	}
	constexpr Vector<2, i32>& Vector<2, i32>::operator-=(Vector<2, i32> const& rhs)
	{
		x -= rhs.x;
		y -= rhs.y;
		return *this;
	}
	constexpr Vector<2, i32>& Vector<2, i32>::operator*=(i32 const& rhs)
	{
		x *= rhs;
		y *= rhs;
		return *this;
	}
	[[nodiscard]] constexpr Vector<2, i32> Vector<2, i32>::operator+(Vector<2, i32> const& rhs) const
	{
		return Vector<2, i32>{ x + rhs.x, y + rhs.y };
	}
	[[nodiscard]] constexpr Vector<2, i32> Vector<2, i32>::operator-(Vector<2, i32> const& rhs) const
	{
		return Vector<2, i32>{ x - rhs.x, y - rhs.y };
	}
	[[nodiscard]] constexpr Vector<2, i32> Vector<2, i32>::operator-() const
	{
		return Vector<2, i32>{ -x, -y };
	}
	[[nodiscard]] constexpr bool Vector<2, i32>::operator==(Vector<2, i32> const& rhs) const
	{
		return x == rhs.x && y == rhs.y;
	}
	[[nodiscard]] constexpr bool Vector<2, i32>::operator!=(Vector<2, i32> const& rhs) const
	{
		return x != rhs.x || y != rhs.y;
	}

	constexpr Vector<2, i32> operator*(Vector<2, i32> const& lhs, i32 const& rhs)
	{
		return Vector<2, i32>{ lhs.x* rhs, lhs.y* rhs };
	}

	constexpr Vector<2, i32> operator*(i32 const& lhs, Vector<2, i32> const& rhs)
	{
		return Vector<2, i32>{ lhs* rhs.x, lhs* rhs.y };
	}
}