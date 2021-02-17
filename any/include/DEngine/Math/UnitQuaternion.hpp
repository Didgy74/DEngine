#pragma once

#include <DEngine/FixedWidthTypes.hpp>
#include <DEngine/Math/Constant.hpp>
#include <DEngine/Math/Trigonometric.hpp>
#include <DEngine/Math/Vector.hpp>

namespace DEngine::Math
{
	template<typename T = f32>
	class UnitQuaternion
	{
	public:
		constexpr UnitQuaternion();

		[[nodiscard]] constexpr T const& S() const;
		[[nodiscard]] constexpr T const& X() const;
		[[nodiscard]] constexpr T const& Y() const;
		[[nodiscard]] constexpr T const& Z() const;
		[[nodiscard]] constexpr T const& operator[](uSize index) const;

		[[nodiscard]] constexpr UnitQuaternion<T> operator*(UnitQuaternion<T> const& right) const;

		[[nodiscard]] constexpr UnitQuaternion<T> GetConjugate() const;
		[[nodiscard]] constexpr UnitQuaternion<T> GetInverse() const;

		[[nodiscard]] static constexpr UnitQuaternion<T> FromEulerAngles(T x, T y, T z);
		[[nodiscard]] static constexpr UnitQuaternion<T> FromVector(Vector<3, T> const& axis, f32 radians);

	private:
		constexpr UnitQuaternion(T s, T x, T y, T z);

		T s = T();
		T x = T();
		T y = T();
		T z = T();
	};

	using UnitQuat = UnitQuaternion<f32>;

	template<typename T>
	inline constexpr UnitQuaternion<T>::UnitQuaternion() :
		s(T(1)), x(), y(), z() {}

	template<typename T>
	constexpr UnitQuaternion<T>::UnitQuaternion(T s, T x, T y, T z) :
		s(s), x(x), y(y), z(z) {}

	template<typename T>
	constexpr T const& UnitQuaternion<T>::S() const { return s; }

	template<typename T>
	constexpr T const& UnitQuaternion<T>::X() const { return x; }

	template<typename T>
	constexpr T const& UnitQuaternion<T>::Y() const { return y; }

	template<typename T>
	constexpr T const& UnitQuaternion<T>::Z() const { return z; }

	template<typename T>
	constexpr T const& UnitQuaternion<T>::operator[](uSize index) const
	{
		switch (index)
		{
		case 0:
			return s;
		case 1:
			return x;
		case 2:
			return y;
		case 3:
			return z;
		default:
			return s;
		}
	}

	template<typename T>
	constexpr UnitQuaternion<T> UnitQuaternion<T>::operator*(UnitQuaternion<T> const& right) const
	{
		return UnitQuaternion<T>
		{
			s * right.s - x * right.x - y * right.y - z * right.z,
			s * right.x + right.s * x + y * right.z - right.y * z,
			s * right.y + right.s * y + z * right.x - right.z * x,
			s * right.z + right.s * z + x * right.y - right.x * y
		};
	}

	template<typename T>
	constexpr UnitQuaternion<T> UnitQuaternion<T>::GetConjugate() const { return GetInverse(); }

	template<typename T>
	constexpr UnitQuaternion<T> UnitQuaternion<T>::GetInverse() const { return UnitQuaternion{ s ,-x, -y, -z }; }

	template<typename T>
	constexpr UnitQuaternion<T> UnitQuaternion<T>::FromEulerAngles(T x, T y, T z)
	{
		T c1 = Cos(y * degToRad / 2.f);
		T c2 = Cos(x * degToRad / 2.f);
		T c3 = Cos(z * degToRad / 2.f);
		T s1 = Sin(y * degToRad / 2.f);
		T s2 = Sin(x * degToRad / 2.f);
		T s3 = Sin(z * degToRad / 2.f);

		UnitQuaternion<T> returnVal{};
		returnVal.s = c1 * c2 * c3 - s1 * s2 * s3;
		returnVal.y = s1 * c2 * c3 + c1 * s2 * s3;
		returnVal.x = c1 * s2 * c3 - c1 * s2 * s3;
		returnVal.z = s1 * s2 * c3 + c1 * c2 * s3;
		return returnVal;
	}

	template<typename T>
	constexpr UnitQuaternion<T> UnitQuaternion<T>::FromVector(Vector<3, T> const& axis, f32 radians)
	{
		UnitQuaternion<T> returnVal{};

		returnVal.s = Cos(radians / 2);

		//assert(axis.Magnitude() > 0.f);
		Vector<3, T> const normalizedAxis = axis.GetNormalized();
		T const sin = Sin(radians / 2);
		returnVal.x = normalizedAxis.x * sin;
		returnVal.y = normalizedAxis.y * sin;
		returnVal.z = normalizedAxis.z * sin;

		return returnVal;
	}
}