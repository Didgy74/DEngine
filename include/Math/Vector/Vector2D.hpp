#pragma once

#include "Core.hpp"

#include "../Trigonometric.hpp"

namespace Math
{
	template<size_t length, typename T>
	struct Vector;

	using Vector2D = Vector<2, float>;
	using Vector2DInt = Vector<2, int32_t>;

	template<typename T>
	struct Vector<2, T> : public detail::VectorBase<2, T>
	{
		/*
			Start of necessary members.
		*/
		using ParentType = detail::VectorBase<2, T>;

		constexpr Vector<2, T>& operator+=(const Vector<2, T>& x);
		constexpr Vector<2, T>& operator-=(const Vector<2, T>& x);
		constexpr Vector<2, T>& operator*=(const T& x);
		/*
			End of necessary members.
		*/

		constexpr void Rotate(float counterClockwise);

		template<AngleUnit angleUnit = Setup::defaultAngleUnit>
		[[nodiscard]] Math::Vector<2, T> GetRotated(float degrees) const;
		[[nodiscard]] constexpr Math::Vector<2, T> GetRotated(bool counterClockwise) const;

		constexpr Math::Vector<3, T> AsVec3(const T& zValue = T{}) const;

		[[nodiscard]] static constexpr Vector<2, T> Up();
		[[nodiscard]] static constexpr Vector<2, T> Down();
		[[nodiscard]] static constexpr Vector<2, T> Left();
		[[nodiscard]] static constexpr Vector<2, T> Right();
	};

	template<typename T>
	constexpr Vector<2, T>& Vector<2, T>::operator+=(const Vector<2, T>& input)
	{
		for (size_t i = 0; i < 2; i++)
			(*this)[i] += input[i];
		return *this;
	}

	template<typename T>
	constexpr Vector<2, T>& Vector<2, T>::operator-=(const Vector<2, T>& input)
	{
		for (size_t i = 0; i < 2; i++)
			(*this)[i] -= input[i];
		return *this;
	}

	template<typename T>
	constexpr Vector<2, T>& Vector<2, T>::operator*=(const T& input)
	{
		for (auto& item : (*this))
			item *= input;
		return *this;
	}

	template<typename T>
	template<Math::AngleUnit angleUnit>
	Math::Vector<2, T> Vector<2, T>::GetRotated(float degrees) const
	{
		const float cos = Cos<angleUnit>(degrees);
		const float sin = Sin<angleUnit>(degrees);
		return { this->x*cos - this->y*sin, this->x*sin + this->y*cos };
	}

	template<typename T>
	inline constexpr void Vector<2, T>::Rotate(float counterClockwise)
	{
		const T originalX = this->x;
		if (counterClockwise)
		{
			this->x = -this->y;
			this->y = originalX;
		}
		else
		{
			this->x = this->y;
			this->y = -originalX;
		}
	}

	template<typename T>
	inline constexpr Math::Vector<2, T> Vector<2, T>::GetRotated(bool counterClockwise) const { return counterClockwise ? Math::Vector<2, T>{ -this->y, this->x} : Math::Vector<2, T>{ this->y, -this->x }; }

	template<typename T>
	constexpr Math::Vector<3, T> Vector<2, T>::AsVec3(const T& zValue) const { return Math::Vector<3, T>{ this->x, this->y, zValue }; }

	template<typename T>
	constexpr Vector<2, T> Vector<2, T>::Up() { return { 0, 1 }; }

	template<typename T>
	constexpr Vector<2, T> Vector<2, T>::Down() { return { 0, -1 }; }

	template<typename T>
	constexpr Vector<2, T> Vector<2, T>::Left() { return { -1, 0 }; }

	template<typename T>
	constexpr Vector<2, T> Vector<2, T>::Right() { return { 1, 0 }; }
}