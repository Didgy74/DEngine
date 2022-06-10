#pragma once

#include <DEngine/Gfx/Gfx.hpp>

#include "VulkanIncluder.hpp"
#include "Constants.hpp"
#include "VMAIncluder.hpp"
#include "ForwardDeclarations.hpp"

#include <DEngine/Std/FrameAllocator.hpp>
#include <DEngine/Std/Containers/AllocRef.hpp>
#include <DEngine/Std/Containers/StackVec.hpp>
#include <DEngine/Math/Matrix.hpp>

#include <vector>
#include <mutex>

namespace DEngine::Gfx::Vk
{
	class NativeWinMgr_WindowData
	{
	public:
		vk::SurfaceKHR surface = {};
		vk::SwapchainKHR swapchain = {};
		Std::StackVec<vk::Image, Const::maxSwapchainLength> swapchainImages;
		Std::StackVec<vk::ImageView, Const::maxSwapchainLength> swapchainImgViews;
		vk::Semaphore swapchainImgReadySem = {};
		Std::StackVec<vk::Framebuffer, Const::maxSwapchainLength> framebuffers;
	};

	class NativeWinMgr_WindowGuiData
	{
	public:
		vk::Extent2D extent = {};
		vk::SurfaceTransformFlagBitsKHR surfaceRotation = vk::SurfaceTransformFlagBitsKHR::eIdentity;
		Math::Mat2 rotation = {};
	};

	// Native Window Manager
	class NativeWinMgr
	{
	public:
		// Insertion job resources
		struct CreateJob
		{
			NativeWindowID id;
		};
		struct DeleteJob
		{
			NativeWindowID id;
		};
		struct InsertionJobsT
		{
			std::mutex lock;
			std::vector<CreateJob> createQueue;
			std::vector<DeleteJob> deleteQueue;
		};
		InsertionJobsT insertionJobs;
		// Insertion locked resources end

		struct Node
		{
			NativeWindowID id = {};
			NativeWinMgr_WindowData windowData = {};
			NativeWinMgr_WindowGuiData gui = {};
		};
		struct MainT
		{
			//std::mutex lock;
			std::vector<Node> nativeWindows;
		};
		MainT main;

		static void ProcessEvents(
			NativeWinMgr& manager,
			GlobUtils const& globUtils,
			DeletionQueue& delQueue,
			Std::AllocRef const& transientAlloc,
			Std::Span<NativeWindowUpdate const> windowUpdates);

		struct InitInfo
		{
			NativeWinMgr& manager;
			NativeWindowID initialWindow;
			DeviceDispatch const& device;
			QueueData const& queues;
			DebugUtilsDispatch const* optional_debugUtils;
		};
		static void Initialize(
			InitInfo const& initInfo);

		static void Destroy(
			NativeWinMgr& manager,
			InstanceDispatch const& instance,
			DeviceDispatch const& device);
	};

	void NativeWinMgr_PushCreateWindowJob(
		NativeWinMgr& manager,
		NativeWindowID windowId);

	void NativeWinMgr_PushDeleteWindowJob(
		NativeWinMgr& manager,
		NativeWindowID windowId);
}