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
		VmaAllocation vmaAlloc{};
		vk::Buffer buffer{};
		void* mappedMem = nullptr;
		// Contains the capacity of amount of elements.
		// This is not amount in bytes.
		uSize capacity = 0;
		// Measured in bytes.
		uSize elementSize = 0;
		vk::DescriptorPool descrPool{};
		vk::DescriptorSetLayout descrSetLayout{};
		vk::DescriptorSet descrSet{};

		static void HandleResizeEvent(
			ObjectDataManager& manager,
			GlobUtils const& globUtils,
			DelQueue& delQueue,
			uSize dataCount);

		static void Update(
			ObjectDataManager& manager,
			Std::Span<Math::Mat4 const> transforms,
			u8 currentInFlightIndex);

		[[nodiscard]] static bool Init(
			ObjectDataManager& manager,
			uSize minUniformBufferOffsetAlignment,
			u8 resourceSetCount,
			VmaAllocator vma,
			DeviceDispatch const& device,
			DebugUtilsDispatch const* debugUtils);
	};
}