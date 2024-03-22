#pragma once

#include "VulkanIncluder.hpp"
#include "VMAIncluder.hpp"
#include "DynamicDispatch.hpp"

#include <DEngine/Std/Containers/Span.hpp>

namespace DEngine::Gfx::Vk {
	struct StagingBufferAlloc {

		StagingBufferAlloc() = default;
		StagingBufferAlloc(StagingBufferAlloc&&) = delete;
		StagingBufferAlloc(StagingBufferAlloc const&) = delete;

		static constexpr int minCapacity = 1024 * 1024 * 4;

		struct Alloc_Return {
			vk::Buffer buffer = {};
			int bufferOffset = 0;
			int bufferSize = 0;
			vk::DeviceMemory memory = {};
			int memoryOffset = 0;
			Std::ByteSpan mappedMem = {};

			[[nodiscard]] int BufferOffset() const { return bufferOffset; }
			[[nodiscard]] int BufferSize() const { return bufferSize; }
			[[nodiscard]] Std::ByteSpan MappedMem() const { return mappedMem; }

		};
		// This does not need to be deleted, it is automatically cleaned up.
		// This buffer is NOT unique or new.
		[[nodiscard]] Alloc_Return Alloc(DeviceDispatch const& device, int size, int align) {
			return Alloc_Internal(*this, device, size, align);
		}

		static void BuildInPlace(
			StagingBufferAlloc& alloc,
			DeviceDispatch const& device,
			VmaAllocator vma);

		static void Reset(StagingBufferAlloc& alloc);

	private:
		[[nodiscard]] static Alloc_Return Alloc_Internal(
			StagingBufferAlloc&,
			DeviceDispatch const& device,
			int size,
			int align);

		vk::Buffer bufferHandle = {};
		VmaAllocation vmaAlloc = {};
		VmaAllocationInfo vmaAllocResultInfo = {};
		Std::ByteSpan mappedMemory = {};
		uSize nextOffset = 0;
		uSize capacity = 0;
	};
}

