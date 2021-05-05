#pragma once

#include <DEngine/FixedWidthTypes.hpp>

namespace DEngine::Math
{
	template<uSize length, typename T>
	struct Vector;

	using Vec3 = Vector<3, f32>;
	using Vec3Int = Vector<3, i32>;

	template<>
	struct Vector<3, f32>
	{
		f32 x;
		f32 y;
		f32 z;

		[[nodiscard]] constexpr Vector<2, f32> AsVec2() const;
		[[nodiscard]] constexpr Vector<4, f32> AsVec4(f32 const& wValue = f32()) const;

		[[nodiscard]] f32 Magnitude() const;
		[[nodiscard]] constexpr f32 MagnitudeSqrd() const;
		[[nodiscard]] Vector<3, f32> GetNormalized() const;
		void Normalize();

		[[nodiscard]] constexpr f32* Data();
		[[nodiscard]] constexpr f32 const* Data() const;

		[[nodiscard]] static constexpr Vector<3, f32> Cross(Vector<3, f32> const& lhs, Vector<3, f32> const& rhs);
		[[nodiscard]] static constexpr f32 Dot(Vector<3, f32> const& lhs, Vector<3, f32> const& rhs);

		[[nodiscard]] static constexpr Vector<3, f32> SingleValue(f32 const& input) noexcept;
		[[nodiscard]] static constexpr Vector<3, f32> Zero() noexcept;
		[[nodiscard]] static constexpr Vector<3, f32> One() noexcept;
		[[nodiscard]] static constexpr Vector<3, f32> Up() noexcept;
		[[nodiscard]] static constexpr Vector<3, f32> Down() noexcept;
		[[nodiscard]] static constexpr Vector<3, f32> Left() noexcept;
		[[nodiscard]] static constexpr Vector<3, f32> Right() noexcept;
		[[nodiscard]] static constexpr Vector<3, f32> Forward() noexcept;
		[[nodiscard]] static constexpr Vector<3, f32> Back() noexcept;

		[[nodiscard]] constexpr f32& operator[](uSize index) noexcept;
		[[nodiscard]] constexpr f32 operator[](uSize index) const noexcept;
		constexpr Vector<3, f32>& operator+=(Vector<3, f32> const& rhs);
		constexpr Vector<3, f32>& operator-=(Vector<3, f32> const& rhs);
		constexpr Vector<3, f32>& operator*=(f32 const& rhs);
		[[nodiscard]] constexpr Vector<3, f32> operator+(Vector<3, f32> const& rhs) const;
		[[nodiscard]] constexpr Vector<3, f32> operator-(Vector<3, f32> const& rhs) const;
		[[nodiscard]] constexpr Vector<3, f32> operator-() const;
		[[nodiscard]] constexpr bool operator==(Vector<3, f32> const& rhs) const;
		[[nodiscard]] constexpr bool operator!=(Vector<3, f32> const& rhs) const;
	};

	static_assert(sizeof(Vector<3, f32>) == sizeof(f32[3]), "Size of Vec3 is not as expected.");
	static_assert(alignof(Vector<3, f32>) == alignof(f32[3]), "Alignment of Vec3 is not as expected.");
}