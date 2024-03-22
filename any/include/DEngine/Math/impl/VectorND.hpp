#pragma once

#include <DEngine/FixedWidthTypes.hpp>

namespace DEngine::Math
{
	template<uSize length, typename T>
	struct Vector {
		T data[length];

		[[nodiscard]] constexpr auto& operator[](uSize index) noexcept { return data[index]; }
		[[nodiscard]] constexpr auto operator[](uSize index) const noexcept { return data[index]; }
	};
}