#pragma once

#include <DEngine/FixedWidthTypes.hpp>

#include <DEngine/Std/Containers/impl/Assert.hpp>

namespace DEngine::Std
{
	// Holds a reference to an immutable string. Supports with and without null-termination.
	class Str
	{
	public:
		constexpr Str() noexcept = default;
		Str(char const* nullTerminString) noexcept;
		constexpr Str(char const* in, uSize length) noexcept : ptr(in), length(length) {}

		[[nodiscard]] constexpr u32 Size() const noexcept { return length; }
		[[nodiscard]] constexpr char const* Data() const noexcept { return ptr; }

		[[nodiscard]] constexpr char operator[](u32 index) const noexcept 
		{
			DENGINE_IMPL_CONTAINERS_ASSERT(index < length);
			return ptr[index];
		}

		[[nodiscard]] constexpr char const* begin() const noexcept { return ptr; }
		[[nodiscard]] constexpr char const* end() const noexcept { return ptr + length; }

	private:
		char const* ptr = nullptr;
		uSize length = 0;
	};
}