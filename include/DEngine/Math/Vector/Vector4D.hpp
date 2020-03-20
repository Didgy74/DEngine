#pragma once

#include "DEngine/FixedWidthTypes.hpp"

namespace DEngine::Math
{
	template<uSize length, typename T>
	struct Vector;

	using Vector4D = Vector<4, f32>;
	using Vector4DInt = Vector<4, i32>;

	template<typename T>
	struct Vector<4, T>
	{
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

		[[nodiscard]] constexpr T& At(uSize index)
		{
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
				return *((T*)0);
			}
		}

		[[nodiscard]] constexpr T const& At(uSize index) const
		{
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
				return *((T const*)0);
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
				return *((T*)0);
			}
		}
		[[nodiscard]] constexpr const T& operator[](size_t index) const
		{
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
				return *((T*)0);
			}
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