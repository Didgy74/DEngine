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

		[[nodiscard]] constexpr Vector<2, f32> AsVec2() const noexcept;
		[[nodiscard]] constexpr Vector<3, f32> AsVec3() const noexcept;

		[[nodiscard]] constexpr f32& At(uSize index) noexcept;
		[[nodiscard]] constexpr f32 At(uSize index) const noexcept;

		[[nodiscard]] static constexpr f32 Dot(Vector<4, f32> const& lhs, Vector<4, f32> const& rhs) noexcept;

		[[nodiscard]] constexpr f32* Data() noexcept;
		[[nodiscard]] constexpr f32 const* Data() const noexcept;

		[[nodiscard]] f32 Magnitude() const;
		[[nodiscard]] constexpr f32 MagnitudeSqrd() const noexcept;
		[[nodiscard]] Vector<4, f32> Normalized() const;

		void Normalize();

		[[nodiscard]] constexpr f32* begin() noexcept;
		[[nodiscard]] constexpr f32 const* begin() const noexcept;
		[[nodiscard]] constexpr f32* end() noexcept;
		[[nodiscard]] constexpr f32 const* end() const noexcept;

		[[nodiscard]] static constexpr Vector<4, f32> SingleValue(f32 input) noexcept;
		[[nodiscard]] static constexpr Vector<4, f32> Zero() noexcept;
		[[nodiscard]] static constexpr Vector<4, f32> One() noexcept;

		[[nodiscard]] constexpr f32& operator[](uSize index) noexcept;
		[[nodiscard]] constexpr f32 operator[](uSize index) const noexcept;
		constexpr Vector<4, f32>& operator+=(Vector<4, f32> const& rhs) noexcept;
		constexpr Vector<4, f32>& operator-=(Vector<4, f32> const& rhs) noexcept;
		constexpr Vector<4, f32>& operator*=(f32 rhs) noexcept;
		[[nodiscard]] constexpr Vector<4, f32> operator+(Vector<4, f32> const& rhs) const noexcept;
		[[nodiscard]] constexpr Vector<4, f32> operator-(Vector<4, f32> const& rhs) const noexcept;
		[[nodiscard]] constexpr Vector<4, f32> operator-() const noexcept;
		[[nodiscard]] constexpr bool operator==(Vector<4, f32> const& rhs) const noexcept;
		[[nodiscard]] constexpr bool operator!=(Vector<4, f32> const& rhs) const noexcept;
	};
}