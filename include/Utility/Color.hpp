#pragma once

#include <cstdint>
#include <cassert>

namespace Utility
{
	struct Color
	{
	public:
		static constexpr uint8_t length = 4;

		using ValueType = float;

		ValueType r;
		ValueType g;
		ValueType b;
		ValueType a;

		constexpr void Clamp();
		constexpr Color GetClamped() const;
		
		constexpr void Normalize();
		constexpr Color GetNormalized() const;

		constexpr ValueType& At(size_t i);
		constexpr const ValueType& At(size_t i) const;

		constexpr ValueType& operator[](size_t i);
		constexpr const ValueType& operator[](size_t i) const;

		static constexpr Color Black();
		static constexpr Color White();
		static constexpr Color Grey();
		static constexpr Color Red();
		static constexpr Color Green();
		static constexpr Color Blue();
	};

	constexpr void Color::Clamp()
	{
		for (size_t i = 0; i < length; i++)
		{
			ValueType& element = At(i);
			if (element < 0)
				element = 0;
			else if (element > 1)
				element = 1;
		}
	}

	constexpr Color Color::GetClamped() const
	{
		Color newColor = *this;
		newColor.Clamp();
		return newColor;
	}

	constexpr Color::ValueType& Color::At(size_t i)
	{
		return const_cast<ValueType&>(std::as_const(*this).At(i));
	}

	constexpr const Color::ValueType& Color::At(size_t i) const
	{
		assert(0 <= i && i < length);
		switch (i)
		{
		case 0:
			return r;
		case 1:
			return g;
		case 2:
			return b;
		case 3:
			return a;
		default:
			assert(false);
			return r;
		}
	}

	constexpr Color::ValueType& Color::operator[](size_t i)
	{
		return At(i);
	}

	constexpr const Color::ValueType& Color::operator[](size_t i) const
	{
		return At(i);
	}

	constexpr Color Color::Black() { return { 0, 0, 0, 1 }; }

	constexpr Color Color::White() { return { 1, 1, 1, 1 }; }

	constexpr Color Color::Grey() { return { 0.5f, 0.5f, 0.5f, 1 }; }

	constexpr Color Color::Red() { return { 1, 0, 0, 1 }; }

	constexpr Color Color::Green() { return { 0, 1, 0, 1 }; }

	constexpr Color Color::Blue() { return { 0, 0, 1, 1 }; }
}