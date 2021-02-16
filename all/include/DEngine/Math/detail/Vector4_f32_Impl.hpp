#pragma once

#include <DEngine/detail/Assert.hpp>
#include "Vector4_f32.hpp"

namespace DEngine::Math
{
	constexpr Vector<2, f32> Vector<4, f32>::AsVec2() const noexcept
	{
		return Vector<2, f32>{ x, y };
	}

	inline constexpr Vector<4, f32>::operator Vector<2, f32>() const noexcept
	{
		return AsVec2();
	}

	constexpr Vector<3, f32> Vector<4, f32>::AsVec3() const noexcept
	{
		return Vector<3, f32>{ x, y, z };
	}

	inline constexpr Vector<4, f32>::operator Vector<3, f32>() const noexcept
	{
		return AsVec3();
	}

	constexpr f32& Vector<4, f32>::At(uSize index) noexcept
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
			DENGINE_DETAIL_UNREACHABLE();
		}
	}

	constexpr f32 Vector<4, f32>::At(uSize index) const noexcept
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
			DENGINE_DETAIL_UNREACHABLE();
		}
	}

	constexpr f32 Vector<4, f32>::Dot(Vector<4, f32> const& lhs, Vector<4, f32> const& rhs) noexcept
	{
		return (lhs.x * rhs.x) + (lhs.y * rhs.y) + (lhs.z * rhs.z) + (lhs.w * rhs.w);
	}

	constexpr f32* Vector<4, f32>::Data() noexcept
	{
		return &x;
	}
	constexpr f32 const* Vector<4, f32>::Data() const noexcept
	{
		return &x;
	}

	constexpr f32 Vector<4, f32>::MagnitudeSqrd() const noexcept
	{
		return (x * x) + (y * y) + (z * z) + (w * w);
	}

	[[nodiscard]] constexpr f32* Vector<4, f32>::begin() noexcept
	{
		return &x;
	}

	[[nodiscard]] constexpr f32 const* Vector<4, f32>::begin() const noexcept
	{
		return &x;
	}

	[[nodiscard]] constexpr f32* Vector<4, f32>::end() noexcept
	{
		return &x + 4;
	}

	[[nodiscard]] constexpr f32 const* Vector<4, f32>::end() const noexcept
	{
		return &x + 4;
	}

	constexpr Vector<4, f32> Vector<4, f32>::SingleValue(f32 input) noexcept
	{
		return { input, input, input, input };
	}
	constexpr Vector<4, f32> Vector<4, f32>::Zero() noexcept
	{
		return Vector<4, f32>{ 0.f, 0.f, 0.f, 0.f };
	}
	constexpr Vector<4, f32> Vector<4, f32>::One() noexcept
	{
		return { 1.f, 1.f, 1.f, 1.f };
	}

	constexpr f32& Vector<4, f32>::operator[](uSize index) noexcept
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
			DENGINE_DETAIL_UNREACHABLE();
		}
	}

	constexpr f32 Vector<4, f32>::operator[](uSize index) const noexcept
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
			DENGINE_DETAIL_UNREACHABLE();
		}
	}

	constexpr Vector<4, f32>& Vector<4, f32>::operator+=(Vector<4, f32> const& rhs) noexcept
	{
		x += rhs.x;
		y += rhs.y;
		z += rhs.z;
		w += rhs.w;
		return *this;
	}

	constexpr Vector<4, f32>& Vector<4, f32>::operator-=(Vector<4, f32> const& rhs) noexcept
	{
		x -= rhs.x;
		y -= rhs.y;
		z -= rhs.z;
		w -= rhs.w;
		return *this;
	}

	constexpr Vector<4, f32>& Vector<4, f32>::operator*=(f32 rhs) noexcept
	{
		x *= rhs;
		y *= rhs;
		z *= rhs;
		return *this;
	}

	constexpr Vector<4, f32> Vector<4, f32>::operator+(Vector<4, f32> const& rhs) const noexcept
	{
		return { x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w };
	}

	constexpr Vector<4, f32> Vector<4, f32>::operator-(Vector<4, f32> const& rhs) const noexcept
	{
		return { x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w };
	}

	constexpr Vector<4, f32> Vector<4, f32>::operator-() const noexcept
	{
		return { -x, -y, -z, -w };
	}

	constexpr bool Vector<4, f32>::operator==(Vector<4, f32> const& rhs) const noexcept
	{
		return x == rhs.x && y == rhs.y && z == rhs.z && rhs.w;
	}

	constexpr bool Vector<4, f32>::operator!=(Vector<4, f32> const& rhs) const noexcept
	{
		return x != rhs.x || y != rhs.y || z != rhs.z || w != rhs.w;
	}

	[[nodiscard]] constexpr Vector<4, f32> operator*(Vector<4, f32> const& lhs, f32 rhs) noexcept
	{
		return { lhs.x * rhs, lhs.y * rhs, lhs.z * rhs, lhs.w * rhs };
	}

	[[nodiscard]] constexpr Vector<4, f32> operator*(f32 lhs, Vector<4, f32> const& rhs) noexcept
	{
		return { lhs * rhs.x, lhs * rhs.y, lhs * rhs.z, lhs * rhs.w };
	}
}