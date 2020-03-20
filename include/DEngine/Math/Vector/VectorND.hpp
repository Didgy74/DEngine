#pragma once

#include "DEngine/FixedWidthTypes.hpp"

namespace DEngine::Math
{
	template<uSize length, typename T = f32>
	struct Vector
	{
		T data[length];

		[[nodiscard]] constexpr T& At(uSize index)
		{
			return data[index];
		}

		[[nodiscard]] constexpr const T& At(uSize index) const
		{
			return data[index];
		}

		[[nodiscard]] static constexpr T Dot(const Vector<length, T>& lhs, const Vector<length, T>& rhs)
		{
			T sum = T(0);
			for (uSize i = 0; i < length; i++)
				sum += lhs[i] * rhs[i];
			return sum;
		}

		[[nodiscard]] static constexpr Vector<length, T> SingleValue(const T& input)
		{
			Vector<length, T> temp;
			for (uSize i = 0; i < length; i++)
				temp.data[i] = input;
			return temp;
		}

		[[nodiscard]] static constexpr Vector<length, T> Zero()
		{
			return Vector<length, T>{};
		}
		[[nodiscard]] static constexpr Vector<length, T> One()
		{
			Vector<length, T> temp;
			for (uSize i = 0; i < length; i++)
				temp.data[i] = T(1);
			return temp;
		}

		constexpr Vector<length, T>& operator+=(const Vector<length, T>& rhs)
		{
			for (uSize i = 0; i < length; i++)
				data[i] += rhs[i];
			return *this;
		}
	};

	template<size_t length, typename T>
	constexpr Vector<length, T> operator*(const Vector<length, T>& lhs, const T& rhs)
	{
		Vector<length, T> returnValue;
		for (size_t i = 0; i < length; i++)
			returnValue[i] = lhs.Data[i] * rhs;
		return returnValue;
	}

	template<size_t length, typename T>
	constexpr Vector<length, T> operator*(const T& lhs, const Vector<length, T>& rhs)
	{
		Vector<length, T> returnValue;
		for (size_t i = 0; i < length; i++)
			returnValue[i] = lhs * rhs.Data[i];
		return returnValue;
	}
}