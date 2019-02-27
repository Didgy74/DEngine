#pragma once

#include "Core.hpp"

namespace Math
{
	namespace detail
	{
		template<size_t length, typename T>
		struct VectorBase;
	}

	template<size_t length, typename T = float>
	struct Vector : public detail::VectorBase<length, T>
	{
		using ParentType = detail::VectorBase<length, T>;

		constexpr Vector<length, T>& operator+=(const Vector<length, T>& x);
		constexpr Vector<length, T>& operator-=(const Vector<length, T>& x);
		constexpr Vector<length, T>& operator*=(const T& x);
	};

	template<size_t length, typename T>
	constexpr Vector<length, T>& Vector<length, T>::operator+=(const Vector<length, T>& input)
	{
		for (size_t i = 0; i < length; i++)
			(*this)[i] += input[i];
	}

	template<size_t length, typename T>
	constexpr Vector<length, T>& Vector<length, T>::operator-=(const Vector<length, T>& input)
	{
		for (size_t i = 0; i < length; i++)
			(*this)[i] -= input[i];
	}

	template<size_t length, typename T>
	constexpr Vector<length, T>& Vector<length, T>::operator*=(const T& input)
	{
		for (auto& item : (*this))
			item *= input;
	}
}