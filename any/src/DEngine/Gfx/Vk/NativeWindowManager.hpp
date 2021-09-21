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
		vk::SurfaceKHR surface = {};
		vk::SwapchainKHR swapchain = {};
		Std::StackVec<vk::Image, Const::maxSwapchainLength> swapchainImages;
		Std::StackVec<vk::ImageView, Const::maxSwapchainLength> swapchainImgViews;
		vk::Semaphore swapchainImgReadySem = {};
		Std::StackVec<vk::Framebuffer, Const::maxSwapchainLength> framebuffers;
	};

	struct WindowGuiData
	{
		vk::Extent2D extent{};

		vk::SurfaceTransformFlagBitsKHR surfaceRotation = vk::SurfaceTransformFlagBitsKHR::eIdentity;
		Math::Mat2 rotation{};
	};

	struct NativeWindowManager
	{
		// Insertion job resources
		std::mutex createQueueLock;
		u64 nativeWindowIdTracker = 0;
		struct CreateJob
		{
			NativeWindowID id;
			WsiInterface* windowConnection;
		};
		std::vector<CreateJob> createJobs;
		// Insertion locked resources end


		// Main data resources start
		// This doesn't need a lock because it's ever accessed
		// from the rendering thread.
		struct Node
		{
			NativeWindowID id;
			NativeWindowData windowData;
			WindowGuiData gui;
		};
		std::vector<Node> nativeWindows;
		// Main data resources end

		// This requires the manager's mutex to already be locked.
		static void ProcessEvents(
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
			DeletionQueue const& delQueue,
			SurfaceInfo const& surfaceInfo,
			VmaAllocator vma,
			u8 inFlightCount,
			WsiInterface& initialWindowConnection,
			vk::SurfaceKHR surface,
			vk::RenderPass guiRenderPass,
			DebugUtilsDispatch const* debugUtils);

		[[nodiscard]] static NativeWindowID PushCreateWindowJob(
			NativeWindowManager& manager,
			WsiInterface& windowConnection);
	};
}