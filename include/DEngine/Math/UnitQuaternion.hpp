#pragma once

#include <cassert>
#include "Matrix/Matrix.hpp"
#include "Vector/Vector3D.hpp"
#include "Common.hpp"
#include "Trigonometric.hpp"

namespace Math
{
	template<size_t width, size_t height, typename T>
	struct Matrix;

	template<typename T = float>
	class UnitQuaternion
	{
	public:
		using ValueType = T;

		constexpr UnitQuaternion() noexcept;
		inline UnitQuaternion(const Vector<3, T>& axis, const T& degrees);
		inline UnitQuaternion(const Vector<3, T>& eulerAngles);

		constexpr T GetS() const;
		constexpr T GetX() const;
		constexpr T GetY() const;
		constexpr T GetZ() const;
		constexpr const T& operator[](size_t index) const;

		constexpr UnitQuaternion<T> operator*(const UnitQuaternion<T>& right) const;
		explicit constexpr operator Matrix<4, 3, T>() const;
		explicit constexpr operator Matrix<4, 4, T>() const;
		

		constexpr UnitQuaternion<T> GetConjugate() const;

		constexpr UnitQuaternion<T> GetInverse() const;

	private:
		constexpr UnitQuaternion(const T& s, const T& x, const T& y, const T& z) noexcept;

		T s;
		T x;
		T y;
		T z;

		static_assert(std::is_floating_point_v<T>, "Error. Math::RotationQuaternion must be floating point type.");
	};
	static_assert(sizeof(UnitQuaternion<float>) == sizeof(float) * 4, "Error. Math::UnitQuaternion's members must be tightly packed.");

	template<typename T>
	inline constexpr UnitQuaternion<T>::UnitQuaternion() noexcept :
		s(T(1)), x(), y(), z() {}

	template<typename T>
	constexpr UnitQuaternion<T>::UnitQuaternion(const T& s, const T& x, const T& y, const T& z) noexcept :
		s(s), x(x), y(y), z(z) {}

	template<typename T>
	inline UnitQuaternion<T>::UnitQuaternion(const Vector<3, T>& axis, const T& degrees)
	{
		s = Cos<AngleUnit::Degrees>(degrees / 2);

		assert(axis.Magnitude() > 0.f);
		Math::Vector3D normalizedAxis = axis.GetNormalized();
		const T sin = Sin<AngleUnit::Degrees>(degrees / 2);
		x = normalizedAxis.x * sin;
		y = normalizedAxis.y * sin;
		z = normalizedAxis.z * sin;
	}

	template<typename T>
	inline UnitQuaternion<T>::UnitQuaternion(const Vector<3, T>& eulerAngles)
	{
		T c1 = Cos<AngleUnit::Degrees>(eulerAngles.y / 2);
		T c2 = Cos<AngleUnit::Degrees>(eulerAngles.x / 2);
		T c3 = Cos<AngleUnit::Degrees>(eulerAngles.z / 2);
		T s1 = Sin<AngleUnit::Degrees>(eulerAngles.y / 2);
		T s2 = Sin<AngleUnit::Degrees>(eulerAngles.x / 2);
		T s3 = Sin<AngleUnit::Degrees>(eulerAngles.z / 2);

		s = c1*c2*c3 - s1*s2*s3;
		y = s1*c2*c3 + c1*s2*s3;
		x = c1*s2*c3 - c1*s2*s3;
		z = s1 * s2 * c3 + c1 * c2 * s3;
	}

	template<typename T>
	constexpr T UnitQuaternion<T>::GetS() const { return s; }

	template<typename T>
	constexpr T UnitQuaternion<T>::GetX() const { return x; }

	template<typename T>
	constexpr T UnitQuaternion<T>::GetY() const { return y; }

	template<typename T>
	constexpr T UnitQuaternion<T>::GetZ() const { return z; }

	template<typename T>
	constexpr const T& UnitQuaternion<T>::operator[](size_t index) const
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
			assert(false);
			return s;
		}
	}

	template<typename T>
	constexpr UnitQuaternion<T> UnitQuaternion<T>::operator*(const UnitQuaternion<T>& right) const
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
	constexpr UnitQuaternion<T>::operator Matrix<4, 3, T>() const
	{
		return Matrix<4, 3, T>
		{
			1 - 2 * Sqrd(y) - 2 * Sqrd(z), 2 * x*y + 2 * z*s, 2 * x*z - 2 * y*s,
			2 * x*y - 2 * z*s, 1 - 2 * Sqrd(x) - 2 * Sqrd(z), 2 * y*z + 2 * x*s,
			2 * x*z + 2 * y*s, 2 * y*z - 2 * x*s, 1 - 2 * Sqrd(x) - 2 * Sqrd(y),
			0, 0, 0,
		};
	}

	template<typename T>
	constexpr UnitQuaternion<T>::operator Matrix<4, 4, T>() const
	{
		return Matrix<4, 4, T>
		{
			1 - 2*Sqrd(y) - 2*Sqrd(z), 2*x*y + 2*z*s, 2*x*z - 2*y*s, 0,
			2*x*y - 2*z*s, 1 - 2*Sqrd(x) - 2*Sqrd(z), 2*y*z + 2*x*s, 0,
			2*x*z + 2*y*s, 2*y*z - 2*x*s, 1 - 2*Sqrd(x) - 2*Sqrd(y), 0,
			0, 0, 0, 1
		};
	}

	template<typename T>
	constexpr UnitQuaternion<T> UnitQuaternion<T>::GetConjugate() const { return GetInverse(); }

	template<typename T>
	constexpr UnitQuaternion<T> UnitQuaternion<T>::GetInverse() const { return UnitQuaternion{ s ,-x, -y, -z }; }
}