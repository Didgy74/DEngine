#pragma once

#include "Vk.hpp"
#include "VulkanIncluder.hpp"
#include "DynamicDispatch.hpp"

#include "DEngine/Std/Containers/Span.hpp"

namespace DEngine::Gfx::Vk::Init
{
	struct CreateVkInstance_Return
	{
		vk::Instance instanceHandle{};
		bool debugUtilsEnabled = false;
	};
	[[nodiscard]] CreateVkInstance_Return CreateVkInstance(
		Std::Span<char const*> requiredExtensions,
		bool enableLayers,
		BaseDispatch const& baseDispatch,
		LogInterface* logger);

	[[nodiscard]] vk::DebugUtilsMessengerEXT CreateLayerMessenger(
		vk::Instance instanceHandle,
		DebugUtilsDispatch const* debugUtilsOpt,
		void* userData);

	[[nodiscard]] PhysDeviceInfo LoadPhysDevice(
		InstanceDispatch const& instance,
		vk::SurfaceKHR surface);

	[[nodiscard]] vk::Device CreateDevice(
		InstanceDispatch const& instance,
		PhysDeviceInfo const& physDevice);

	[[nodiscard]] vk::ResultValue<VmaAllocator> InitializeVMA(
		InstanceDispatch const& instance,
		vk::PhysicalDevice physDeviceHandle,
		DeviceDispatch const& device,
		VMA_MemoryTrackingData* vma_trackingData);

	[[nodiscard]] Std::StackVec<vk::Fence, Constants::maxInFlightCount> CreateMainFences(
		DevDispatch const& device,
		u8 resourceSetCount,
		DebugUtilsDispatch const* debugUtils);

	[[nodiscard]] vk::RenderPass BuildMainGfxRenderPass(
		DeviceDispatch const& device,
		bool useEditorPipeline,
		DebugUtilsDispatch const* debugUtils);

	[[nodiscard]] vk::RenderPass CreateGuiRenderPass(
		DeviceDispatch const& device,
		vk::Format guiTargetFormat,
		DebugUtilsDispatch const* debugUtils);

	void TransitionGfxImage(
		DeviceDispatch const& device,
		DeletionQueue const& deletionQueue,
		QueueData const& queues,
		vk::Image img,
		bool useEditorPipeline);
}