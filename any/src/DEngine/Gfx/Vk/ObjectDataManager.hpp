#pragma once

#include <DEngine/FixedWidthTypes.hpp>
#include <DEngine/Std/Containers/Span.hpp>

#include <DEngine/Math/Matrix.hpp>

#include "VulkanIncluder.hpp"
#include "VMAIncluder.hpp"
#include "ForwardDeclarations.hpp"

namespace DEngine::Gfx::Vk
{
	struct ObjectDataManager
	{
		// Contains minimum capacity of amount of elements.
		// Not measured in bytes.
		static constexpr uSize minCapacity = 256;
		// Measured in bytes.
		static constexpr uSize minElementSize = 64;
		VmaAllocation vmaAlloc = {};
		vk::Buffer buffer = {};
		void* mappedMem = nullptr;
		// Contains the capacity of amount of elements.
		// This is not amount in bytes.
		int capacity = 0;
		// Measured in bytes.
		uSize elementSize = 0;
		vk::DescriptorPool descrPool{};
		vk::DescriptorSetLayout descrSetLayout{};
		vk::DescriptorSet descrSet{};

		static void Update(
			ObjectDataManager& manager,
			GlobUtils const& globUtils,
			Std::Span<Math::Mat4 const> transforms,
			vk::CommandBuffer cmdBuffer,
			DeletionQueue& delQueue,
			u8 inFlightIndex,
			DebugUtilsDispatch const* debugUtils);

		[[nodiscard]] static bool Init(
			ObjectDataManager& manager,
			uSize minUniformBufferOffsetAlignment,
			DeviceDispatch const& device,
			DebugUtilsDispatch const* debugUtils);
	};
}