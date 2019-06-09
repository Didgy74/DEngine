#pragma once

#include "volk/volk.hpp"
#include <vulkan/vulkan.hpp>

#include "DRenderer/VulkanInitInfo.hpp"
#include "QueueInfo.hpp"
#include "DeletionQueues.hpp"
#include "AssetSystem.hpp"
#include "MemoryTypes.hpp"
#include "MainUniforms.hpp"
#include "Constants.hpp"


#include <array>
#include <vector>

namespace DRenderer::Vulkan
{
	struct PhysDeviceInfo;

	struct DeletionQueues;

	struct APIData;
	void APIDataDeleter(void*& ptr);
	APIData& GetAPIData();

	std::array<vk::VertexInputBindingDescription, 3> GetVertexBindings();
	std::array<vk::VertexInputAttributeDescription, 3> GetVertexAttributes();

	VKAPI_ATTR VkBool32 VKAPI_CALL Callback(
			vk::DebugUtilsMessageTypeFlagBitsEXT messageSeverity,
			vk::DebugUtilsMessageTypeFlagsEXT messageType,
			const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
			void* pUserData);
}

struct DRenderer::Vulkan::PhysDeviceInfo
{
	vk::PhysicalDevice handle = nullptr;
	vk::PhysicalDeviceProperties properties{};
	vk::PhysicalDeviceMemoryProperties memProperties{};
	vk::SampleCountFlagBits maxFramebufferSamples{};
	QueueInfo preferredQueues{};
	MemoryTypes memInfo{};
};

struct DRenderer::Vulkan::APIData
{
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

	PhysDeviceInfo physDevice{};

	vk::Device device = nullptr;

	DeletionQueues deletionQueue;

	QueueInfo queues{};

	struct RenderTarget
	{
		vk::DeviceMemory memory = nullptr;
		vk::SampleCountFlagBits sampleCount{};
		vk::Image colorImg = nullptr;
		vk::ImageView colorImgView = nullptr;
		vk::Image depthImg = nullptr;
		vk::ImageView depthImgView = nullptr;
		vk::Framebuffer framebuffer = nullptr;
	};
	RenderTarget renderTarget{};

	struct Swapchain
	{
		vk::SwapchainKHR handle = nullptr;
		uint32_t length = 0;
		uint32_t currentImage = Constants::invalidIndex;
		std::vector<vk::Image> images;
		std::vector<vk::ImageView> imageViews;
	};
	Swapchain swapchain{};

	// Resource set means in-flight image
	uint32_t resourceSetCount = 0;
	// Resource set means in-flight image
	uint32_t currentResourceSet = 0;
	// Has swapchain length
	uint32_t imageAvailableActiveIndex = 0;

	
	MainUniforms mainUniforms{};
	std::vector<vk::Fence> resourceSetAvailable;

	vk::RenderPass renderPass = nullptr;

	vk::CommandPool renderCmdPool = nullptr;
	// Has the length of Swapchain-length
	std::vector<vk::CommandBuffer> renderCmdBuffer;

	vk::CommandPool presentCmdPool = nullptr;
	// Has swapchain length
	std::vector<vk::CommandBuffer> presentCmdBuffer;

	vk::Pipeline pipeline = nullptr;
	vk::PipelineLayout pipelineLayout = nullptr;

	// Has swapchain length
	std::vector<vk::Semaphore> swapchainImageAvailable;

	AssetData assetSystem{};
};
