#pragma once

#include "VulkanIncluder.hpp"
#include "Constants.hpp"
#include "VMAIncluder.hpp"
#include "ForwardDeclarations.hpp"
#include "DynamicDispatch.hpp"
#include "RaiiHandles.hpp"

#include <DEngine/Gfx/impl/Assert.hpp>
#include <DEngine/Std/Containers/FnScratchList.hpp>

#include <vector>
#include <concepts>

import DEngine.Std.StackVec;

namespace DEngine::Gfx::Vk
{
	namespace impl { class DeletionQueueImpl; }

	class DeletionQueue {
	public:
		DeletionQueue() = default;
		DeletionQueue(DeletionQueue const&) = delete;
		DeletionQueue(DeletionQueue&&) = delete;
		DeletionQueue& operator=(DeletionQueue const&) = delete;
		DeletionQueue& operator=(DeletionQueue&&) = delete;

		// Controls how we should align custom data in our custom-data vector.
		static constexpr auto customDataAlignment = sizeof(u64);

		template<typename T> requires Std::Trait::isTrivial<T>
		using TestCallback = void(*)(GlobUtils const& globUtils, T const& customData);

		template<typename T>
		inline void DestroyTest(
			TestCallback<T> callback, 
			T const& customData);
		template<typename T>
		inline void DestroyTest(
			vk::Fence fence,
			TestCallback<T> callback, 
			T const& customData);

		using CallbackPFN = void(*)(GlobUtils const& globUtils, Std::ConstByteSpan customData);

		// Do NOT call this, only if you're initializing the entire shit
		[[nodiscard]] static bool Init(
			DeletionQueue& delQueue,
			u8 resourceSetCount);

		static void ExecuteTick(
			DeletionQueue& queue,
			GlobUtils const& globUtils,
			u8 currentInFlightIndex);

		static void FlushAllJobs(
			DeletionQueue& queue,
			GlobUtils const& globUtils);

		// Custom data is mem-copied.
		void Destroy(
			CallbackPFN callback, 
			Std::ConstByteSpan customData);

		// Waits for a fence to be signalled and then executes
		// the job, and afterwards destroys the Fence.
		// Custom data is mem-copied.
		void Destroy(
			vk::Fence fence,
			CallbackPFN callback,
			Std::Span<char const> customData);

		void Destroy(VmaAllocation, vk::Image);
		void Destroy(vk::Fence fence, VmaAllocation alloc, vk::Image img);
		void Destroy(BoxVmaImg&&);
		void Destroy(VmaAllocation alloc, vk::Buffer buffer);
		void Destroy(vk::Fence fence, VmaAllocation alloc, vk::Buffer buffer);
		void Destroy(BoxVmaBuffer&&);

		// Frees the command buffers
		// The contents of the span will be copied into the deletion queue.
		// Does NOT free the commandpool.
		void Destroy(vk::CommandPool cmdPool, Std::Span<vk::CommandBuffer const> commandBuffers);
		void Destroy(vk::CommandPool in);
		void Destroy(vk::Fence fence, vk::CommandPool in);
		void FreeDescriptorSets(vk::DescriptorPool in, Std::Span<vk::DescriptorSet const> descrSets);
		void Destroy(vk::DescriptorPool in);
		void Destroy(vk::Fence fence, vk::DescriptorPool in);
		void Destroy(vk::Framebuffer in);
		void Destroy(Std::Span<vk::Framebuffer const> in);
		void Destroy(vk::Fence fence, vk::Framebuffer in);
		void Destroy(vk::ImageView);
		void Destroy(BoxVkImageView&&);
		void Destroy(Std::Span<vk::ImageView const> in);
		void Destroy(vk::Fence fence, vk::ImageView in);
		void Destroy(vk::Semaphore);
		void Destroy(BoxVkSemaphore&&);
		void Destroy(vk::SurfaceKHR in);
		void Destroy(vk::Fence fence, vk::SurfaceKHR in);
		void Destroy(vk::SwapchainKHR in);
		void Destroy(vk::Fence fence, vk::SwapchainKHR in);

		template<class T>
		void Destroy(BoxVkHandle<T>&& in) {
			DENGINE_IMPL_GFX_ASSERT(!in.IsNull());
			Destroy(in.Release());
		}

	private:
		u64 idTracker = 0;

		struct Job {
			// Offset in bytes.
			uSize dataOffset = 0;
			// Size in bytes
			uSize dataSize = 0;
			u64 id = 0;
		};
		struct InFlightQueue {
			std::vector<char> customData;
			std::vector<Job> jobs;
			Std::FnScratchList<GlobUtils const&, Std::ConstByteSpan> fnList;
		};
		Std::StackVec<InFlightQueue, Const::maxInFlightCount> jobQueues;
		InFlightQueue tempQueue;

		struct FencedJob {
			vk::Fence fence = {};
			Job job = {};
		};
		struct FencedJobQueue {
			std::vector<FencedJob> jobs;
			std::vector<char> customData;
		};
		static constexpr int fencedJobQueueCount = 2;
		FencedJobQueue fencedJobQueues[fencedJobQueueCount];
		int currFencedJobQueueIndex = 0;

		friend class impl::DeletionQueueImpl;

		[[nodiscard]] static DeviceDispatch DeviceDispatchFromGlobUtils(GlobUtils const&) noexcept;

		template<typename T>
		auto DestroyDeviceLevelHandle(T in) {
			DENGINE_IMPL_GFX_ASSERT(in != T{});
			return this->DestroyInternal(
				[=](GlobUtils const& globUtils, Std::ConstByteSpan) {
					auto const& device = DeviceDispatchFromGlobUtils(globUtils);
					device.Destroy(in);
				},
				{});
		}

		template<class CallableT>
		auto DestroyInternal(
			CallableT&& callable,
			Std::ConstByteSpan data)
		{
			auto& currentQueue = this->tempQueue;

			auto inputSize = data.Size();
			auto offset = (int)Std::CeilToMultiple((u32)currentQueue.customData.size(), (u32)customDataAlignment);
			auto newCustomDataSize = offset + inputSize;
			currentQueue.customData.resize(newCustomDataSize);

			std::memcpy(
				(char*)currentQueue.customData.data() + offset,
				data.Data(),
				inputSize);

			Job newJob = {};
			newJob.dataOffset = offset;
			newJob.dataSize = inputSize;
			newJob.id = this->idTracker;
			currentQueue.jobs.push_back(newJob);
			currentQueue.fnList.Push(Std::Move(callable));

			this->idTracker++;

			return idTracker;
		}
	};

	using DelQueue = DeletionQueue;

	template<typename T>
	void DeletionQueue::DestroyTest(
		TestCallback<T> callback, 
		T const& customData)
	{
		struct TempData {
			TestCallback<T> callback;
			T customData;
		};
		TempData tempData {
			.callback = callback,
			.customData = customData };
		CallbackPFN wrapperFunc = [](GlobUtils const& globUtils, Std::Span<char const> customData)
		{
			DENGINE_IMPL_GFX_ASSERT(reinterpret_cast<uSize>(customData.Data()) % alignof(TempData) == 0);
			DENGINE_IMPL_GFX_ASSERT(sizeof(TempData) == customData.Size());
			auto const& tempData = *reinterpret_cast<TempData const*>(customData.Data());
			tempData.callback(globUtils, tempData.customData);
		};

		Std::Span span = { &tempData, 1 };
		Destroy(wrapperFunc, span.ToConstByteSpan());
	}

	template<class T>
	inline void DeletionQueue::DestroyTest(
		vk::Fence fence,
		TestCallback<T> callback, 
		T const& customData)
	{
		struct TempData {
			TestCallback<T> callback;
			T customData;
		};
		TempData tempData {
			.callback = callback,
			.customData = customData };
		CallbackPFN wrapperFunc = [](GlobUtils const& globUtils, Std::Span<char const> customData)
		{
			DENGINE_IMPL_GFX_ASSERT(reinterpret_cast<uSize>(customData.Data()) % alignof(TempData) == 0);
			DENGINE_IMPL_GFX_ASSERT(sizeof(TempData) == customData.Size());
			auto& tempData = *reinterpret_cast<TempData const*>(customData.Data());
			tempData.callback(globUtils, tempData.customData);
		};
		Std::Span span = { &tempData, 1 };
		Destroy(fence, wrapperFunc, span.ToConstByteSpan());
	}
}