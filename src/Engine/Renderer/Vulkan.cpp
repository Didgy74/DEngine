#include "Vulkan.hpp"

#include "volk/volk.hpp"
#include <vulkan/vulkan.hpp>

#include "VulkanExtensionConfig.hpp"

#include <cassert>
#include <cstdint>
#include <array>
#include <vector>
#include <fstream>

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
	InitInfo initInfo{};

	struct Version
	{
		uint32_t major;
		uint32_t minor;
		uint32_t patch;
	};
	Version apiVersion{};
	std::vector<vk::ExtensionProperties> instanceExtensionProperties;
	std::vector<vk::LayerProperties> instanceLayerProperties;

	vk::Instance instance = nullptr;

	vk::DebugUtilsMessengerEXT debugMessenger = nullptr;

	vk::SurfaceKHR surface = nullptr;
	vk::Extent2D surfaceExtents{};

	struct PhysDevice
	{
		vk::PhysicalDevice handle = nullptr;

		vk::PhysicalDeviceProperties properties{};
		vk::PhysicalDeviceMemoryProperties memProperties{};
		uint32_t deviceLocalMemory = std::numeric_limits<uint32_t>::max();
		uint32_t hostVisibleMemory = std::numeric_limits<uint32_t>::max();
		bool hostMemoryIsDeviceLocal = false;
	};
	PhysDevice physDevice{};

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
		uint8_t length = 0;
		uint8_t currentImage = 0;
		std::vector<vk::Image> images;
		std::vector<vk::ImageView> imageViews;
	};
	vk::SwapchainKHR swapchain = nullptr;
	uint8_t swapchainLength = 0;
	uint8_t currentSwapchainImage = 0;
	// Resource set means in-flight image
	uint8_t resourceSetCount = 0;
	// Resource set means in-flight image
	uint8_t currentResourceSet = 0;
	// Has swapchain length
	std::vector<vk::Image> swapchainImages;
	// Has swapchain length
	std::vector<vk::ImageView> swapchainImageViews;
	// Has swapchain length
	std::vector<vk::Fence> imageAvailableForPresentation;
	uint8_t imageAvailableActiveIndex = 0;

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

void DRenderer::Vulkan::APIDataDeleter(void*& ptr)
{
	APIData* data = static_cast<APIData*>(ptr);
	ptr = nullptr;
	delete data;
}

DRenderer::Vulkan::APIData& DRenderer::Vulkan::GetAPIData()
{
	return *static_cast<APIData*>(Core::GetAPIData());
}

struct DRenderer::Vulkan::SwapchainSettings
{
	vk::SurfaceCapabilitiesKHR capabilities{};
	vk::PresentModeKHR presentMode{};
	vk::SurfaceFormatKHR surfaceFormat{};
	uint8_t numImages{};
};

DRenderer::Vulkan::SwapchainSettings DRenderer::Vulkan::GetSwapchainSettings(vk::PhysicalDevice device, vk::SurfaceKHR surface)
{
	SwapchainSettings settings{};

	// Query surface capabilities
	auto capabilities = device.getSurfaceCapabilitiesKHR(surface);
	settings.capabilities = capabilities;

	// Handle swapchain length
	constexpr uint8_t preferredSwapchainLength = 3;
	settings.numImages = preferredSwapchainLength;
	if (settings.numImages > capabilities.maxImageCount && capabilities.maxImageCount != 0)
		settings.numImages = capabilities.maxImageCount;
	if (settings.numImages < 2)
	{
		Core::LogDebugMessage("Error. Maximum swapchain-length cannot be less than 2.");
		std::abort();
	}

	auto presentModes = device.getSurfacePresentModesKHR(surface);
	vk::PresentModeKHR presentMode = vk::PresentModeKHR::eFifo;
	settings.presentMode = presentMode;

	// Handle formats
	constexpr std::array preferredFormats
	{
		vk::Format::eR8G8B8A8Unorm,
		vk::Format::eB8G8R8A8Unorm
	};
	auto formats = device.getSurfaceFormatsKHR(surface);
	vk::SurfaceFormatKHR formatToUse = { vk::Format::eUndefined, vk::ColorSpaceKHR::eSrgbNonlinear };
	bool foundPreferredFormat = false;
	for (const auto& preferredFormat : preferredFormats)
	{
		for (const auto& format : formats)
		{
			if (format.format == preferredFormat)
			{
				formatToUse.format = preferredFormat;
				foundPreferredFormat = true;
				break;
			}
		}
	}
	if constexpr (Core::debugLevel >= 2)
	{
		if (!foundPreferredFormat)
		{
			Core::LogDebugMessage("Found no usable surface formats.");
			std::abort();
		}
	}
	settings.surfaceFormat = formatToUse;

	return settings;
}

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
	vk::SwapchainKHR Init_CreateSwapchain(vk::Device device, vk::SurfaceKHR surface, const SwapchainSettings& settings);
	std::vector<vk::ImageView> Init_CreateSwapchainImageViews(vk::Device device, const std::vector<vk::Image>& imageArr, vk::Format format);
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

DRenderer::Vulkan::APIData::Version DRenderer::Vulkan::Init_LoadAPIVersion()
{
	APIData::Version version;

	// Query the highest API version available.
	uint32_t apiVersion = 0;
	auto enumerateVersionResult = Volk::GetInstanceVersion();
	assert(enumerateVersionResult != 0);
	version.major = VK_VERSION_MAJOR(apiVersion);
	version.minor = VK_VERSION_MINOR(apiVersion);
	version.patch = VK_VERSION_PATCH(apiVersion);

	return version;
}

std::vector<vk::ExtensionProperties> DRenderer::Vulkan::Init_LoadInstanceExtensionProperties()
{
	auto availableExtensions = vk::enumerateInstanceExtensionProperties();

	std::vector<vk::ExtensionProperties> activeExts;
	activeExts.reserve(Vulkan::requiredExtensions.size());

	// Check if all required extensions are available. Load the required ones into vecRef when done.
	for (const auto& requiredExtension : Vulkan::requiredExtensions)
	{
		bool foundExtension = false;
		for (const auto& availableExtension : availableExtensions)
		{
			if (std::strcmp(requiredExtension, availableExtension.extensionName) == 0)
			{
				foundExtension = true;
				activeExts.push_back(availableExtension);
				break;
			}
		}
		// If there's a required extension that doesn't exist, return failure.
		if (foundExtension == false)
			return {};
	}

	return std::move(activeExts);
}

std::vector<vk::LayerProperties> DRenderer::Vulkan::Init_LoadInstanceLayerProperties()
{
	// Load all the available validation layers, load all the ones we require into vecRef.
	auto availableLayers = vk::enumerateInstanceLayerProperties();

	std::vector<vk::LayerProperties> activeLayers;
	activeLayers.reserve(requiredValidLayers.size());

	for (const auto& requiredLayer : requiredValidLayers)
	{
		bool foundLayer = false;
		for (const auto& availableLayer : availableLayers)
		{
			if (std::strcmp(availableLayer.layerName, requiredLayer) == 0)
			{
				foundLayer = true;
				activeLayers.push_back(availableLayer);
				break;
			}
		}
		// If we couldn't find the required validation layer, return failure.
		if (foundLayer == false)
			return {};

		return std::move(activeLayers);
	}
	return {};
}

vk::Instance DRenderer::Vulkan::Init_CreateInstance(const std::vector<vk::ExtensionProperties>& extensions,
													const std::vector<vk::LayerProperties>& layers,
													vk::ApplicationInfo appInfo,
													const std::vector<std::string_view>& extensions2)
{
	vk::InstanceCreateInfo createInfo{};

	// Create const char* array of the extension names.
	std::vector<const char*> extensionList;
	extensionList.reserve(extensions.size());
	for (const auto& extension : extensions)
		extensionList.push_back(extension.extensionName);

	// Add extensions2 array, without adding duplicates
	for (const auto& extension2 : extensions2)
	{
		bool extensionFound = false;
		for (const auto& existingExtension : extensionList)
		{
			if (std::strcmp(extension2.data(), existingExtension) == 0)
			{
				extensionFound = true;
				break;
			}
		}
		if (extensionFound == false)
			extensionList.push_back(extension2.data());
	}

	createInfo.enabledExtensionCount = uint32_t(extensionList.size());
	createInfo.ppEnabledExtensionNames = extensionList.data();

	// Create const char* array of the validation layer names.
	std::vector<const char*> layerList;
	if constexpr (Core::debugLevel >= 2)
	{
		layerList.reserve(layers.size());
		for (const auto& layer : layers)
			layerList.push_back(layer.layerName);
		createInfo.enabledLayerCount = uint32_t(layerList.size());
		createInfo.ppEnabledLayerNames = layerList.data();
	}

	createInfo.pApplicationInfo = &appInfo;

	auto result = vk::createInstance(createInfo);
	if (!result)
		return {};

	return result;
}

void DRenderer::Vulkan::Init_LoadMemoryTypeIndex(const vk::PhysicalDeviceMemoryProperties& memProperties, uint32_t& deviceLocalIndex, uint32_t& hostVisibleIndex)
{
    // Find deviceLocal memory
    for (size_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if ((memProperties.memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal) == vk::MemoryPropertyFlagBits::eDeviceLocal)
        {
            deviceLocalIndex = i;
            break;
        }
    }

    // Find host-visible memory
    for (size_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if ((memProperties.memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eHostVisible) == vk::MemoryPropertyFlagBits::eHostVisible)
        {
            hostVisibleIndex = i;
            break;
        }
    }
}

VKAPI_ATTR VkBool32 VKAPI_CALL DRenderer::Vulkan::Callback(
	vk::DebugUtilsMessageTypeFlagBitsEXT messageSeverity,
	vk::DebugUtilsMessageTypeFlagsEXT messageType,
	const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	Core::LogDebugMessage(pCallbackData->pMessage);

	return true;
}

vk::DebugUtilsMessengerEXT DRenderer::Vulkan::Init_CreateDebugMessenger(vk::Instance instance)
{
	vk::DebugUtilsMessengerCreateInfoEXT createInfo{};
	createInfo.messageSeverity =
		vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
		vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning;
	createInfo.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation;
	createInfo.pfnUserCallback = (PFN_vkDebugUtilsMessengerCallbackEXT)&Callback;

	vk::DebugUtilsMessengerEXT temp = instance.createDebugUtilsMessengerEXT(createInfo);
	assert(temp);
	return temp;
}

vk::SurfaceKHR DRenderer::Vulkan::Init_CreateSurface(vk::Instance instance, void* hwnd)
{
	vk::SurfaceKHR tempSurfaceHandle;
	bool surfaceCreationTest = GetAPIData().initInfo.test(&instance, hwnd, &tempSurfaceHandle);
	assert(surfaceCreationTest);
	return tempSurfaceHandle;
}

vk::SwapchainKHR DRenderer::Vulkan::Init_CreateSwapchain(vk::Device device, vk::SurfaceKHR surface, const SwapchainSettings& settings)
{
	vk::SwapchainCreateInfoKHR swapchainCreateInfo{};

	swapchainCreateInfo.imageArrayLayers = 1;
	swapchainCreateInfo.imageExtent = settings.capabilities.currentExtent;
	swapchainCreateInfo.imageFormat = settings.surfaceFormat.format;
	swapchainCreateInfo.imageColorSpace = settings.surfaceFormat.colorSpace;
	swapchainCreateInfo.imageSharingMode = vk::SharingMode::eExclusive;
	swapchainCreateInfo.presentMode = settings.presentMode;
	swapchainCreateInfo.surface = surface;
	swapchainCreateInfo.preTransform = settings.capabilities.currentTransform;
	swapchainCreateInfo.clipped = 1;
	swapchainCreateInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
	swapchainCreateInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst;
	swapchainCreateInfo.minImageCount = settings.numImages;

	auto swapchainResult = device.createSwapchainKHR(swapchainCreateInfo);
	assert(swapchainResult);

	return swapchainResult;
}

std::vector<vk::ImageView> DRenderer::Vulkan::Init_CreateSwapchainImageViews(vk::Device device, const std::vector<vk::Image>& imageArr, vk::Format format)
{
	std::vector<vk::ImageView> imageViews(imageArr.size());
	for (size_t i = 0; i < imageViews.size(); i++)
	{
		vk::ImageViewCreateInfo createInfo{};

		createInfo.image = imageArr[i];

		createInfo.viewType = vk::ImageViewType::e2D;

		vk::ComponentMapping compMapping{};
		compMapping.r = vk::ComponentSwizzle::eIdentity;
		compMapping.g = vk::ComponentSwizzle::eIdentity;
		compMapping.b = vk::ComponentSwizzle::eIdentity;
		compMapping.a = vk::ComponentSwizzle::eIdentity;
		createInfo.components = compMapping;

		createInfo.format = format;

		vk::ImageSubresourceRange subresourceRange;
		subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		subresourceRange.layerCount = 1;
		subresourceRange.levelCount = 1;
		createInfo.subresourceRange = subresourceRange;

		vk::ImageView temp = device.createImageView(createInfo);
		assert(temp);
		imageViews[i] = temp;
	}
	return std::move(imageViews);
}

// Note! This does NOT create the associated framebuffer.
DRenderer::Vulkan::APIData::RenderTarget DRenderer::Vulkan::Init_CreateRenderTarget(vk::Device device, vk::Extent2D extents, vk::Format format)
{
	// Setup image for render target
	vk::ImageCreateInfo imgCreateInfo{};
	imgCreateInfo.format = format;
	imgCreateInfo.imageType = vk::ImageType::e2D;
	imgCreateInfo.initialLayout = vk::ImageLayout::eUndefined;
	imgCreateInfo.mipLevels = 1;
	imgCreateInfo.arrayLayers = 1;
	imgCreateInfo.samples = vk::SampleCountFlagBits::e1;
	imgCreateInfo.sharingMode = vk::SharingMode::eExclusive;
	imgCreateInfo.tiling = vk::ImageTiling::eOptimal;
	imgCreateInfo.usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc;
	imgCreateInfo.extent = vk::Extent3D{ extents.width, extents.height, 1 };

	vk::Image img = device.createImage(imgCreateInfo);
	assert(img);

	vk::MemoryRequirements memReqs = device.getImageMemoryRequirements(img);

	vk::MemoryAllocateInfo allocInfo;
	allocInfo.allocationSize = memReqs.size;

	allocInfo.memoryTypeIndex = GetAPIData().physDevice.deviceLocalMemory;

	vk::DeviceMemory mem = device.allocateMemory(allocInfo);
	assert(mem);

	device.bindImageMemory(img, mem, 0);

	vk::ImageViewCreateInfo imgViewCreateInfo{};
	imgViewCreateInfo.image = img;
	imgViewCreateInfo.format = imgCreateInfo.format;
	imgViewCreateInfo.viewType = vk::ImageViewType::e2D;
	imgViewCreateInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	imgViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	imgViewCreateInfo.subresourceRange.layerCount = 1;
	imgViewCreateInfo.subresourceRange.baseMipLevel = 0;
	imgViewCreateInfo.subresourceRange.levelCount = 1;

	vk::ImageView imgView = device.createImageView(imgViewCreateInfo);
	assert(imgView);

	APIData::RenderTarget renderTarget{};
	renderTarget.memory = mem;
	renderTarget.image = img;
	renderTarget.imageView = imgView;

	if constexpr (Core::debugLevel >= 2)
	{
		// Name our render target frame
		vk::DebugUtilsObjectNameInfoEXT renderTargetNameInfo{};
		renderTargetNameInfo.objectType = vk::ObjectType::eDeviceMemory;
		renderTargetNameInfo.objectHandle = (uint64_t)(VkDeviceMemory)renderTarget.memory;
		renderTargetNameInfo.pObjectName = "Render Target 0 - DeviceMemory";
		device.setDebugUtilsObjectNameEXT(renderTargetNameInfo);

		renderTargetNameInfo.objectType = vk::ObjectType::eImage;
		renderTargetNameInfo.objectHandle = (uint64_t)(VkImage)renderTarget.image;
		renderTargetNameInfo.pObjectName = "Render Target 0 - Image";
		device.setDebugUtilsObjectNameEXT(renderTargetNameInfo);

		renderTargetNameInfo.objectType = vk::ObjectType::eImageView;
		renderTargetNameInfo.objectHandle = (uint64_t)(VkImageView)renderTarget.imageView;
		renderTargetNameInfo.pObjectName = "Render Target 0 - ImageView";
		device.setDebugUtilsObjectNameEXT(renderTargetNameInfo);
	}

	return renderTarget;
}

vk::RenderPass DRenderer::Vulkan::Init_CreateMainRenderPass(vk::Device device, vk::SampleCountFlagBits sampleCount, vk::Format format)
{
	vk::AttachmentDescription colorAttachment{};
	colorAttachment.initialLayout = vk::ImageLayout::eColorAttachmentOptimal;
	colorAttachment.finalLayout = vk::ImageLayout::eColorAttachmentOptimal;
	colorAttachment.format = format;
	colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
	colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
	colorAttachment.samples = sampleCount;
	colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
	colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;

	std::array attachments = { colorAttachment };

	vk::AttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

	vk::SubpassDescription subpassDescription{};
	subpassDescription.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
	subpassDescription.colorAttachmentCount = 1;
	subpassDescription.pColorAttachments = &colorAttachmentRef;

	// Set up render pass
	vk::RenderPassCreateInfo createInfo{};
	createInfo.attachmentCount = uint32_t(attachments.size());
	createInfo.pAttachments = attachments.data();
	createInfo.subpassCount = 1;
	createInfo.pSubpasses = &subpassDescription;

	auto renderPass = device.createRenderPass(createInfo);
	assert(renderPass);
	return renderPass;
}

vk::Framebuffer DRenderer::Vulkan::Init_CreateRenderTargetFramebuffer(vk::Device device, vk::Extent2D extents, vk::ImageView imgView, vk::RenderPass renderPass)
{
	vk::FramebufferCreateInfo fbCreateInfo{};
	fbCreateInfo.renderPass = renderPass;
	fbCreateInfo.width = extents.width;
	fbCreateInfo.height = extents.height;
	fbCreateInfo.layers = 1;
	fbCreateInfo.attachmentCount = 1;
	fbCreateInfo.pAttachments = &imgView;

	vk::Framebuffer fb = device.createFramebuffer(fbCreateInfo);
	assert(fb);

	if constexpr (Core::debugLevel >= 2)
	{
		vk::DebugUtilsObjectNameInfoEXT renderTargetNameInfo{};
		renderTargetNameInfo.objectType = vk::ObjectType::eFramebuffer;
		renderTargetNameInfo.objectHandle = (uint64_t)(VkFramebuffer)fb;
		renderTargetNameInfo.pObjectName = "Render Target 0 - Framebuffer";
		device.setDebugUtilsObjectNameEXT(renderTargetNameInfo);
	}

	return fb;
}

void DRenderer::Vulkan::Init_TransitionRenderTargetAndSwapchain(vk::Device device, vk::Queue queue, vk::Image renderTarget, const std::vector<vk::Image>& swapchainImages)
{
	vk::CommandPoolCreateInfo createInfo{};
	createInfo.queueFamilyIndex = 0;

	vk::CommandPool pool = device.createCommandPool(createInfo);

	vk::CommandBufferAllocateInfo allocInfo{};
	allocInfo.commandPool = pool;
	allocInfo.commandBufferCount = 1;
	allocInfo.level = vk::CommandBufferLevel::ePrimary;

	auto vkCmdBuffers = device.allocateCommandBuffers(allocInfo);
	auto cmdBuffer = vkCmdBuffers[0];

	vk::CommandBufferBeginInfo beginInfo{};

	cmdBuffer.begin(beginInfo);

	vk::ImageMemoryBarrier barrier{};
	barrier.image = renderTarget;
	barrier.srcAccessMask = {};
	barrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
	barrier.srcQueueFamilyIndex = 0;
	barrier.dstQueueFamilyIndex = 0;
	barrier.oldLayout = vk::ImageLayout::eUndefined;
	barrier.newLayout = vk::ImageLayout::eColorAttachmentOptimal;
	barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;

	vk::PipelineStageFlags srcStage = vk::PipelineStageFlagBits::eTopOfPipe;
	vk::PipelineStageFlags dstStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;

	cmdBuffer.pipelineBarrier(srcStage, dstStage, vk::DependencyFlags(), {}, {}, { barrier });

	std::vector<vk::ImageMemoryBarrier> swapchainBarriers(swapchainImages.size());
	for (size_t i = 0; i < swapchainBarriers.size(); i++)
	{
		vk::ImageMemoryBarrier& barrier = swapchainBarriers[i];
		barrier.image = swapchainImages[i];
		barrier.srcAccessMask = {};
		barrier.dstAccessMask = {};
		barrier.srcQueueFamilyIndex = 0;
		barrier.dstQueueFamilyIndex = 0;
		barrier.oldLayout = vk::ImageLayout::eUndefined;
		barrier.newLayout = vk::ImageLayout::ePresentSrcKHR;
		barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
	}

	dstStage = vk::PipelineStageFlagBits::eBottomOfPipe;

	cmdBuffer.pipelineBarrier(srcStage, dstStage, vk::DependencyFlags(), {}, {}, swapchainBarriers);

	cmdBuffer.end();

	vk::SubmitInfo submitInfo{};
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuffer;

	queue.submit(submitInfo, vk::Fence());

	queue.waitIdle();

	device.destroyCommandPool(pool);
}

void DRenderer::Vulkan::Init_SetupRenderingCmdBuffers(vk::Device device, uint8_t swapchainLength, vk::CommandPool& pool, std::vector<vk::CommandBuffer>& commandBuffers)
{
	vk::CommandPoolCreateInfo createInfo{};
	createInfo.queueFamilyIndex = 0;
	createInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;

	pool = device.createCommandPool(createInfo);

	vk::CommandBufferAllocateInfo allocInfo{};
	allocInfo.commandPool = pool;
	allocInfo.commandBufferCount = swapchainLength;
	allocInfo.level = vk::CommandBufferLevel::ePrimary;

	commandBuffers = device.allocateCommandBuffers(allocInfo);
}

void DRenderer::Vulkan::Init_SetupPresentCmdBuffers(
	vk::Device device,
	vk::Image renderTarget, 
	vk::Extent2D extents, 
	const std::vector<vk::Image>& swapchain, 
	vk::CommandPool& pool,
	std::vector<vk::CommandBuffer>& cmdBuffers)
{
	vk::CommandPoolCreateInfo createInfo{};
	createInfo.queueFamilyIndex = 0;

	pool = device.createCommandPool(createInfo);

	vk::CommandBufferAllocateInfo allocInfo{};
	allocInfo.commandPool = pool;
	allocInfo.commandBufferCount = uint32_t(swapchain.size());
	allocInfo.level = vk::CommandBufferLevel::ePrimary;

	cmdBuffers = device.allocateCommandBuffers(allocInfo);

	for (size_t i = 0; i < swapchain.size(); i++)
	{
		auto& cmdBuffer = cmdBuffers[i];

		if constexpr (Core::debugLevel >= 2)
		{
			vk::DebugUtilsObjectNameInfoEXT renderTargetNameInfo{};
			renderTargetNameInfo.objectType = vk::ObjectType::eCommandBuffer;
			renderTargetNameInfo.objectHandle = (uint64_t)(VkCommandBuffer)cmdBuffer;
			std::string name = "Presentation Command Buffer " + std::to_string(i);
			renderTargetNameInfo.pObjectName = name.data();
			device.setDebugUtilsObjectNameEXT(renderTargetNameInfo);
		}

		vk::CommandBufferBeginInfo beginInfo{};
		beginInfo.flags = vk::CommandBufferUsageFlagBits::eSimultaneousUse;

		cmdBuffer.begin(beginInfo);

		vk::ImageMemoryBarrier renderTargetPreBarrier{};
		renderTargetPreBarrier.image = renderTarget;
		renderTargetPreBarrier.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
		renderTargetPreBarrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;
		renderTargetPreBarrier.srcQueueFamilyIndex = 0;
		renderTargetPreBarrier.dstQueueFamilyIndex = 0;
		renderTargetPreBarrier.oldLayout = vk::ImageLayout::eColorAttachmentOptimal;
		renderTargetPreBarrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
		renderTargetPreBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		renderTargetPreBarrier.subresourceRange.baseArrayLayer = 0;
		renderTargetPreBarrier.subresourceRange.layerCount = 1;
		renderTargetPreBarrier.subresourceRange.baseMipLevel = 0;
		renderTargetPreBarrier.subresourceRange.levelCount = 1;


		vk::PipelineStageFlags srcStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		vk::PipelineStageFlags dstStage = vk::PipelineStageFlagBits::eTransfer;
		cmdBuffer.pipelineBarrier(srcStage, dstStage, vk::DependencyFlags(), {}, {}, { renderTargetPreBarrier });

		vk::ImageMemoryBarrier swapchainPreBarrier{};
		swapchainPreBarrier.image = swapchain[i];
		swapchainPreBarrier.srcAccessMask = {};
		swapchainPreBarrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
		swapchainPreBarrier.srcQueueFamilyIndex = 0;
		swapchainPreBarrier.dstQueueFamilyIndex = 0;
		swapchainPreBarrier.oldLayout = vk::ImageLayout::ePresentSrcKHR;
		swapchainPreBarrier.newLayout = vk::ImageLayout::eTransferDstOptimal;
		swapchainPreBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		swapchainPreBarrier.subresourceRange.baseArrayLayer = 0;
		swapchainPreBarrier.subresourceRange.layerCount = 1;
		swapchainPreBarrier.subresourceRange.baseMipLevel = 0;
		swapchainPreBarrier.subresourceRange.levelCount = 1;

		srcStage = vk::PipelineStageFlagBits::eTopOfPipe;
		dstStage = vk::PipelineStageFlagBits::eTransfer;
		cmdBuffer.pipelineBarrier(srcStage, dstStage, vk::DependencyFlags(), {}, {}, { swapchainPreBarrier });

		vk::ImageCopy imgCopy{};
		imgCopy.extent = vk::Extent3D{ extents.width, extents.height, 1 };
		imgCopy.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
		imgCopy.dstSubresource.baseArrayLayer = 0;
		imgCopy.dstSubresource.layerCount = 1;
		imgCopy.dstSubresource.mipLevel = 0;
		imgCopy.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
		imgCopy.srcSubresource.baseArrayLayer = 0;
		imgCopy.srcSubresource.layerCount = 1;
		imgCopy.srcSubresource.mipLevel = 0;
		cmdBuffer.copyImage(renderTarget, vk::ImageLayout::eTransferSrcOptimal, swapchain[i], vk::ImageLayout::eTransferDstOptimal, imgCopy);


		vk::ImageMemoryBarrier renderTargetPostBarrier{};
		renderTargetPostBarrier.image = renderTarget;
		renderTargetPostBarrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
		renderTargetPostBarrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
		renderTargetPostBarrier.srcQueueFamilyIndex = 0;
		renderTargetPostBarrier.dstQueueFamilyIndex = 0;
		renderTargetPostBarrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
		renderTargetPostBarrier.newLayout = vk::ImageLayout::eColorAttachmentOptimal;
		renderTargetPostBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		renderTargetPostBarrier.subresourceRange.baseArrayLayer = 0;
		renderTargetPostBarrier.subresourceRange.layerCount = 1;
		renderTargetPostBarrier.subresourceRange.baseMipLevel = 0;
		renderTargetPostBarrier.subresourceRange.levelCount = 1;

		srcStage = vk::PipelineStageFlagBits::eTransfer;
		dstStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		cmdBuffer.pipelineBarrier(srcStage, dstStage, vk::DependencyFlags(), {}, {}, { renderTargetPostBarrier });

		vk::ImageMemoryBarrier swapchainPostBarrier{};
		swapchainPostBarrier.image = swapchain[i];
		swapchainPostBarrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
		swapchainPostBarrier.dstAccessMask = {};
		swapchainPostBarrier.srcQueueFamilyIndex = 0;
		swapchainPostBarrier.dstQueueFamilyIndex = 0;
		swapchainPostBarrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
		swapchainPostBarrier.newLayout = vk::ImageLayout::ePresentSrcKHR;
		swapchainPostBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		swapchainPostBarrier.subresourceRange.baseArrayLayer = 0;
		swapchainPostBarrier.subresourceRange.layerCount = 1;
		swapchainPostBarrier.subresourceRange.baseMipLevel = 0;
		swapchainPostBarrier.subresourceRange.levelCount = 1;

		srcStage = vk::PipelineStageFlagBits::eTransfer;
		dstStage = vk::PipelineStageFlagBits::eBottomOfPipe;
		cmdBuffer.pipelineBarrier(srcStage, dstStage, vk::DependencyFlags(), {}, {}, { swapchainPostBarrier });

		cmdBuffer.end();
	}
}

namespace DRenderer::Vulkan
{
	void MakePipeline();
}


void DRenderer::Vulkan::Initialize(Core::APIDataPointer& apiData, InitInfo& initInfo)
{
	// Load the first function pointers.
	Volk::InitializeCustom((PFN_vkGetInstanceProcAddr)initInfo.getInstanceProcAddr());

	apiData.data = new APIData();
	apiData.deleterPfn = &APIDataDeleter;
	APIData& data = *static_cast<APIData*>(apiData.data);
	data.initInfo = initInfo;

	// Start initialization
	data.apiVersion = Init_LoadAPIVersion();
	data.instanceExtensionProperties = Init_LoadInstanceExtensionProperties();
	if constexpr (Core::debugLevel >= 2)
		data.instanceLayerProperties = Init_LoadInstanceLayerProperties();

	// Create our instance
	vk::ApplicationInfo appInfo{};
	appInfo.apiVersion = VK_MAKE_VERSION(data.apiVersion.major, data.apiVersion.minor, data.apiVersion.patch);
	appInfo.pApplicationName = "DEngine";
	appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
	appInfo.pEngineName = "DEngine";
	appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
	auto wsiRequiredExtensions = data.initInfo.getRequiredInstanceExtensions();
	data.instance = Init_CreateInstance(data.instanceExtensionProperties, data.instanceLayerProperties, appInfo, wsiRequiredExtensions);

	// Load function pointers for our instance.
	Volk::LoadInstance(data.instance);

	// Enable debugging only if we are in debug config, and the user requested debugging.
	if constexpr (Core::debugLevel >= 2)
	{
		if (Core::GetData().debugData.useDebugging == true)
			data.debugMessenger = Init_CreateDebugMessenger(data.instance);
	}

	// Create our surface we will render onto
	void* hwnd = Engine::Renderer::GetViewport(0).GetSurfaceHandle();
	data.surface = Init_CreateSurface(data.instance, hwnd);

	// Find physical device
	// TODO: Make code that is not hardcoded.
	auto devices = data.instance.enumeratePhysicalDevices();
	assert(!devices.empty());
	data.physDevice.handle = devices[0];

	// We've picked a physical device. Grab some information from it.

	data.physDevice.properties = data.physDevice.handle.getProperties();
	data.physDevice.memProperties = data.physDevice.handle.getMemoryProperties();
	Init_LoadMemoryTypeIndex(data.physDevice.memProperties, data.physDevice.deviceLocalMemory, data.physDevice.hostVisibleMemory);
	data.physDevice.hostMemoryIsDeviceLocal = false;

	// Get swapchain creation details
	SwapchainSettings swapchainSettings = GetSwapchainSettings(data.physDevice.handle, data.surface);
	data.surfaceExtents = vk::Extent2D{ swapchainSettings.capabilities.currentExtent.width, swapchainSettings.capabilities.currentExtent.height };


	// Create logical device
	auto queues = data.physDevice.handle.getQueueFamilyProperties();

	auto presentSupport = data.physDevice.handle.getSurfaceSupportKHR(0, data.surface);

	uint32_t graphicsFamily = 0;
	uint32_t graphicsQueue = 0;

	vk::DeviceCreateInfo createInfo{};

	// Feature configuration
	auto physDeviceFeatures = data.physDevice.handle.getFeatures();

	vk::PhysicalDeviceFeatures features{};
	if (physDeviceFeatures.robustBufferAccess == 1)
		features.robustBufferAccess = true;

	createInfo.pEnabledFeatures = &features;

	// Queue configuration
	float priority[2] = { 1.f, 1.f };
	std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
	
	vk::DeviceQueueCreateInfo firstQueueCreateInfo;
	firstQueueCreateInfo.pQueuePriorities = priority;
	firstQueueCreateInfo.queueCount = 1;
	firstQueueCreateInfo.queueFamilyIndex = 0;
	queueCreateInfos.push_back(firstQueueCreateInfo);

	createInfo.queueCreateInfoCount = uint32_t(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();

	auto deviceExtensionsAvailable = data.physDevice.handle.enumerateDeviceExtensionProperties();
	// TODO: Check if all required device extensions are available
	createInfo.ppEnabledExtensionNames = Vulkan::requiredDeviceExtensions.data();
	createInfo.enabledExtensionCount = uint32_t(Vulkan::requiredDeviceExtensions.size());

	//auto deviceLayersAvailable = data.physDevice.handle.enumerateDeviceLayerProperties();
	//createInfo.enabledLayerCount = uint32_t(Vulkan::requiredDeviceLayers.size());
	//createInfo.ppEnabledLayerNames = Vulkan::requiredDeviceLayers.data();

	auto result = data.physDevice.handle.createDevice(createInfo);
	if (!result)
		std::abort();
	data.device = result;

	// Load function pointers for our device.
	Volk::LoadDevice(data.device);

	// Create our Render Target
	data.renderTarget = Init_CreateRenderTarget(data.device, data.surfaceExtents, swapchainSettings.surfaceFormat.format);

	// Set up main renderpass
	data.renderPass = Init_CreateMainRenderPass(data.device, vk::SampleCountFlagBits::e1, swapchainSettings.surfaceFormat.format);

	data.renderTarget.framebuffer = Init_CreateRenderTargetFramebuffer(data.device, data.surfaceExtents, data.renderTarget.imageView, data.renderPass);


	// Set up swapchain
	data.swapchain = Init_CreateSwapchain(data.device, data.surface, swapchainSettings);
	data.swapchainImages = data.device.getSwapchainImagesKHR(data.swapchain);
	data.swapchainLength = uint8_t(data.swapchainImages.size());
	data.resourceSetCount = data.swapchainLength - 1;
	data.swapchainImageViews = Init_CreateSwapchainImageViews(data.device, data.swapchainImages, swapchainSettings.surfaceFormat.format);

	Init_SetupPresentCmdBuffers(data.device, data.renderTarget.image, data.surfaceExtents, data.swapchainImages, data.presentCmdPool, data.presentCmdBuffer);

	Init_SetupRenderingCmdBuffers(data.device, data.swapchainLength, data.renderCmdPool, data.renderCmdBuffer);

	data.graphicsQueue = data.device.getQueue(graphicsFamily, graphicsQueue);

	Init_TransitionRenderTargetAndSwapchain(data.device, data.graphicsQueue, data.renderTarget.image, data.swapchainImages);

	data.imageAvailableForPresentation.resize(data.swapchainLength);
	data.renderCmdBufferAvailable.resize(data.swapchainLength);
	for (size_t i = 0; i < data.swapchainLength; i++)
	{
		vk::FenceCreateInfo createInfo1{};
		createInfo1.flags = vk::FenceCreateFlagBits::eSignaled;
	
		data.renderCmdBufferAvailable[i] = data.device.createFence(createInfo1);

		data.imageAvailableForPresentation[i] = data.device.createFence({});
	}

	auto imageResult = data.device.acquireNextImageKHR(data.swapchain, std::numeric_limits<uint64_t>::max(), vk::Semaphore(), data.imageAvailableForPresentation[data.imageAvailableActiveIndex]);
	if constexpr (Core::debugLevel >= 2)
	{
		if (imageResult.result != vk::Result::eSuccess)
		{
			Core::LogDebugMessage("Result of vkAcquireNextImageKHR was not successful.");
			std::abort();
		}
	}
	data.currentSwapchainImage = imageResult.value;

	MakePipeline();
}

void DRenderer::Vulkan::PrepareRenderingEarly(const Core::PrepareRenderingEarlyParams& in)
{
	APIData& data = GetAPIData();

	data.device.waitForFences(data.renderCmdBufferAvailable[data.currentSwapchainImage], 1, std::numeric_limits<uint64_t>::max());
	data.device.resetFences(data.renderCmdBufferAvailable[data.currentSwapchainImage]);

	auto cmdBuffer = data.renderCmdBuffer[data.currentSwapchainImage];

	vk::CommandBufferBeginInfo beginInfo{};
	beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

	cmdBuffer.begin(beginInfo);

	vk::RenderPassBeginInfo renderPassBeginInfo{};
	renderPassBeginInfo.renderPass = data.renderPass;
	renderPassBeginInfo.framebuffer = data.renderTarget.framebuffer;

	vk::ClearValue clearValue;
	clearValue.color.float32[0] = 0.25f;
	clearValue.color.float32[1] = 0.f;
	clearValue.color.float32[2] = 0.f;
	clearValue.color.float32[3] = 1.f;
	renderPassBeginInfo.clearValueCount = 1;
	renderPassBeginInfo.pClearValues = &clearValue;

	vk::Rect2D renderArea{};
	renderArea.extent = data.surfaceExtents;
	renderPassBeginInfo.renderArea = renderArea;

	cmdBuffer.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);

	cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, data.pipeline);

	cmdBuffer.draw(3, 1, 0, 0);

	cmdBuffer.endRenderPass();

	cmdBuffer.end();
}

void DRenderer::Vulkan::PrepareRenderingLate()
{
}

void DRenderer::Vulkan::Draw()
{
	APIData& data = GetAPIData();

	data.device.waitForFences(data.imageAvailableForPresentation[data.imageAvailableActiveIndex], 1, std::numeric_limits<uint64_t>::max());
	data.device.resetFences(data.imageAvailableForPresentation[data.imageAvailableActiveIndex]);
	
	vk::SubmitInfo renderSubmitInfo{};

	vk::SubmitInfo submitInfo{};
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &data.renderCmdBuffer[data.currentSwapchainImage];
	data.graphicsQueue.submit(submitInfo, data.renderCmdBufferAvailable[data.currentSwapchainImage]);

	vk::SubmitInfo submitInfo2{};
	submitInfo2.commandBufferCount = 1;
	submitInfo2.pCommandBuffers = &data.presentCmdBuffer[data.currentSwapchainImage];
	data.graphicsQueue.submit(submitInfo2, {});

	vk::PresentInfoKHR presentInfo{};
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &data.swapchain;

	uint32_t swapchainIndex = data.currentSwapchainImage;
	presentInfo.pImageIndices = &swapchainIndex;

	vk::Result presentResult{};
	presentInfo.pResults = &presentResult;

	data.graphicsQueue.presentKHR(presentInfo);

	if constexpr (Core::debugLevel >= 2)
	{
		if (presentResult != vk::Result::eSuccess)
		{
			Core::LogDebugMessage("Presentation result of index " + std::to_string(swapchainIndex) + " was not success.");
			std::abort();
		}
	}

	data.imageAvailableActiveIndex = (data.imageAvailableActiveIndex + 1) % data.swapchainLength;
	auto imageResult = data.device.acquireNextImageKHR(data.swapchain, std::numeric_limits<uint64_t>::max(), {}, data.imageAvailableForPresentation[data.imageAvailableActiveIndex]);
	if constexpr (Core::debugLevel >= 2)
	{
		if (imageResult.result != vk::Result::eSuccess)
		{
			Core::LogDebugMessage("Result of vkAcquireNextImageKHR was not successful.");
			std::abort();
		}
	}
	data.currentSwapchainImage = imageResult.value;
}

void DRenderer::Vulkan::MakePipeline()
{
	APIData& data = GetAPIData();

	vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
	data.pipelineLayout = data.device.createPipelineLayout(pipelineLayoutCreateInfo);

	std::ifstream vertFile("data/Shaders/VulkanTest/vert.spv", std::ios::ate | std::ios::binary);
	if (!vertFile.is_open())
		std::abort();

	std::vector<uint8_t> vertCode(vertFile.tellg());
	vertFile.seekg(0);
	vertFile.read(reinterpret_cast<char*>(vertCode.data()), vertCode.size());
	vertFile.close();

	vk::ShaderModuleCreateInfo vertModCreateInfo{};
	vertModCreateInfo.codeSize = vertCode.size();
	vertModCreateInfo.pCode = reinterpret_cast<const uint32_t*>(vertCode.data());

	vk::ShaderModule vertModule = data.device.createShaderModule(vertModCreateInfo);

	vk::PipelineShaderStageCreateInfo vertStageCreateInfo{};
	vertStageCreateInfo.stage = vk::ShaderStageFlagBits::eVertex;
	vertStageCreateInfo.module = vertModule;
	vertStageCreateInfo.pName = "main";

	std::ifstream fragFile("data/Shaders/VulkanTest/frag.spv", std::ios::ate | std::ios::binary);
	if (!fragFile.is_open())
		std::abort();

	std::vector<uint8_t> fragCode(fragFile.tellg());
	fragFile.seekg(0);
	fragFile.read(reinterpret_cast<char*>(fragCode.data()), fragCode.size());
	fragFile.close();

	vk::ShaderModuleCreateInfo fragModCreateInfo{};
	fragModCreateInfo.codeSize = fragCode.size();
	fragModCreateInfo.pCode = reinterpret_cast<const uint32_t*>(fragCode.data());

	vk::ShaderModule fragModule = data.device.createShaderModule(fragModCreateInfo);

	vk::PipelineShaderStageCreateInfo fragStageCreateInfo{};
	fragStageCreateInfo.stage = vk::ShaderStageFlagBits::eFragment;
	fragStageCreateInfo.module = fragModule;
	fragStageCreateInfo.pName = "main";

	std::array shaderStages = { vertStageCreateInfo, fragStageCreateInfo };


	vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};

	vk::PipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;


	vk::Viewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)data.surfaceExtents.width;
	viewport.height = (float)data.surfaceExtents.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vk::Rect2D scissor{};
	scissor.offset = vk::Offset2D{ 0, 0 };
	scissor.extent = data.surfaceExtents;
	vk::PipelineViewportStateCreateInfo viewportState{};
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	vk::PipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.lineWidth = 1.f;
	rasterizer.polygonMode = vk::PolygonMode::eFill;

	vk::PipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;

	vk::PipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

	vk::PipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;

	vk::GraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.layout = data.pipelineLayout;
	pipelineInfo.renderPass = data.renderPass;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages.data();


	data.pipeline = data.device.createGraphicsPipeline({}, pipelineInfo);
}
