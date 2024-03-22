#pragma once

#include <DEngine/FixedWidthTypes.hpp>

namespace DEngine::Std
{
	class DefaultAllocator
	{
	public:
		static constexpr bool stateless = true;

		[[nodiscard]] void* Alloc(uSize size, uSize alignment) noexcept;
		[[nodiscard]] bool Realloc(void* ptr, uSize newSize) noexcept;
		void Free(void* in) noexcept;
	};

	constexpr DefaultAllocator defaultAllocator = {};
}