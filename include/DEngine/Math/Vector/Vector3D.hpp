#pragma once

#include "DEngine/FixedWidthTypes.hpp"

#include "DEngine/Math/Enums.hpp"

namespace DEngine::Math
{
	template<uSize length, typename T>
	struct Vector;

	using Vec3D = Vector<3, f32>;
	using Vec3DInt = Vector<3, i32>;

	template<typename T>
	struct Vector<3, T>
	{
		T x = T();
		T y = T();
		T z = T();

		[[nodiscard]] constexpr Vector<2, T> AsVec2() const;
		[[nodiscard]] constexpr Vector<4, T> AsVec4(T const& wValue = T()) const;

		[[nodiscard]] f32 Magnitude() const;
		[[nodiscard]] f32 MagnitudeSqrd() const;
		[[nodiscard]] Vector<3, T> Normalized() const;

		[[nodiscard]] constexpr T* Data();
		[[nodiscard]] constexpr T const* Data() const;

		[[nodiscard]] static constexpr Vector<3, T> Cross(Vector<3, T> const& lhs, Vector<3, T> const& rhs);
		[[nodiscard]] static constexpr T Dot(Vector<3, T> const& lhs, Vector<3, T> const& rhs);

		[[nodiscard]] static constexpr Vector<3, T> SingleValue(T const& input);
		[[nodiscard]] static constexpr Vector<3, T> Zero();
		[[nodiscard]] static constexpr Vector<3, T> One();
		[[nodiscard]] static constexpr Vector<3, T> Up();
		[[nodiscard]] static constexpr Vector<3, T> Down();
		[[nodiscard]] static constexpr Vector<3, T> Left();
		[[nodiscard]] static constexpr Vector<3, T> Right();
		[[nodiscard]] static constexpr Vector<3, T> Forward();
		[[nodiscard]] static constexpr Vector<3, T> Back();

		constexpr Vector<3, T>& operator+=(Vector<3, T> const& rhs);
		constexpr Vector<3, T>& operator-=(Vector<3, T> const& rhs);
		constexpr Vector<3, T>& operator*=(T const& rhs);
		[[nodiscard]] constexpr Vector<3, T> operator+(Vector<3, T> const& rhs) const;
		[[nodiscard]] constexpr Vector<3, T> operator-(Vector<3, T> const& rhs) const;
		[[nodiscard]] constexpr Vector<3, T> operator-() const;
		[[nodiscard]] constexpr bool operator==(Vector<3, T> const& rhs) const;
		[[nodiscard]] constexpr bool operator!=(Vector<3, T> const& rhs) const;
	};

	template<typename T>
	constexpr Vector<2, T> Vector<3, T>::AsVec2() const
	{
		return Vector<2, T>{ x, y };
	}

	template<typename T>
	constexpr Vector<4, T> Vector<3, T>::AsVec4(T const& wValue) const
	{
		return Vector<4, T>{ x, y, z, wValue };
	}

	template<typename T>
	f32 Vector<3, T>::Magnitude() const
	{
		return Sqrt(MagnitudeSqrd());
	}

	template<typename T>
	f32 Vector<3, T>::MagnitudeSqrd() const
	{
		return x*x + y*y + z*z;
	}

	template<typename T>
	Vector<3, T> Vector<3, T>::Normalized() const
	{
		f32 mag = Magnitude();
		return Vector<3, T>{ x / mag, y / mag, z / mag };
	}

	template<typename T>
	constexpr T* Vector<3, T>::Data()
	{
		return &x;
	}

	template<typename T>
	constexpr T const* Vector<3, T>::Data() const
	{
		return &x;
	}

	template<typename T>
	constexpr Vector<3, T> Vector<3, T>::Cross(Vector<3, T> const& lhs, Vector<3, T> const& rhs)
	{
		return { lhs.y * rhs.z - lhs.z * rhs.y, lhs.z * rhs.x - lhs.x * rhs.z, lhs.x * rhs.y - lhs.y * rhs.x };
	}

	template<typename T>
	constexpr T Vector<3, T>::Dot(Vector<3, T> const& lhs, Vector<3, T> const& rhs)
	{
		return (lhs.x * rhs.x) + (lhs.y * rhs.y) + (lhs.z * rhs.z);
	}

	template<typename T>
	constexpr Vector<3, T> Vector<3, T>::SingleValue(T const& input)
	{
		return Vector<3, T>{ input, input, input };
	}
	template<typename T>
	constexpr Vector<3, T> Vector<3, T>::Zero()
	{
		return Vector<3, T>{ };
	}
	template<typename T>
	constexpr Vector<3, T> Vector<3, T>::One()
	{
		return Vector<3, T>{ T(1), T(1), T(1) };
	}
	template<typename T>
	constexpr Vector<3, T> Vector<3, T>::Up()
	{
		return Vector<3, T>{ T(0), T(1), T(0) };
	}
	template<typename T>
	constexpr Vector<3, T> Vector<3, T>::Down()
	{
		return Vector<3, T>{ T(0), T(-1), T(0) };
	}
	template<typename T>
	constexpr Vector<3, T> Vector<3, T>::Left()
	{
		return Vector<3, T>{ T(-1), T(1), T(0) };
	}
	template<typename T>
	constexpr Vector<3, T> Vector<3, T>::Right()
	{
		return Vector<3, T>{ T(1), T(0), T(0) };
	}
	template<typename T>
	constexpr Vector<3, T> Vector<3, T>::Forward()
	{
		return Vector<3, T>{ T(0), T(0), T(1) };
	}
	template<typename T>
	constexpr Vector<3, T> Vector<3, T>::Back()
	{
		return Vector<3, T>{ T(0), T(0), T(-1) };
	}

	template<typename T>
	constexpr Vector<3, T>& Vector<3, T>::operator+=(Vector<3, T> const& rhs)
	{
		x += rhs.x;
		y += rhs.y;
		z += rhs.z;
		return *this;
	}
	template<typename T>
	constexpr Vector<3, T>& Vector<3, T>::operator-=(Vector<3, T> const& rhs)
	{
		x -= rhs.x;
		y -= rhs.y;
		z -= rhs.z;
		return *this;
	}
	template<typename T>
	constexpr Vector<3, T>& Vector<3, T>::operator*=(T const& rhs)
	{
		x *= rhs;
		y *= rhs;
		z *= rhs;
		return *this;
	}
	template<typename T>
	[[nodiscard]] constexpr Vector<3, T> Vector<3, T>::operator+(Vector<3, T> const& rhs) const
	{
		return Vector<3, T>{ x + rhs.x, y + rhs.y, z + rhs.z };
	}
	template<typename T>
	[[nodiscard]] constexpr Vector<3, T> Vector<3, T>::operator-(Vector<3, T> const& rhs) const
	{
		return Vector<3, T>{ x - rhs.x, y - rhs.y, z - rhs.z };
	}
	template<typename T>
	[[nodiscard]] constexpr Vector<3, T> Vector<3, T>::operator-() const
	{
		return Vector<3, T>{ -x, -y, -z };
	}
	template<typename T>
	[[nodiscard]] constexpr bool Vector<3, T>::operator==(Vector<3, T> const& rhs) const
	{
		return x == rhs.x && y == rhs.y && z == rhs.z;
	}
	template<typename T>
	[[nodiscard]] constexpr bool Vector<3, T>::operator!=(Vector<3, T> const& rhs) const
	{
		return x != rhs.x || y != rhs.y || z != rhs.z;
	}

	template<typename T>
	constexpr Vector<3, T> operator*(Vector<3, T> const& lhs, T const& rhs)
	{
		return Vector<3, T>{ lhs.x * rhs, lhs.y * rhs, lhs.z * rhs };
	}

	template<typename T>
	constexpr Vector<3, T> operator*(T const& lhs, Vector<3, T> const& rhs)
	{
		return Vector<3, T>{ lhs * rhs.x, lhs * rhs.y, lhs * rhs.z };
	}
}