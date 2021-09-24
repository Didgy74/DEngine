#include "VulkanIncluder.hpp"

#include <DEngine/Std/Utility.hpp>

#include "DeletionQueue.hpp"
#include "Vk.hpp"
#include "GlobUtils.hpp"

#include "VMAIncluder.hpp"

using namespace DEngine;
using namespace DEngine::Gfx;
using namespace DEngine::Gfx::Vk;

namespace DEngine::Gfx::Vk::DeletionQueueImpl
{
	constexpr uSize customDataAlignment = sizeof(u64);
}

bool DeletionQueue::Init(DeletionQueue& delQueue, u8 resourceSetCount)
{
	delQueue.jobQueues.Resize(resourceSetCount);

	return true;
}

void DeletionQueue::ExecuteTick(
	DeletionQueue& queue, 
	GlobUtils const& globUtils, 
	u8 currentInFlightIndex)
{
	std::lock_guard lockGuard{ queue.accessMutex };
	
	{
		// First we execute all the deletion-job planned for this tick
		auto& currentQueue = queue.jobQueues[currentInFlightIndex];
		DENGINE_IMPL_GFX_ASSERT((uSize)currentQueue.customData.data() % DeletionQueueImpl::customDataAlignment == 0);
		for (auto const& item : currentQueue.jobs)
		{
			item.callback(
				globUtils,
				{ currentQueue.customData.data() + item.dataOffset, item.dataSize });
		}
		currentQueue.jobs.clear();
		currentQueue.customData.clear();
		// Then we switch out the active queue with the temporary one
		// This is to make sure we defer deletion-jobs to the next time we
		// hit this resourceSetIndex. If we didn't do it, we would immediately
		// perform all the deletion jobs that was submitted this tick.
		std::swap(currentQueue, queue.tempQueue);
	}

	{
		// We go over the fenced-jobs planned for this tick.
		// For any fence that is not yet signalled: The fence, the job it's tied to and
		// it's custom-data will be pushed to the next tick.
		auto& currentQueue = queue.fencedJobQueues[queue.currFencedJobQueueIndex];
		for (auto const& item : currentQueue.jobs)
		{
			vk::Result waitResult = globUtils.device.getFenceStatus(item.fence);
			if (waitResult == vk::Result::eSuccess)
			{
				// Fence is ready, execute the job and delete the fence.
				item.job.callback(
					globUtils, 
					{ currentQueue.customData.data() + item.job.dataOffset, item.job.dataSize });
				globUtils.device.destroy(item.fence);
			}
			else if (waitResult == vk::Result::eNotReady)
			{
				// Fence not ready, we push the job to the next tick.
				auto& nextQueue = queue.fencedJobQueues[!queue.currFencedJobQueueIndex];

				FencedJob newJob{};
				newJob.fence = item.fence;
				newJob.job.callback = item.job.callback;
				newJob.job.dataOffset = nextQueue.customData.size();
				newJob.job.dataSize = item.job.dataSize;
				nextQueue.jobs.push_back(newJob);

				// Then we pad to align to 8 bytes.
				uSize addSize = Math::CeilToMultiple(newJob.job.dataSize, DeletionQueueImpl::customDataAlignment);

				nextQueue.customData.resize(nextQueue.customData.size() + addSize);

				// Copy the custom data
				std::memcpy(
					nextQueue.customData.data() + newJob.job.dataOffset,
					currentQueue.customData.data() + item.job.dataOffset,
					newJob.job.dataSize);
			}
			else
			{
				// If we got any other result, it' an error
				throw std::runtime_error("DEngine - Vulkan: Deletion queue encountered invalid "
										 "VkResult value when waiting for fence of fenced job.");
			}
		}
		currentQueue.jobs.clear();
		currentQueue.customData.clear();
	}

	queue.currFencedJobQueueIndex = !queue.currFencedJobQueueIndex;
}

void Vk::DeletionQueue::Destroy(VmaAllocation vmaAlloc, vk::Image img) const
{
	DENGINE_IMPL_GFX_ASSERT(vmaAlloc != nullptr);
	struct Data
	{
		VmaAllocation alloc{};
		vk::Image img{};
	};
	Data source{};
	source.alloc = vmaAlloc;
	source.img = img;
	TestCallback<Data> callback = [](GlobUtils const& globUtils, Data source)
	{
		vmaDestroyImage(globUtils.vma, static_cast<VkImage>(source.img), source.alloc);
	};
	// Push the job to the queue
	DestroyTest(callback, source);
}

void DeletionQueue::Destroy(VmaAllocation vmaAlloc, vk::Buffer buff) const
{
	DENGINE_IMPL_GFX_ASSERT(vmaAlloc != nullptr);
	struct Data
	{
		VmaAllocation alloc{};
		vk::Buffer buff{};
	};
	Data source{};
	source.alloc = vmaAlloc;
	source.buff = buff;
	TestCallback<Data> callback = [](GlobUtils const& globUtils, Data source)
	{
		vmaDestroyBuffer(globUtils.vma, static_cast<VkBuffer>(source.buff), source.alloc);
	};
	// Push the job to the queue
	DestroyTest(callback, source);
}

namespace DEngine::Gfx::Vk::DeletionQueueImpl
{
	template<typename T>
	static void DestroyDeviceLevelHandle(
			DeletionQueue const& queue,
			T in,
			vk::Optional<vk::AllocationCallbacks> callbacks)
	{
		struct Data
		{
			T handle;
			vk::Optional<vk::AllocationCallbacks> callbacks;
			Data(T handle, vk::Optional<vk::AllocationCallbacks> callbacks) :
				handle(handle), callbacks(callbacks) {}
		};
		Data source(in, callbacks);
		DeletionQueue::TestCallback<Data> callback = [](GlobUtils const& globUtils, Data source)
		{
			globUtils.device.destroy(source.handle, source.callbacks);
		};
		queue.DestroyTest(callback, source);
	}

	template<typename T>
	static void DestroyDeviceLevelHandle(
			DeletionQueue const& queue,
			vk::Fence fence,
			T in,
			vk::Optional<vk::AllocationCallbacks> callbacks)
	{
		struct Data
		{
			T handle{};
			vk::Optional<vk::AllocationCallbacks> callbacks = nullptr;
		};
		Data source{};
		source.handle = in;
		source.callbacks = callbacks;
		DeletionQueue::TestCallback<Data> callback = [](GlobUtils const& globUtils, Data source)
		{
			globUtils.device.destroy(source.handle, source.callbacks);
		};
		queue.DestroyTest(fence, callback, source);
	}
}

void DeletionQueue::Destroy(
	vk::CommandPool cmdPool, 
	Std::Span<vk::CommandBuffer const> cmdBuffers) const
{
	DENGINE_IMPL_GFX_ASSERT(cmdPool != vk::CommandPool());
	DENGINE_IMPL_GFX_ASSERT(
		Std::AllOf(cmdBuffers.AsRange(),
		[](vk::CommandBuffer cmdBuffer) -> bool { return cmdBuffer != vk::CommandBuffer{}; }));

	std::lock_guard lockGuard{ accessMutex };

	// We are storing the data like this:
	//
	// vk::CommandPool cmdPool;
	// uSize cmdBufferCount;
	// If 32-bit: 4 bytes of padding to align to 8
	// vk::CommandBuffer cmdBuffers[cmdBufferCount]
	static constexpr uSize cmdPool_Offset = 0;
	static constexpr uSize cmdBufferCount_Offset = cmdPool_Offset + sizeof(vk::CommandPool);
	static constexpr uSize cmdBuffers_Offset = cmdBufferCount_Offset + sizeof(u64);

	CallbackPFN callback = [](GlobUtils const& globUtils, Std::Span<u8> customData)
	{
		vk::CommandPool cmdPool = *reinterpret_cast<vk::CommandPool*>(customData.Data() + cmdPool_Offset);

		uSize cmdBufferCount = *reinterpret_cast<uSize*>(customData.Data() + cmdBufferCount_Offset);

		globUtils.device.freeCommandBuffers(
			cmdPool, 
			{ (u32)cmdBufferCount, reinterpret_cast<vk::CommandBuffer const*>(customData.Data() + cmdBuffers_Offset) });
	};

	auto& currentQueue = tempQueue;
	Job newJob{};
	newJob.callback = callback;
	newJob.dataOffset = currentQueue.customData.size();
	newJob.dataSize = sizeof(vk::CommandPool) + sizeof(u64) + cmdBuffers.Size() * sizeof(vk::CommandBuffer);
	currentQueue.jobs.push_back(newJob);

	// Then we pad to align to 8 bytes.
	uSize addSize = Math::CeilToMultiple(newJob.dataSize, DeletionQueueImpl::customDataAlignment);

	currentQueue.customData.resize(currentQueue.customData.size() + addSize);

	// Copy the commandpool handle
	std::memcpy(
		reinterpret_cast<char*>(currentQueue.customData.data()) + newJob.dataOffset + cmdPool_Offset,
		&cmdPool,
		sizeof(cmdPool));
	// Copy the cmdBufferCount
	uSize cmdBufferCount = cmdBuffers.Size();
	std::memcpy(
		reinterpret_cast<char*>(currentQueue.customData.data()) + newJob.dataOffset + cmdBufferCount_Offset,
		&cmdBufferCount,
		sizeof(cmdBufferCount));
	// Copy the cmdBuffers
	std::memcpy(
		reinterpret_cast<char*>(currentQueue.customData.data()) + newJob.dataOffset + cmdBuffers_Offset,
		cmdBuffers.Data(),
		cmdBuffers.Size() * sizeof(vk::CommandBuffer));
}

void DeletionQueue::Destroy(
	vk::CommandPool in, 
	vk::Optional<vk::AllocationCallbacks> callbacks) const
{
	DeletionQueueImpl::DestroyDeviceLevelHandle(*this, in, callbacks);
}

void DeletionQueue::Destroy(
	vk::Fence fence, 
	vk::CommandPool in, 
	vk::Optional<vk::AllocationCallbacks> callbacks) const
{
	DENGINE_IMPL_GFX_ASSERT(fence != vk::Fence());
	DENGINE_IMPL_GFX_ASSERT(in != vk::CommandPool());
	DeletionQueueImpl::DestroyDeviceLevelHandle(*this, fence, in, callbacks);
}

void DEngine::Gfx::Vk::DeletionQueue::Destroy(
		vk::DescriptorPool in,
		vk::Optional<vk::AllocationCallbacks> callbacks) const
{
	DeletionQueueImpl::DestroyDeviceLevelHandle(*this, in, callbacks);
}

void DEngine::Gfx::Vk::DeletionQueue::Destroy(
		vk::Fence fence,
		vk::DescriptorPool in,
		vk::Optional<vk::AllocationCallbacks> callbacks) const
{
	DENGINE_IMPL_GFX_ASSERT(fence != vk::Fence());
	DENGINE_IMPL_GFX_ASSERT(in != vk::DescriptorPool());
	DeletionQueueImpl::DestroyDeviceLevelHandle(*this, fence, in, callbacks);
}

void DEngine::Gfx::Vk::DeletionQueue::Destroy(
	vk::Framebuffer in,
	vk::Optional<vk::AllocationCallbacks> callbacks) const
{
	DeletionQueueImpl::DestroyDeviceLevelHandle(*this, in, callbacks);
}

void DEngine::Gfx::Vk::DeletionQueue::Destroy(
	vk::ImageView in, 
	vk::Optional<vk::AllocationCallbacks> callbacks) const
{
	DeletionQueueImpl::DestroyDeviceLevelHandle(*this, in, callbacks);
}

void DEngine::Gfx::Vk::DeletionQueue::Destroy(
	vk::SurfaceKHR in, 
	vk::Optional<vk::AllocationCallbacks> callbacks) const
{
	struct Data
	{
		vk::SurfaceKHR handle{};
		vk::Optional<vk::AllocationCallbacks> callbacks = nullptr;
	};
	Data source{};
	source.handle = in;
	source.callbacks = callbacks;
	DeletionQueue::TestCallback<Data> callback = [](GlobUtils const& globUtils, Data source)
	{
		globUtils.instance.destroy(source.handle, source.callbacks);
	};
	DestroyTest(callback, source);
}

void DEngine::Gfx::Vk::DeletionQueue::Destroy(
	vk::SwapchainKHR in, 
	vk::Optional<vk::AllocationCallbacks> callbacks) const
{
	DeletionQueueImpl::DestroyDeviceLevelHandle(*this, in, callbacks);
}

void DEngine::Gfx::Vk::DeletionQueue::Destroy(
	CallbackPFN callback, 
	Std::Span<u8 const> customData) const
{
	std::lock_guard lockGuard{ accessMutex };

	auto& currentQueue = tempQueue;
	Job newJob{};
	newJob.callback = callback;
	newJob.dataOffset = currentQueue.customData.size();
	newJob.dataSize = customData.Size();
	currentQueue.jobs.push_back(newJob);

	// Then we pad to align to 8 bytes.
	uSize addSize = Math::CeilToMultiple(newJob.dataSize, DeletionQueueImpl::customDataAlignment);

	currentQueue.customData.resize(currentQueue.customData.size() + addSize);

	// Copy the custom data
	std::memcpy(
		currentQueue.customData.data() + newJob.dataOffset,
		customData.Data(),
		customData.Size());
}

void DEngine::Gfx::Vk::DeletionQueue::Destroy(
	vk::Fence fence,
	CallbackPFN callback,
	Std::Span<u8 const> customData) const
{
	std::lock_guard lockGuard{ accessMutex };

	auto& currentQueue = fencedJobQueues[currFencedJobQueueIndex];
	FencedJob newJob{};
	newJob.fence = fence;
	newJob.job.callback = callback;
	newJob.job.dataOffset = currentQueue.customData.size();
	newJob.job.dataSize = customData.Size();
	currentQueue.jobs.push_back(newJob);

	// Then we pad to align to 8 bytes.
	uSize addSize = Math::CeilToMultiple(newJob.job.dataSize, DeletionQueueImpl::customDataAlignment);

	currentQueue.customData.resize(currentQueue.customData.size() + addSize);

	// Copy the custom data
	std::memcpy(
		currentQueue.customData.data() + newJob.job.dataOffset,
		customData.Data(),
		customData.Size());
}
