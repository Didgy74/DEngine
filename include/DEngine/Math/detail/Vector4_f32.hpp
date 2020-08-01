#pragma once

#include <DEngine/FixedWidthTypes.hpp>

namespace DEngine::Math
{
	template<uSize length, typename T>
	struct Vector;

	using Vec4 = Vector<4, f32>;
	using Vec4Int = Vector<4, i32>;

	template<>
	struct Vector<4, f32>
	{
		f32 x;
		f32 y;
		f32 z;
		f32 w;

		[[nodiscard]] constexpr Vector<2, f32> AsVec2() const;
		[[nodiscard]] constexpr Vector<3, f32> AsVec3() const;

		[[nodiscard]] constexpr f32& At(uSize index);
		[[nodiscard]] constexpr f32 const& At(uSize index) const;

		[[nodiscard]] static constexpr f32 Dot(Vector<4, f32> const& lhs, Vector<4, f32> const& rhs);

		[[nodiscard]] constexpr f32* Data();
		[[nodiscard]] constexpr f32 const* Data() const;

		[[nodiscard]] f32 Magnitude() const;
		[[nodiscard]] constexpr f32 MagnitudeSqrd() const;
		[[nodiscard]] Vector<4, f32> Normalized() const;

		void Normalize();

		[[nodiscard]] static constexpr Vector<4, f32> SingleValue(f32 const& input);
		[[nodiscard]] static constexpr Vector<4, f32> Zero();
		[[nodiscard]] static constexpr Vector<4, f32> One();

		constexpr f32& operator[](uSize i) noexcept;
		constexpr f32 operator[](uSize i) const noexcept;
		constexpr Vector<4, f32>& operator+=(Vector<4, f32> const& rhs);
		constexpr Vector<4, f32>& operator-=(Vector<4, f32> const& rhs);
		constexpr Vector<4, f32>& operator*=(f32 const& rhs);
		[[nodiscard]] constexpr Vector<4, f32> operator+(Vector<4, f32> const& rhs) const;
		[[nodiscard]] constexpr Vector<4, f32> operator-(Vector<4, f32> const& rhs) const;
		[[nodiscard]] constexpr Vector<4, f32> operator-() const;
		[[nodiscard]] constexpr bool operator==(Vector<4, f32> const& rhs) const;
		[[nodiscard]] constexpr bool operator!=(Vector<4, f32> const& rhs) const;
	};
}