#pragma once

#include <DEngine/Gfx/Gfx.hpp>

#include "VulkanIncluder.hpp"
#include "Constants.hpp"
#include "VMAIncluder.hpp"

#include <DEngine/Std/Containers/StackVec.hpp>

#include <vector>
#include <mutex>

namespace DEngine::Gfx::Vk
{
	class InstanceDispatch;
	class DeviceDispatch;
	class DebugUtilsDispatch;
	class DeletionQueue;
	class QueueData;
	class GlobUtils;
	struct SurfaceInfo;

	struct NativeWindowData
	{
		WsiInterface* wsiConnection = nullptr;
		vk::SurfaceKHR surface{};
		vk::SwapchainKHR swapchain{};
		Std::StackVec<vk::Image, Constants::maxSwapchainLength> swapchainImages{};
		// Has length of resource sets
		Std::StackVec<vk::CommandBuffer, Constants::maxInFlightCount> copyCmdBuffers{};
		vk::Semaphore swapchainImageReady{};
	};

	struct WindowGuiData
	{
		vk::Extent2D extent{};
		VmaAllocation vmaAllocation{};
		vk::Image img{};
		vk::ImageView imgView{};
		vk::Framebuffer framebuffer{};

		vk::SurfaceTransformFlagBitsKHR surfaceRotation = vk::SurfaceTransformFlagBitsKHR::eIdentity;
		Math::Mat2 rotation{};

		vk::CommandPool cmdPool{};
		Std::StackVec<vk::CommandBuffer, Constants::maxInFlightCount> cmdBuffers{};
	};

	struct NativeWindowManager
	{
		std::mutex lock;

		u64 nativeWindowIdTracker = 0;

		struct CreateJob
		{
			NativeWindowID id;
			WsiInterface* windowConnection;
		};
		std::vector<CreateJob> createJobs;

		struct Node
		{
			NativeWindowID id;
			NativeWindowData windowData;
			WindowGuiData gui;
		};
		std::vector<Node> nativeWindows;

		vk::CommandPool copyToSwapchainCmdPool{};

		// This requires the manager's mutex to already be locked.
		static void Update(
			NativeWindowManager& manager,
			GlobUtils const& globUtils,
			Std::Span<NativeWindowUpdate const> windowUpdates);

		static void Initialize(
			NativeWindowManager& manager,
			DeviceDispatch const& device,
			QueueData const& queues,
			DebugUtilsDispatch const* debugUtils);

		static void BuildInitialNativeWindow(
			NativeWindowManager& manager,
			InstanceDispatch const& instance,
			DeviceDispatch const& device,
			vk::PhysicalDevice physDevice,
			QueueData const& queues,
			DeletionQueue const& deletionQueue,
			SurfaceInfo const& surfaceInfo,
			VmaAllocator vma,
			u8 inFlightCount,
			WsiInterface& initialWindowConnection,
			vk::SurfaceKHR surface,
			vk::RenderPass guiRenderPass,
			DebugUtilsDispatch const* debugUtils);

		static NativeWindowID PushCreateWindowJob(
			NativeWindowManager& manager,
			WsiInterface& windowConnection);
	};
}