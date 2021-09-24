#pragma once

#include <DEngine/FixedWidthTypes.hpp>

namespace DEngine::Math
{
	template<uSize length, typename T>
	struct Vector;

	using Vec2UInt = Vector<2, u32>;

	template<>
	struct Vector<2, u32>
	{
		using ValueType = u32;

		u32 x;
		u32 y;

		[[nodiscard]] constexpr Vector<3, u32> AsVec3(u32 zValue = 0) const;
		[[nodiscard]] constexpr Vector<4, u32> AsVec4(u32 zValue = 0, u32 wValue = 0) const;

		[[nodiscard]] f32 Magnitude() const;
		[[nodiscard]] constexpr f32 MagnitudeSqrd() const;
		[[nodiscard]] Vector<2, f32> GetNormalized() const;

		[[nodiscard]] constexpr u32* Data();
		[[nodiscard]] constexpr u32 const* Data() const;

		[[nodiscard]] static constexpr u32 Dot(Vector<2, u32> const& lhs, Vector<2, u32> const& rhs);

		[[nodiscard]] static constexpr Vector<2, u32> SingleValue(u32 const& input);
		[[nodiscard]] static constexpr Vector<2, u32> Zero();
		[[nodiscard]] static constexpr Vector<2, u32> One();
		[[nodiscard]] static constexpr Vector<2, u32> Up();
		[[nodiscard]] static constexpr Vector<2, u32> Right();

		constexpr Vector<2, u32>& operator+=(Vector<2, u32> const& rhs);
		constexpr Vector<2, u32>& operator-=(Vector<2, u32> const& rhs);
		constexpr Vector<2, u32>& operator*=(u32 const& rhs);
		[[nodiscard]] constexpr Vector<2, u32> operator+(Vector<2, u32> const& rhs) const;
		[[nodiscard]] constexpr Vector<2, u32> operator-(Vector<2, u32> const& rhs) const;
		[[nodiscard]] constexpr Vector<2, u32> operator-() const;
		[[nodiscard]] constexpr bool operator==(Vector<2, u32> const& rhs) const;
		[[nodiscard]] constexpr bool operator!=(Vector<2, u32> const& rhs) const;
	};
}