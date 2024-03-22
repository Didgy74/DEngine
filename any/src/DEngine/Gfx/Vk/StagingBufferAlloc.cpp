#include "StagingBufferAlloc.hpp"

#include <DEngine/Math/Common.hpp>

using namespace DEngine::Gfx::Vk;

auto StagingBufferAlloc::Alloc_Internal(
	StagingBufferAlloc& alloc,
	DeviceDispatch const& device,
	int size,
	int align)
	-> Alloc_Return
{
	auto tempOffset = Math::CeilToMultiple((u32)alloc.nextOffset, (u32)align);

	if (tempOffset + size <= alloc.capacity) {
		// Allocate stuff

		Alloc_Return out = {};
		out.buffer = alloc.bufferHandle;
		out.bufferOffset = (int)tempOffset;
		out.bufferSize = (int)size;
		out.mappedMem = alloc.mappedMemory.Subspan(tempOffset, size);
		out.memory = alloc.vmaAllocResultInfo.deviceMemory;
		out.memoryOffset = (int)alloc.vmaAllocResultInfo.offset;

		alloc.nextOffset = tempOffset + size;
		return out;
	}

	DENGINE_IMPL_GFX_UNREACHABLE();
}

void StagingBufferAlloc::BuildInPlace(
	StagingBufferAlloc& alloc,
	DeviceDispatch const& device,
	VmaAllocator vma)
{
	vk::BufferCreateInfo bufferInfo = {};
	bufferInfo.size = StagingBufferAlloc::minCapacity;
	bufferInfo.sharingMode = vk::SharingMode::eExclusive;
	bufferInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;
	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
	allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

	vk::Buffer bufferOut = {};
	VmaAllocation vmaAllocation = {};
	VmaAllocationInfo vmaResultInfo = {};
	auto vkResult = (vk::Result)vmaCreateBuffer(
		vma,
		(VkBufferCreateInfo const*)&bufferInfo,
		&allocInfo,
		(VkBuffer*)&bufferOut,
		&vmaAllocation,
		&vmaResultInfo);
	if (vkResult != vk::Result::eSuccess) {
		throw std::runtime_error("");
	}

	alloc.bufferHandle = bufferOut;
	alloc.capacity = bufferInfo.size;
	alloc.vmaAlloc = vmaAllocation;
	alloc.vmaAllocResultInfo = vmaResultInfo;
	alloc.mappedMemory = { (char*)vmaResultInfo.pMappedData, bufferInfo.size };
}

void StagingBufferAlloc::Reset(StagingBufferAlloc& alloc) {
	alloc.nextOffset = 0;
}