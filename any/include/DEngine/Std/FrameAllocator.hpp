#pragma once

#include <DEngine/FixedWidthTypes.hpp>
#include <DEngine/Std/Containers/Opt.hpp>

namespace DEngine::Std
{
	class FrameAllocator
	{
	public:
		static constexpr bool clearUnusedMemory = false;

		FrameAllocator() noexcept = default;
		FrameAllocator(FrameAllocator const&) = delete;
		FrameAllocator(FrameAllocator&&) noexcept;
		~FrameAllocator() noexcept;

		FrameAllocator& operator=(FrameAllocator const&) = delete;
		FrameAllocator& operator=(FrameAllocator&&) noexcept;

		[[nodiscard]] static Std::Opt<FrameAllocator> PreAllocate(uSize size) noexcept;

		[[nodiscard]] void* Alloc(uSize size, uSize alignment) noexcept;
		[[nodiscard]] bool Realloc(void* ptr, uSize newSize) noexcept;
		void Free(void* in) noexcept;

		void Reset() noexcept;

	protected:
		uSize offset = 0;
		Std::Opt<uSize> lastAllocOffset;

		struct Pool
		{
			using DataPtrT = char;
			DataPtrT* data = nullptr;
			uSize totalSize = 0;
			[[maybe_unused]] uSize allocCount = 0;
		};
		static constexpr uSize maxPoolCount = 5;
		Pool memoryPools[maxPoolCount] = {};
		uSize poolCount = 0;
	};
}