#pragma once

import DEngine.FixedWidthTypes;
#include <DEngine/Std/Containers/AllocRef.hpp>
#include <DEngine/Gfx/Gfx.hpp>

#include "DynamicDispatch.hpp"
#include "QueueData.hpp"
#include "VulkanIncluder.hpp"
#include "VMAIncluder.hpp"
#include "StagingBufferAlloc.hpp"
#include "RaiiHandles.hpp"

#include <unordered_map>

namespace DEngine::Gfx::Vk
{
	class GlobUtils;

	struct TextureManager {
		BoxVkSampler sampler = {};
		BoxVkCommandPool cmdPool = {};
		BoxVkDescriptorSetLayout descrSetLayout = {};
		BoxVkDescriptorPool descrPool = {};
		static constexpr uSize descrPool_minCapacity = 64;
		uSize descrPoolCapacity = 0;

		struct Inner {
			bool isNeededHelper = false;

			VmaAllocation imgVmaAlloc{};
			vk::Image img{};
			vk::ImageView imgView{};
			vk::DescriptorSet descrSet{};
		};
		std::unordered_map<TextureID, Inner> database;

		static void Init(
			TextureManager& manager,
			DeviceDispatch const& device,
			QueueData const& queues,
			DebugUtilsDispatch const* debugUtils);

		static void Update(
			TextureManager& manager,
			GlobUtils const& globUtils,
			DelQueue& delQueue,
			StagingBufferAlloc& stagingBufferAlloc,
			vk::CommandBuffer cmdBuffer,
			DrawParams const& drawParams,
			Gfx::TextureAssetInterface const& texAssetInterface,
			Std::AllocRef const& transientAlloc);
	};
}