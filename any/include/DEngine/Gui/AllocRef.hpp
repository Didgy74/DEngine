#pragma once

#include <DEngine/Std/FrameAllocator.hpp>

namespace DEngine::Gui
{
	class AllocRef
	{
	public:
		static constexpr bool stateless = Std::FrameAlloc::stateless;

		AllocRef(Std::FrameAlloc& in) : frameAlloc{ &in } {}
		AllocRef(AllocRef const&) = default;

		[[nodiscard]] void* Alloc(uSize size, uSize alignment) const noexcept {
			return frameAlloc->Alloc(size, alignment);
		}
		[[nodiscard]] bool Realloc(void* ptr, uSize newSize) const noexcept {
			return frameAlloc->Realloc(ptr, newSize);
		}
		// Passing in a pointer value that was not returned by Alloc is UB.
		void Free(void* in) const noexcept {
			frameAlloc->Free(in);
		}

	private:
		Std::FrameAlloc* frameAlloc = nullptr;
	};
}