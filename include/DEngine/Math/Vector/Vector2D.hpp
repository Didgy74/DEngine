#pragma once

#include "DEngine/FixedWidthTypes.hpp"

#include "DEngine/Math/Common.hpp"

#include <type_traits>

namespace DEngine::Math
{
	template<uSize length, typename T>
	struct Vector;

	using Vec2D = Vector<2, float>;
	using Vec2DInt = Vector<2, i32>;

	template<typename T>
	struct Vector<2, T>
	{
		using ValueType = T;
		static constexpr uSize dimCount = 2;

		T x = {};
		T y = {};

		[[nodiscard]] constexpr Vector<3, T> AsVec3(const T& zValue = T()) const
		{
			return Vector<3, T>{ x, y, zValue };
		}

		[[nodiscard]] constexpr Vector<4, T> AsVec4(const T& zValue = T(), const T& wValue = T()) const
		{
			return Vector<4, T>{ x, y, zValue, wValue };
		}

		[[nodiscard]] constexpr T& At(uSize index)
		{
			return const_cast<T&>(std::as_const(*this).At(index)); 
		}

		[[nodiscard]] constexpr const T& At(uSize index) const
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
			default:
#if defined( _MSC_VER )
				__assume(0);
#endif
			}
		}

		[[nodiscard]] static constexpr T Dot(const Vector<2, T>& lhs, const Vector<2, T>& rhs)
		{
			return (lhs.x * rhs.x) + (lhs.y * rhs.y);
		}

		[[nodiscard]] constexpr T* GetData()
		{
			return &x;
		}

		[[nodiscard]] constexpr const T* GetData() const
		{
			return &x;
		}

		[[nodiscard]] auto GetNormalized() const -> Vector<2, typename std::conditional<std::is_integral<T>::value, float, T>::type>
		{
			using ReturnValueType = typename std::conditional<std::is_integral<T>::value, float, T>::type;
			const auto& magnitude = Magnitude();
			return Vector<2, ReturnValueType>{ x / magnitude, y / magnitude };
		}

		[[nodiscard]] auto Magnitude() const -> typename std::conditional<std::is_integral<T>::value, float, T>::type
		{
			if constexpr (std::is_integral<T>::value)
				return Sqrt(float((x * x) + (y * y)));
			else
				return Sqrt((x * x) + (y * y));
		}

		[[nodiscard]] constexpr T MagnitudeSqrd() const
		{
			return (x * x) + (y * y);
		}

		void Normalize()
		{
			static_assert(std::is_floating_point<T>::value, "Cannot normalize an integral vector.");
			const auto& magnitude = Magnitude();
			x /= magnitude;
			y /= magnitude;
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
				stream << +x << ", " << +y;
			else
				stream << x << ", " << y;
			stream << ')';
			return stream.str();
		}

		[[nodiscard]] static constexpr Vector<2, T> SingleValue(const T& input)
		{
			return Vector<2, T>{ input, input };
		}
		[[nodiscard]] static constexpr Vector<2, T> Zero()
		{
			return Vector<2, T>{};
		}
		[[nodiscard]] static constexpr Vector<2, T> One()
		{
			return Vector<2, T>{ T(1), T(1) };
		}
		[[nodiscard]] static constexpr Vector<2, T> Up()
		{
			return Vector<2, T>{T(0), T(1)};
		}
		[[nodiscard]] static constexpr Vector<2, T> Down()
		{
			return Vector<2, T>{T(0), T(-1)};
		}
		[[nodiscard]] static constexpr Vector<2, T> Left()
		{
			return Vector<2, T>{T(-1), T(0)};
		}
		[[nodiscard]] static constexpr Vector<2, T> Right()
		{
			return Vector<2, T>{T(1), T(0)};
		}

		constexpr Vector<2, T>& operator+=(const Vector<2, T>& rhs)
		{
			x += rhs.x;
			y += rhs.y;
			return *this;
		}
		constexpr Vector<2, T>& operator-=(const Vector<2, T>& rhs)
		{
			x -= rhs.x;
			y -= rhs.y;
			return *this;
		}
		constexpr Vector<2, T>& operator*=(const T& rhs)
		{
			x *= rhs;
			y *= rhs;
			return *this;
		}
		[[nodiscard]] constexpr Vector<2, T> operator+(const Vector<2, T>& rhs) const
		{
			return Vector<2, T>{ x + rhs.x, y + rhs.y };
		}
		[[nodiscard]] constexpr Vector<2, T> operator-(const Vector<2, T>& rhs) const
		{
			return Vector<2, T>{ x - rhs.x, y - rhs.y };
		}
		[[nodiscard]] constexpr Vector<2, T> operator-() const
		{
			return Vector<2, T>{ -x, -y };
		}
		[[nodiscard]] constexpr bool operator==(const Vector<2, T>& rhs) const
		{
			return x == rhs.x && y == rhs.y;
		}
		[[nodiscard]] constexpr bool operator!=(const Vector<2, T>& rhs) const
		{
			return x != rhs.x || y != rhs.y;
		}
		[[nodiscard]] constexpr T& operator[](size_t index) { return const_cast<T&>(std::as_const(*this)[index]); }
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
			default:
#if defined( _MSC_VER )
				__assume(0);
#elif defined( __GNUC__ )
				__builtin_unreachable();
#endif
			}
		}
		template<typename U>
		[[nodiscard]] constexpr explicit operator Vector<2, U>() const
		{
			static_assert(std::is_convertible<T, U>::value, "Can't convert to this type.");

			return Vector<2, U>{static_cast<U>(x), static_cast<U>(y)};
		}
	};

	template<typename T>
	constexpr Vector<2, T> operator*(const Vector<2, T>& lhs, const T& rhs)
	{
		return Vector<2, T>{ lhs.x * rhs, lhs.y * rhs };
	}

	template<typename T>
	constexpr Vector<2, T> operator*(const T& lhs, const Vector<2, T>& rhs)
	{
		return Vector<2, T>{ lhs * rhs.x, lhs * rhs.y };
	}
}