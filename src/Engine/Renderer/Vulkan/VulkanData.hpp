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

	struct VertexBufferObject;
	using VBO = VertexBufferObject;

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

struct DRenderer::Vulkan::VertexBufferObject
{
	enum class Attribute
	{
		Position,
		TexCoord,
		Normal,
		Index,
		COUNT
	};

	vk::Buffer buffer = nullptr;
	vk::DeviceMemory deviceMemory = nullptr;
	std::array<size_t, static_cast<size_t>(Attribute::COUNT)> attributeSizes{};

	vk::DeviceSize indexCount = 0;
	vk::IndexType indexType{};

	size_t GetByteOffset(Attribute attribute) const;
	size_t& GetAttrSize(Attribute attr);
	size_t GetAttrSize(Attribute attr) const;
};

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
		vk::SampleCountFlagBits maxFramebufferSamples{};
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
		uint32_t length = invalidIndex;
		uint32_t currentImage = invalidIndex;
		std::vector<vk::Image> images;
		std::vector<vk::ImageView> imageViews;
	};
	Swapchain swapchain{};

	// Resource set means in-flight image
	uint32_t resourceSetCount = invalidIndex;
	// Resource set means in-flight image
	uint32_t currentResourceSet = 0;
	// Has swapchain length
	uint32_t imageAvailableActiveIndex = 0;

	struct MainUniforms
	{
		vk::DeviceMemory cameraBuffersMem = nullptr;
		size_t cameraUBOByteLength = std::numeric_limits<size_t>::max();
		// Has resourceSetCount length
		std::vector<vk::Buffer> cameraBuffer;
		uint8_t* cameraMemoryMap = nullptr;

		vk::DeviceMemory perObjectUBOMem = nullptr;

	};
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

	vk::DescriptorSetLayout descriptorSetLayout = nullptr;
	vk::DescriptorPool descriptorSetPool = nullptr;
	std::vector<vk::DescriptorSet> descriptorSets;

	VBO testVBO{};
};
