#pragma once

#include "../APIDataBase.hpp"

#include "Constants.hpp"
#include "DeletionQueue.hpp"
#include "DynamicDispatch.hpp"
#include "GizmoManager.hpp"
#include "GlobUtils.hpp"
#include "GuiResourceManager.hpp"
#include "NativeWindowManager.hpp"
#include "ObjectDataManager.hpp"
#include "QueueData.hpp"
#include "StagingBufferAlloc.hpp"
#include "TextureManager.hpp"
#include "ViewportManager.hpp"
#include "VMAIncluder.hpp"
#include "VulkanIncluder.hpp"

#include <DEngine/Gfx/Gfx.hpp>

#include <DEngine/FixedWidthTypes.hpp>
#include <DEngine/Std/BumpAllocator.hpp>
#include <DEngine/Std/Containers/StackVec.hpp>
#include <DEngine/Std/Containers/Array.hpp>
#include <DEngine/Std/Containers/Pair.hpp>

#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

namespace DEngine::Gfx::Vk
{
	constexpr u32 invalidIndex = static_cast<u32>(-1);
	
	template<typename T>
	[[nodiscard]] inline constexpr bool IsValidIndex(T in) = delete;
	template<>
	[[nodiscard]] inline constexpr bool IsValidIndex<u32>(u32 in) { return in != static_cast<u32>(-1); }
	template<>
	[[nodiscard]] inline constexpr bool IsValidIndex<u64>(u64 in) { return in != static_cast<u64>(-1); }

	struct Device
	{
		vk::Device handle{};
		vk::PhysicalDevice physDeviceHandle{};
	};

	struct VMA_MemoryTrackingData {
		DebugUtilsDispatch const* debugUtils = nullptr;
		vk::Device deviceHandle{};
		std::mutex vma_idTracker_lock{};
		u64 vma_idTracker = 0;
	};

	class APIData final : public APIDataBase
	{
	public:
		APIData();
		virtual ~APIData() override;
		virtual void Draw(DrawParams const& drawParams) override;
		static void InternalDraw(APIData& apiData, DrawParams const& drawParams);

		// Thread safe
		virtual void NewNativeWindow(NativeWindowID windowId) override;
		// Thread safe
		virtual void DeleteNativeWindow(NativeWindowID windowId) override;

		// Thread safe
		virtual void NewViewport(ViewportID& viewportID) override;
		// Thread safe
		virtual void DeleteViewport(ViewportID id) override;

		// Thread safe
		virtual void NewFontFace(FontFaceId fontFaceId) override;

		// Thread safe
		virtual void NewFontTextures(Std::Span<FontBitmapUploadJob const> const&) override;

		Gfx::TextureAssetInterface const* test_textureAssetInterface = nullptr;

		u64 tickCount = 0;
		int inFlightCount = 0;
		u8 currInFlightIndex = 0;
		[[nodiscard]] int InFlightIndex() const { return currInFlightIndex; }
		void IncrementInFlightIndex() {
			currInFlightIndex = (currInFlightIndex + 1) % inFlightCount;
		}
		u8 currFenceIndex = 0;
		void IncrementMainFenceIndex() {
			currFenceIndex = (currFenceIndex + 1) % (inFlightCount - 1);
		}
		[[nodiscard]] int MainFenceIndex() const { return currFenceIndex; }

		// Do not touch this.
		VMA_MemoryTrackingData vma_trackingData{};

		Std::StackVec<vk::Fence, Const::maxInFlightCount> mainFences;
		Std::StackVec<vk::CommandPool, Const::maxInFlightCount> mainCmdPools;
		Std::StackVec<vk::CommandBuffer, Const::maxInFlightCount> mainCmdBuffers;
		Std::StackVec<StagingBufferAlloc, Const::maxInFlightCount> mainStagingBufferAllocs;

		GlobUtils globUtils = {};

		Std::BumpAllocator frameAllocator;
		DeletionQueue delQueue;

		GizmoManager gizmoManager = {};
		GuiResourceManager guiResourceManager = {};
		NativeWinMgr nativeWindowManager = {};
		ObjectDataManager objectDataManager = {};
		TextureManager textureManager = {};
		ViewportManager viewportManager = {};

		vk::PipelineLayout testPipelineLayout{};
		vk::Pipeline testPipeline{};

		std::mutex threadLock;
		struct Thread {
			std::thread renderingThread;
			bool shutdownThread = false;
			bool nextJobReady = false;
			using JobFnT = void(*)(APIData& apiData);
			JobFnT nextJobFn = nullptr;
			DrawParams drawParams;
			std::condition_variable drawParamsCondVarWorker;
			std::condition_variable drawParamsCondVarProducer;
		};
		Thread thread;
	};
}