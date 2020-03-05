#pragma once

#include "DEngine/FixedWidthTypes.hpp"

namespace DEngine::Math
{
	template<uSize length, typename T = float>
	struct Vector
	{
		using ValueType = T;
		static constexpr uSize dimCount = length;

		T Data[length] = {};

		[[nodiscard]] constexpr T& At(uSize index)
		{
#if defined( _MSC_VER )
			__assume(index < dimCount);
#endif
			return const_cast<T&>(std::as_const(*this).At(index));
		}

		[[nodiscard]] constexpr const T& At(uSize index) const
		{
#if defined( _MSC_VER )
			__assume(index < dimCount);
#endif
			assert(index < dimCount);
			return Data.at(index);
		}

		[[nodiscard]] static constexpr T Dot(const Vector<length, T>& lhs, const Vector<length, T>& rhs)
		{
			T sum = T(0);
			for (size_t i = 0; i < length; i++)
				sum += lhs[i] * rhs[i];
		}

		[[nodiscard]] constexpr T* GetData()
		{
			return Data.Data();
		}

		[[nodiscard]] constexpr const T* GetData() const
		{
			return Data.Data();
		}

		void Normalize()
		{
			static_assert(std::is_floating_point<T>::value, "Cannot normalize an integral vector.");
			const auto& magnitude = Magnitude();
			for (size_t i = 0; i < length; i++)
				Data[i] /= magnitude;
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
			for (size_t i = 0; i < length; i++)
			{
				if constexpr (std::is_same<char, T>::value || std::is_same<unsigned char, T>::value)
					stream << +Data[i];
				else
					stream << Data[i];

				if (i < length - 1)
					stream << ", ";
			}
			stream << ')';
			return stream.str();
		}

		[[nodiscard]] static constexpr Vector<length, T> SingleValue(const T& input)
		{
			Vector<length, T> temp;
			for (size_t i = 0; i < length; i++)
				Data[i] = input;
			return temp;
		}
		[[nodiscard]] static constexpr Vector<length, T> Zero()
		{
			return Vector<length, T>{};
		}
		[[nodiscard]] static constexpr Vector<length, T> One()
		{
			Vector<length, T> temp;
			for (size_t i = 0; i < length; i++)
				Data[i] = T(1);
			return temp;
		}

		constexpr Vector<length, T>& operator+=(const Vector<length, T>& rhs)
		{
			for (size_t i = 0; i < length; i++)
				Data[i] += rhs[i];
			return *this;
		}
		constexpr Vector<4, T>& operator-=(const Vector<4, T>& rhs)
		{
			for (size_t i = 0; i < length; i++)
				Data[i] -= rhs[i];
			return *this;
		}
		constexpr Vector<4, T>& operator*=(const T& rhs)
		{
			for (size_t i = 0; i < length; i++)
				Data[i] *= rhs;
			return *this;
		}
		[[nodiscard]] constexpr Vector<length, T> operator+(const Vector<length, T>& rhs) const
		{
			Vector<length, T> returnValue;
			for (size_t i = 0; i < length; i++)
				returnValue[i] = Data[i] + rhs[i];
			return returnValue;
		}
		[[nodiscard]] constexpr Vector<length, T> operator-(const Vector<length, T>& rhs) const
		{
			Vector<length, T> returnValue;
			for (size_t i = 0; i < length; i++)
				returnValue[i] = Data[i] - rhs[i];
			return returnValue;
		}
		[[nodiscard]] constexpr Vector<length, T> operator-() const
		{
			Vector<length, T> returnValue;
			for (size_t i = 0; i < length; i++)
				returnValue[i] = -Data[i];
			return returnValue;
		}
		[[nodiscard]] constexpr bool operator==(const Vector<length, T>& rhs) const
		{
			for (size_t i = 0; i < length; i++)
			{
				if (Data[i] != rhs[i])
					return false;
			}
			return true;
		}
		[[nodiscard]] constexpr bool operator!=(const Vector<length, T>& rhs) const
		{
			for (size_t i = 0; i < length; i++)
			{
				if (Data[i] != rhs[i])
					return true;
			}
			return false;
		}
		[[nodiscard]] constexpr T& operator[](size_t index)
		{
#if defined( _MSC_VER )
			__assume(index < dimCount);
#endif
			return const_cast<T&>(std::as_const(*this)[index]);
		}
		[[nodiscard]] constexpr const T& operator[](size_t index) const
		{
#if defined( _MSC_VER )
			__assume(index < dimCount);
#endif
			assert(index < dimCount);
			return Data[index];
		}
		template<typename U>
		[[nodiscard]] constexpr explicit operator Vector<length, U>() const
		{
			static_assert(std::is_convertible<T, U>::value, "Can't convert to this type.");
			
			Vector<length, U> returnValue;
			for (size_t i = 0; i < length; i++)
				returnValue[i] = static_cast<U>(Data[i]);
			return returnValue;
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