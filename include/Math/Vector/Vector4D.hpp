#pragma once

#include "Core.hpp"

namespace Math
{
	template<size_t length, typename T>
	struct Vector;

	using Vector4D = Vector<4, float>;
	using Vector4DInt = Vector<4, int32_t>;

	template<typename T>
	struct Vector<4, T> : public detail::VectorBase<4, T>
	{
		/*
			Start of necessary members.
		*/
		using ParentType = detail::VectorBase<4, T>;

		constexpr Vector<4, T>& operator+=(const Vector<4, T>& x);
		constexpr Vector<4, T>& operator-=(const Vector<4, T>& x);
		constexpr Vector<4, T>& operator*=(const T& x);
		/*
			End of necessary members.
		*/
	};

	template<typename T>
	constexpr Vector<4, T>& Vector<4, T>::operator+=(const Vector<4, T>& input)
	{
		for (size_t i = 0; i < 4; i++)
			(*this)[i] += input[i];
	}

	template<typename T>
	constexpr Vector<4, T>& Vector<4, T>::operator-=(const Vector<4, T>& input)
	{
		for (size_t i = 0; i < 4; i++)
			(*this)[i] -= input[i];
	}

	template<typename T>
	constexpr Vector<4, T>& Vector<4, T>::operator*=(const T& input)
	{
		for (auto& item : (*this))
			item *= input;
	}
}