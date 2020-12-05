#pragma once

#include <DEngine/FixedWidthTypes.hpp>

#include <mutex>

namespace DEngine::Std
{
	class FrameAllocator
	{
	public:
		FrameAllocator(uSize reserveSize);
		FrameAllocator(FrameAllocator const&) = delete;
		FrameAllocator(FrameAllocator&&) = delete;
		~FrameAllocator();

		FrameAllocator& operator=(FrameAllocator const&) = delete;
		FrameAllocator& operator=(FrameAllocator&&) = delete;

		[[nodiscard]] void* Alloc(uSize size);
		void Reserve(uSize size);
		void Clear();

	private:
		std::mutex lock{};

		char* data = nullptr;
		uSize size = 0;
		uSize offset = 0;
		uSize prevOffset = 0;

		struct OverAllocationNode
		{
			char* data = nullptr;
			uSize size = 0;
		};
		OverAllocationNode* overAllocationNodes = nullptr;
		uSize overAllocationNodeCount = 0;
	};
}