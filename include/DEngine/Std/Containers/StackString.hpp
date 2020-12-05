#pragma once

#include <DEngine/FixedWidthTypes.hpp>

namespace DEngine::Std
{
	template<uSize length>
	class StackString
	{
		constexpr StackString() noexcept;

	private:
			char data[length];
			uSize length = 0;
	};
}

constexpr DEngine::Std::StackString::StackString() noexcept
{
	data[0] = 0;
}