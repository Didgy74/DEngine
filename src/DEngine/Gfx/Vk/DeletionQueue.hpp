#pragma once

#include "VulkanIncluder.hpp"
#include "Constants.hpp"
#include "DynamicDispatch.hpp"

#include "DEngine/FixedWidthTypes.hpp"
#include "DEngine/Containers/FixedVector.hpp"

#include <vector>
#include <mutex>

namespace DEngine::Gfx::Vk
{
	class DeletionQueue
	{
	public:
		static constexpr uSize jobBufferSize = 64;

		using CallbackPFN = void(*)(DevDispatch const& device, char const(&buffer)[jobBufferSize]);

		// Do NOT call this, only if you're initializing the entire shit
		inline void Initialize(DevDispatch const& device, u8 resourceSetCount)
		{
			currentResourceSetIndex = 0;
			this->device = &device;
			this->resourceSetCount = resourceSetCount;

			jobQueues.Resize(resourceSetCount);
			for (auto& item : jobQueues)
				item.reserve(25);
			tempQueue.reserve(25);

			fencedJobQueues.Resize(resourceSetCount);
			for (auto& item : fencedJobQueues)
				item.reserve(25);
		}

		inline static void ExecuteCurrentTick(DeletionQueue& queue)
		{
			std::lock_guard lockGuard{ queue.accessMutex };
			u8 nextResourceSetIndex = (queue.currentResourceSetIndex + 1) % queue.resourceSetCount;

			{
				// First we execute all the deletion-job planned for this tick
				std::vector<Job>& currentQueue = queue.jobQueues[queue.currentResourceSetIndex];
				for (auto& item : currentQueue)
				{
					item.callback(*queue.device, item.buffer);
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
					vk::Result waitResult = queue.device->waitForFences({ item.fence }, true, 0);
					if (waitResult == vk::Result::eTimeout)
					{
						// Fence not ready, we push the job to the next tick.
						std::vector<FencedJob>& nextQueue = queue.fencedJobQueues[nextResourceSetIndex];
						nextQueue.push_back(item);
					}
					else if (waitResult == vk::Result::eSuccess)
					{
						// Fence is ready, execute the job and delete the fence.
						item.job.callback(*queue.device, item.job.buffer);
						queue.device->destroyFence(item.fence);
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

		inline void Destroy(CallbackPFN callback, char const(&buffer)[jobBufferSize]) const
		{
			std::lock_guard lockGuard{ accessMutex };

			tempQueue.push_back({});
			Job& newJob = tempQueue.back();
			newJob.callback = callback;
			std::memcpy(newJob.buffer, buffer, jobBufferSize);
		}

		// Warning: Only works with VulkanHPP handles.
		template<typename T>
		inline void Destroy(T objectHandle) const
		{
			char buffer[jobBufferSize] = {};
			std::memcpy(buffer, &objectHandle, sizeof(objectHandle));

			Destroy(&DestroyVulkanHppHandle<T>, buffer);
		}

		// Waits for a fence to be signalled and then executes
		// the job, and afterwards destroys the Fence.
		inline void Destroy(
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

		// Warning: Only works with VulkanHPP handles.
		template<typename T>
		inline void Destroy(
			vk::Fence fence,
			T objectHandle) const
		{
			char buffer[jobBufferSize] = {};
			std::memcpy(buffer, &objectHandle, sizeof(objectHandle));

			Destroy(fence, &DestroyVulkanHppHandle<T>, buffer);
		}

	private:
		DevDispatch const* device{};
		u8 resourceSetCount = 0;
		u8 currentResourceSetIndex = 255;
		mutable std::mutex accessMutex{};
		struct Job
		{
			CallbackPFN callback = nullptr;
			alignas(std::max_align_t) char buffer[jobBufferSize] = {};
		};
		mutable Cont::FixedVector<std::vector<Job>, Constants::maxResourceSets> jobQueues{};
		mutable std::vector<Job> tempQueue{};
		struct FencedJob
		{
			Job job{};
			vk::Fence fence{};
		};
		mutable Cont::FixedVector<std::vector<FencedJob>, Constants::maxResourceSets> fencedJobQueues{};

		template<typename T>
		static void DestroyVulkanHppHandle(DevDispatch const& device, char const(&buffer)[jobBufferSize])
		{
			T objectHandle{};
			std::memcpy(&objectHandle, buffer, sizeof(T));
			device.handle.destroy(objectHandle, nullptr, device.raw);
		}
	};
}