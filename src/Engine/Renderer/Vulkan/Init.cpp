#include "Init.hpp"

#include "DMath/Common.hpp"

#include "../RendererData.hpp"

#include "VulkanExtensionConfig.hpp"

#include <array>

DRenderer::Vulkan::APIData::Version DRenderer::Vulkan::Init::LoadAPIVersion()
{
	APIData::Version version;

	// Query the highest API version available.
	
	uint32_t apiVersion = Volk::GetInstanceVersion();
	assert(apiVersion != 0);
	version.major = VK_VERSION_MAJOR(apiVersion);
	version.minor = VK_VERSION_MINOR(apiVersion);
	version.patch = VK_VERSION_PATCH(apiVersion);

	return version;
}

std::vector<vk::ExtensionProperties> DRenderer::Vulkan::Init::LoadInstanceExtensionProperties()
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
		if (!foundExtension)
			return {};
	}

	return activeExts;
}

std::vector<vk::LayerProperties> DRenderer::Vulkan::Init::LoadInstanceLayerProperties()
{
	// Load all the available validation layers, load all the ones we require into vecRef.
	auto availableLayers = vk::enumerateInstanceLayerProperties();

	std::vector<vk::LayerProperties> activeLayers;
	activeLayers.reserve(Constants::requiredValidLayers.size());

	for (const auto& requiredLayer : Constants::requiredValidLayers)
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
		if (!foundLayer)
			return {};

		return std::move(activeLayers);
	}
	return {};
}

vk::Instance DRenderer::Vulkan::Init::CreateInstance(const std::vector<vk::ExtensionProperties>& extensions,
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
		if (!extensionFound)
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

vk::SurfaceKHR DRenderer::Vulkan::Init::CreateSurface(vk::Instance instance, void* hwnd)
{
	vk::SurfaceKHR tempSurfaceHandle;
	bool surfaceCreationTest = GetAPIData().initInfo.test(&instance, hwnd, &tempSurfaceHandle);
	assert(surfaceCreationTest);
	return tempSurfaceHandle;
}

DRenderer::Vulkan::PhysDeviceInfo DRenderer::Vulkan::Init::LoadPhysDevice(
		vk::Instance instance,
		vk::SurfaceKHR surface)
{
	PhysDeviceInfo physDeviceInfo{};

	auto physDevices = instance.enumeratePhysicalDevices();
	assert(!physDevices.empty());

	physDeviceInfo.handle = physDevices[0];

	// Check presentation support
	auto temp = physDeviceInfo.handle.getQueueFamilyProperties();
	bool presentSupport = physDeviceInfo.handle.getSurfaceSupportKHR(0, surface);
	assert(presentSupport && "Error. Queuefamily 0 does not have presentation support.");

	physDeviceInfo.properties = physDeviceInfo.handle.getProperties();

	physDeviceInfo.memProperties = physDeviceInfo.handle.getMemoryProperties();

	// Find device-local memory
	for (uint32_t i = 0; i < physDeviceInfo.memProperties.memoryTypeCount; i++)
	{
		if (physDeviceInfo.memProperties.memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal)
		{
			physDeviceInfo.memInfo.deviceLocal = i;
			break;
		}
	}
	if (physDeviceInfo.memInfo.deviceLocal == Constants::invalidIndex)
	{
		Core::LogDebugMessage("Error. Found no device local memory.");
		std::abort();
	}

	// Find host-visible memory
	for (uint32_t i = 0; i < physDeviceInfo.memProperties.memoryTypeCount; i++)
	{
		if (physDeviceInfo.memProperties.memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eHostVisible)
		{
			physDeviceInfo.memInfo.hostVisible = i;
			break;
		}
	}
	if (physDeviceInfo.memInfo.hostVisible == Constants::invalidIndex)
	{
		Core::LogDebugMessage("Error. Found no host visible memory.");
		std::abort();
	}

	// Find host-visible | device local memory
	for (uint32_t i = 0; i < physDeviceInfo.memProperties.memoryTypeCount; i++)
	{
		vk::MemoryPropertyFlags searchFlags = vk::MemoryPropertyFlagBits::eDeviceLocal | vk::MemoryPropertyFlagBits::eHostVisible;
		if ((physDeviceInfo.memProperties.memoryTypes[i].propertyFlags & searchFlags) == searchFlags)
		{
			if (physDeviceInfo.memProperties.memoryHeaps[i].flags & vk::MemoryHeapFlagBits::eDeviceLocal)
			{
				physDeviceInfo.memInfo.deviceLocalAndHostVisible = i;
				break;
			}
		}
	}

	// Check framebuffer sample count
	assert(physDeviceInfo.properties.limits.framebufferColorSampleCounts == physDeviceInfo.properties.limits.framebufferDepthSampleCounts);
	if (physDeviceInfo.properties.limits.framebufferColorSampleCounts & vk::SampleCountFlagBits::e1)
		physDeviceInfo.maxFramebufferSamples = vk::SampleCountFlagBits::e1;
	if (physDeviceInfo.properties.limits.framebufferColorSampleCounts & vk::SampleCountFlagBits::e2)
		physDeviceInfo.maxFramebufferSamples = vk::SampleCountFlagBits::e2;
	if (physDeviceInfo.properties.limits.framebufferColorSampleCounts & vk::SampleCountFlagBits::e4)
		physDeviceInfo.maxFramebufferSamples = vk::SampleCountFlagBits::e4;


	// Find preferred queues
	const auto queueFamilyProperties = physDeviceInfo.handle.getQueueFamilyProperties();
	// Find render queue
	for (uint32_t i = 0; i < queueFamilyProperties.size(); i++)
	{
		const auto& queueFamily = queueFamilyProperties[i];
		if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
		{
			physDeviceInfo.preferredQueues.graphics.familyIndex = i;
			physDeviceInfo.preferredQueues.graphics.queueIndex = 0;
			break;
		}
	}
	assert(physDeviceInfo.preferredQueues.graphics.familyIndex != Constants::invalidIndex);

	// Find transfer queue, prefer a queue on a different family
	for (uint32_t i = 0; i < queueFamilyProperties.size(); i++)
	{
		if (i == physDeviceInfo.preferredQueues.graphics.familyIndex)
			continue;

		const auto& queueFamily = queueFamilyProperties[i];
		if (queueFamily.queueFlags & vk::QueueFlagBits::eTransfer)
		{
			physDeviceInfo.preferredQueues.transfer.familyIndex = i;
			physDeviceInfo.preferredQueues.transfer.queueIndex = 0;
			break;
		}
	}
	if (physDeviceInfo.preferredQueues.transfer.familyIndex == Constants::invalidIndex)
		physDeviceInfo.preferredQueues.transfer.familyIndex = physDeviceInfo.preferredQueues.graphics.familyIndex;

	return physDeviceInfo;
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

vk::DebugUtilsMessengerEXT DRenderer::Vulkan::Init::CreateDebugMessenger(vk::Instance instance)
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

DRenderer::Vulkan::SwapchainSettings DRenderer::Vulkan::Init::GetSwapchainSettings(vk::PhysicalDevice device, vk::SurfaceKHR surface)
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

vk::Device DRenderer::Vulkan::Init::CreateDevice(const PhysDeviceInfo& physDevice)
{
	// Create logical device
	

	vk::DeviceCreateInfo createInfo{};

	// Feature configuration
	auto physDeviceFeatures = physDevice.handle.getFeatures();

	vk::PhysicalDeviceFeatures features{};
	if (physDeviceFeatures.robustBufferAccess == 1)
		features.robustBufferAccess = true;
	if (physDeviceFeatures.sampleRateShading == 1)
		features.sampleRateShading = true;

	createInfo.pEnabledFeatures = &features;

	// Queue configuration
	float priority[2] = { 1.f, 1.f };
	std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;

	vk::DeviceQueueCreateInfo queueCreateInfo;

	queueCreateInfo.pQueuePriorities = priority;
	queueCreateInfo.queueCount = 1;
	queueCreateInfo.queueFamilyIndex = physDevice.preferredQueues.graphics.familyIndex;
	queueCreateInfos.push_back(queueCreateInfo);

	if (physDevice.preferredQueues.TransferIsSeparate())
	{
		queueCreateInfo.pQueuePriorities = priority;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.queueFamilyIndex = physDevice.preferredQueues.transfer.familyIndex;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	createInfo.queueCreateInfoCount = uint32_t(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();

	auto deviceExtensionsAvailable = physDevice.handle.enumerateDeviceExtensionProperties();
	// TODO: Check if all required device extensions are available
	createInfo.ppEnabledExtensionNames = Constants::requiredDeviceExtensions.data();
	createInfo.enabledExtensionCount = uint32_t(Constants::requiredDeviceExtensions.size());

	//auto deviceLayersAvailable = data.physDevice.handle.enumerateDeviceLayerProperties();
	//createInfo.enabledLayerCount = uint32_t(Vulkan::requiredDeviceLayers.size());
	//createInfo.ppEnabledLayerNames = Vulkan::requiredDeviceLayers.data();

	auto result = physDevice.handle.createDevice(createInfo);
	if (!result)
		std::abort();
	return result;
}

DRenderer::Vulkan::APIData::Swapchain DRenderer::Vulkan::Init::CreateSwapchain(vk::Device device, vk::SurfaceKHR surface, const SwapchainSettings& settings)
{
	APIData::Swapchain swapchain;

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

	swapchain.handle = swapchainResult;

	swapchain.images = device.getSwapchainImagesKHR(swapchain.handle);

	assert(swapchain.images.size() == settings.numImages);

	swapchain.length = uint32_t(swapchain.images.size());

	swapchain.imageViews.resize(swapchain.length);
	for (size_t i = 0; i < swapchain.imageViews.size(); i++)
	{
		vk::ImageViewCreateInfo imageViewCreateInfo{};
		imageViewCreateInfo.image = swapchain.images[i];
		imageViewCreateInfo.format = swapchainCreateInfo.imageFormat;
		imageViewCreateInfo.viewType = vk::ImageViewType::e2D;
		imageViewCreateInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		imageViewCreateInfo.subresourceRange.layerCount = 1;
		imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		imageViewCreateInfo.subresourceRange.levelCount = 1;

		swapchain.imageViews[i] = device.createImageView(imageViewCreateInfo);
	}

	if constexpr (Core::debugLevel >= 2)
	{
		if (Core::GetData().debugData.useDebugging)
		{
			// Make debug names for objects
			for (size_t i = 0; i < swapchain.length; i++)
			{
				vk::DebugUtilsObjectNameInfoEXT imageObjectNameInfo{};
				imageObjectNameInfo.objectType = vk::ObjectType::eImage;
				imageObjectNameInfo.objectHandle = (uint64_t)(VkImage)swapchain.images[i];
				std::string imageName = "Swapchain Image #" + std::to_string(i);
				imageObjectNameInfo.pObjectName = imageName.data();
				device.setDebugUtilsObjectNameEXT(imageObjectNameInfo);

				vk::DebugUtilsObjectNameInfoEXT imageViewObjectNameInfo{};
				imageViewObjectNameInfo.objectType = vk::ObjectType::eImageView;
				imageViewObjectNameInfo.objectHandle = (uint64_t)(VkImageView)swapchain.imageViews[i];
				std::string imageViewName = "Swapchain Image #" + std::to_string(i);
				imageViewObjectNameInfo.pObjectName = imageName.data();
				device.setDebugUtilsObjectNameEXT(imageObjectNameInfo);
			}
		}
	}

	return swapchain;
}

// Note! This does NOT create the associated framebuffer.
DRenderer::Vulkan::APIData::RenderTarget DRenderer::Vulkan::Init::CreateRenderTarget(
		vk::Device device,
		vk::Extent2D extents,
		vk::Format format,
		vk::SampleCountFlagBits sampleCount)
{
	// Setup image for render target
	vk::ImageCreateInfo colorImgInfo{};
	colorImgInfo.format = format;
	colorImgInfo.imageType = vk::ImageType::e2D;
	colorImgInfo.initialLayout = vk::ImageLayout::eUndefined;
	colorImgInfo.mipLevels = 1;
	colorImgInfo.arrayLayers = 1;
	colorImgInfo.samples = sampleCount;
	colorImgInfo.sharingMode = vk::SharingMode::eExclusive;
	colorImgInfo.tiling = vk::ImageTiling::eOptimal;
	colorImgInfo.usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc;
	colorImgInfo.extent = vk::Extent3D{ extents.width, extents.height, 1 };

	vk::Image colorImg = device.createImage(colorImgInfo);
	assert(colorImg);

	vk::MemoryRequirements colorMemReqs = device.getImageMemoryRequirements(colorImg);

	vk::ImageCreateInfo depthImgInfo{};
	depthImgInfo.format = vk::Format::eD32Sfloat;
	depthImgInfo.imageType = vk::ImageType::e2D;
	depthImgInfo.initialLayout = vk::ImageLayout::eUndefined;
	depthImgInfo.mipLevels = 1;
	depthImgInfo.arrayLayers = 1;
	depthImgInfo.samples = sampleCount;
	depthImgInfo.sharingMode = vk::SharingMode::eExclusive;
	depthImgInfo.tiling = vk::ImageTiling::eOptimal;
	depthImgInfo.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment;
	depthImgInfo.extent = vk::Extent3D{ extents.width, extents.height, 1 };

	vk::Image depthImg = device.createImage(depthImgInfo);
	assert(depthImg);

	vk::MemoryRequirements depthMemReqs = device.getImageMemoryRequirements(depthImg);

	size_t depthImgMemOffset = depthMemReqs.alignment * size_t(std::ceil(colorMemReqs.size / double(depthMemReqs.alignment)));
	size_t allocationSize = depthMemReqs.size + depthImgMemOffset;
	vk::MemoryAllocateInfo allocInfo{};
	allocInfo.allocationSize = allocationSize;
	allocInfo.memoryTypeIndex = GetAPIData().physDevice.memInfo.deviceLocal;

	vk::DeviceMemory mem = device.allocateMemory(allocInfo);
	assert(mem);

	device.bindImageMemory(colorImg, mem, 0);
	device.bindImageMemory(depthImg, mem, depthImgMemOffset);

	vk::ImageViewCreateInfo colorImgViewInfo{};
	colorImgViewInfo.image = colorImg;
	colorImgViewInfo.format = colorImgInfo.format;
	colorImgViewInfo.viewType = vk::ImageViewType::e2D;
	colorImgViewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	colorImgViewInfo.subresourceRange.baseArrayLayer = 0;
	colorImgViewInfo.subresourceRange.layerCount = 1;
	colorImgViewInfo.subresourceRange.baseMipLevel = 0;
	colorImgViewInfo.subresourceRange.levelCount = 1;

	vk::ImageView colorImgView = device.createImageView(colorImgViewInfo);
	assert(colorImgView);

	vk::ImageViewCreateInfo depthImgViewInfo{};
	depthImgViewInfo.image = depthImg;
	depthImgViewInfo.format = depthImgInfo.format;
	depthImgViewInfo.viewType = vk::ImageViewType::e2D;
	depthImgViewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
	depthImgViewInfo.subresourceRange.baseArrayLayer = 0;
	depthImgViewInfo.subresourceRange.layerCount = 1;
	depthImgViewInfo.subresourceRange.baseMipLevel = 0;
	depthImgViewInfo.subresourceRange.levelCount = 1;

	vk::ImageView depthImgView = device.createImageView(depthImgViewInfo);
	assert(depthImgView);

	APIData::RenderTarget renderTarget{};
	renderTarget.memory = mem;
	renderTarget.sampleCount = colorImgInfo.samples;
	renderTarget.colorImg = colorImg;
	renderTarget.colorImgView = colorImgView;
	renderTarget.depthImg = depthImg;
	renderTarget.depthImgView = depthImgView;

	if constexpr (Core::debugLevel >= 2)
	{
		if (Core::GetData().debugData.useDebugging)
		{
			// Name our render target frame
			vk::DebugUtilsObjectNameInfoEXT renderTargetNameInfo{};
			renderTargetNameInfo.objectType = vk::ObjectType::eDeviceMemory;
			renderTargetNameInfo.objectHandle = (uint64_t)(VkDeviceMemory)renderTarget.memory;
			renderTargetNameInfo.pObjectName = "Render Target - DeviceMemory";
			device.setDebugUtilsObjectNameEXT(renderTargetNameInfo);

			renderTargetNameInfo.objectType = vk::ObjectType::eImage;
			renderTargetNameInfo.objectHandle = (uint64_t)(VkImage)renderTarget.colorImg;
			renderTargetNameInfo.pObjectName = "Render Target 0 - Color Image";
			device.setDebugUtilsObjectNameEXT(renderTargetNameInfo);

			renderTargetNameInfo.objectType = vk::ObjectType::eImageView;
			renderTargetNameInfo.objectHandle = (uint64_t)(VkImageView)renderTarget.colorImgView;
			renderTargetNameInfo.pObjectName = "Render Target 0 - Color ImageView";
			device.setDebugUtilsObjectNameEXT(renderTargetNameInfo);

			renderTargetNameInfo.objectType = vk::ObjectType::eImage;
			renderTargetNameInfo.objectHandle = (uint64_t)(VkImage)renderTarget.depthImg;
			renderTargetNameInfo.pObjectName = "Render Target 0 - Depth Image";
			device.setDebugUtilsObjectNameEXT(renderTargetNameInfo);

			renderTargetNameInfo.objectType = vk::ObjectType::eImageView;
			renderTargetNameInfo.objectHandle = (uint64_t)(VkImageView)renderTarget.depthImgView;
			renderTargetNameInfo.pObjectName = "Render Target 0 - Depth ImageView";
			device.setDebugUtilsObjectNameEXT(renderTargetNameInfo);
		}
	}

	return renderTarget;
}

vk::RenderPass DRenderer::Vulkan::Init::CreateMainRenderPass(vk::Device device, vk::SampleCountFlagBits sampleCount, vk::Format format)
{
	vk::AttachmentDescription colorAttachment{};
	colorAttachment.initialLayout = vk::ImageLayout::eColorAttachmentOptimal;
	colorAttachment.finalLayout = vk::ImageLayout::eColorAttachmentOptimal;
	colorAttachment.format = format;
	colorAttachment.samples = sampleCount;
	colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
	colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
	colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
	colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;

	vk::AttachmentDescription depthAttach{};
	depthAttach.initialLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
	depthAttach.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
	depthAttach.format = vk::Format::eD32Sfloat;
	depthAttach.samples = sampleCount;
	depthAttach.loadOp = vk::AttachmentLoadOp::eClear;
	depthAttach.storeOp = vk::AttachmentStoreOp::eDontCare;
	depthAttach.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
	depthAttach.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;

	std::array attachments = { colorAttachment, depthAttach };

	vk::AttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

	vk::AttachmentReference depthAttachRef{};
	depthAttachRef.attachment = 1;
	depthAttachRef.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

	vk::SubpassDescription subpassDescription{};
	subpassDescription.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
	subpassDescription.colorAttachmentCount = 1;
	subpassDescription.pColorAttachments = &colorAttachmentRef;
	subpassDescription.pDepthStencilAttachment = &depthAttachRef;

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

vk::Framebuffer DRenderer::Vulkan::Init::CreateRenderTargetFramebuffer(
		vk::Device device,
		vk::RenderPass renderPass,
		vk::Extent2D extents,
		vk::ImageView colorImgView,
		vk::ImageView depthImgView)
{
	std::array attachImgViews
	{
		colorImgView,
		depthImgView
	};

	vk::FramebufferCreateInfo fbCreateInfo{};
	fbCreateInfo.renderPass = renderPass;
	fbCreateInfo.width = extents.width;
	fbCreateInfo.height = extents.height;
	fbCreateInfo.layers = 1;
	fbCreateInfo.attachmentCount = uint32_t(attachImgViews.size());
	fbCreateInfo.pAttachments = attachImgViews.data();

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

void DRenderer::Vulkan::Init::TransitionRenderTargetAndSwapchain(
		vk::Device device,
		vk::Queue queue,
		const APIData::RenderTarget& renderTarget,
		const std::vector<vk::Image>& swapchainImages)
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

	vk::ImageMemoryBarrier colorImgBarrier{};
	colorImgBarrier.image = renderTarget.colorImg;
	colorImgBarrier.srcAccessMask = {};
	colorImgBarrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
	colorImgBarrier.srcQueueFamilyIndex = 0;
	colorImgBarrier.dstQueueFamilyIndex = 0;
	colorImgBarrier.oldLayout = vk::ImageLayout::eUndefined;
	colorImgBarrier.newLayout = vk::ImageLayout::eColorAttachmentOptimal;
	colorImgBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	colorImgBarrier.subresourceRange.baseArrayLayer = 0;
	colorImgBarrier.subresourceRange.layerCount = 1;
	colorImgBarrier.subresourceRange.baseMipLevel = 0;
	colorImgBarrier.subresourceRange.levelCount = 1;

	vk::ImageMemoryBarrier depthImgBarrier{};
	depthImgBarrier.image = renderTarget.depthImg;
	depthImgBarrier.srcAccessMask = {};
	depthImgBarrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
	depthImgBarrier.srcQueueFamilyIndex = 0;
	depthImgBarrier.dstQueueFamilyIndex = 0;
	depthImgBarrier.oldLayout = vk::ImageLayout::eUndefined;
	depthImgBarrier.newLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
	depthImgBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
	depthImgBarrier.subresourceRange.baseArrayLayer = 0;
	depthImgBarrier.subresourceRange.layerCount = 1;
	depthImgBarrier.subresourceRange.baseMipLevel = 0;
	depthImgBarrier.subresourceRange.levelCount = 1;

	vk::PipelineStageFlags srcStage = vk::PipelineStageFlagBits::eTopOfPipe;
	vk::PipelineStageFlags dstStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;

	cmdBuffer.pipelineBarrier(srcStage, dstStage, vk::DependencyFlags(), {}, {}, { colorImgBarrier, depthImgBarrier });

	std::vector<vk::ImageMemoryBarrier> swapchainBarriers(swapchainImages.size());
	for (size_t i = 0; i < swapchainBarriers.size(); i++)
	{
		vk::ImageMemoryBarrier& imgMemBarrier = swapchainBarriers[i];
		imgMemBarrier.image = swapchainImages[i];
		imgMemBarrier.srcAccessMask = {};
		imgMemBarrier.dstAccessMask = {};
		imgMemBarrier.srcQueueFamilyIndex = 0;
		imgMemBarrier.dstQueueFamilyIndex = 0;
		imgMemBarrier.oldLayout = vk::ImageLayout::eUndefined;
		imgMemBarrier.newLayout = vk::ImageLayout::ePresentSrcKHR;
		imgMemBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		imgMemBarrier.subresourceRange.baseArrayLayer = 0;
		imgMemBarrier.subresourceRange.layerCount = 1;
		imgMemBarrier.subresourceRange.baseMipLevel = 0;
		imgMemBarrier.subresourceRange.levelCount = 1;
	}

	dstStage = vk::PipelineStageFlagBits::eBottomOfPipe;

	cmdBuffer.pipelineBarrier(srcStage, dstStage, vk::DependencyFlags(), {}, {}, swapchainBarriers);

	cmdBuffer.end();

	vk::SubmitInfo submitInfo{};
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuffer;

	queue.submit(submitInfo, vk::Fence());

	device.waitIdle();

	device.destroyCommandPool(pool);
}

void DRenderer::Vulkan::Init::SetupRenderingCmdBuffers(vk::Device device, uint8_t swapchainLength, vk::CommandPool& pool, std::vector<vk::CommandBuffer>& commandBuffers)
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

void DRenderer::Vulkan::Init::SetupPresentCmdBuffers(
		vk::Device device,
		const APIData::RenderTarget& renderTarget,
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
		renderTargetPreBarrier.image = renderTarget.colorImg;
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

		if (renderTarget.sampleCount == vk::SampleCountFlagBits::e1)
		{
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
			cmdBuffer.copyImage(renderTarget.colorImg, vk::ImageLayout::eTransferSrcOptimal, swapchain[i], vk::ImageLayout::eTransferDstOptimal, imgCopy);
		}
		else
		{
			vk::ImageResolve imgResolve{};
			imgResolve.extent = vk::Extent3D{ extents.width, extents.height, 1 };
			imgResolve.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
			imgResolve.dstSubresource.baseArrayLayer = 0;
			imgResolve.dstSubresource.layerCount = 1;
			imgResolve.dstSubresource.mipLevel = 0;
			imgResolve.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
			imgResolve.srcSubresource.baseArrayLayer = 0;
			imgResolve.srcSubresource.layerCount = 1;
			imgResolve.srcSubresource.mipLevel = 0;
			cmdBuffer.resolveImage(renderTarget.colorImg, vk::ImageLayout::eTransferSrcOptimal, swapchain[i], vk::ImageLayout::eTransferDstOptimal, imgResolve);
		}

		vk::ImageMemoryBarrier renderTargetPostBarrier{};
		renderTargetPostBarrier.image = renderTarget.colorImg;
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
