#pragma once

#include <DEngine/FixedWidthTypes.hpp>

namespace DEngine::Math
{
	template<uSize length, typename T>
	struct Vector;

	using Vec2Int = Vector<2, i32>;

	template<>
	struct Vector<2, i32>
	{
		using ValueType = i32;

		i32 x;
		i32 y;

		[[nodiscard]] constexpr Vector<3, i32> AsVec3(i32 zValue = 0) const;
		[[nodiscard]] constexpr Vector<4, i32> AsVec4(i32 zValue = 0, i32 wValue = 0) const;

		[[nodiscard]] f32 Magnitude() const;
		[[nodiscard]] constexpr f32 MagnitudeSqrd() const;
		[[nodiscard]] Vector<2, f32> GetNormalized() const;

		[[nodiscard]] constexpr i32* Data();
		[[nodiscard]] constexpr i32 const* Data() const;

		[[nodiscard]] static constexpr i32 Dot(Vector<2, i32> const& lhs, Vector<2, i32> const& rhs);

		[[nodiscard]] static constexpr Vector<2, i32> SingleValue(i32 const& input);
		[[nodiscard]] static constexpr Vector<2, i32> Zero();
		[[nodiscard]] static constexpr Vector<2, i32> One();
		[[nodiscard]] static constexpr Vector<2, i32> Up();
		[[nodiscard]] static constexpr Vector<2, i32> Down();
		[[nodiscard]] static constexpr Vector<2, i32> Left();
		[[nodiscard]] static constexpr Vector<2, i32> Right();

		[[nodiscard]] i32& operator[](uSize i) noexcept;
		[[nodiscard]] i32 operator[](uSize i) const noexcept;

		constexpr Vector<2, i32>& operator+=(Vector<2, i32> const& rhs);
		constexpr Vector<2, i32>& operator-=(Vector<2, i32> const& rhs);
		constexpr Vector<2, i32>& operator*=(i32 const& rhs);
		[[nodiscard]] constexpr Vector<2, i32> operator+(Vector<2, i32> const& rhs) const;
		[[nodiscard]] constexpr Vector<2, i32> operator-(Vector<2, i32> const& rhs) const;
		[[nodiscard]] constexpr Vector<2, i32> operator-() const;
		[[nodiscard]] constexpr bool operator==(Vector<2, i32> const& rhs) const;
		[[nodiscard]] constexpr bool operator!=(Vector<2, i32> const& rhs) const;
	};
}