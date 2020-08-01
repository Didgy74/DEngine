#pragma once

#include "DEngine/FixedWidthTypes.hpp"
#include "DEngine/Gfx/Gfx.hpp"

#include "DynamicDispatch.hpp"
#include "QueueData.hpp"
#include "VulkanIncluder.hpp"
#include "VMAIncluder.hpp"

#include <unordered_map>

namespace DEngine::Gfx::Vk
{
	class GlobUtils;

	struct TextureManager
	{
		vk::Sampler sampler{};
		vk::CommandPool cmdPool{};
		vk::DescriptorSetLayout descrSetLayout{};
		vk::DescriptorPool descrPool{};
		static constexpr uSize descrPool_minCapacity = 64;
		uSize descrPoolCapacity = 0;

		struct Inner
		{
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
			DrawParams const& drawParams,
			Gfx::TextureAssetInterface const& texAssetInterface);
	};
}