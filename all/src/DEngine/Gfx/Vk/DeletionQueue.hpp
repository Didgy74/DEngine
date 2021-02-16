#pragma once

#include "VulkanIncluder.hpp"
#include "Constants.hpp"
#include "DynamicDispatch.hpp"

#include <DEngine/Gfx/detail/Assert.hpp>
#include <DEngine/FixedWidthTypes.hpp>
#include <DEngine/Std/Containers/StackVec.hpp>
#include <DEngine/Std/Containers/Pair.hpp>

#include "VMAIncluder.hpp"

#include <vector>
#include <mutex>

namespace DEngine::Gfx::Vk
{
	class GlobUtils;

	class DeletionQueue
	{
	public:
		DeletionQueue() = default;
		DeletionQueue(DeletionQueue const&) = delete;
		DeletionQueue(DeletionQueue&&) = delete;
		DeletionQueue& operator=(DeletionQueue const&) = delete;
		DeletionQueue& operator=(DeletionQueue&&) = delete;

		template<typename T>
		using TestCallback = void(*)(GlobUtils const& globUtils, T customData);
		template<typename T>
		inline void DestroyTest(
			TestCallback<T> callback, 
			T const& customData) const;
		template<typename T>
		inline void DestroyTest(
			vk::Fence fence,
			TestCallback<T> callback, 
			T const& customData) const;

		using CallbackPFN = void(*)(GlobUtils const& globUtils, Std::Span<u8> customData);

		// Do NOT call this, only if you're initializing the entire shit
		[[nodiscard]] static bool Init(
			DeletionQueue& delQueue,
			u8 resourceSetCount);

		static void ExecuteCurrentTick(
			DeletionQueue& queue,
			GlobUtils const& globUtils,
			u8 currentInFlightIndex);

		// Custom data is mem-copied.
		void Destroy(
			CallbackPFN callback, 
			Std::Span<u8 const> customData) const;

		// Waits for a fence to be signalled and then executes
		// the job, and afterwards destroys the Fence.
		// Custom data is mem-copied.
		void Destroy(
			vk::Fence fence,
			CallbackPFN callback,
			Std::Span<u8 const> customData) const;

		void Destroy(VmaAllocation alloc, vk::Image img) const;
		void Destroy(vk::Fence fence, VmaAllocation alloc, vk::Image img) const;
		void Destroy(VmaAllocation alloc, vk::Buffer buffer) const;
		void Destroy(vk::Fence fence, VmaAllocation alloc, vk::Buffer buffer) const;

		// Frees the command buffers
		// The contents of the span will be copied into the deletion queue.
		// Does NOT free the commandpool.
		void Destroy(vk::CommandPool cmdPool, Std::Span<vk::CommandBuffer const> commandBuffers) const;

		void Destroy(vk::CommandPool in, vk::Optional<vk::AllocationCallbacks> callbacks = nullptr) const;
		void Destroy(vk::Fence fence, vk::CommandPool in, vk::Optional<vk::AllocationCallbacks> callbacks = nullptr) const;
		void Destroy(vk::DescriptorPool in, vk::Optional<vk::AllocationCallbacks> callbacks = nullptr) const;
		void Destroy(vk::Fence fence,vk::DescriptorPool in, vk::Optional<vk::AllocationCallbacks> callbacks = nullptr) const;
		void Destroy(vk::Framebuffer in, vk::Optional<vk::AllocationCallbacks> callbacks = nullptr) const;
		void Destroy(vk::Fence fence, vk::Framebuffer in, vk::Optional<vk::AllocationCallbacks> callbacks = nullptr) const;
		void Destroy(vk::ImageView in, vk::Optional<vk::AllocationCallbacks> callbacks = nullptr) const;
		void Destroy(vk::Fence fence, vk::ImageView in, vk::Optional<vk::AllocationCallbacks> callbacks = nullptr) const;

		void Destroy(vk::SurfaceKHR in, vk::Optional<vk::AllocationCallbacks> callbacks = nullptr) const;
		void Destroy(vk::Fence fence, vk::SurfaceKHR in, vk::Optional<vk::AllocationCallbacks> callbacks = nullptr) const;
		void Destroy(vk::SwapchainKHR in, vk::Optional<vk::AllocationCallbacks> callbacks = nullptr) const;
		void Destroy(vk::Fence fence, vk::SwapchainKHR in, vk::Optional<vk::AllocationCallbacks> callbacks = nullptr) const;

	private:
		mutable std::mutex accessMutex{};
		struct Job
		{
			CallbackPFN callback = nullptr;
			// Offset in bytes.
			uSize dataOffset = 0;
			// Size in bytes
			uSize dataSize = 0;
		};
		// First is the custom-data vector. We use u64 for alignment purposes.
		struct InFlightQueue
		{
			std::vector<Job> jobs;
			std::vector<u8> customData;
		};
		mutable Std::StackVec<InFlightQueue, Constants::maxInFlightCount> jobQueues{};
		mutable InFlightQueue tempQueue{};

		struct FencedJob
		{
			vk::Fence fence{};
			Job job{};
		};
		struct FencedJobQueue
		{
			std::vector<FencedJob> jobs;
			std::vector<u8> customData;
		};
		mutable FencedJobQueue fencedJobQueues[2];
		bool currFencedJobQueueIndex = 0;
	};

	template<typename T>
	void DeletionQueue::DestroyTest(
		TestCallback<T> callback, 
		T const& customData) const
	{
		struct TempData
		{
			TestCallback<T> callback;
			T customData;
			TempData(TestCallback<T> callback, T const& customData) :
				callback(callback),
				customData(customData) {}
		};
		TempData tempData(callback, customData);
		CallbackPFN wrapperFunc = [](GlobUtils const& globUtils, Std::Span<u8> customData)
		{
			DENGINE_DETAIL_GFX_ASSERT(reinterpret_cast<uSize>(customData.Data()) % alignof(TempData) == 0);
			DENGINE_DETAIL_GFX_ASSERT(sizeof(TempData) == customData.Size());
			TempData& tempData = *reinterpret_cast<TempData*>(customData.Data());
			tempData.callback(globUtils, tempData.customData);
		};
		Destroy(wrapperFunc, { reinterpret_cast<u8 const*>(&tempData), sizeof(tempData) });
	}

	template<typename T>
	inline void DeletionQueue::DestroyTest(
		vk::Fence fence,
		TestCallback<T> callback, 
		T const& customData) const
	{
		struct TempData
		{
			TestCallback<T> callback = nullptr;
			T customData;
		};
		TempData tempData{};
		tempData.callback = callback;
		tempData.customData = customData;
		CallbackPFN wrapperFunc = [](GlobUtils const& globUtils, Std::Span<u8> customData)
		{
			DENGINE_DETAIL_GFX_ASSERT(reinterpret_cast<uSize>(customData.Data()) % alignof(TempData) == 0);
			DENGINE_DETAIL_GFX_ASSERT(sizeof(TempData) == customData.Size());
			TempData& tempData = *reinterpret_cast<TempData*>(customData.Data());
			tempData.callback(globUtils, tempData.customData);
		};
		Destroy(fence, wrapperFunc, { reinterpret_cast<u8 const*>(&tempData), sizeof(tempData) });
	}
}