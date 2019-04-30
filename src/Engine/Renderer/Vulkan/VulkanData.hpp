#pragma once

#include "DRenderer/VulkanInitInfo.hpp"

#include "volk/volk.hpp"
#include <vulkan/vulkan.hpp>

#include <array>
#include <vector>

namespace DRenderer::Vulkan
{
	constexpr std::array requiredValidLayers
	{
		"VK_LAYER_LUNARG_standard_validation",
		//"VK_LAYER_RENDERDOC_Capture"
	};

	constexpr std::array requiredDeviceExtensions
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	};

	constexpr std::array requiredDeviceLayers
	{
		"VK_LAYER_LUNARG_standard_validation"
	};

	struct APIData;
	void APIDataDeleter(void*& ptr);
	APIData& GetAPIData();

	struct SwapchainSettings;
	SwapchainSettings GetSwapchainSettings(vk::PhysicalDevice device, vk::SurfaceKHR surface);

	VKAPI_ATTR VkBool32 VKAPI_CALL Callback(
			vk::DebugUtilsMessageTypeFlagBitsEXT messageSeverity,
			vk::DebugUtilsMessageTypeFlagsEXT messageType,
			const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
			void* pUserData);
}

struct DRenderer::Vulkan::APIData
{
	static constexpr uint32_t invalidIndex = std::numeric_limits<uint32_t>::max();

	APIData() = default;
	APIData(const APIData&) = delete;
	APIData(APIData&&) = delete;
	const APIData& operator=(const APIData&) = delete;
	const APIData& operator=(APIData&&) = delete;

	InitInfo initInfo{};

	struct Version
	{
		uint32_t major = 0;
		uint32_t minor = 0;
		uint32_t patch = 0;
	};
	Version apiVersion{};
	std::vector<vk::ExtensionProperties> instanceExtensionProperties;
	std::vector<vk::LayerProperties> instanceLayerProperties;

	vk::Instance instance = nullptr;

	vk::DebugUtilsMessengerEXT debugMessenger = nullptr;

	vk::SurfaceKHR surface = nullptr;
	vk::Extent2D surfaceExtents{};

	struct PhysDeviceInfo
	{
		vk::PhysicalDevice handle = nullptr;

		vk::PhysicalDeviceProperties properties{};
		vk::PhysicalDeviceMemoryProperties memProperties{};
		uint32_t deviceLocalMemory = invalidIndex;
		uint32_t hostVisibleMemory = invalidIndex;
		bool hostMemoryIsDeviceLocal = false;
	};
	PhysDeviceInfo physDevice{};

	vk::Device device = nullptr;

	vk::Queue graphicsQueue = nullptr;

	struct RenderTarget
	{
		vk::DeviceMemory memory = nullptr;
		vk::Image image = nullptr;
		vk::ImageView imageView = nullptr;
		vk::Framebuffer framebuffer = nullptr;
	};
	RenderTarget renderTarget{};

	struct Swapchain
	{
		vk::SwapchainKHR handle = nullptr;
		uint32_t length = invalidIndex;
		uint32_t currentImage = invalidIndex;
		std::vector<vk::Image> images;
		std::vector<vk::ImageView> imageViews;
	};
	Swapchain swapchain{};
	// Resource set means in-flight image
	uint32_t resourceSetCount = invalidIndex;
	// Resource set means in-flight image
	uint32_t currentResourceSet = invalidIndex;
	// Has swapchain length
	std::vector<vk::Fence> imageAvailableForPresentation;
	uint32_t imageAvailableActiveIndex = 0;

	vk::RenderPass renderPass = nullptr;

	vk::CommandPool renderCmdPool = nullptr;
	// Has the length of Swapchain-length
	std::vector<vk::CommandBuffer> renderCmdBuffer;
	// Has swapchain length
	std::vector<vk::Fence> renderCmdBufferAvailable;

	vk::CommandPool presentCmdPool = nullptr;
	// Has swapchain length
	std::vector<vk::CommandBuffer> presentCmdBuffer;

	vk::Pipeline pipeline = nullptr;
	vk::PipelineLayout pipelineLayout = nullptr;
};

struct DRenderer::Vulkan::SwapchainSettings
{
	vk::SurfaceCapabilitiesKHR capabilities{};
	vk::PresentModeKHR presentMode{};
	vk::SurfaceFormatKHR surfaceFormat{};
	uint8_t numImages{};
};