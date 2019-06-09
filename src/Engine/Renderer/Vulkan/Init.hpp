#pragma once

#include "VulkanData.hpp"

#include "Constants.hpp"

#include <vector>

namespace DRenderer::Vulkan
{
	struct SwapchainSettings;
}

struct DRenderer::Vulkan::SwapchainSettings
{
	vk::SurfaceCapabilitiesKHR capabilities{};
	vk::PresentModeKHR presentMode{};
	vk::SurfaceFormatKHR surfaceFormat{};
	uint32_t numImages = Constants::invalidIndex;
};

namespace DRenderer::Vulkan::Init
{
	APIData::Version LoadAPIVersion();

	std::vector<vk::ExtensionProperties> LoadInstanceExtensionProperties();

	std::vector<vk::LayerProperties> LoadInstanceLayerProperties();

	vk::Instance CreateInstance(
			const std::vector<vk::ExtensionProperties>& extensions,
			const std::vector<vk::LayerProperties>& layers,
			vk::ApplicationInfo appInfo,
			const std::vector<std::string_view>& extensions2);

	vk::DebugUtilsMessengerEXT CreateDebugMessenger(vk::Instance instance);

	vk::SurfaceKHR CreateSurface(vk::Instance instance, void* hwnd);

	PhysDeviceInfo LoadPhysDevice(
		vk::Instance instance, 
		vk::SurfaceKHR surface);

	SwapchainSettings GetSwapchainSettings(vk::PhysicalDevice device, vk::SurfaceKHR surface);

	vk::Device CreateDevice(const PhysDeviceInfo& physDevice);

	APIData::Swapchain CreateSwapchain(vk::Device device, vk::SurfaceKHR surface, const SwapchainSettings& settings);

	// Note! This does NOT create the associated framebuffer
	APIData::RenderTarget CreateRenderTarget(vk::Device device, vk::Extent2D extents, vk::Format format, vk::SampleCountFlagBits sampleCount);

	vk::RenderPass CreateMainRenderPass(vk::Device device, vk::SampleCountFlagBits sampleCount, vk::Format format);

	vk::Framebuffer CreateRenderTargetFramebuffer(
			vk::Device device,
			vk::RenderPass renderPass,
			vk::Extent2D extents,
			vk::ImageView colorImgView,
			vk::ImageView depthImgView
			);

	void TransitionRenderTargetAndSwapchain(
			vk::Device device,
			vk::Queue queue,
			const APIData::RenderTarget& renderTarget,
			const std::vector<vk::Image>& swapchainImages
			);

	void SetupRenderingCmdBuffers(vk::Device device, uint8_t swapchainLength, vk::CommandPool& pool, std::vector<vk::CommandBuffer>& commandBuffers);

	void SetupPresentCmdBuffers(
			vk::Device device,
			const APIData::RenderTarget& renderTarget,
			vk::Extent2D extents,
			const std::vector<vk::Image>& swapchain,
			vk::CommandPool& pool,
			std::vector<vk::CommandBuffer>& cmdBuffers
			);


}
