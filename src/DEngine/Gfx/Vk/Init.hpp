#pragma once

#include "Vk.hpp"
#include "VulkanIncluder.hpp"
#include "DynamicDispatch.hpp"

#include "DEngine/Containers/Span.hpp"

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
		ILog* logger);

	[[nodiscard]] vk::DebugUtilsMessengerEXT CreateLayerMessenger(
		vk::Instance instanceHandle,
		DebugUtilsDispatch const* debugUtilsOpt,
		const void* userData);

	[[nodiscard]] PhysDeviceInfo LoadPhysDevice(
		InstanceDispatch const& instance,
		vk::SurfaceKHR surface);

	[[nodiscard]] vk::Device CreateDevice(
		InstanceDispatch const& instance,
		PhysDeviceInfo const& physDevice);

	void InitializeVMA(
		GlobUtils& globUtils,
		VMA_MemoryTrackingData* vma_trackingData);

	[[nodiscard]] bool InitializeViewportManager(
		ViewportManager& vpManager,
		PhysDeviceInfo const& physDevice,
		DevDispatch const& device);

	[[nodiscard]] Std::StaticVector<vk::Fence, Constants::maxResourceSets> CreateMainFences(
		DevDispatch const& device,
		u8 resourceSetCount,
		DebugUtilsDispatch const* debugUtils);

	SurfaceInfo BuildSurfaceInfo(
		InstanceDispatch const& instance,
		vk::PhysicalDevice physDevice,
		vk::SurfaceKHR surface,
		ILog* logger);

	void RebuildSurfaceInfo(
		InstanceDispatch const& instance,
		PhysDeviceInfo const& physDevice,
		vk::SurfaceKHR newSurface,
		SurfaceInfo& outSurfaceInfo);

	[[nodiscard]] SwapchainSettings BuildSwapchainSettings(
		InstanceDispatch const& instance,
		vk::PhysicalDevice physDevice,
		SurfaceInfo const& surfaceCaps,
		ILog* logger);

	[[nodiscard]] SwapchainData CreateSwapchain(
		Vk::DeviceDispatch const& device,
		QueueData const& queues,
		DeletionQueue const& deletionQueue,
		SwapchainSettings const& settings,
		DebugUtilsDispatch const* debugUtilsOpt);

	void RecreateSwapchain(
		GlobUtils const& globUtils,
		SurfaceInfo& surface,
		SwapchainData& swapchain);

	bool TransitionSwapchainImages(
		DeviceDispatch const& device,
		DeletionQueue const& deletionQueue,
		QueueData const& queues,
		Std::Span<const vk::Image> images);

	void RecordSwapchainCmdBuffers(
		DeviceDispatch const& device,
		SwapchainData const& swapchainData,
		vk::Image srcImg);

	[[nodiscard]] GUIRenderTarget CreateGUIRenderTarget(
		DeviceDispatch const& device,
		VmaAllocator vma,
		DeletionQueue const& deletionQueue,
		QueueData const& queues,
		vk::Extent2D swapchainDimensions,
		vk::Format swapchainFormat,
		vk::RenderPass renderPass,
		DebugUtilsDispatch const* debugUtils);

	[[nodiscard]] GUIData CreateGUIData(
		DeviceDispatch const& device,
		VmaAllocator vma,
		DeletionQueue const& deletionQueue,
		QueueData const& queues,
		vk::Format swapchainFormat,
		u8 resourceSetCount,
		vk::Extent2D swapchainDimensions,
		DebugUtilsDispatch const* debugUtils);

	void InitializeImGui(
		APIData& apiData,
		DevDispatch const& device,
		PFN_vkGetInstanceProcAddr instanceProcAddr,
		DebugUtilsDispatch const* debugUtils);

	[[nodiscard]] vk::RenderPass BuildMainGfxRenderPass(
		DeviceDispatch const& device,
		bool useEditorPipeline,
		DebugUtilsDispatch const* debugUtils);

	void TransitionGfxImage(
		DeviceDispatch const& device,
		DeletionQueue const& deletionQueue,
		QueueData const& queues,
		vk::Image img,
		bool useEditorPipeline);

	[[nodiscard]] GfxRenderTarget InitializeGfxViewportRenderTarget(
		GlobUtils const& globUtils,
		uSize viewportID,
		vk::Extent2D viewportSize);
}