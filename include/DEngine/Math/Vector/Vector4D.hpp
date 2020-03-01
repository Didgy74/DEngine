#pragma once

#include "DEngine/FixedWidthTypes.hpp"

namespace DEngine::Math
{
	template<uSize length, typename T>
	struct Vector;

	using Vector4D = Vector<4, float>;
	using Vector4DInt = Vector<4, i32>;

	template<typename T>
	struct Vector<4, T>
	{
		using ValueType = T;
		static constexpr uSize dimCount = 4;

		T x;
		T y;
		T z;
		T w;

		[[nodiscard]] constexpr Vector<2, T> AsVec2() const
		{
			return Vector<2, T>{ x, y };
		}
		[[nodiscard]] constexpr Vector<3, T> AsVec3() const
		{
			return Vector<3, T>{ x, y, z };
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
			case 3:
				return w;
			default:
#if defined( _MSC_VER )
				__assume(0);
#elif defined( __GNUC__ )
				__builtin_unreachable();
#endif
			}
		}

		[[nodiscard]] static constexpr T Dot(const Vector<4, T>& lhs, const Vector<4, T>& rhs)
		{
			return (lhs.x * rhs.x) + (lhs.y * rhs.y) + (lhs.z * rhs.z) + (lhs.w * rhs.w);
		}

		[[nodiscard]] constexpr T* GetData()
		{
			return &x;
		}

		[[nodiscard]] constexpr const T* GetData() const
		{
			return &x;
		}

		[[nodiscard]] auto GetNormalized() const -> Vector<4, typename std::conditional<std::is_integral<T>::value, float, T>::type>
		{
			using ReturnValueType = typename std::conditional<std::is_integral<T>::value, float, T>::type;
			const auto& magnitude = Magnitude();
			return Vector<3, ReturnValueType>{ x / magnitude, y / magnitude, z / magnitude, w / magnitude };
		}

		[[nodiscard]] auto Magnitude() const -> typename std::conditional<std::is_integral<T>::value, float, T>::type
		{
			if constexpr (std::is_integral<T>::value)
				return Sqrt(float((x * x) + (y * y) + (z * z) + (w * w)));
			else
				return Sqrt((x * x) + (y * y) + (z * z) + (w * w));
		}

		[[nodiscard]] constexpr T MagnitudeSqrd() const
		{
			return (x * x) + (y * y) + (z * z) + (w * w);
		}

		void Normalize()
		{
			static_assert(std::is_floating_point<T>::value, "Cannot normalize an integral vector.");
			const auto& magnitude = Magnitude();
			x /= magnitude;
			y /= magnitude;
			z /= magnitude;
			w /= magnitude;
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
				stream << +x << ", " << +y << ", " << +z << ", " << +w;
			else
				stream << x << ", " << y << ", " << z << ", " << w;
			stream << ')';
			return stream.str();
		}

		[[nodiscard]] static constexpr Vector<4, T> SingleValue(const T& input)
		{
			return Vector<4, T>{ input, input, input, input };
		}
		[[nodiscard]] static constexpr Vector<4, T> Zero()
		{
			return Vector<4, T>{ };
		}
		[[nodiscard]] static constexpr Vector<4, T> One()
		{
			return Vector<4, T>{ T(1), T(1), T(1), T(1) };
		}

		constexpr Vector<4, T>& operator+=(const Vector<4, T>& rhs)
		{
			x += rhs.x;
			y += rhs.y;
			z += rhs.z;
			w += rhs.w;
			return *this;
		}
		constexpr Vector<4, T>& operator-=(const Vector<4, T>& rhs)
		{
			x -= rhs.x;
			y -= rhs.y;
			z -= rhs.z;
			w -= rhs.w;
			return *this;
		}
		constexpr Vector<4, T>& operator*=(const T& rhs)
		{
			x *= rhs;
			y *= rhs;
			z *= rhs;
			return *this;
		}
		[[nodiscard]] constexpr Vector<4, T> operator+(const Vector<4, T>& rhs) const
		{
			return Vector<4, T>{ x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w };
		}
		[[nodiscard]] constexpr Vector<4, T> operator-(const Vector<4, T>& rhs) const
		{
			return Vector<4, T>{ x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w };
		}
		[[nodiscard]] constexpr Vector<4, T> operator-() const
		{
			return Vector<4, T>{ -x, -y, -z, -w };
		}
		[[nodiscard]] constexpr bool operator==(const Vector<4, T>& rhs) const
		{
			return x == rhs.x && y == rhs.y && z == rhs.z && rhs.w;
		}
		[[nodiscard]] constexpr bool operator!=(const Vector<4, T>& rhs) const
		{
			return x != rhs.x || y != rhs.y || z != rhs.z || w != rhs.w;
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
		[[nodiscard]] constexpr explicit operator Vector<4, U>() const
		{
			static_assert(std::is_convertible<T, U>::value, "Can't convert to this type.");
			return Vector<4, U>{ static_cast<U>(x), static_cast<U>(y), static_cast<U>(z), static_cast<U>(w) };
		}
	};

	template<typename T>
	constexpr Vector<4, T> operator*(const Vector<4, T>& lhs, const T& rhs)
	{
		return Vector<4, T>{ lhs.x * rhs, lhs.y * rhs, lhs.z * rhs, lhs.w * rhs };
	}

	template<typename T>
	constexpr Vector<4, T> operator*(const T& lhs, const Vector<4, T>& rhs)
	{
		return Vector<4, T>{ lhs * rhs.x, lhs * rhs.y, lhs * rhs.z, lhs * rhs.w };
	}
}