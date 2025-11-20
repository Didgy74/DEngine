#pragma once

#include <DEngine/Gfx/Gfx.hpp>

#include "VulkanIncluder.hpp"
#include "VMAIncluder.hpp"
#include "Constants.hpp"
#include "RaiiHandles.hpp"
#include "ForwardDeclarations.hpp"

#include <DEngine/Std/Containers/AllocRef.hpp>

#include <mutex>
#include <vector>

import DEngine.Std.StackVec;

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

		// The stencil depth attachment is transient our GUI render-pass, and so
		// use the same one across all swap-chain images.
		BoxVmaImg depthStencilImg = {};
		BoxVkImageView depthStencilImgView = {};

		// Signaled by vkAcquireNextImageKHR and waited on by the render submit. There is one per
		// frame-in-flight slot, indexed by mainFenceIndex (so MainFencesCount() of them). A single
		// shared semaphore is unsafe with multiple frames in flight: the previous frame's submit-wait
		// on it may still be pending when the next frame re-acquires, which vkAcquireNextImageKHR
		// forbids. mainFenceIndex is the only index the per-frame fence guards: the frame waits on
		// and resets mainFences[mainFenceIndex] at the top and signals it in its submit, so when the
		// same slot comes around again the prior wait on its acquire semaphore has retired.
		Std::StackVec<vk::Semaphore, Const::maxInFlightCount> swapchainImgReadySems;
		// Signaled by the render submit and waited on by the present operation, so that
		// presentation does not begin before rendering into the swapchain image is finished.
		// There is one per swapchain image, indexed by the acquired image index. A present-wait
		// semaphore has no CPU-observable point at which the present engine is done waiting on it
		// (mainFence only covers the submit, not the present), so it cannot be safely re-signaled
		// the next frame. Re-acquiring an image index guarantees that image's previous present has
		// retired, which is the only safe point to reuse its render-finished semaphore.
		Std::StackVec<vk::Semaphore, Const::maxSwapchainLength> renderFinishedSems;
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
			Std::Opt<vk::Extent2D> sizeHint;
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
			std::vector<Node> nativeWindows;
		};
		MainT main;

		[[nodiscard]] NativeWinMgr_WindowData const& GetWindowData(NativeWindowID in) const;
		[[nodiscard]] NativeWinMgr_WindowData& GetWindowData(NativeWindowID in);
		[[nodiscard]] Node const& GetWindowData(int index) const {
			DENGINE_IMPL_GFX_ASSERT(index < main.nativeWindows.size());
			return main.nativeWindows[index];
		}

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
			Std::Opt<vk::Extent2D> const& sizeHint;
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
		Std::Opt<vk::SurfaceKHR> const& surface,
		Std::Opt<vk::Extent2D> const& sizeHint);

	void NativeWinMgr_PushDeleteWindowJob(
		NativeWinMgr& manager,
		NativeWindowID windowId);
}