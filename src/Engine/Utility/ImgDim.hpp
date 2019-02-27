#pragma once

#include <string>
#include <sstream>
#include <utility>

namespace Utility
{
	struct ImgDim
	{
		using ValueType = uint16_t;

		ValueType width;
		ValueType height;

		constexpr float AspectRatio() const;

		void Swap();

		std::string ToString() const;
	};

	constexpr float ImgDim::AspectRatio() const { return static_cast<float>(width) / static_cast<float>(height); }

	inline void ImgDim::Swap() { std::swap(width, height); }

	inline std::string ImgDim::ToString() const
	{
		std::stringstream out;
		out << '(' << width << ", " << height << ')';
		return out.str();
	}
}

