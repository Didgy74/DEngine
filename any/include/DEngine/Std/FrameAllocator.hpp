#pragma once

#include <DEngine/FixedWidthTypes.hpp>
#include <DEngine/Std/Containers/Opt.hpp>

namespace DEngine::Std
{
	// No part of this is thread-safe.
	class FrameAllocator
	{
	public:
		static constexpr bool stateless = false;

		static constexpr bool clearUnusedMemory = false;
		static constexpr bool checkForAllocFoundOnFree = true;

		FrameAllocator() noexcept = default;
		FrameAllocator(FrameAllocator const&) = delete;
		FrameAllocator(FrameAllocator&&) noexcept;
		~FrameAllocator() noexcept;

		FrameAllocator& operator=(FrameAllocator const&) = delete;
		FrameAllocator& operator=(FrameAllocator&&) noexcept;

		[[nodiscard]] static Std::Opt<FrameAllocator> PreAllocate(uSize size) noexcept;

		[[nodiscard]] void* Alloc(uSize size, uSize alignment) noexcept;
		[[nodiscard]] bool Realloc(void* ptr, uSize newSize) noexcept;
		// Passing in a pointer value that was not returned by Alloc is UB.
		void Free(void* in) noexcept;

		// Reset allocated memory without freeing
		void Reset() noexcept;

		void ReleaseAllMemory();

	protected:

		// Describes the offset for the next allocation
		Std::Opt<uSize> prevAllocOffset;

		struct Block
		{
			using DataPtrT = char;
			DataPtrT* data = nullptr;
			uSize size = 0;
			// Holds the offset for the next available memory,
			// it can also be used to track how much memory is currently used.
			uSize offset = 0;

			// On release builds, we shouldn't have to track this.
			// It's used to make sure everything is
			// freed before we release the block.
			[[maybe_unused]] uSize allocCount = 0;
		};
		Block activeBlock = {};

		// Essentially a simple vector for holding the block pointers.
		struct BlockList
		{
			Block* ptr = nullptr;
			uSize count = 0;
			static constexpr uSize minCapacity = 5;
			uSize capacity = 0;
		};
		BlockList blockList = {};

		struct Impl;
		friend Impl;
	};

	using FrameAlloc = FrameAllocator;
}