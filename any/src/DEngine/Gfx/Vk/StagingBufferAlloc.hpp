#pragma once

#include "VMAIncluder.hpp"
#include "DynamicDispatch.hpp"

namespace DEngine::Gfx::Vk {
	/**
		We keep around in-flight-count number of staging-buffer-allocs. Each StagingBufferAlloc
		will act as a single linear allocator. One VkBuffer. It contains all the
		staging data for a single frame. We copy all our staging memory into this buffer
		and then schedule copies into device-local memory.

		TODO: We might want to have a larger StagingBufferAllocator for the initialization/
		first frame when we a lot of stuff to transfer, then shrink it down.

		TODO: We have not implemented support for growing this buffer.
	 */
	struct StagingBufferAlloc {
		StagingBufferAlloc() = default;
		StagingBufferAlloc(StagingBufferAlloc&&) = delete;
		StagingBufferAlloc(StagingBufferAlloc const&) = delete;

		// 16 MB
		static constexpr int minCapacity = 1024 * 1024 * 16;

		struct SubAlloc_Return {
			vk::Buffer buffer = {};
			int bufferOffset = 0;

			[[nodiscard]] vk::Buffer Buffer() const { return buffer; }
			[[nodiscard]] int BufferOffset() const { return bufferOffset; }
		};

		/**
			The point of this function is to both allocate and copy data into the staging buffer.

			The return value includes the VkBuffer and the bufferOffset for where to find this
			memory within the staging-buffer, so that the caller can schedule the memory transfers.

			TODO: Do we need to care about alignment?
		 */
		[[nodiscard]] SubAlloc_Return SubAlloc(
			DeviceDispatch const& device,
			Std::Span<char const> bytes)
		{
			return Alloc_Internal(*this, device, bytes);
		}

		struct SubAlloc2_Return {
			vk::Buffer buffer = {};
			unsigned int bufferOffset = 0;
			unsigned int bufferSize = 0;
			Std::ByteSpan mappedMem = {};

			[[nodiscard]] vk::Buffer Buffer() const { return buffer; }
			[[nodiscard]] auto BufferOffset() const { return bufferOffset; }
			[[nodiscard]] auto BufferSize() const { return bufferSize; }
			[[nodiscard]] Std::ByteSpan MappedMem() const { return mappedMem; }
		};

		/**
			The point of this function is to pre-allocate space in the staging buffer.

			The caller is responsible for copying data into the mapped memory.

			The mapped memory should NEVER be read. The caller should only ever do
			sequential reads into the memory. Preferably in as few memcpy commands
			as possible.
		*/
		[[nodiscard]] SubAlloc2_Return SubAlloc2(
			DeviceDispatch const& device,
			unsigned int size,
			unsigned int bufferOffsetAlignment)
		{
			return Alloc2_Internal(*this, device, size, bufferOffsetAlignment);
		}

		static void BuildInPlace(
			StagingBufferAlloc& alloc,
			DeviceDispatch const& device,
			VmaAllocator vma);

		// This should be called at the beginning of recording an in-flight frame.
		static void Reset(StagingBufferAlloc& alloc);

		// Finalized should be called before we submit the main command buffer
		// for the in-flight frame associated with this StagingBufferAlloc.
		// Being finalized implies we are no longer allowed to do allocations.
		static void Finalize(StagingBufferAlloc& alloc, VmaAllocator& vma);

	protected:
		[[nodiscard]] static SubAlloc_Return Alloc_Internal(
			StagingBufferAlloc&,
			DeviceDispatch const& device,
			Std::Span<char const> bytes);
		[[nodiscard]] static SubAlloc2_Return Alloc2_Internal(
			StagingBufferAlloc&,
			DeviceDispatch const& device,
			unsigned int size,
			unsigned int bufferOffsetAlignment);

		vk::Buffer bufferHandle = {};
		VmaAllocation vmaAlloc = {};
		VmaAllocationInfo vmaAllocInfo = {};
		uSize nextOffset = 0;

		bool finalized = true;
	};
}

