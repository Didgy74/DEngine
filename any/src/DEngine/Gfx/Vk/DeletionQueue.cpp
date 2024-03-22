#include "VulkanIncluder.hpp"

#include <DEngine/Std/Utility.hpp>
#include <DEngine/Std/Containers/Vec.hpp>

#include "DeletionQueue.hpp"
#include "Vk.hpp"
#include "GlobUtils.hpp"

#include "VMAIncluder.hpp"

using namespace DEngine;
using namespace DEngine::Gfx;
using namespace DEngine::Gfx::Vk;

namespace DEngine::Gfx::Vk::impl
{


	class DeletionQueueImpl
	{
	public:
		static void FlushQueue(
			DeletionQueue::InFlightQueue& queue,
			GlobUtils const& globUtils)
		{
			DENGINE_IMPL_GFX_ASSERT((uSize)queue.customData.data() % DeletionQueue::customDataAlignment == 0);

			auto jobCount = queue.fnList.Size();
			for (int i = 0; i < jobCount; i++) {
				auto const& jobFn = queue.fnList.At(i);
				auto const& jobData = queue.jobs[i];
				jobFn.Invoke(
					globUtils,
					{ queue.customData.data() + jobData.dataOffset, jobData.dataSize });
			}
			queue.fnList.Clear();
			queue.jobs.clear();
			queue.customData.clear();
		}

		static void FlushFencedQueue(
			DeletionQueue::FencedJobQueue& currQueue,
			DeletionQueue::FencedJobQueue& nextQueue,
			GlobUtils const& globUtils)
		{
			DENGINE_IMPL_GFX_ASSERT(&currQueue != &nextQueue);
			DENGINE_IMPL_GFX_ASSERT(currQueue.jobs.empty());
			/*
			auto jobCount = currQueue.jobs.size();
			for (int i = 0; i < jobCount; i++) {
				auto
			}



			// We go over the fenced-jobs planned for this tick.
			// For any fence that is not yet signalled: The fence, the job it's tied to and
			// its custom-data will be pushed to the next tick.
			for (auto const& item : currQueue.jobs)
			{
				vk::Result waitResult = globUtils.device.getFenceStatus(item.fence);
				if (waitResult == vk::Result::eSuccess) {


					// Fence is ready, execute the job and delete the fence.
					item.job.callback(
						globUtils,
						{ currQueue.customData.data() + item.job.dataOffset, item.job.dataSize });
					globUtils.device.Destroy(item.fence);
				}
				else if (waitResult == vk::Result::eNotReady)
				{
					// Fence not ready, we push the job to the next tick.
					DeletionQueue::FencedJob newJob;
					newJob.fence = item.fence;
					newJob.job.callback = item.job.callback;
					newJob.job.dataOffset = nextQueue.customData.size();
					newJob.job.dataSize = item.job.dataSize;
					nextQueue.jobs.push_back(newJob);

					// Then we pad to align to 8 bytes.
					uSize addSize = Math::CeilToMultiple((u64)newJob.job.dataSize, (u64)customDataAlignment);

					nextQueue.customData.resize(nextQueue.customData.size() + addSize);

					// Copy the custom data
					std::memcpy(
						nextQueue.customData.data() + newJob.job.dataOffset,
						currQueue.customData.data() + item.job.dataOffset,
						newJob.job.dataSize);
				}
				else
				{
					// If we got any other result, it' an error
					throw std::runtime_error("DEngine - Vulkan: Deletion queue encountered invalid "
											 "VkResult value when waiting for fence of fenced job.");
				}
			}
			currQueue.jobs.clear();
			currQueue.customData.clear();
			 */
		}

		static void FlushFencedQueue_All(
			DeletionQueue::FencedJobQueue& queue,
			GlobUtils const& globUtils)
		{
			DENGINE_IMPL_GFX_UNREACHABLE();
			/*
			for (auto const& item : queue.jobs)
			{
				vk::Result waitResult = globUtils.device.getFenceStatus(item.fence);
				if (waitResult == vk::Result::eSuccess)
				{
					// Fence is ready, execute the job and delete the fence.
					item.job.callback(
						globUtils,
						{ queue.customData.data() + item.job.dataOffset, item.job.dataSize });
					globUtils.device.Destroy(item.fence);
				}
				else
					DENGINE_IMPL_GFX_UNREACHABLE();
			}

			queue.jobs.clear();
			queue.customData.clear();
			 */
		}




		template<class T>
		static inline void DestroyInstanceLevelHandle(
			DeletionQueue& queue,
			T in)
		{
			DENGINE_IMPL_GFX_ASSERT(in != T{});

			using CType = typename T::CType;
			DeletionQueue::CallbackPFN callback = [](GlobUtils const& globUtils, Std::Span<char const> customData)
			{
				DENGINE_IMPL_GFX_ASSERT(!customData.Empty());
				DENGINE_IMPL_GFX_ASSERT(customData.Size() == sizeof(CType));
				DENGINE_IMPL_GFX_ASSERT((std::intptr_t)(customData.Data()) % alignof(CType) == 0);
				CType cHandle = *reinterpret_cast<CType const*>(customData.Data());
				globUtils.instance.Destroy((T)cHandle);
			};

			auto const cHandle = (CType)in;
			Std::Span span = { &cHandle, 1 };
			queue.Destroy(callback, span.ToConstByteSpan());
		}

		template<typename T>
		static inline auto DestroyDeviceLevelHandle(
			DeletionQueue& queue,
			T in)
		{
			DENGINE_IMPL_GFX_ASSERT(in != T{});
			return queue.DestroyInternal(
				[=](GlobUtils const& globUtils, Std::ConstByteSpan customData) {
					globUtils.device.Destroy(in);
				},
				{});
		}
		template<typename T>
		static inline auto DestroyDeviceLevelHandle(
			DeletionQueue& queue,
			Std::Span<T const> in)
		{
			DENGINE_IMPL_GFX_ASSERT(
				Std::AllOf(
					in.AsRange(),
					[](auto const& cmdBuffer) { return cmdBuffer != T{}; }));
			return queue.DestroyInternal(
				[=](GlobUtils const& globUtils, Std::ConstByteSpan customData) {
					auto items = customData.Cast<T const>();
					for (auto const& item : items)
						globUtils.device.Destroy(item);
				},
				in.ToConstByteSpan());
		}

		template<class T>
		static void DestroyDeviceLevelHandle(
			DeletionQueue& queue,
			vk::Fence fence,
			T in)
		{
			DENGINE_IMPL_GFX_UNREACHABLE();
			DENGINE_IMPL_GFX_ASSERT(in != T{});

			using CType = typename T::CType;

			DeletionQueue::CallbackPFN callback = [](GlobUtils const& globUtils, Std::Span<char const> customData)
			{
				DENGINE_IMPL_GFX_ASSERT(!customData.Empty());
				DENGINE_IMPL_GFX_ASSERT(customData.Size() == sizeof(CType));
				DENGINE_IMPL_GFX_ASSERT((std::intptr_t)(customData.Data()) % alignof(CType) == 0);
				CType cHandle = *reinterpret_cast<CType const*>(customData.Data());
				globUtils.device.Destroy((T)cHandle);
			};

			auto const cHandle = (CType)in;
			Std::Span span = { &cHandle, 1 };
			queue.Destroy(fence, callback, span.ToConstByteSpan());
		}
	};
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
	{
		// First we execute all the deletion-job planned for this tick
		auto& currentQueue = queue.jobQueues[currentInFlightIndex];
		impl::DeletionQueueImpl::FlushQueue(currentQueue, globUtils);
		// Then we switch out the active queue with the temporary one
		// This is to make sure we defer deletion-jobs to the next time we
		// hit this resourceSetIndex. If we didn't do it, we would immediately
		// perform all the deletion jobs that was submitted this tick.
		Std::Swap(currentQueue, queue.tempQueue);
	}

	int nextFencedQueueIndex = (queue.currFencedJobQueueIndex + 1) % DeletionQueue::fencedJobQueueCount;
	{
		impl::DeletionQueueImpl::FlushFencedQueue(
			queue.fencedJobQueues[queue.currFencedJobQueueIndex],
			queue.fencedJobQueues[nextFencedQueueIndex],
			globUtils);
	}
	queue.currFencedJobQueueIndex = nextFencedQueueIndex;
}

void DeletionQueue::FlushAllJobs(
	DeletionQueue& queue,
	GlobUtils const& globUtils)
{
	for (int i = 0; i < queue.jobQueues.Size(); i += 1) {
		auto& innerQueue = queue.jobQueues[i];
		impl::DeletionQueueImpl::FlushQueue(innerQueue, globUtils);
	}

	for (int i = 0; i < DeletionQueue::fencedJobQueueCount; i += 1)
	{
		auto& innerQueue = queue.fencedJobQueues[i];
		impl::DeletionQueueImpl::FlushFencedQueue_All(innerQueue, globUtils);
	}
}

void Vk::DeletionQueue::Destroy(VmaAllocation vmaAlloc, vk::Image img)
{
	DENGINE_IMPL_GFX_ASSERT(vmaAlloc != nullptr);
	DENGINE_IMPL_GFX_ASSERT(img != vk::Image{});
	this->DestroyInternal(
		[=](GlobUtils const& globUtils, auto) {
			vmaDestroyImage(globUtils.vma, static_cast<VkImage>(img), vmaAlloc);
		},
		{});
}

void DeletionQueue::Destroy(VmaAllocation vmaAlloc, vk::Buffer buffer)
{
	DENGINE_IMPL_GFX_ASSERT(vmaAlloc != nullptr);
	DENGINE_IMPL_GFX_ASSERT(buffer != vk::Buffer{});
	this->DestroyInternal(
		[=](GlobUtils const& globUtils, Std::ConstByteSpan) {
			vmaDestroyBuffer(globUtils.vma, (VkBuffer)buffer, vmaAlloc);
		},
		{});
}

void DeletionQueue::Destroy(
	vk::CommandPool cmdPool,
	Std::Span<vk::CommandBuffer const> cmdBuffersIn)
{
	DENGINE_IMPL_GFX_ASSERT(cmdPool != vk::CommandPool());
	DENGINE_IMPL_GFX_ASSERT(
		Std::AllOf(
			cmdBuffersIn.AsRange(),
			[](auto const& cmdBuffer) { return cmdBuffer != vk::CommandBuffer{}; }));

	this->DestroyInternal(
		[=](GlobUtils const& globUtils, Std::ConstByteSpan customData) {
			auto items = customData.Cast<vk::CommandBuffer const>();
			globUtils.device.Free(
				cmdPool,
				{ (u32)items.Size(), items.Data() });
		},
		cmdBuffersIn.ToConstByteSpan());
}

void DeletionQueue::Destroy(
	vk::CommandPool in)
{
	impl::DeletionQueueImpl::DestroyDeviceLevelHandle(*this, in);
}

void DeletionQueue::Destroy(
	vk::Fence fence, 
	vk::CommandPool in)
{
	impl::DeletionQueueImpl::DestroyDeviceLevelHandle(*this, fence, in);
}

void DeletionQueue::FreeDescriptorSets(vk::DescriptorPool in, Std::Span<vk::DescriptorSet const> descrSetsIn)
{
	DENGINE_IMPL_GFX_ASSERT(in != vk::DescriptorPool{});
	DENGINE_IMPL_GFX_ASSERT(
		Std::AllOf(
			descrSetsIn.AsRange(),
			[](auto const& item) { return item != vk::DescriptorSet{}; }));

	this->DestroyInternal(
		[=](GlobUtils const& globUtils, Std::ConstByteSpan customData) {
			auto items = customData.Cast<vk::DescriptorSet const>();
			globUtils.device.Free(
				in,
				{ (u32)items.Size(), items.Data() });
		},
		descrSetsIn.ToConstByteSpan());

}

void DeletionQueue::Destroy(
	vk::DescriptorPool in)
{
	impl::DeletionQueueImpl::DestroyDeviceLevelHandle(*this, in);
}

void DeletionQueue::Destroy(
	vk::Fence fence,
	vk::DescriptorPool in)
{
	impl::DeletionQueueImpl::DestroyDeviceLevelHandle(*this, fence, in);
}

void DeletionQueue::Destroy(vk::Framebuffer in) {
	impl::DeletionQueueImpl::DestroyDeviceLevelHandle(*this, in);
}

void DeletionQueue::Destroy(Std::Span<vk::Framebuffer const> in) {
	impl::DeletionQueueImpl::DestroyDeviceLevelHandle(*this, in);
}

void DeletionQueue::Destroy(vk::ImageView in) {
	impl::DeletionQueueImpl::DestroyDeviceLevelHandle(*this, in);
}

void DeletionQueue::Destroy(Std::Span<vk::ImageView const> in) {
	impl::DeletionQueueImpl::DestroyDeviceLevelHandle(*this, in);
}

void DeletionQueue::Destroy(
	vk::Semaphore in)
{
	impl::DeletionQueueImpl::DestroyDeviceLevelHandle(*this, in);
}

void DeletionQueue::Destroy(
	vk::SurfaceKHR in)
{
	impl::DeletionQueueImpl::DestroyInstanceLevelHandle(*this, in);
}

void DeletionQueue::Destroy(
	vk::SwapchainKHR in)
{
	impl::DeletionQueueImpl::DestroyDeviceLevelHandle(*this, in);
}

void DeletionQueue::Destroy(
	CallbackPFN callback, 
	Std::ConstByteSpan customData)
{
	this->DestroyInternal(
		Std::Move(callback),
		customData);
}

void DeletionQueue::Destroy(
	vk::Fence fence,
	CallbackPFN callback,
	Std::Span<char const> customData)
{
	DENGINE_IMPL_GFX_UNREACHABLE();
	/*
	auto& currentQueue = fencedJobQueues[currFencedJobQueueIndex];
	FencedJob newJob = {};
	newJob.fence = fence;
	newJob.job.dataOffset = currentQueue.customData.size();
	newJob.job.dataSize = customData.Size();
	currentQueue.jobs.emplace_back(newJob);

	// Then we pad to align to 8 bytes.
	uSize addSize = Math::CeilToMultiple((u64)newJob.job.dataSize, (u64)impl::customDataAlignment);

	currentQueue.customData.resize(currentQueue.customData.size() + addSize);

	// Copy the custom data
	std::memcpy(
		currentQueue.customData.data() + newJob.job.dataOffset,
		customData.Data(),
		customData.Size());
	 */
}