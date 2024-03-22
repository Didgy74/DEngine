#include <DEngine/Std/BumpAllocator.hpp>

#include <DEngine/Std/Containers/impl/Assert.hpp>
#include <DEngine/Std/Utility.hpp>

#include <DEngine/Math/Common.hpp>

#include <cstdlib>
#include <cstdint>
#include <new>

using namespace DEngine;

struct DEngine::Std::BumpAllocator::Impl
{
public:
	static constexpr uSize minAllocAlignment = 8;

	struct RAIIPtr
	{
		void* ptr = nullptr;
		~RAIIPtr() noexcept
		{
			if (ptr) {
				free(ptr);
				ptr = nullptr;
			}
		}
	};

	static void FreeBlockListElements(BlockList& in, bool safetyOn) noexcept {
		if (in.ptrElements == nullptr || in.count == 0)
			return;

		DENGINE_IMPL_CONTAINERS_ASSERT(in.capacity != 0);
		for (uSize i = 0; i < in.count; i++) {
			auto& block = in.ptrElements[i];
			DENGINE_IMPL_CONTAINERS_ASSERT(block.data);
			if (!noSafetyOverride && safetyOn) {
				DENGINE_IMPL_CONTAINERS_ASSERT(block.allocCount == 0);
			}

			free(block.data);
			block.data = nullptr;
			block = {};
		}
		in.count = 0;
	}

	static void FreeBlockList(BlockList& in, bool safetyOn) noexcept {
		FreeBlockListElements(in, safetyOn);

		if (in.ptrElements) {
			free(in.ptrElements);
			in = {};
		}
	}

	static BlockList AllocateNewBlockList(uSize newCapacity = BlockList::minCapacity) {
		BlockList blockList = {};
		blockList.ptrElements = (Block*)malloc(sizeof(Block) * newCapacity);
		// TODO: Error handling for failed allocation.
		blockList.capacity = newCapacity;
		return blockList;
	}

	static void PushToBlockList(BlockList& list, Block block) {
		DENGINE_IMPL_ASSERT(list.count <= list.capacity);
		DENGINE_IMPL_ASSERT(block.data != nullptr);

		if (list.ptrElements == nullptr || list.capacity == 0) {
			list = AllocateNewBlockList();
		}

		if (list.count == list.capacity) {
			auto oldList = list;

			// We need to allocate a new block-list and destroy the old one.
			auto const newCapacity = oldList.capacity * 2;
			auto ptr = (Block*)malloc(sizeof(Block) * newCapacity);
			// Copy over the blocks to the new memory.
			for (int i = 0; i < oldList.count; i++) {
				auto& oldElement = oldList.ptrElements[i];

				auto* newElement = ptr + i;
				new(newElement) Block(oldElement);

				oldElement.data = nullptr;
			}

			list.count = oldList.count;
			list.capacity = newCapacity;
			list.ptrElements = ptr;

			free(oldList.ptrElements);
		}

		// We have guaranteed space
		new(list.ptrElements + list.count) Block(block);
		list.count += 1;
	}

	static Block AllocateBlock(uSize newSize) {
		Block returnVal = {};
		// Allocate new block with specified size.
		auto* newMem = malloc(newSize);
		returnVal.data = (Block::DataPtrT*)newMem;
		returnVal.size = newSize;
		returnVal.allocCount = 0;
		returnVal.offset = 0;
		return returnVal;
	}
};

Std::BumpAllocator::BumpAllocator(Std::BumpAllocator&& other) noexcept :
	prevAllocOffset{ Std::Move(other.prevAllocOffset) },
	activeBlock{ other.activeBlock },
	blockList{ other.blockList }
{
	other.activeBlock = {};
	other.blockList = {};
}

Std::BumpAllocator::~BumpAllocator() noexcept
{
	ReleaseAllMemory();
}

Std::BumpAllocator& Std::BumpAllocator::operator=(Std::BumpAllocator&& other) noexcept
{
	ReleaseAllMemory();

	prevAllocOffset = Std::Move(other.prevAllocOffset);

	activeBlock = other.activeBlock;
	other.activeBlock = {};
	blockList = other.blockList;
	other.blockList = {};

	return *this;
}

Std::Opt<Std::BumpAllocator> Std::BumpAllocator::PreAllocate(uSize size) noexcept
{
	BumpAllocator returnVal;
	return returnVal;

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
	static auto CalcAlignedOffset(void const* ptr, uSize offset, uSize alignment) {
		u64 const asInt = (uintptr_t)ptr + offset;
		auto const alignedAsInt = Math::CeilToMultiple(asInt, (u64)alignment);
		DENGINE_IMPL_CONTAINERS_ASSERT((alignedAsInt % alignment) == 0);
		auto const alignedOffset = offset + alignedAsInt - asInt;
		return alignedOffset;
	}
}

void* Std::BumpAllocator::Alloc(uSize allocSize, uSize allocAlignment) noexcept
{
	DENGINE_IMPL_CONTAINERS_ASSERT(allocSize != 0);
	DENGINE_IMPL_CONTAINERS_ASSERT(allocAlignment > 0);

	//return malloc(size);
	if (activeBlock.data == nullptr) {
		activeBlock = Impl::AllocateBlock(allocSize);
	}

	{
		// Check if we need to resize block.
		DENGINE_IMPL_CONTAINERS_ASSERT(activeBlock.size != 0);
		auto const alignedOffset = CalcAlignedOffset(
			activeBlock.data,
			activeBlock.offset,
			Math::Max(allocAlignment, Impl::minAllocAlignment));

		// Check if there is enough remaining space in the block
		if (alignedOffset + allocSize > activeBlock.size) {
			Impl::PushToBlockList(blockList, activeBlock);

			// Alloc new block with larger capacity.
			auto newSize = Math::Max(allocSize, activeBlock.size * 2);
			activeBlock = Impl::AllocateBlock(newSize);
		}
	}



	// It's now guaranteed that we can fit the memory.
	DENGINE_IMPL_ASSERT(activeBlock.data != nullptr);
	auto const alignedOffset = CalcAlignedOffset(
		activeBlock.data,
		activeBlock.offset,
		Math::Max(allocAlignment, Impl::minAllocAlignment));
	DENGINE_IMPL_ASSERT(alignedOffset + allocSize <= activeBlock.size);
	auto returnVal = activeBlock.data + alignedOffset;
	activeBlock.allocCount += 1;
	activeBlock.offset = alignedOffset + allocSize;

	return returnVal;
}

bool Std::BumpAllocator::Resize(void* ptr, uSize newSize) noexcept
{
	//return false;

	DENGINE_IMPL_CONTAINERS_ASSERT(activeBlock.data);
	DENGINE_IMPL_CONTAINERS_ASSERT(activeBlock.allocCount > 0);

	if (prevAllocOffset.Has()) {
		auto const lastAllocOffsetVal = prevAllocOffset.Get();
		// First we check that the ptr is the most recent allocation.
		// If we are the most recent allocation, as long as we can
		// fit the new size, the operation is succesful.
		if (ptr == activeBlock.data + lastAllocOffsetVal && lastAllocOffsetVal + newSize <= activeBlock.size) {
			activeBlock.offset = lastAllocOffsetVal + newSize;
			return true;
		}
	}

	return false;
}

void Std::BumpAllocator::Free(void* in, uSize size) noexcept
{
	//free(in);
	//return;

	if constexpr (noSafetyOverride)
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
			if (activeBlock.allocCount == 0) {
				activeBlock.offset = 0;
				prevAllocOffset = Std::nullOpt;
			}
		}
	}

	if (prevAllocOffset.HasValue()) {
		auto const lastAllocOffsetValue = prevAllocOffset.Value();
		// If the freed pointer was the previously allocated
		// memory, then we pop the allocation so we can reuse it
		// immediately
		if (in == activeBlock.data + lastAllocOffsetValue) {
			activeBlock.offset = lastAllocOffsetValue;
			prevAllocOffset = Std::nullOpt;
		}
	}


	if (!allocFound && blockList.ptrElements) {
		// Then check the other pools
		for (uSize i = 0; i < blockList.count; i += 1) {
			auto& block = blockList.ptrElements[i];
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

void Std::BumpAllocator::Reset([[maybe_unused]] bool safetyOn) noexcept
{
	Impl::FreeBlockListElements(blockList, safetyOn);

	DENGINE_IMPL_CONTAINERS_ASSERT(noSafetyOverride || (!safetyOn || activeBlock.allocCount == 0));
	activeBlock.allocCount = 0;
	activeBlock.offset = 0;
	prevAllocOffset = Std::nullOpt;
	// For testing purposes
	if constexpr (clearUnusedMemory) {
		for (uSize i = 0; i < activeBlock.size; i += 1)
			activeBlock.data[i] = 0;
	}
}

void Std::BumpAllocator::ReleaseAllMemory()
{
	Impl::FreeBlockList(blockList, false);

	if (activeBlock.data) {
		DENGINE_IMPL_CONTAINERS_ASSERT(noSafetyOverride || activeBlock.allocCount == 0);
		free(activeBlock.data);
		activeBlock = {};
	}
}
