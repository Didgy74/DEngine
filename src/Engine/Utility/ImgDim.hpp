#pragma once

#include <string>
#include <sstream>
#include <utility>
#include <cassert>

namespace Utility
{
	struct ImgDim
	{
		using ValueType = uint16_t;

		ValueType width;
		ValueType height;

		constexpr float GetAspectRatio() const;

		void Swap();

		std::string ToString() const;

		constexpr ValueType& At(size_t i);
		constexpr const ValueType& At(size_t i) const;
		constexpr ValueType& operator[](size_t i);
		constexpr const ValueType& operator[](size_t i) const;
	};

	constexpr float ImgDim::GetAspectRatio() const { return static_cast<float>(width) / static_cast<float>(height); }

	inline void ImgDim::Swap() { std::swap(width, height); }

	inline std::string ImgDim::ToString() const
	{
		std::stringstream out;
		out << '(' << width << ", " << height << ')';
		return out.str();
	}

	constexpr ImgDim::ValueType& ImgDim::At(size_t i)
	{
		return const_cast<ValueType&>(std::as_const(*this).At(i));
	}

	const constexpr ImgDim::ValueType& ImgDim::At(size_t i) const
	{
		assert(i >= 0 && i < 2);
		switch (i)
		{
		case 0:
			return width;
		case 1:
			return height;
		default:
			assert(false);
			return width;
		}
	}

	constexpr ImgDim::ValueType& ImgDim::operator[](size_t i)
	{
		return At(i);
	}

	constexpr const ImgDim::ValueType& ImgDim::operator[](size_t i) const
	{
		return At(i);
	}
}

