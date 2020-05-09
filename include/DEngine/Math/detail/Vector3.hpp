#pragma once

#include "DEngine/FixedWidthTypes.hpp"

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
		[[nodiscard]] Vector<3, f32> Normalized() const;
		void Normalize();

		[[nodiscard]] constexpr f32* Data();
		[[nodiscard]] constexpr f32 const* Data() const;

		[[nodiscard]] static constexpr Vector<3, f32> Cross(Vector<3, f32> const& lhs, Vector<3, f32> const& rhs);
		[[nodiscard]] static constexpr f32 Dot(Vector<3, f32> const& lhs, Vector<3, f32> const& rhs);

		[[nodiscard]] static constexpr Vector<3, f32> SingleValue(f32 const& input);
		[[nodiscard]] static constexpr Vector<3, f32> Zero();
		[[nodiscard]] static constexpr Vector<3, f32> One();
		[[nodiscard]] static constexpr Vector<3, f32> Up();
		[[nodiscard]] static constexpr Vector<3, f32> Down();
		[[nodiscard]] static constexpr Vector<3, f32> Left();
		[[nodiscard]] static constexpr Vector<3, f32> Right();
		[[nodiscard]] static constexpr Vector<3, f32> Forward();
		[[nodiscard]] static constexpr Vector<3, f32> Back();

		[[nodiscard]] f32& operator[](uSize index);
		[[nodiscard]] f32 const& operator[](uSize index) const;
		constexpr Vector<3, f32>& operator+=(Vector<3, f32> const& rhs);
		constexpr Vector<3, f32>& operator-=(Vector<3, f32> const& rhs);
		constexpr Vector<3, f32>& operator*=(f32 const& rhs);
		[[nodiscard]] constexpr Vector<3, f32> operator+(Vector<3, f32> const& rhs) const;
		[[nodiscard]] constexpr Vector<3, f32> operator-(Vector<3, f32> const& rhs) const;
		[[nodiscard]] constexpr Vector<3, f32> operator-() const;
		[[nodiscard]] constexpr bool operator==(Vector<3, f32> const& rhs) const;
		[[nodiscard]] constexpr bool operator!=(Vector<3, f32> const& rhs) const;
	};
}