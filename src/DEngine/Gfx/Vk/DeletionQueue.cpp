#include "DeletionQueue.hpp"
#include "Vk.hpp"

void DEngine::Gfx::Vk::DeletionQueue::ExecuteCurrentTick(DeletionQueue& queue)
{
	std::lock_guard lockGuard{ queue.accessMutex };
	u8 nextResourceSetIndex = (queue.currentResourceSetIndex + 1) % queue.resourceSetCount;

	{
		// First we execute all the deletion-job planned for this tick
		std::vector<Job>& currentQueue = queue.jobQueues[queue.currentResourceSetIndex];
		for (auto& item : currentQueue)
		{
			item.callback(*queue.globUtils, item.buffer);
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
			vk::Result waitResult = queue.globUtils->device.waitForFences({ item.fence }, true, 0);
			if (waitResult == vk::Result::eTimeout)
			{
				// Fence not ready, we push the job to the next tick.
				std::vector<FencedJob>& nextQueue = queue.fencedJobQueues[nextResourceSetIndex];
				nextQueue.push_back(item);
			}
			else if (waitResult == vk::Result::eSuccess)
			{
				// Fence is ready, execute the job and delete the fence.
				item.job.callback(*queue.globUtils, item.job.buffer);
				queue.globUtils->device.destroyFence(item.fence);
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