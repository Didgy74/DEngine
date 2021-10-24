#pragma once

#include <DEngine/Gfx/Gfx.hpp>

#include "VulkanIncluder.hpp"
#include "Constants.hpp"
#include "VMAIncluder.hpp"

#include <DEngine/Std/FrameAllocator.hpp>
#include <DEngine/Std/Containers/StackVec.hpp>
#include <DEngine/Math/Matrix.hpp>

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
		struct CreateJob
		{
			NativeWindowID id;
		};
		struct InsertionJobsT
		{
			std::mutex lock;
			std::vector<CreateJob> queue;
		};
		InsertionJobsT insertionJobs;
		// Insertion locked resources end

		struct Node
		{
			NativeWindowID id;
			NativeWindowData windowData;
			WindowGuiData gui;
		};
		struct MainT
		{
			std::mutex lock;
			std::vector<Node> nativeWindows;
		};
		MainT main;

		static void ProcessEvents(
			NativeWindowManager& manager,
			GlobUtils const& globUtils,
			Std::FrameAlloc& transientAlloc,
			Std::Span<NativeWindowUpdate const> windowUpdates);

		struct InitInfo
		{
			NativeWindowManager* manager;
			NativeWindowID initialWindow = {};
			DeviceDispatch const* device;
			QueueData const* queues;
			DebugUtilsDispatch const* optional_debugUtils;
		};
		static void Initialize(
			InitInfo const& initInfo);

		static void PushCreateWindowJob(
			NativeWindowManager& manager,
			NativeWindowID windowId) noexcept;
	};
}