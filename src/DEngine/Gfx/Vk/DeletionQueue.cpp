#include "DeletionQueue.hpp"
#include "Vk.hpp"

#include "VMAIncluder.hpp"

void DEngine::Gfx::Vk::DeletionQueue::Initialize(
	void const* globUtilsIn, 
	u8 resourceSetCountIn)
{
	currentResourceSetIndex = 0;
	this->globUtils = globUtilsIn;
	this->resourceSetCount = resourceSetCountIn;

	jobQueues.Resize(resourceSetCountIn);
	for (auto& item : jobQueues)
		item.reserve(25);
	tempQueue.reserve(25);

	fencedJobQueues.Resize(resourceSetCountIn);
	for (auto& item : fencedJobQueues)
		item.reserve(25);
}

void DEngine::Gfx::Vk::DeletionQueue::ExecuteCurrentTick(DeletionQueue& queue)
{
	std::lock_guard lockGuard{ queue.accessMutex };
	u8 nextResourceSetIndex = (queue.currentResourceSetIndex + 1) % queue.resourceSetCount;

	{
		// First we execute all the deletion-job planned for this tick
		std::vector<Job>& currentQueue = queue.jobQueues[queue.currentResourceSetIndex];
		for (auto& item : currentQueue)
		{
			item.callback(*reinterpret_cast<GlobUtils const*>(queue.globUtils), item.buffer);
		}
		currentQueue.clear();
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
		std::vector<FencedJob>& currentQueue = queue.fencedJobQueues[queue.currentResourceSetIndex];
		for (auto& item : currentQueue)
		{
			vk::Result waitResult = reinterpret_cast<GlobUtils const*>(queue.globUtils)->device.getFenceStatus(item.fence);
			if (waitResult == vk::Result::eSuccess)
			{
				// Fence is ready, execute the job and delete the fence.
				item.job.callback(*reinterpret_cast<GlobUtils const*>(queue.globUtils), item.job.buffer);
				reinterpret_cast<GlobUtils const*>(queue.globUtils)->device.Destroy(item.fence);
			}
			else if (waitResult == vk::Result::eNotReady)
			{
				// Fence not ready, we push the job to the next tick.
				std::vector<FencedJob>& nextQueue = queue.fencedJobQueues[nextResourceSetIndex];
				nextQueue.push_back(item);
			}
			else
			{
				// If we got any other result, it' an error
				throw std::runtime_error("DEngine - Vulkan: Deletion queue encountered invalid \
										  VkResult value when waiting for fence of fenced job.");
			}
		}
		currentQueue.clear();
	}

	queue.currentResourceSetIndex = nextResourceSetIndex;
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
	char buffer[jobBufferSize] = {};
	std::memcpy(buffer, &source, sizeof(source));
	CallbackPFN callback = [](GlobUtils const& globUtils, char const(&buffer)[jobBufferSize])
	{
		Data source{};
		std::memcpy(&source, buffer, sizeof(source));
		vmaDestroyImage(globUtils.vma, static_cast<VkImage>(source.img), source.alloc);
	};
	// Push the job to the queue
	Destroy(callback, buffer);
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
	char buffer[jobBufferSize] = {};
	std::memcpy(buffer, &source, sizeof(source));
	CallbackPFN callback = [](GlobUtils const& globUtils, char const(&buffer)[jobBufferSize])
	{
		Data source{};
		std::memcpy(&source, buffer, sizeof(source));
		vmaDestroyBuffer(globUtils.vma, static_cast<VkBuffer>(source.buff), source.alloc);
	};

	// Push the job to the queue
	Destroy(callback, buffer);
}

#define DENGINE_GFX_VK_DELETIONQUEUE_MAKEDESTROYFUNC_FENCED(typeName) \
	struct Data { vk::##typeName handle{}; vk::Optional<vk::AllocationCallbacks> callbacks = nullptr; }; \
	Data source{}; \
	source.handle = in; \
	source.callbacks = callbacks; \
	char buffer[jobBufferSize] = {}; \
	std::memcpy(buffer, &source, sizeof(source)); \
	CallbackPFN callback = [](GlobUtils const& globUtils, char const(&buffer)[jobBufferSize]) \
	{ \
		Data source{}; \
		std::memcpy(&source, buffer, sizeof(source)); \
		globUtils.device.Destroy(source.handle, source.callbacks); \
	}; \
	Destroy(callback, buffer); \

#define DENGINE_GFX_VK_DELETIONQUEUE_MAKEDESTROYFUNC(typeName) \
	struct Data { vk::##typeName handle{}; vk::Optional<vk::AllocationCallbacks> callbacks = nullptr; }; \
	Data source{}; \
	source.handle = in; \
	source.callbacks = callbacks; \
	char buffer[jobBufferSize] = {}; \
	std::memcpy(buffer, &source, sizeof(source)); \
	CallbackPFN callback = [](GlobUtils const& globUtils, char const(&buffer)[jobBufferSize]) \
	{ \
		Data source{}; \
		std::memcpy(&source, buffer, sizeof(source)); \
		globUtils.device.Destroy(source.handle, source.callbacks); \
	}; \
	Destroy(callback, buffer); \

void DEngine::Gfx::Vk::DeletionQueue::Destroy(
	vk::CommandPool in, 
	vk::Optional<vk::AllocationCallbacks> callbacks) const
{
	DENGINE_GFX_VK_DELETIONQUEUE_MAKEDESTROYFUNC(CommandPool)
}

void DEngine::Gfx::Vk::DeletionQueue::Destroy(
	vk::Fence fence, 
	vk::CommandPool commandPool, 
	vk::Optional<vk::AllocationCallbacks> callbacks) const
{
}

void DEngine::Gfx::Vk::DeletionQueue::Destroy(
		vk::Framebuffer in,
		vk::Optional<vk::AllocationCallbacks> callbacks) const
{
	DENGINE_GFX_VK_DELETIONQUEUE_MAKEDESTROYFUNC(Framebuffer)
}

void DEngine::Gfx::Vk::DeletionQueue::Destroy(
	vk::ImageView in, 
	vk::Optional<vk::AllocationCallbacks> callbacks) const
{
	DENGINE_GFX_VK_DELETIONQUEUE_MAKEDESTROYFUNC(ImageView)
}

void DEngine::Gfx::Vk::DeletionQueue::Destroy(CallbackPFN callback, char const(&buffer)[jobBufferSize]) const
{
	std::lock_guard lockGuard{ accessMutex };

	tempQueue.push_back({});
	Job& newJob = tempQueue.back();
	newJob.callback = callback;
	std::memcpy(newJob.buffer, buffer, jobBufferSize);
}

void DEngine::Gfx::Vk::DeletionQueue::Destroy(
	vk::Fence fence,
	CallbackPFN callback,
	char const(&buffer)[jobBufferSize]) const
{
	std::lock_guard lockGuard{ accessMutex };

	FencedJob newJob{};
	newJob.fence = fence;
	newJob.job.callback = callback;
	std::memcpy(newJob.job.buffer, buffer, jobBufferSize);

	std::vector<FencedJob>& currentQueue = fencedJobQueues[currentResourceSetIndex];
	currentQueue.push_back(newJob);
}