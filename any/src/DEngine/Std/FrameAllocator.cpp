#include <DEngine/Std/FrameAllocator.hpp>

#include <DEngine/Std/Containers/impl/Assert.hpp>
#include <DEngine/Std/Utility.hpp>

#include <DEngine/Math/Common.hpp>

#include <cstdlib>
#include <cstdint>

using namespace DEngine;

struct DEngine::Std::FrameAllocator::Impl
{
public:
	struct RAIIPtr
	{
		void* ptr = nullptr;
		~RAIIPtr() noexcept
		{
			if (ptr)
			{
				free(ptr);
				ptr = nullptr;
			}
		}
	};

	static void FreeBlockListElements(BlockList& in) noexcept
	{
		if (in.ptr)
		{
			DENGINE_IMPL_CONTAINERS_ASSERT(in.capacity != 0);
			for (uSize i = 0; i < in.count; i++)
			{
				auto& block = in.ptr[i];
				DENGINE_IMPL_CONTAINERS_ASSERT(block.data);
				DENGINE_IMPL_CONTAINERS_ASSERT(block.allocCount == 0);

				free(block.data);
				block = {};
			}
			in.count = 0;
		}
	}

	static void FreeBlockList(BlockList& in) noexcept
	{
		FreeBlockListElements(in);

		if (in.ptr)
		{
			free(in.ptr);
			in = {};
		}
	}
};

Std::FrameAllocator::FrameAllocator(Std::FrameAllocator&& other) noexcept :
	prevAllocOffset{ Std::Move(other.prevAllocOffset) },
	activeBlock{ other.activeBlock },
	blockList{ other.blockList }
{
	other.activeBlock = {};
	other.blockList = {};
}

Std::FrameAllocator::~FrameAllocator() noexcept
{
	ReleaseAllMemory();
}

Std::FrameAllocator& Std::FrameAllocator::operator=(Std::FrameAllocator&& other) noexcept
{
	ReleaseAllMemory();

	prevAllocOffset = Std::Move(other.prevAllocOffset);

	activeBlock = other.activeBlock;
	other.activeBlock = {};
	blockList = other.blockList;
	other.blockList = {};

	return *this;
}

Std::Opt<Std::FrameAllocator> Std::FrameAllocator::PreAllocate(uSize size) noexcept
{
	FrameAllocator returnVal;

	auto ptr = malloc(size);
	if (!ptr)
		// Just in case the allocation failed.
		return Std::nullOpt;
	else
	{
		auto& block = returnVal.activeBlock;
		block.data = static_cast<Block::DataPtrT*>(ptr);
		block.size = size;

		if constexpr (clearUnusedMemory)
		{
			for (uSize i = 0; i < block.size; i += 0)
				block.data[i] = 0;
		}

		return returnVal;
	}
}

namespace DEngine::Std
{
	static auto GetAlignedOffset(void const* ptr, uSize offset, uSize alignment)
	{
		u64 const asInt = (uintptr_t)ptr + offset;
		auto const alignedAsInt = Math::CeilToMultiple(asInt, (u64)alignment);
		DENGINE_IMPL_CONTAINERS_ASSERT((alignedAsInt % alignment) == 0);
		auto const alignedOffset = offset + alignedAsInt - asInt;
		return alignedOffset;
	}
}

void* Std::FrameAllocator::Alloc(uSize size, uSize alignment) noexcept
{
	bool allocActiveBlock = false;
	uSize allocActiveBlockSize = 0;

	if (activeBlock.data)
	{
		DENGINE_IMPL_CONTAINERS_ASSERT(activeBlock.size != 0);

		auto& block = activeBlock;

		auto const alignedOffset = GetAlignedOffset(activeBlock.data, activeBlock.offset, alignment);
		// Check if there is enough remaining space in the block
		if (alignedOffset + size <= block.size)
		{
			// There is enough remaining space. Allocate here.
			activeBlock.allocCount += 1;
			prevAllocOffset = activeBlock.offset;
			activeBlock.offset = alignedOffset + size;
			return block.data + alignedOffset;
		}
		else
		{
			allocActiveBlock = true;
			allocActiveBlockSize = Math::Max(size, activeBlock.size * 2);

			// First move the active block onto the prev-block-array
			if (blockList.ptr)
			{
				// Check if we can fit the current block onto our block-list.
				if (blockList.capacity == blockList.count)
				{
					// We need to fit more space for the block.
					// Allocate new space and move the data over.
					auto const newCapacity = sizeof(Block) * blockList.capacity * 2;

					Impl::RAIIPtr newArray = {};
					newArray.ptr = malloc(newCapacity);
					if (!newArray.ptr)
						return nullptr;

					for (int i = 0; i < blockList.count; i++)
						static_cast<Block*>(newArray.ptr)[i] = blockList.ptr[i];

					// Delete our old one and assign our new one.
					free(blockList.ptr);
					blockList.ptr = static_cast<Block*>(newArray.ptr);
					newArray.ptr = nullptr;
					blockList.capacity = newCapacity;
				}

				// We can fit the current active block. Push it to the end.
				blockList.ptr[blockList.count] = activeBlock;
				activeBlock = {};
				blockList.count += 1;
			}
			else
			{
				// Allocate space for the previous block-structs.
				uSize blockListCapacity = BlockList::minCapacity;
				blockList.ptr = static_cast<Block*>(malloc(sizeof(Block) * blockListCapacity));
				if (!blockList.ptr)
					return nullptr;
				blockList.capacity = blockListCapacity;

				blockList.ptr[0] = activeBlock;
				activeBlock = {};
				blockList.count += 1;
			}
		}
	}
	else
	{
		allocActiveBlock = true;
		allocActiveBlockSize = size;
	}

	if (allocActiveBlock)
	{
		// Allocate data on the active block

		auto* newMem = malloc(allocActiveBlockSize);
		if (!newMem)
			return nullptr;

		activeBlock.data = static_cast<Block::DataPtrT*>(newMem);
		activeBlock.size = allocActiveBlockSize;

		activeBlock.allocCount = 1;
		activeBlock.offset = size;
		prevAllocOffset = 0;
		return activeBlock.data;
	}

	return nullptr;
}

bool Std::FrameAllocator::Realloc(void* ptr, uSize newSize) noexcept
{
	DENGINE_IMPL_CONTAINERS_ASSERT(activeBlock.data);
	DENGINE_IMPL_CONTAINERS_ASSERT(activeBlock.allocCount > 0);

	bool returnVal = false;
	if (prevAllocOffset.HasValue())
	{
		auto const lastAllocOffsetVal = prevAllocOffset.Value();
		auto const oldSize = activeBlock.offset - lastAllocOffsetVal;
		returnVal =
			// First we check that the ptr is the most recent allocation
			ptr == activeBlock.data + lastAllocOffsetVal &&
			// Then we check that the newSize is bigger than the old size
			oldSize < newSize &&
			// Now we check if the new size can fit our block.
			lastAllocOffsetVal + newSize <= activeBlock.size;

		if (returnVal)
			activeBlock.offset = lastAllocOffsetVal + newSize;
	}

	return returnVal;
}

void Std::FrameAllocator::Free(void* in) noexcept
{
	if constexpr (!checkForAllocFoundOnFree)
		return;

	// Ideally we shouldn't have to confirm this on a release
	// build. In a release build, all of this could be skipped.

	[[maybe_unused]] bool allocFound = false;

	{
		// Check the active block first.
		DENGINE_IMPL_CONTAINERS_ASSERT(activeBlock.data);
		// Check if the pointer is within range of our allocated data
		auto const ptrOffset = (intptr_t)in - (intptr_t)activeBlock.data;
		if (0 <= ptrOffset && ptrOffset < activeBlock.offset)
		{
			allocFound = true;
			activeBlock.allocCount -= 1;
			if (activeBlock.allocCount == 0)
			{
				activeBlock.offset = 0;
				prevAllocOffset = Std::nullOpt;
			}
		}
	}

	if (prevAllocOffset.HasValue())
	{
		auto const lastAllocOffsetValue = prevAllocOffset.Value();
		// If the freed pointer was the previously allocated
		// memory, then we pop the allocation so we can reuse it
		// immediately
		if (in == activeBlock.data + lastAllocOffsetValue)
		{
			activeBlock.offset = lastAllocOffsetValue;
			prevAllocOffset = Std::nullOpt;
		}
	}


	if (!allocFound && blockList.ptr)
	{
		// Then check the other pools
		for (uSize i = 0; i < blockList.count; i += 1)
		{
			auto& block = blockList.ptr[i];
			// Check if the pointer is within range of our allocated data
			auto const ptrOffset = (intptr_t)in - (intptr_t)block.data;
			if (ptrOffset >= 0 && ptrOffset < block.offset)
			{
				allocFound = true;
				block.allocCount -= 1;
				break;
			}
		}
	}

	DENGINE_IMPL_CONTAINERS_ASSERT(allocFound);
}

void Std::FrameAllocator::Reset() noexcept
{
	Impl::FreeBlockListElements(blockList);

	DENGINE_IMPL_CONTAINERS_ASSERT(activeBlock.allocCount == 0);
	activeBlock.allocCount = 0;
	activeBlock.offset = 0;
	prevAllocOffset = Std::nullOpt;

	// For testing purposes
	if constexpr (clearUnusedMemory)
	{
		for (uSize i = 0; i < activeBlock.size; i += 1)
			activeBlock.data[i] = 0;
	}
}

void Std::FrameAllocator::ReleaseAllMemory()
{
	if (activeBlock.data)
	{
		DENGINE_IMPL_CONTAINERS_ASSERT(activeBlock.allocCount == 0);
		free(activeBlock.data);
		activeBlock = {};
	}

	Impl::FreeBlockList(blockList);
}
