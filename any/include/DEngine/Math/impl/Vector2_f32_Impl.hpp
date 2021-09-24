#pragma once

#include <DEngine/Math/impl/Vector2_f32.hpp>

namespace DEngine::Math
{
	[[nodiscard]] constexpr Vector<2, f32> operator*(Vector<2, f32> const& lhs, f32 rhs) noexcept
	{
		return Vector<2, f32>{ lhs.x * rhs, lhs.y * rhs };
	}

	[[nodiscard]] constexpr Vector<2, f32> operator*(f32 lhs, Vector<2, f32> const& rhs) noexcept
	{
		return Vector<2, f32>{ lhs * rhs.x, lhs * rhs.y };
	}

	constexpr Vector<3, f32> Vector<2, f32>::AsVec3(f32 zValue) const noexcept
	{
		return Vector<3, f32>{ x, y, zValue };
	}
	/*
	constexpr Vector<2, f32>::operator Vector<3, f32>() const noexcept
	{
		return Vector<3, f32>{ x, y, 0.f };
	}
	*/

	constexpr Vector<4, f32> Vector<2, f32>::AsVec4(f32 zValue, f32 wValue) const noexcept
	{
		return Vector<4, f32>{ x, y, zValue, wValue };
	}

	constexpr f32 Vector<2, f32>::MagnitudeSqrd() const noexcept
	{
		return (x * x) + (y * y);
	}

	constexpr Vector<2, f32> Vector<2, f32>::GetRotated90(bool counterClock) const noexcept
	{
		// Multiply the vector with -1 if input is false
		// Didgy: I tried doing this branchless. Might be stupid, idk
		return Vector<2, f32>{ -y, x } * (f32)(((i8)counterClock * 2) - 1);
	}

	constexpr f32* Vector<2, f32>::Data() noexcept
	{
		return &x;
	}

	constexpr f32 const* Vector<2, f32>::Data() const noexcept
	{
		return &x;
	}

	constexpr f32 Vector<2, f32>::Dot(Vector<2, f32> const& lhs, Vector<2, f32> const& rhs) noexcept
	{
		return (lhs.x * rhs.x) + (lhs.y * rhs.y);
	}

	constexpr Vector<2, f32> Vector<2, f32>::SingleValue(f32 input) noexcept
	{
		return Vector<2, f32>{ input, input };
	}

	constexpr Vector<2, f32> Vector<2, f32>::Zero() noexcept
	{
		return Vector<2, f32>{ f32(0), f32(0) };
	}

	constexpr Vector<2, f32> Vector<2, f32>::One() noexcept
	{
		return Vector<2, f32>{ f32(1), f32(1) };
	}

	constexpr Vector<2, f32> Vector<2, f32>::Up() noexcept
	{
		return Vector<2, f32>{ f32(0), f32(1) };
	}

	constexpr Vector<2, f32> Vector<2, f32>::Down() noexcept
	{
		return Vector<2, f32>{ f32(0), f32(-1) };
	}

	constexpr Vector<2, f32> Vector<2, f32>::Left() noexcept
	{
		return Vector<2, f32>{ f32(-1), f32(0) };
	}

	constexpr Vector<2, f32> Vector<2, f32>::Right() noexcept
	{
		return Vector<2, f32> {f32(1), f32(0) };
	}

	constexpr f32& Vector<2, f32>::operator[](uSize index) noexcept
	{
		switch (index)
		{
		case 0:
			return x;
		case 1:
			return y;
		default:
			DENGINE_IMPL_MATH_UNREACHABLE();
		}
	}

	constexpr f32 Vector<2, f32>::operator[](uSize index) const noexcept
	{
		switch (index)
		{
		case 0:
			return x;
		case 1:
			return y;
		default:
			DENGINE_IMPL_MATH_UNREACHABLE();
		}
	}

	constexpr Vector<2, f32>& Vector<2, f32>::operator+=(Vector<2, f32> const& rhs) noexcept
	{
		x += rhs.x;
		y += rhs.y;
		return *this;
	}

	constexpr Vector<2, f32>& Vector<2, f32>::operator-=(Vector<2, f32> const& rhs) noexcept
	{
		x -= rhs.x;
		y -= rhs.y;
		return *this;
	}

	constexpr Vector<2, f32>& Vector<2, f32>::operator*=(f32 rhs) noexcept
	{
		x *= rhs;
		y *= rhs;
		return *this;
	}
	
	constexpr Vector<2, f32> Vector<2, f32>::operator+(Vector<2, f32> const& rhs) const noexcept
	{
		return Vector<2, f32>{ x + rhs.x, y + rhs.y };
	}
	
	constexpr Vector<2, f32> Vector<2, f32>::operator-(Vector<2, f32> const& rhs) const noexcept
	{
		return Vector<2, f32>{ x - rhs.x, y - rhs.y };
	}

	constexpr Vector<2, f32> Vector<2, f32>::operator-() const noexcept
	{
		return Vector<2, f32>{ -x, -y };
	}

	constexpr bool Vector<2, f32>::operator==(Vector<2, f32> const& rhs) const noexcept
	{
		return x == rhs.x && y == rhs.y;
	}

	constexpr bool Vector<2, f32>::operator!=(Vector<2, f32> const& rhs) const noexcept
	{
		return x != rhs.x || y != rhs.y;
	}
}