#include "StagingBufferAlloc.hpp"

import DEngine.Std.Common;

using namespace DEngine::Gfx::Vk;

auto StagingBufferAlloc::Alloc_Internal(
	StagingBufferAlloc& alloc,
	DeviceDispatch const& device,
	Std::Span<char const> bytes)
	-> SubAlloc_Return
{
	DENGINE_IMPL_GFX_ASSERT(alloc.finalized == false);

	auto offset = alloc.nextOffset;
	DENGINE_IMPL_GFX_ASSERT(offset + bytes.Size() < alloc.vmaAllocInfo.size);

	DENGINE_IMPL_GFX_ASSERT(alloc.vmaAllocInfo.pMappedData != nullptr);
	DENGINE_IMPL_GFX_ASSERT(bytes.Data() != nullptr);
	DENGINE_IMPL_GFX_ASSERT(bytes.Size() != 0);
	std::memcpy((char*)alloc.vmaAllocInfo.pMappedData + offset, bytes.Data(), bytes.Size());

	SubAlloc_Return out = {};
	out.buffer = alloc.bufferHandle;
	out.bufferOffset = (int)offset;

	alloc.nextOffset += bytes.Size();

	return out;
}

auto StagingBufferAlloc::Alloc2_Internal(
	StagingBufferAlloc& alloc,
	DeviceDispatch const& device,
	unsigned int size,
	unsigned int bufferOffsetAlignment)
	-> SubAlloc2_Return
{
	DENGINE_IMPL_GFX_ASSERT(alloc.finalized == false);

	auto offset = Std::CeilToMultiple((u64)alloc.nextOffset, (u64)bufferOffsetAlignment);
	alloc.nextOffset = offset + size;
	DENGINE_IMPL_GFX_ASSERT(offset + size < alloc.vmaAllocInfo.size);
	DENGINE_IMPL_GFX_ASSERT(alloc.vmaAllocInfo.pMappedData != nullptr);

	SubAlloc2_Return out = {};
	out.buffer = alloc.bufferHandle;
	out.bufferOffset = (int)offset;
	out.bufferSize = size;
	out.mappedMem = Std::ByteSpan{ (char*)(alloc.vmaAllocInfo.pMappedData) + offset, size };
	return out;
}

void StagingBufferAlloc::BuildInPlace(
	StagingBufferAlloc& outAlloc,
	DeviceDispatch const& device,
	VmaAllocator vma)
{
	vk::BufferCreateInfo bufferInfo = {};
	bufferInfo.size = StagingBufferAlloc::minCapacity;
	bufferInfo.sharingMode = vk::SharingMode::eExclusive;
	bufferInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;
	VmaAllocationCreateInfo allocCreateInfo = {};
	allocCreateInfo.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO;
	allocCreateInfo.flags =
		VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_MAPPED_BIT
		| VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

	vk::Buffer bufferOut = {};
	VmaAllocation alloc = {};
	VmaAllocationInfo allocInfo = {};
	auto vkResult = (vk::Result)vmaCreateBuffer(
		vma,
		(VkBufferCreateInfo const*)&bufferInfo,
		&allocCreateInfo,
		(VkBuffer*)&bufferOut,
		&alloc,
		&allocInfo);
	if (vkResult != vk::Result::eSuccess) {
		throw std::runtime_error("");
	}

	outAlloc.bufferHandle = bufferOut;
	outAlloc.vmaAlloc = alloc;
	outAlloc.vmaAllocInfo = allocInfo;
}

void StagingBufferAlloc::Reset(StagingBufferAlloc& alloc) {
	alloc.finalized = false;
	alloc.nextOffset = 0;
}

void StagingBufferAlloc::Finalize(StagingBufferAlloc& alloc, VmaAllocator& vma) {
	DENGINE_IMPL_GFX_ASSERT(alloc.finalized == false);
	vmaFlushAllocation(
		vma,
		alloc.vmaAlloc,
		0,
		alloc.nextOffset);
	alloc.finalized = true;
}