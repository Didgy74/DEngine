#pragma once

#include <DEngine/Gfx/Gfx.hpp>

#include "VulkanIncluder.hpp"
#include "Constants.hpp"
#include "VMAIncluder.hpp"
#include "ForwardDeclarations.hpp"

#include <DEngine/Std/BumpAllocator.hpp>
#include <DEngine/Std/Containers/AllocRef.hpp>
#include <DEngine/Std/Containers/StackVec.hpp>
#include <DEngine/Math/Matrix.hpp>

#include <vector>
#include <mutex>

namespace DEngine::Gfx::Vk
{
	struct NativeWinMgr_SwapchainSettings {
		vk::PresentModeKHR presentMode = {};
		vk::SurfaceFormatKHR surfaceFormat = {};
		vk::SurfaceTransformFlagBitsKHR transform = {};
		vk::CompositeAlphaFlagBitsKHR compositeAlphaFlag = {};
		vk::Extent2D extents = {};
		u32 numImages = {};
	};

	class NativeWinMgr_WindowData {
	public:
		vk::SurfaceKHR surface = {};
		vk::SwapchainKHR swapchain = {};
		bool tagOutOfDate = false;
		// Contains the original swapchain settings for this swapchain. C
		// Can be reused to avoid requerying stuff
		NativeWinMgr_SwapchainSettings swapchainSettings = {};
		Std::StackVec<vk::Image, Const::maxSwapchainLength> swapchainImages;
		Std::StackVec<vk::ImageView, Const::maxSwapchainLength> swapchainImgViews;
		vk::Semaphore swapchainImgReadySem = {};
		Std::StackVec<vk::Framebuffer, Const::maxSwapchainLength> framebuffers;

		vk::Extent2D extent = {};
		vk::SurfaceTransformFlagBitsKHR surfaceTransform = {};

		[[nodiscard]] auto GfxRotation() const {
			using Out = DEngine::Gfx::WindowRotation;
			using T = vk::SurfaceTransformFlagBitsKHR;
			// Gfx::Rotation uses counter-clockwise degrees, VkSurfaceTransform uses counter-clockwise.
			switch (this->surfaceTransform) {
				case T::eIdentity: return Out::e0;
				case T::eRotate90: return Out::e270;
				case T::eRotate180: return Out::e180;
				case T::eRotate270: return Out::e90;
				default: return (Out)-1;
			}
		}

	};

	// Native Window Manager
	class NativeWinMgr {
	public:
		// Insertion job resources
		struct CreateJob {
			NativeWindowID id;
			Std::Opt<vk::SurfaceKHR> surface;
		};
		struct DeleteJob {
			NativeWindowID id;
		};
		struct InsertionJobsT {
			std::mutex lock;
			std::vector<CreateJob> createQueue;
			std::vector<DeleteJob> deleteQueue;
		};
		InsertionJobsT insertionJobs;
		// Insertion locked resources end

		struct Node {
			NativeWindowID id = {};
			NativeWinMgr_WindowData windowData = {};
		};
		struct MainT {
			//std::mutex lock;
			std::vector<Node> nativeWindows;
		};
		MainT main;

		[[nodiscard]] NativeWinMgr_WindowData const& GetWindowData(NativeWindowID in) const;
		[[nodiscard]] NativeWinMgr_WindowData& GetWindowData(NativeWindowID in);
		[[nodiscard]] auto const& GetWindowData(int index) const { return main.nativeWindows[index]; }

		[[nodiscard]] int WindowCount() const { return (int)main.nativeWindows.size(); }
		static void TagSwapchainOutOfDate(NativeWinMgr& manager, NativeWindowID in);

		static void ProcessEvents(
			NativeWinMgr& manager,
			GlobUtils const& globUtils,
			DeletionQueue& delQueue,
			Std::AllocRef const& transientAlloc,
			Std::Span<NativeWindowUpdate const> windowUpdates);

		struct InitInfo {
			NativeWinMgr& manager;
			NativeWindowID initialWindow;
			vk::SurfaceKHR surface;
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
		NativeWindowID windowId,
		Std::Opt<vk::SurfaceKHR> const& surface = Std::nullOpt);

	void NativeWinMgr_PushDeleteWindowJob(
		NativeWinMgr& manager,
		NativeWindowID windowId);
}