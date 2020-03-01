#pragma once

#include "DEngine/FixedWidthTypes.hpp"

#include "DEngine/Math/Enums.hpp"

namespace DEngine::Math
{
	template<uSize length, typename T>
	struct Vector;

	using Vec3D = Vector<3, float>;
	using Vec3DInt = Vector<3, int32_t>;

	template<typename T>
	struct Vector<3, T>
	{
		using ValueType = T;
		static constexpr uSize dimCount = 3;

		T x;
		T y;
		T z;

		[[nodiscard]] constexpr Vector<2, T> AsVec2() const
		{
			return Vector<2, T>{ x, y };
		}
		[[nodiscard]] constexpr Vector<4, T> AsVec4(const T& wValue = T()) const
		{
			return Vector<4, T>{x, y, z, wValue };
		}

		[[nodiscard]] constexpr T& At(size_t index) 
		{ 
			return const_cast<T&>(std::as_const(*this).At(index));
		}

		[[nodiscard]] constexpr const T& At(size_t index) const
		{
#if defined( _MSC_VER )
			__assume(index < dimCount);
#endif
			assert(index < dimCount);
			switch (index)
			{
			case 0:
				return x;
			case 1:
				return y;
			case 2:
				return z;
			default:
#if defined( _MSC_VER )
				__assume(0);
#elif defined( __GNUC__ )
				__builtin_unreachable();
#endif
			}
		}

		[[nodiscard]] static constexpr Vector<3, T> Cross(const Vector<3, T>& lhs, const Vector<3, T>& rhs)
		{
			return { lhs.y * rhs.z - lhs.z * rhs.y, lhs.z * rhs.x - lhs.x * rhs.z, lhs.x * rhs.y - lhs.y * rhs.x };
		}

		[[nodiscard]] static constexpr T Dot(const Vector<3, T>& lhs, const Vector<3, T>& rhs)
		{
			return (lhs.x * rhs.x) + (lhs.y * rhs.y) + (lhs.z * rhs.z);
		}

		[[nodiscard]] constexpr T* GetData()
		{
			return &x;
		}

		[[nodiscard]] constexpr const T* GetData() const
		{
			return &x;
		}

		[[nodiscard]] auto GetNormalized() const -> Vector<3, typename std::conditional<std::is_integral<T>::value, float, T>::type>
		{
			using ReturnValueType = typename std::conditional<std::is_integral<T>::value, float, T>::type;
			const auto& magnitude = Magnitude();
			return Vector<3, ReturnValueType>{ x / magnitude, y / magnitude, z / magnitude };
		}

		[[nodiscard]] Vector<3, T> GetRotated(ElementaryAxis axis, float degrees) const
		{
#if defined( _MSC_VER )
			__assume(axis == ElementaryAxis::X || axis == ElementaryAxis::Y || axis == ElementaryAxis::Z);
#endif
			assert(axis == ElementaryAxis::X || axis == ElementaryAxis::Y || axis == ElementaryAxis::Z);
			const float cos = Cos<AngleUnit::Degrees>(degrees);
			const float sin = Sin<AngleUnit::Degrees>(degrees);
			switch (axis)
			{
			case ElementaryAxis::X:
				return { x, y * cos - z * sin, y * sin + z * cos };
			case ElementaryAxis::Y:
				return { x * cos - z * sin, y, x * sin + z * cos };
			case ElementaryAxis::Z:
				return { x * cos - y * sin, x * sin + y * cos, z };
			default:
#if defined( _MSC_VER )
				__assume(0);
#elif defined( __GNUC__ )
				__builtin_unreachable();
#endif
			}
		}

		[[nodiscard]] auto Magnitude() const -> typename std::conditional<std::is_integral<T>::value, float, T>::type
		{
			if constexpr (std::is_integral<T>::value)
				return Sqrt(float((x * x) + (y * y) + (z * z)));
			else
				return Sqrt((x * x) + (y * y) + (z * z));
		}

		[[nodiscard]] constexpr T MagnitudeSqrd() const
		{
			return (x * x) + (y * y) + (z * z);
		}

		void Normalize()
		{
			static_assert(std::is_floating_point<T>::value, "Cannot normalize an integral vector.");
			const auto& magnitude = Magnitude();
			x /= magnitude;
			y /= magnitude;
			z /= magnitude;
		}

		[[nodiscard]] std::string ToString() const
		{
			std::ostringstream stream;

			if constexpr (std::is_floating_point<T>::value)
			{
				stream.flags(std::ios::fixed);
				stream.precision(4);
			}

			stream << '(';
			if constexpr (std::is_same<char, T>::value || std::is_same<unsigned char, T>::value)
				stream << +x << ", " << +y << ", " << +z;
			else
				stream << x << ", " << y << ", " << z;
			stream << ')';
			return stream.str();
		}

		[[nodiscard]] static constexpr Vector<3, T> SingleValue(const T& input)
		{
			return Vector<3, T>{ input, input, input };
		}
		[[nodiscard]] static constexpr Vector<3, T> Zero()
		{
			return Vector<3, T>{ };
		}
		[[nodiscard]] static constexpr Vector<3, T> One()
		{
			return Vector<3, T>{ T(1), T(1), T(1) };
		}
		[[nodiscard]] static constexpr Vector<3, T> Up()
		{
			return Vector<3, T>{ T(0), T(1), T(0) };
		}
		[[nodiscard]] static constexpr Vector<3, T> Down()
		{
			return Vector<3, T>{ T(0), T(-1), T(0) };
		}
		[[nodiscard]] static constexpr Vector<3, T> Left()
		{
			return Vector<3, T>{ T(-1), T(1), T(0) };
		}
		[[nodiscard]] static constexpr Vector<3, T> Right()
		{
			return Vector<3, T>{ T(1), T(0), T(0) };
		}
		[[nodiscard]] static constexpr Vector<3, T> Forward()
		{
			return Vector<3, T>{ T(0), T(0), T(1) };
		}
		[[nodiscard]] static constexpr Vector<3, T> Back()
		{
			return Vector<3, T>{ T(0), T(0), T(-1) };
		}

		constexpr Vector<3, T>& operator+=(const Vector<3, T>& rhs)
		{
			x += rhs.x;
			y += rhs.y;
			z += rhs.z;
			return *this;
		}
		constexpr Vector<3, T>& operator-=(const Vector<3, T>& rhs)
		{
			x -= rhs.x;
			y -= rhs.y;
			z -= rhs.z;
			return *this;
		}
		constexpr Vector<3, T>& operator*=(const T& rhs)
		{
			x *= rhs;
			y *= rhs;
			z *= rhs;
			return *this;
		}
		[[nodiscard]] constexpr Vector<3, T> operator+(const Vector<3, T>& rhs) const
		{
			return Vector<3, T>{ x + rhs.x, y + rhs.y, z + rhs.z };
		}
		[[nodiscard]] constexpr Vector<3, T> operator-(const Vector<3, T>& rhs) const
		{
			return Vector<3, T>{ x - rhs.x, y - rhs.y, z - rhs.z };
		}
		[[nodiscard]] constexpr Vector<3, T> operator-() const
		{
			return Vector<3, T>{ -x, -y, -z };
		}
		[[nodiscard]] constexpr bool operator==(const Vector<3, T>& rhs) const
		{
			return x == rhs.x && y == rhs.y && z == rhs.z;
		}
		[[nodiscard]] constexpr bool operator!=(const Vector<3, T>& rhs) const
		{
			return x != rhs.x || y != rhs.y || z != rhs.z;
		}
		[[nodiscard]] constexpr T& operator[](size_t index) 
		{
			return const_cast<T&>(std::as_const(*this)[index]);
		}
		[[nodiscard]] constexpr const T& operator[](size_t index) const
		{
#if defined( _MSC_VER )
			__assume(index < dimCount);
#endif
			assert(index < dimCount);
			switch (index)
			{
			case 0:
				return x;
			case 1:
				return y;
			case 2:
				return z;
			default:
#if defined( _MSC_VER )
				__assume(0);
#elif defined( __GNUC__ )
				__builtin_unreachable();
#endif
			}
		}
		template<typename U>
		[[nodiscard]] constexpr explicit operator Vector<3, U>() const
		{
			static_assert(std::is_convertible<T, U>::value, "Can't convert to this type.");
			return Vector<3, U>{ static_cast<U>(x), static_cast<U>(y), static_cast<U>(z) };
		}
	};

	template<typename T>
	constexpr Vector<3, T> operator*(const Vector<3, T>& lhs, const T& rhs)
	{
		return Vector<3, T>{ lhs.x * rhs, lhs.y * rhs, lhs.z * rhs };
	}

	template<typename T>
	constexpr Vector<3, T> operator*(const T& lhs, const Vector<2, T>& rhs)
	{
		return Vector<3, T>{ lhs * rhs.x, lhs * rhs.y, lhs * rhs.z };
	}
}