#include "Init.hpp"

#include "VulkanExtensionConfig.hpp"

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
		if (!foundExtension)
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

vk::SurfaceKHR DRenderer::Vulkan::Init_CreateSurface(vk::Instance instance, void* hwnd)
{
	vk::SurfaceKHR tempSurfaceHandle;
	bool surfaceCreationTest = GetAPIData().initInfo.test(&instance, hwnd, &tempSurfaceHandle);
	assert(surfaceCreationTest);
	return tempSurfaceHandle;
}

DRenderer::Vulkan::APIData::PhysDeviceInfo DRenderer::Vulkan::Init_LoadPhysDevice(
		vk::Instance instance,
		vk::SurfaceKHR surface)
{
	APIData::PhysDeviceInfo physDeviceInfo{};

	auto physDevices = instance.enumeratePhysicalDevices();
	assert(!physDevices.empty());

	physDeviceInfo.handle = physDevices[0];

	// Check presentation support
	physDeviceInfo.handle.getQueueFamilyProperties();
	bool presentSupport = physDeviceInfo.handle.getSurfaceSupportKHR(0, surface);
	assert(presentSupport && "Error. Queuefamily 0 does not have presentation support.");

	physDeviceInfo.properties = physDeviceInfo.handle.getProperties();

	physDeviceInfo.memProperties = physDeviceInfo.handle.getMemoryProperties();

	// Find device-local memory
	for (uint32_t i = 0; i < physDeviceInfo.memProperties.memoryTypeCount; i++)
	{
		if ((physDeviceInfo.memProperties.memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal)
			== vk::MemoryPropertyFlagBits::eDeviceLocal)
		{
			physDeviceInfo.deviceLocalMemory = i;
			break;
		}
	}
	if (physDeviceInfo.deviceLocalMemory == APIData::invalidIndex)
	{
		Core::LogDebugMessage("Error. Found no device local memory.");
		std::abort();
	}

	// Find host-visible memory
	for (uint32_t i = 0; i < physDeviceInfo.memProperties.memoryTypeCount; i++)
	{
		if ((physDeviceInfo.memProperties.memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eHostVisible)
			== vk::MemoryPropertyFlagBits::eHostVisible)
		{
			physDeviceInfo.hostVisibleMemory = i;
			break;
		}
	}
	if (physDeviceInfo.hostVisibleMemory == APIData::invalidIndex)
	{
		Core::LogDebugMessage("Error. Found no host visible memory.");
		std::abort();
	}


	if (physDeviceInfo.hostVisibleMemory == physDeviceInfo.deviceLocalMemory)
		physDeviceInfo.hostMemoryIsDeviceLocal = true;

	return std::move(physDeviceInfo);
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

DRenderer::Vulkan::APIData::Swapchain DRenderer::Vulkan::Init_CreateSwapchain(vk::Device device, vk::SurfaceKHR surface, const SwapchainSettings& settings)
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

	swapchain.length = swapchain.images.size();

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

	return std::move(swapchain);
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