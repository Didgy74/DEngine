#include "VulkanIncluder.hpp"

#include "DeletionQueue.hpp"
#include "Vk.hpp"

#include "VMAIncluder.hpp"

#include "ImGui/imgui_impl_vulkan.h"

void DEngine::Gfx::Vk::DeletionQueue::Initialize(
	GlobUtils const* globUtilsIn, 
	u8 resourceSetCountIn)
{
	currentResourceSetIndex = 0;
	globUtils = globUtilsIn;
	resourceSetCount = resourceSetCountIn;

	jobQueues.Resize(resourceSetCount);
}

void DEngine::Gfx::Vk::DeletionQueue::ExecuteCurrentTick(DeletionQueue& queue)
{
	std::lock_guard lockGuard{ queue.accessMutex };

	u8 nextResourceSetIndex = (queue.currentResourceSetIndex + 1) % queue.resourceSetCount;

	{
		// First we execute all the deletion-job planned for this tick
		QueueType& currentQueue = queue.jobQueues[queue.currentResourceSetIndex];
		for (auto& item : currentQueue.a)
		{
			item.callback(
				*queue.globUtils,
				{ reinterpret_cast<char*>(currentQueue.b.data()) + item.dataOffset, item.dataSize });
		}
		currentQueue.a.clear();
		currentQueue.b.clear();
		// Then we switch out the active queue with the temporary one
		// This is to make sure we defer deletion-jobs to the next time we
		// hit this resourceSetIndex. If we didn't do it, we would immediately
		// perform all the deletion jobs that was submitted this tick.
		std::swap(currentQueue, queue.tempQueue);
	}



	{
		// We go over the fenced-jobs planned for this tick.
		// For any fence that is not yet signalled, the job it owns and
		// it's VkFence will be pushed to the next tick.
		FencedQueueType& currentQueue = queue.fencedJobQueues[queue.currentFencedJobQueueIndex];
		for (auto& item : currentQueue.a)
		{
			vk::Result waitResult = queue.globUtils->device.GetFenceStatus(item.fence);
			if (waitResult == vk::Result::eSuccess)
			{
				// Fence is ready, execute the job and delete the fence.
				item.job.callback(
					*queue.globUtils, 
					{ reinterpret_cast<char*>(currentQueue.b.data()) + item.job.dataOffset, item.job.dataSize });
				queue.globUtils->device.Destroy(item.fence);
			}
			else if (waitResult == vk::Result::eNotReady)
			{
				// Fence not ready, we push the job to the next tick.
				FencedQueueType& nextQueue = queue.fencedJobQueues[!queue.currentFencedJobQueueIndex];
				nextQueue.a.push_back(item);

				FencedJob& newJob = nextQueue.a.back();
				newJob.job.dataOffset = nextQueue.b.size();

				// Resize the nextQueue's data-buffer to fit this job's custom data.
				uSize addSize = newJob.job.dataSize;
				addSize += ((sizeof(u64)-1) - ((addSize + sizeof(u64)-1) % sizeof(u64)));
				// Divide by 8 to get it in amount of u64
				addSize /= sizeof(u64);

				nextQueue.b.resize(nextQueue.b.size() + addSize);
				
				// Copy the custom data
				std::memcpy(
					nextQueue.b.data() + newJob.job.dataOffset,
					currentQueue.b.data() + item.job.dataOffset,
					item.job.dataSize);
			}
			else
			{
				// If we got any other result, it' an error
				throw std::runtime_error("DEngine - Vulkan: Deletion queue encountered invalid "
										 "VkResult value when waiting for fence of fenced job.");
			}
		}
		currentQueue.a.clear();
		currentQueue.b.clear();
	}

	queue.currentResourceSetIndex = nextResourceSetIndex;
	queue.currentFencedJobQueueIndex = !queue.currentFencedJobQueueIndex;
}

void DEngine::Gfx::Vk::DeletionQueue::DestroyImGuiTexture(void* texId) const
{
	TestCallback<void*> callback = [](GlobUtils const& globUtils, void* texId)
	{
		ImGui_ImplVulkan_RemoveTexture(texId);
	};

	DestroyTest(callback, texId);
}

void DEngine::Gfx::Vk::DeletionQueue::Destroy(VmaAllocation vmaAlloc, vk::Image img) const
{
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

void DEngine::Gfx::Vk::DeletionQueue::Destroy(VmaAllocation vmaAlloc, vk::Buffer buff) const
{
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

namespace DEngine::Gfx::Vk
{
	template<typename T>
	static void DeletionQueue_DestroyHandle(
			DeletionQueue const& queue,
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
			globUtils.device.Destroy(source.handle, source.callbacks);
		};
		queue.DestroyTest(callback, source);
	}

	template<typename T>
	static void DeletionQueue_DestroyHandle(
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
			globUtils.device.Destroy(source.handle, source.callbacks);
		};
		queue.DestroyTest(fence, callback, source);
	}
}

void DEngine::Gfx::Vk::DeletionQueue::Destroy(
	vk::CommandPool cmdPool, 
	Std::Span<vk::CommandBuffer const> cmdBuffers) const
{
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

	CallbackPFN callback = [](GlobUtils const& globUtils, Std::Span<char> customData)
	{
		vk::CommandPool cmdPool = *reinterpret_cast<vk::CommandPool*>(customData.Data() + cmdPool_Offset);

		uSize cmdBufferCount = *reinterpret_cast<uSize*>(customData.Data() + cmdBufferCount_Offset);

		globUtils.device.FreeCommandBuffers(
			cmdPool, 
			{ (u32)cmdBufferCount, reinterpret_cast<vk::CommandBuffer const*>(customData.Data() + cmdBuffers_Offset) });
	};

	QueueType& currentQueue = tempQueue;
	Job newJob{};
	newJob.callback = callback;
	newJob.dataOffset = currentQueue.b.size() * sizeof(u64);
	newJob.dataSize = sizeof(vk::CommandPool) + sizeof(u64) + cmdBuffers.Size() * sizeof(vk::CommandBuffer);
	currentQueue.a.push_back(newJob);

	uSize addSize = newJob.dataSize;
	// Then we pad to align to 8 bytes.
	addSize += ((sizeof(u64)-1) - ((addSize + sizeof(u64)-1) % sizeof(u64)));
	assert(addSize % sizeof(u64) == 0);
	// Divide by 8 to get it in amount of u64
	addSize /= sizeof(u64);

	currentQueue.b.resize(currentQueue.b.size() + addSize);

	// Copy the commandpool handle
	std::memcpy(
		reinterpret_cast<char*>(currentQueue.b.data()) + newJob.dataOffset + cmdPool_Offset,
		&cmdPool,
		sizeof(cmdPool));
	// Copy the cmdBufferCount
	uSize cmdBufferCount = cmdBuffers.Size();
	std::memcpy(
		reinterpret_cast<char*>(currentQueue.b.data()) + newJob.dataOffset + cmdBufferCount_Offset,
		&cmdBufferCount,
		sizeof(cmdBufferCount));
	// Copy the cmdBuffers
	std::memcpy(
		reinterpret_cast<char*>(currentQueue.b.data()) + newJob.dataOffset + cmdBuffers_Offset,
		cmdBuffers.Data(),
		cmdBuffers.Size() * sizeof(vk::CommandBuffer));
}

void DEngine::Gfx::Vk::DeletionQueue::Destroy(
	vk::CommandPool in, 
	vk::Optional<vk::AllocationCallbacks> callbacks) const
{
	DeletionQueue_DestroyHandle(*this, in, callbacks);
}

void DEngine::Gfx::Vk::DeletionQueue::Destroy(
	vk::Fence fence, 
	vk::CommandPool in, 
	vk::Optional<vk::AllocationCallbacks> callbacks) const
{
	DeletionQueue_DestroyHandle(*this, fence, in, callbacks);
}

void DEngine::Gfx::Vk::DeletionQueue::Destroy(
		vk::DescriptorPool in,
		vk::Optional<vk::AllocationCallbacks> callbacks) const
{
	DeletionQueue_DestroyHandle(*this, in, callbacks);
}

void DEngine::Gfx::Vk::DeletionQueue::Destroy(
		vk::Fence fence,
		vk::DescriptorPool in,
		vk::Optional<vk::AllocationCallbacks> callbacks) const
{
	DeletionQueue_DestroyHandle(*this, fence, in, callbacks);
}

void DEngine::Gfx::Vk::DeletionQueue::Destroy(
	vk::Framebuffer in,
	vk::Optional<vk::AllocationCallbacks> callbacks) const
{
	DeletionQueue_DestroyHandle(*this, in, callbacks);
}

void DEngine::Gfx::Vk::DeletionQueue::Destroy(
	vk::ImageView in, 
	vk::Optional<vk::AllocationCallbacks> callbacks) const
{
	DeletionQueue_DestroyHandle(*this, in, callbacks);
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
		globUtils.instance.Destroy(source.handle, source.callbacks);
	};
	DestroyTest(callback, source);
}

void DEngine::Gfx::Vk::DeletionQueue::Destroy(
	vk::SwapchainKHR in, 
	vk::Optional<vk::AllocationCallbacks> callbacks) const
{
	DeletionQueue_DestroyHandle(*this, in, callbacks);
}

void DEngine::Gfx::Vk::DeletionQueue::Destroy(
	CallbackPFN callback, 
	Std::Span<char const> customData) const
{
	std::lock_guard lockGuard{ accessMutex };

	QueueType& currentQueue = tempQueue;
	Job newJob{};
	newJob.callback = callback;
	newJob.dataOffset = currentQueue.b.size() * sizeof(u64);
	newJob.dataSize = customData.Size();
	currentQueue.a.push_back(newJob);

	uSize addSize = newJob.dataSize;
	// Then we pad to align to 8 bytes.
	addSize += ((sizeof(u64)-1) - ((addSize + sizeof(u64)-1) % sizeof(u64)));
	assert(addSize % sizeof(u64) == 0);
	// Divide by 8 to get it in amount of u64
	addSize /= sizeof(u64);

	currentQueue.b.resize(currentQueue.b.size() + addSize);

	// Copy the custom data
	std::memcpy(
		reinterpret_cast<char*>(currentQueue.b.data()) + newJob.dataOffset,
		customData.Data(),
		customData.Size());
}

void DEngine::Gfx::Vk::DeletionQueue::Destroy(
	vk::Fence fence,
	CallbackPFN callback,
	Std::Span<char const> customData) const
{
	std::lock_guard lockGuard{ accessMutex };

	FencedQueueType& currentQueue = fencedJobQueues[currentFencedJobQueueIndex];
	FencedJob newJob{};
	newJob.fence = fence;
	newJob.job.callback = callback;
	newJob.job.dataOffset = currentQueue.b.size() * sizeof(u64);
	newJob.job.dataSize = customData.Size();
	currentQueue.a.push_back(newJob);

	uSize addSize = newJob.job.dataSize;
	// Then we pad to align to 8 bytes.
	addSize += ((sizeof(u64)-1) - ((addSize + sizeof(u64)-1) % sizeof(u64)));
	assert(addSize % sizeof(u64) == 0);
	// Divide by 8 to get it in amount of u64
	addSize /= sizeof(u64);

	currentQueue.b.resize(currentQueue.b.size() + addSize);

	// Copy the custom data
	std::memcpy(
		reinterpret_cast<char*>(currentQueue.b.data()) + newJob.job.dataOffset,
		customData.Data(),
		customData.Size());
}
