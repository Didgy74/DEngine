#pragma once

#include "../APIDataBase.hpp"

#include "Constants.hpp"
#include "DeletionQueue.hpp"
#include "DynamicDispatch.hpp"
#include "GlobUtils.hpp"
#include "GuiResourceManager.hpp"
#include "NativeWindowManager.hpp"
#include "ObjectDataManager.hpp"
#include "QueueData.hpp"
#include "TextureManager.hpp"
#include "ViewportManager.hpp"
#include "VMAIncluder.hpp"
#include "VulkanIncluder.hpp"

#include "DEngine/Gfx/Gfx.hpp"

#include "DEngine/FixedWidthTypes.hpp"
#include "DEngine/Containers/StaticVector.hpp"
#include "DEngine/Containers/Array.hpp"
#include "DEngine/Containers/Pair.hpp"

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

	struct VMA_MemoryTrackingData
	{
		DebugUtilsDispatch const* debugUtils = nullptr;
		vk::Device deviceHandle{};
		std::mutex vma_idTracker_lock{};
		u64 vma_idTracker = 0;
	};

	struct APIData final : public APIDataBase
	{
		APIData();
		virtual ~APIData() override;
		virtual void Draw(Context& gfxData, DrawParams const& drawParams) override;

		// Thread safe
		virtual NativeWindowID NewNativeWindow(WsiInterface& wsiConnection) override;

		// Thread safe
		virtual void NewViewport(ViewportID& viewportID) override;
		// Thread safe
		virtual void DeleteViewport(ViewportID id) override;

		// Thread safe
		virtual void NewFontTexture(
			u32 id,
			u32 width,
			u32 height,
			u32 pitch,
			Std::Span<std::byte const> data) override;

		Gfx::LogInterface* logger = nullptr;
		Gfx::TextureAssetInterface const* test_textureAssetInterface = nullptr;

		SurfaceInfo surface{};

		u8 currInFlightIndex = 0;

		// Do not touch this.
		VMA_MemoryTrackingData vma_trackingData{};

		Std::StaticVector<vk::Fence, Constants::maxInFlightCount> mainFences{};

		GlobUtils globUtils{};

		GuiResourceManager guiResourceManager{};
		NativeWindowManager nativeWindowManager{};
		ObjectDataManager objectDataManager{};
		TextureManager textureManager{};
		ViewportManager viewportManager{};

		vk::PipelineLayout testPipelineLayout{};
		vk::Pipeline testPipeline{};
	};
}