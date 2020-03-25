#pragma once

#include "VulkanIncluder.hpp"
#include "Constants.hpp"
#include "DynamicDispatch.hpp"

#include "DEngine/FixedWidthTypes.hpp"
#include "DEngine/Containers/StaticVector.hpp"

#include <vector>
#include <mutex>

namespace DEngine::Gfx::Vk
{
	struct GlobUtils;

	class DeletionQueue
	{
	public:
		static constexpr uSize jobBufferSize = 128;

		using CallbackPFN = void(*)(GlobUtils const& globUtils, char const(&buffer)[jobBufferSize]);

		// Do NOT call this, only if you're initializing the entire shit
		inline void Initialize(GlobUtils const& globUtilsIn, u8 resourceSetCountIn)
		{
			currentResourceSetIndex = 0;
			this->globUtils = &globUtilsIn;
			this->resourceSetCount = resourceSetCountIn;

			jobQueues.Resize(resourceSetCountIn);
			for (auto& item : jobQueues)
				item.reserve(25);
			tempQueue.reserve(25);

			fencedJobQueues.Resize(resourceSetCountIn);
			for (auto& item : fencedJobQueues)
				item.reserve(25);
		}

		static void ExecuteCurrentTick(DeletionQueue& queue);

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
		GlobUtils const* globUtils = nullptr;
		u8 resourceSetCount = 0;
		u8 currentResourceSetIndex = 255;
		mutable std::mutex accessMutex{};
		struct Job
		{
			CallbackPFN callback = nullptr;
			alignas(std::max_align_t) char buffer[jobBufferSize] = {};
		};
		mutable Std::StaticVector<std::vector<Job>, Constants::maxResourceSets> jobQueues{};
		mutable std::vector<Job> tempQueue{};
		struct FencedJob
		{
			Job job{};
			vk::Fence fence{};
		};
		mutable Std::StaticVector<std::vector<FencedJob>, Constants::maxResourceSets> fencedJobQueues{};

		template<typename T>
		static inline void DestroyVulkanHppHandle(GlobUtils const& globUtils, char const(&buffer)[jobBufferSize])
		{
			T objectHandle{};
			std::memcpy(&objectHandle, buffer, sizeof(T));
			//globUtils.device.handle.destroy(objectHandle, nullptr, globUtils.device.raw);
		}
	};
}