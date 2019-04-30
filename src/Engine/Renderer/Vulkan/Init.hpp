#pragma once

#include "VulkanData.hpp"

#include <vector>

namespace DRenderer::Vulkan
{
	APIData::Version Init_LoadAPIVersion();
	std::vector<vk::ExtensionProperties> Init_LoadInstanceExtensionProperties();
	std::vector<vk::LayerProperties> Init_LoadInstanceLayerProperties();
	vk::Instance Init_CreateInstance(
			const std::vector<vk::ExtensionProperties>& extensions,
			const std::vector<vk::LayerProperties>& layers,
			vk::ApplicationInfo appInfo,
			const std::vector<std::string_view>& extensions2);
	void Init_LoadMemoryTypeIndex(const vk::PhysicalDeviceMemoryProperties& memProperties, uint32_t& deviceLocalIndex, uint32_t& hostVisibleIndex);
	vk::DebugUtilsMessengerEXT Init_CreateDebugMessenger(vk::Instance instance);
	vk::SurfaceKHR Init_CreateSurface(vk::Instance instance, void* hwnd);
	APIData::PhysDeviceInfo Init_LoadPhysDevice(vk::Instance instance, vk::SurfaceKHR surface);
	APIData::Swapchain Init_CreateSwapchain(vk::Device device, vk::SurfaceKHR surface, const SwapchainSettings& settings);
	// Note! This does NOT create the associated framebuffer
	APIData::RenderTarget Init_CreateRenderTarget(vk::Device device, vk::Extent2D extents, vk::Format format);
	vk::RenderPass Init_CreateMainRenderPass(vk::Device device, vk::SampleCountFlagBits sampleCount, vk::Format format);
	vk::Framebuffer Init_CreateRenderTargetFramebuffer(vk::Device device, vk::Extent2D extents, vk::ImageView imgView, vk::RenderPass renderPass);
	void Init_TransitionRenderTargetAndSwapchain(vk::Device device, vk::Queue queue, vk::Image renderTarget, const std::vector<vk::Image>& swapchainImages);
	void Init_SetupRenderingCmdBuffers(vk::Device device, uint8_t swapchainLength, vk::CommandPool& pool, std::vector<vk::CommandBuffer>& commandBuffers);
	void Init_SetupPresentCmdBuffers(
			vk::Device device,
			vk::Image renderTarget,
			vk::Extent2D extents,
			const std::vector<vk::Image>& swapchain,
			vk::CommandPool& pool,
			std::vector<vk::CommandBuffer>& cmdBuffers
	);
}
