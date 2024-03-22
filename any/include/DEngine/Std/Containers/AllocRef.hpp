#pragma once

#include <DEngine/Std/BumpAllocator.hpp>

namespace DEngine::Std {
	class AllocRef {
	public:
		static constexpr bool stateless = Std::FrameAlloc::stateless;

		AllocRef(Std::FrameAlloc& in) : frameAlloc{ &in } {}

		[[nodiscard]] void* Alloc(uSize size, uSize alignment) const noexcept {
			return frameAlloc->Alloc(size, alignment);
		}
		[[nodiscard]] bool Resize(void* ptr, uSize newSize) const noexcept {
			return frameAlloc->Resize(ptr, newSize);
		}
		// Passing in a pointer value that was not returned by Alloc is UB.
		void Free(void* in, uSize size) const noexcept {
			frameAlloc->Free(in, size);
		}

	private:
		Std::FrameAlloc* frameAlloc = nullptr;
	};
}