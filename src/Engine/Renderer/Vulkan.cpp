#include "Vulkan.hpp"

#include "volk/volk.hpp"
#include <vulkan/vulkan.hpp>

#include "VulkanExtensionConfig.hpp"

#include <cassert>
#include <cstdint>
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
	vk::Queue presentQueue = nullptr;
	vk::Queue transferQueue = nullptr;
	vk::Queue transferImageQueue = nullptr;
	vk::Queue computeQueue = nullptr;

	struct RenderTarget
	{
		vk::DeviceMemory memory = nullptr;
		vk::Image image = nullptr;
		vk::ImageView imageView = nullptr;
		vk::Framebuffer framebuffer = nullptr;
	};
	RenderTarget renderTarget{};

	vk::SwapchainKHR swapchain = nullptr;
	uint8_t swapchainLength = 0;
	uint8_t resourceSetCount = 0;
	std::vector<vk::Image> swapchainImages;

	vk::RenderPass renderPass = nullptr;
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
};

DRenderer::Vulkan::SwapchainSettings DRenderer::Vulkan::GetSwapchainSettings(vk::PhysicalDevice device, vk::SurfaceKHR surface)
{
	// Query surface capabilities
	auto capabilities = device.getSurfaceCapabilitiesKHR(surface);

	auto presentModes = device.getSurfacePresentModesKHR(surface);
	vk::PresentModeKHR presentMode = vk::PresentModeKHR::eFifo;

	auto formats = device.getSurfaceFormatsKHR(surface);
	vk::SurfaceFormatKHR surfaceFormat = { vk::Format::eR8G8B8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear };

	SwapchainSettings settings{};
	settings.capabilities = capabilities;
	settings.presentMode = presentMode;
	settings.surfaceFormat = surfaceFormat;

	return settings;
}

namespace DRenderer::Vulkan
{
	APIData::Version Init_LoadAPIVersion();
	std::vector<vk::ExtensionProperties> Init_LoadInstanceExtensionProperties();
	std::vector<vk::LayerProperties> Init_LoadInstanceLayerProperties();
	vk::Instance Init_CreateInstance(const std::vector<vk::ExtensionProperties>& extensions, 
									 const std::vector<vk::LayerProperties>& layers,
									 vk::ApplicationInfo appInfo,
									 const std::vector<std::string_view>& extensions2);
	vk::DebugUtilsMessengerEXT Init_CreateDebugMessenger(vk::Instance instance);
	vk::SurfaceKHR Init_CreateSurface(vk::Instance instance, void* hwnd);
	vk::SwapchainKHR Init_CreateSwapchain(vk::Device device, vk::SurfaceKHR surface, const SwapchainSettings& settings);
	std::vector<vk::ImageView> Init_CreateSwapchainImageViews(vk::Device device, const std::vector<vk::Image>& imageArr, vk::Format format);
	// Note! This does NOT create the associated framebuffer
	APIData::RenderTarget Init_CreateRenderTarget(vk::Device device, vk::Extent2D extents);
	vk::RenderPass Init_CreateMainRenderPass(vk::Device device, vk::SampleCountFlagBits sampleCount);
	vk::Framebuffer Init_CreateRenderTargetFramebuffer(vk::Device device, vk::Extent2D extents, vk::ImageView imgView, vk::RenderPass renderPass);
}

DRenderer::Vulkan::APIData::Version DRenderer::Vulkan::Init_LoadAPIVersion()
{
	APIData::Version version;

	// Query the highest API version available.
	uint32_t apiVersion = 0;
	auto enumerateVersionResult = vk::enumerateInstanceVersion(&apiVersion);
	assert(enumerateVersionResult == vk::Result::eSuccess);
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
	for (size_t i = 0; i < Vulkan::requiredExtensions.size(); i++)
	{
		const auto& requiredExtension = Vulkan::requiredExtensions[i];

		bool foundExtension = false;
		for (size_t j = 0; j < availableExtensions.size(); j++)
		{
			const auto& availableExtension = availableExtensions[j];
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
	if constexpr (debugConfig)
	{
		auto availableLayers = vk::enumerateInstanceLayerProperties();

		std::vector<vk::LayerProperties> activeLayers;
		activeLayers.reserve(requiredValidLayers.size());

		for (size_t i = 0; i < requiredValidLayers.size(); i++)
		{
			const auto& requiredLayer = requiredValidLayers[i];

			bool foundLayer = false;
			for (size_t j = 0; j < availableLayers.size(); j++)
			{
				const auto& availableLayer = availableLayers[j];
				
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
		}

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
	if constexpr (debugConfig == true)
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
		vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
		vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
		vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose;
	createInfo.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation;
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
	swapchainCreateInfo.minImageCount = 3;
	if (settings.capabilities.maxImageCount < swapchainCreateInfo.minImageCount)
		swapchainCreateInfo.minImageCount = settings.capabilities.maxImageCount;

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
DRenderer::Vulkan::APIData::RenderTarget DRenderer::Vulkan::Init_CreateRenderTarget(vk::Device device, vk::Extent2D extents)
{
	// Setup image for render target
	vk::ImageCreateInfo imgCreateInfo{};
	imgCreateInfo.format = vk::Format::eR8G8B8A8Unorm;
	imgCreateInfo.imageType = vk::ImageType::e2D;
	imgCreateInfo.initialLayout = vk::ImageLayout::eUndefined;
	imgCreateInfo.mipLevels = 1;
	imgCreateInfo.arrayLayers = 1;
	imgCreateInfo.samples = vk::SampleCountFlagBits::e1;
	imgCreateInfo.sharingMode = vk::SharingMode::eExclusive;
	imgCreateInfo.tiling = vk::ImageTiling::eOptimal;
	imgCreateInfo.usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc;
	imgCreateInfo.extent = { extents.width, extents.height, 1 };

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

	return renderTarget;
}

vk::RenderPass DRenderer::Vulkan::Init_CreateMainRenderPass(vk::Device device, vk::SampleCountFlagBits sampleCount)
{
	// Set up render pass
	vk::RenderPassCreateInfo createInfo{};

	vk::AttachmentDescription colorAttachment{};
	colorAttachment.initialLayout = vk::ImageLayout::eColorAttachmentOptimal;
	colorAttachment.finalLayout = vk::ImageLayout::eColorAttachmentOptimal;
	colorAttachment.format = vk::Format::eR8G8B8A8Unorm;
	colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
	colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
	colorAttachment.samples = sampleCount;
	colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
	colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;

	createInfo.attachmentCount = 1;
	createInfo.pAttachments = &colorAttachment;


	vk::AttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;
	vk::SubpassDescription sbDesc{};
	sbDesc.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
	sbDesc.colorAttachmentCount = 1;
	sbDesc.pColorAttachments = &colorAttachmentRef;

	createInfo.subpassCount = 1;
	createInfo.pSubpasses = &sbDesc;

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
	return fb;
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
	if constexpr (DRenderer::debugConfig == true)
	{
		if (DRenderer::Core::GetData().debugData.useDebugging == true)
			data.debugMessenger = Init_CreateDebugMessenger(data.instance);
	}

	// Create our surface we will render onto
	void* hwnd = Engine::Renderer::GetViewport(0).GetSurfaceHandle();
	data.surface = Init_CreateSurface(data.instance, hwnd);

	// Find physical device
	// TODO: Make code that is not hardcoded.
	auto devices = data.instance.enumeratePhysicalDevices();
	assert(devices.size() > 0);
	data.physDevice.handle = devices[0];

	// We've picked a physical device. Grab some information from it.

	data.physDevice.properties = data.physDevice.handle.getProperties();
	data.physDevice.memProperties = data.physDevice.handle.getMemoryProperties();
	data.physDevice.deviceLocalMemory = 0;
	data.physDevice.hostVisibleMemory = 1;
	data.physDevice.hostMemoryIsDeviceLocal = false;

	SwapchainSettings swapchainSettings = GetSwapchainSettings(data.physDevice.handle, data.surface);



	// Create logical device
	auto queues = data.physDevice.handle.getQueueFamilyProperties();
	bool presentSupport = data.physDevice.handle.getSurfaceSupportKHR(2, data.surface);

	uint32_t graphicsFamily = 0;
	uint32_t graphicsQueue = 0;
	uint32_t presentFamily = 2;
	uint32_t presentQueue = 1;
	uint32_t computeFamily = 1;
	uint32_t computeQueue = 0;
	uint32_t transferFamily = 2;
	uint32_t transferQueue = 0;
	uint32_t transferImageFamily = 1;
	uint32_t transferImageQueue = 1;

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

	vk::DeviceQueueCreateInfo secondQueueCreateInfo;
	secondQueueCreateInfo.pQueuePriorities = priority;
	secondQueueCreateInfo.queueCount = 2;
	secondQueueCreateInfo.queueFamilyIndex = 1;
	queueCreateInfos.push_back(secondQueueCreateInfo);

	vk::DeviceQueueCreateInfo thirdQueueCreateInfo;
	thirdQueueCreateInfo.pQueuePriorities = priority;
	thirdQueueCreateInfo.queueCount = 2;
	thirdQueueCreateInfo.queueFamilyIndex = 2;
	queueCreateInfos.push_back(thirdQueueCreateInfo);

	createInfo.queueCreateInfoCount = uint32_t(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();

	auto deviceExtensionsAvailable = data.physDevice.handle.enumerateDeviceExtensionProperties();
	// TODO: Check if all required device extensions are available
	createInfo.ppEnabledExtensionNames = Vulkan::requiredDeviceExtensions.data();
	createInfo.enabledExtensionCount = uint32_t(Vulkan::requiredDeviceExtensions.size());

	auto deviceLayersAvailable = data.physDevice.handle.enumerateDeviceLayerProperties();
	//createInfo.enabledLayerCount = uint32_t(Vulkan::requiredDeviceLayers.size());
	//createInfo.ppEnabledLayerNames = Vulkan::requiredDeviceLayers.data();

	auto result = data.physDevice.handle.createDevice(createInfo);
	if (!result)
		std::abort();
	data.device = result;

	// Load function pointers for our device.
	Volk::LoadDevice(data.device);

	vk::Extent2D renderImgExtents = { swapchainSettings.capabilities.currentExtent.width, swapchainSettings.capabilities.currentExtent.height };
	data.renderTarget = Init_CreateRenderTarget(data.device, renderImgExtents);

	// Set up main renderpass
	data.renderPass = Init_CreateMainRenderPass(data.device, vk::SampleCountFlagBits::e1);

	data.renderTarget.framebuffer = Init_CreateRenderTargetFramebuffer(data.device, renderImgExtents, data.renderTarget.imageView, data.renderPass);

	// Set up swapchain
	data.swapchain = Init_CreateSwapchain(data.device, data.surface, swapchainSettings);
	data.swapchainImages = data.device.getSwapchainImagesKHR(data.swapchain);
	data.swapchainLength = uint8_t(data.swapchainImages.size());
	data.resourceSetCount = data.swapchainLength - 1;




	data.graphicsQueue = data.device.getQueue(graphicsFamily, graphicsQueue);
	data.presentQueue = data.device.getQueue(presentFamily, presentQueue);
	data.transferQueue = data.device.getQueue(transferFamily, transferQueue);
	data.computeQueue = data.device.getQueue(computeFamily, computeQueue);
	data.transferImageQueue = data.device.getQueue(transferImageFamily, transferImageQueue);
}
