#pragma once

#include <DEngine/FixedWidthTypes.hpp>

namespace DEngine::Math
{
	template<uSize length, typename T>
	struct Vector;

	using Vec2 = Vector<2, f32>;

	template<>
	struct Vector<2, f32>
	{
		using ValueType = f32;

		f32 x;
		f32 y;

		[[nodiscard]] constexpr Vector<3, f32> AsVec3(f32 zValue = 0.f) const noexcept;
		//[[nodiscard]] constexpr explicit operator Vector<3, f32>() const noexcept;
		[[nodiscard]] constexpr Vector<4, f32> AsVec4(f32 zValue = 0.f, f32 wValue = 0.f) const noexcept;

		[[nodiscard]] f32 Magnitude() const;
		[[nodiscard]] constexpr f32 MagnitudeSqrd() const noexcept;

		[[nodiscard]] Vector<2, f32> GetNormalized() const;
		void Normalize();

		// Returns this vector rotated radians amount around the Z axis.
		[[nodiscard]] Vector<2, f32> GetRotated(f32 radians) const noexcept;

		[[nodiscard]] constexpr f32* Data() noexcept;
		[[nodiscard]] constexpr f32 const* Data() const noexcept;

		[[nodiscard]] static constexpr f32 Dot(Vector<2, f32> const& a, Vector<2, f32> const& b) noexcept;
		// Result in radians.
		// Input must be normalized!
		[[nodiscard]] static f32 Angle(Vector<2, f32> const& a, Vector<2, f32> const& b) noexcept;
		// Result in radians.
		// Input must be normalized!
		[[nodiscard]] static f32 SignedAngle(Vector<2, f32> const& a, Vector<2, f32> const& b) noexcept;

		[[nodiscard]] static constexpr Vector<2, f32> SingleValue(f32 input) noexcept;
		[[nodiscard]] static constexpr Vector<2, f32> Zero() noexcept;
		[[nodiscard]] static constexpr Vector<2, f32> One() noexcept;
		[[nodiscard]] static constexpr Vector<2, f32> Up() noexcept;
		[[nodiscard]] static constexpr Vector<2, f32> Down() noexcept;
		[[nodiscard]] static constexpr Vector<2, f32> Left() noexcept;
		[[nodiscard]] static constexpr Vector<2, f32> Right() noexcept;

		[[nodiscard]] constexpr f32& operator[](uSize i) noexcept;
		[[nodiscard]] constexpr f32 operator[](uSize i) const noexcept;
		constexpr Vector<2, f32>& operator+=(Vector<2, f32> const& rhs) noexcept;
		constexpr Vector<2, f32>& operator-=(Vector<2, f32> const& rhs) noexcept;
		constexpr Vector<2, f32>& operator*=(f32 rhs) noexcept;
		[[nodiscard]] constexpr Vector<2, f32> operator+(Vector<2, f32> const& rhs) const noexcept;
		[[nodiscard]] constexpr Vector<2, f32> operator-(Vector<2, f32> const& rhs) const noexcept;
		[[nodiscard]] constexpr Vector<2, f32> operator-() const noexcept;
		[[nodiscard]] constexpr bool operator==(Vector<2, f32> const& rhs) const noexcept;
		[[nodiscard]] constexpr bool operator!=(Vector<2, f32> const& rhs) const noexcept;
	};

	static_assert(sizeof(Vector<2, f32>) == sizeof(f32[2]), "Size of Vec2 is not as expected.");
	static_assert(alignof(Vector<2, f32>) == alignof(f32[2]), "Alignment of Vec2 is not as expected.");
}