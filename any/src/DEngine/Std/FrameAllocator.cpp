#include <DEngine/Std/FrameAllocator.hpp>

#include <DEngine/Std/Containers/impl/Assert.hpp>
#include <DEngine/Std/Utility.hpp>

#include <DEngine/Math/Common.hpp>

#include <cstdlib>
#include <cstdint>

using namespace DEngine;

Std::FrameAllocator::FrameAllocator(Std::FrameAllocator&& other) noexcept :
	offset{ other.offset },
	lastAllocOffset{ Std::Move(other.lastAllocOffset) },
	poolCount{ other.poolCount }
{
	for (uSize i = 0; i < other.poolCount; i += 1)
		memoryPools[i] = other.memoryPools[i];

	other.poolCount = 0;
}

Std::FrameAllocator::~FrameAllocator() noexcept
{
	ReleaseMemory();
}

Std::FrameAllocator& Std::FrameAllocator::operator=(Std::FrameAllocator&& other) noexcept
{
	ReleaseMemory();

	offset = other.offset;
	lastAllocOffset = Std::Move(other.lastAllocOffset);
	poolCount = other.poolCount;

	for (uSize i = 0; i < other.poolCount; i += 1)
		memoryPools[i] = other.memoryPools[i];

	other.poolCount = 0;

	return *this;
}

Std::Opt<Std::FrameAllocator> Std::FrameAllocator::PreAllocate(uSize size) noexcept
{
	FrameAllocator returnVal;

	auto ptr = malloc(size);
	if (ptr)
	{
		auto& pool = returnVal.memoryPools[0];
		pool.data = static_cast<Pool::DataPtrT*>(ptr);
		pool.totalSize = size;

		returnVal.poolCount = 1;

		if constexpr (clearUnusedMemory)
		{
			for (uSize i = 0; i < pool.totalSize; i += 0)
				pool.data[i] = 0;
		}

		return returnVal;
	}
	else
		return Std::nullOpt;
}

void* Std::FrameAllocator::Alloc(uSize size, uSize alignment) noexcept
{
	void* returnVal = nullptr;

	auto getAlignedOffset = [](Pool::DataPtrT const* ptr, uSize offsetIn, uSize alignment) {
		auto const asInt = (uintptr_t)ptr + offsetIn;
		auto const alignedInt = Math::CeilToMultiple(asInt,  alignment);
		auto const alignedOffset = offsetIn + (alignedInt - asInt);
		DENGINE_IMPL_CONTAINERS_ASSERT((((uintptr_t)ptr + alignedOffset) % alignment) == 0);
		return alignedOffset;
	};
	auto updatePoolState_SetReturn = [=, &returnVal](Pool& pool, uSize alignedOffset) {
		pool.allocCount += 1;

		lastAllocOffset = alignedOffset;
		offset = alignedOffset + size;

		returnVal = pool.data + alignedOffset;
	};
	auto makePoolFn = [=](uSize poolSize)
		{
			Pool newPool = {};
			newPool.data = static_cast<Pool::DataPtrT*>(malloc(poolSize));
			newPool.totalSize = poolSize;

			// For testing purposes
			if constexpr (clearUnusedMemory)
			{
				for (uSize i = 0; i < newPool.totalSize; i += 1)
					newPool.data[i] = 0;
			}

			memoryPools[poolCount] = newPool;
			auto& pool =  memoryPools[poolCount];
			poolCount += 1;

			auto const alignedOffset = getAlignedOffset(pool.data, offset, alignment);

			DENGINE_IMPL_CONTAINERS_ASSERT(alignedOffset + size <= pool.totalSize);

			updatePoolState_SetReturn(pool, alignedOffset);
		};


	if (poolCount > 0)
	{
		auto& pool = memoryPools[poolCount - 1];

		auto const alignedOffset = getAlignedOffset(pool.data, offset, alignment);

		// Check if there is enough remaining space in the pool
		if (alignedOffset + size <= pool.totalSize)
		{
			updatePoolState_SetReturn(pool, alignedOffset);
		}
		else
		{
			DENGINE_IMPL_CONTAINERS_ASSERT(poolCount < maxPoolCount - 1);

			// We ran out of space, allocate a new pool.
			auto const newSize = Math::Max(pool.totalSize * 2, pool.totalSize + size);
			makePoolFn(newSize);
		}
	}
	else
	{
		auto const newSize = size;
		makePoolFn(newSize);
	}

	return returnVal;
}

bool Std::FrameAllocator::Realloc(void* ptr, uSize newSize) noexcept
{
	DENGINE_IMPL_CONTAINERS_ASSERT(poolCount > 0);

	auto& pool = memoryPools[poolCount - 1];

	DENGINE_IMPL_CONTAINERS_ASSERT(pool.allocCount > 0);

	bool returnVal = false;
	if (lastAllocOffset.HasValue())
	{
		auto const lastAllocOffsetVal = lastAllocOffset.Value();
		auto const oldSize = offset - lastAllocOffsetVal;
		returnVal =
			// First we check that the ptr is the most recent allocation
			ptr == pool.data + lastAllocOffsetVal &&
			// Then we check that the newSize is bigger than the old size
			oldSize < newSize &&
			// Now we check if the new size can fit our pool.
			lastAllocOffsetVal + newSize <= pool.totalSize;

		if (returnVal)
			offset = lastAllocOffsetVal + newSize;
	}

	return returnVal;
}

void Std::FrameAllocator::Free(void* in) noexcept
{
	DENGINE_IMPL_CONTAINERS_ASSERT(poolCount > 0);

	bool allocFound = false;

	auto poolCheckFn = [=, &allocFound](Pool& pool)
		{
			auto const ptrOffset = (uintptr_t)in - (uintptr_t)pool.data;
			if (ptrOffset >= 0 && ptrOffset < pool.totalSize)
			{
				allocFound = true;
				pool.allocCount -= 1;
			}
		};

	// Check most recent pool first
	{
		auto& pool = memoryPools[poolCount - 1];
		poolCheckFn(pool);

		if (pool.allocCount == 0)
		{
			offset = 0;
			lastAllocOffset = Std::nullOpt;
		}
		else if (lastAllocOffset.HasValue() && in == pool.data + lastAllocOffset.Value())
		{
			offset = lastAllocOffset.Value();
			lastAllocOffset = Std::nullOpt;
		}
	}

	if (!allocFound)
	{
		// Then check the other pools
		for (uSize i = 0; i < poolCount - 1; i += 1)
		{
			auto& pool = memoryPools[i];
			poolCheckFn(pool);
			if (allocFound)
				break;
		}
	}

	DENGINE_IMPL_CONTAINERS_ASSERT(allocFound);
}

void Std::FrameAllocator::Reset() noexcept
{
	if (poolCount == 0)
		return;

	// Check that all active pools are valid and no longer have any allocs
	DENGINE_IMPL_CONTAINERS_ASSERT(Std::AllOf(
		&memoryPools[0],
		&memoryPools[poolCount],
		[](auto const& item) { return item.allocCount == 0 && item.data; }));

	// Clear all pools except the last one and move the last one to the front.
	if (poolCount > 1)
	{
		for (uSize i = 0; i < poolCount - 1; i += 1)
		{
			auto& pool = memoryPools[i];

			free(pool.data);

			// Clear the struct for good measure.
			pool = {};
		}

		auto& firstPool = memoryPools[0];
		firstPool = memoryPools[poolCount - 1];
		memoryPools[poolCount - 1] = {};
		poolCount = 1;
	}

	memoryPools[0].allocCount = 0;
	offset = 0;

	// For testing purposes
	if constexpr (clearUnusedMemory)
	{
		auto& firstPool = memoryPools[0];
		for (uSize i = 0; i < firstPool.totalSize; i += 1)
			firstPool.data[i] = 0;
	}
}

void Std::FrameAllocator::ReleaseMemory() noexcept
{
	for (uSize i = 0; i < poolCount; i++)
	{
		auto& pool = memoryPools[i];
		DENGINE_IMPL_CONTAINERS_ASSERT(pool.data);

		free(pool.data);
	}
	poolCount = 0;
}
