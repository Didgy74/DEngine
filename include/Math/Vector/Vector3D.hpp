#pragma once

#include "Core.hpp"

#include "../Enum.hpp"

namespace Math
{
	template<size_t length, typename T>
	struct Vector;

	using Vector3D = Vector<3, float>;
	using Vector3DInt = Vector<3, int32_t>;

	template<typename T>
	struct Vector<3, T> : public detail::VectorBase<3, T>
	{
		/*
			Start of necessary members.
		*/
		using ParentType = detail::VectorBase<3, T>;

		constexpr Vector<3, T>& operator+=(const Vector<3, T>& x);
		constexpr Vector<3, T>& operator-=(const Vector<3, T>& x);
		constexpr Vector<3, T>& operator*=(const T& x);
		/*
			End of necessary members.
		*/

		[[nodiscard]] static constexpr Vector<3, T> Up();
		[[nodiscard]] static constexpr Vector<3, T> Down();
		[[nodiscard]] static constexpr Vector<3, T> Left();
		[[nodiscard]] static constexpr Vector<3, T> Right();
		[[nodiscard]] static constexpr Vector<3, T> Forward();
		[[nodiscard]] static constexpr Vector<3, T> Back();

		[[nodiscard]] Vector<3, T> GetRotated(ElementaryAxis axis, float degrees) const;

		[[nodiscard]] constexpr Vector<2, T> AsVec2() const;

		[[nodiscard]] static constexpr Vector<3, T> Cross(const Vector<3, T>& left, const Vector<3, T>& right);
	};

	template<typename T>
	constexpr Vector<3, T>& Vector<3, T>::operator+=(const Vector<3, T>& input)
	{
		for (size_t i = 0; i < 3; i++)
			(*this)[i] += input[i];
		return *this;
	}

	template<typename T>
	constexpr Vector<3, T>& Vector<3, T>::operator-=(const Vector<3, T>& input)
	{
		for (size_t i = 0; i < 3; i++)
			(*this)[i] -= input[i];
		return *this;
	}

	template<typename T>
	constexpr Vector<3, T>& Vector<3, T>::operator*=(const T& input)
	{
		for (auto& item : (*this))
			item *= input;
		return *this;
	}

	template<typename T>
	constexpr Vector<3, T> Vector<3, T>::Up() { return { 0, 1, 0 }; }

	template<typename T>
	constexpr Vector<3, T> Vector<3, T>::Down() { return { 0, -1, 0}; }

	template<typename T>
	constexpr Vector<3, T> Vector<3, T>::Left() { return { -1, 0, 0}; }

	template<typename T>
	constexpr Vector<3, T> Vector<3, T>::Right() { return { 1, 0, 0 }; }

	template<typename T>
	constexpr Vector<3, T> Vector<3, T>::Forward() { return { 0, 0, 1 }; }

	template<typename T>
	constexpr Vector<3, T> Vector<3, T>::Back() { return { 0, 0, -1 }; }

	template<typename T>
	Vector<3, T> Vector<3, T>::GetRotated(ElementaryAxis axis, float degrees) const
	{
		assert(axis == ElementaryAxis::X || axis == ElementaryAxis::Y || axis == ElementaryAxis::Z);
		const float cos = Cos<AngleUnit::Degrees>(degrees);
		const float sin = Sin<AngleUnit::Degrees>(degrees);
		switch (axis)
		{
		case ElementaryAxis::X:
			return { this->x, this->y*cos - this->z*sin, this->y*sin + this->z*cos };
		case ElementaryAxis::Y:
			return { this->x*cos - this->z*sin, this->y, this->x*sin + this->z*cos };
		default:
			return { this->x*cos - this->y*sin, this->x*sin + this->y*cos, this->z };
		}
	}

	template<typename T>
	constexpr Vector<2, T> Vector<3, T>::AsVec2() const { return Math::Vector<2, T>{this->x, this->y }; }

	template<typename T>
	constexpr Vector<3, T> Vector<3, T>::Cross(const Vector<3, T>& left, const Vector<3, T>& right) 
	{
		return { left.y * right.z - left.z * right.y, left.z * right.x - left.x * right.z, left.x * right.y - left.y * right.x };
	}
}