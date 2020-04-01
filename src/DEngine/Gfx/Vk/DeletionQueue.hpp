#pragma once

#include "VulkanIncluder.hpp"
#include "Constants.hpp"
#include "DynamicDispatch.hpp"

#include "DEngine/FixedWidthTypes.hpp"
#include "DEngine/Containers/StaticVector.hpp"
#include "DEngine/Containers/Optional.hpp"

#include "VMAIncluder.hpp"

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
		void Initialize(
			void const* globUtilsIn,
			u8 resourceSetCountIn);

		static void ExecuteCurrentTick(DeletionQueue& queue);

		void Destroy(
			CallbackPFN callback, 
			char const(&buffer)[jobBufferSize]) const;

		// Waits for a fence to be signalled and then executes
		// the job, and afterwards destroys the Fence.
		void Destroy(
			vk::Fence fence,
			CallbackPFN callback,
			char const(&buffer)[jobBufferSize]) const;

		void Destroy(VmaAllocation alloc, vk::Image img) const;
		void Destroy(vk::Fence fence, VmaAllocation alloc, vk::Image img) const;
		void Destroy(VmaAllocation alloc, vk::Buffer buffer) const;
		void Destroy(vk::Fence fence, VmaAllocation alloc, vk::Buffer buffer) const;

		void Destroy(vk::CommandPool commandPool, vk::Optional<vk::AllocationCallbacks> callbacks = nullptr) const;
		void Destroy(vk::Fence fence, vk::CommandPool commandPool, vk::Optional<vk::AllocationCallbacks> callbacks = nullptr) const;
		void Destroy(vk::Framebuffer in, vk::Optional<vk::AllocationCallbacks> callbacks = nullptr) const;
		void Destroy(vk::Fence fence, vk::Framebuffer in, vk::Optional<vk::AllocationCallbacks> callbacks = nullptr) const;
		void Destroy(vk::ImageView in, vk::Optional<vk::AllocationCallbacks> callbacks = nullptr) const;
		void Destroy(vk::Fence in, vk::Optional<vk::AllocationCallbacks> callbacks = nullptr) const;



	private:
		void const* globUtils = nullptr;
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
	};
}