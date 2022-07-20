#pragma once

#include "VulkanIncluder.hpp"
#include "Constants.hpp"
#include "VMAIncluder.hpp"
#include "ForwardDeclarations.hpp"

#include <DEngine/Gfx/impl/Assert.hpp>
#include <DEngine/FixedWidthTypes.hpp>
#include <DEngine/Std/Containers/StackVec.hpp>
#include <DEngine/Std/Containers/Pair.hpp>
#include <DEngine/Std/Containers/Span.hpp>
#include <DEngine/Std/Trait.hpp>

#include <vector>

namespace DEngine::Gfx::Vk
{
	namespace impl { class DeletionQueueImpl; }

	class DeletionQueue
	{
	public:
		DeletionQueue() = default;
		DeletionQueue(DeletionQueue const&) = delete;
		DeletionQueue(DeletionQueue&&) = delete;
		DeletionQueue& operator=(DeletionQueue const&) = delete;
		DeletionQueue& operator=(DeletionQueue&&) = delete;

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

		using CallbackPFN = void(*)(GlobUtils const& globUtils, Std::Span<char const> customData);

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
			Std::Span<char const> customData);

		// Waits for a fence to be signalled and then executes
		// the job, and afterwards destroys the Fence.
		// Custom data is mem-copied.
		void Destroy(
			vk::Fence fence,
			CallbackPFN callback,
			Std::Span<char const> customData);

		void Destroy(VmaAllocation alloc, vk::Image img);
		void Destroy(vk::Fence fence, VmaAllocation alloc, vk::Image img);
		void Destroy(VmaAllocation alloc, vk::Buffer buffer);
		void Destroy(vk::Fence fence, VmaAllocation alloc, vk::Buffer buffer);

		// Frees the command buffers
		// The contents of the span will be copied into the deletion queue.
		// Does NOT free the commandpool.
		void Destroy(vk::CommandPool cmdPool, Std::Span<vk::CommandBuffer const> commandBuffers);
		void Destroy(vk::CommandPool in);
		void Destroy(vk::Fence fence, vk::CommandPool in);
		void Destroy(vk::DescriptorPool in);
		void Destroy(vk::Fence fence,vk::DescriptorPool in);
		void Destroy(vk::Framebuffer in);
		void Destroy(Std::Span<vk::Framebuffer const> in);
		void Destroy(vk::Fence fence, vk::Framebuffer in);
		void Destroy(vk::ImageView in);
		void Destroy(Std::Span<vk::ImageView const> in);
		void Destroy(vk::Fence fence, vk::ImageView in);
		void Destroy(vk::Semaphore in);

		void Destroy(vk::SurfaceKHR in);
		void Destroy(vk::Fence fence, vk::SurfaceKHR in);
		void Destroy(vk::SwapchainKHR in);
		void Destroy(vk::Fence fence, vk::SwapchainKHR in);

	private:
		struct Job
		{
			CallbackPFN callback = nullptr;
			// Offset in bytes.
			uSize dataOffset = 0;
			// Size in bytes
			uSize dataSize = 0;
		};
		struct InFlightQueue
		{
			std::vector<Job> jobs;
			std::vector<char> customData;
		};
		Std::StackVec<InFlightQueue, Const::maxInFlightCount> jobQueues;
		InFlightQueue tempQueue;

		struct FencedJob
		{
			vk::Fence fence = {};
			Job job = {};
		};
		struct FencedJobQueue
		{
			std::vector<FencedJob> jobs;
			std::vector<char> customData;
		};
		static constexpr int fencedJobQueueCount = 2;
		FencedJobQueue fencedJobQueues[fencedJobQueueCount];
		int currFencedJobQueueIndex = 0;

		friend class impl::DeletionQueueImpl;
	};

	using DelQueue = DeletionQueue;

	template<typename T>
	void DeletionQueue::DestroyTest(
		TestCallback<T> callback, 
		T const& customData)
	{
		struct TempData
		{
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
		struct TempData
		{
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