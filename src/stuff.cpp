#include "stuff.hpp"

namespace Test
{
	VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData)
	{
		

		return 0;
	}

	void AssertVkResult(VkResult result)
	{
		if ((vk::Result)result != vk::Result::eSuccess)
		{
			std::cout << "AssertVkResult went wrong with result value: " << (int)result << std::endl;
			abort();
		}
	}

	Instance CreateVkInstance(Span<const char*> requiredInstanceExtensions)
	{
		vk::Result vkResult{};
		Instance returnVal{};

		// Build the array of the extensions that are required in total
		std::vector<const char*> totalRequiredInstanceExtensions;
		totalRequiredInstanceExtensions.reserve(
			requiredInstanceExtensions.Size() +
			Constants::requiredInstanceExtensions.size() +
			10);
		for (const auto& item : requiredInstanceExtensions)
		{
			totalRequiredInstanceExtensions.push_back(item);
		}
		// Add the constant required extensions
		// But check if they're already present first.
		for (const auto& item : Constants::requiredInstanceExtensions)
		{
			bool extensionAlreadyPresent = false;
			for (const auto& existingExtension : totalRequiredInstanceExtensions)
			{
				if (std::strcmp(item, existingExtension) == 0)
				{
					extensionAlreadyPresent = true;
					break;
				}
			}
			if (extensionAlreadyPresent == false)
			{
				totalRequiredInstanceExtensions.push_back(item);
			}
		}

		uint32_t availableInstanceExtensionCount = 0;
		vkResult = vk::enumerateInstanceExtensionProperties((const char*)nullptr, &availableInstanceExtensionCount, nullptr);
		assert(vkResult == vk::Result::eSuccess || vkResult == vk::Result::eIncomplete);
		std::array<vk::ExtensionProperties, 20> availableExtensions{};
		assert(availableInstanceExtensionCount <= availableExtensions.size());
		vk::enumerateInstanceExtensionProperties(nullptr, &availableInstanceExtensionCount, availableExtensions.data());

		// Check that all total required extensions are available
		for (const auto& required : totalRequiredInstanceExtensions)
		{
			bool requiredExtensionIsAvailable = false;
			for (size_t i = 0; i < (size_t)availableInstanceExtensionCount; i++)
			{
				const char* available = availableExtensions[i].extensionName;
				if (std::strcmp(required, available) == 0)
				{
					requiredExtensionIsAvailable = true;
					break;
				}
			}
			if (requiredExtensionIsAvailable == false)
			{
				std::cout << "Error. Required extension not found." << std::endl;
				std::abort();
			}
		}

		auto& extensionsToUse = totalRequiredInstanceExtensions;

		// Add debug utils if it's present
		bool debugUtilsIsAvailable = false;
		if constexpr (Constants::enableDebugUtils == true)
		{
			for (const auto& item : availableExtensions)
			{
				if (std::strcmp(item.extensionName, Constants::debugUtilsExtensionName.data()) == 0)
				{
					debugUtilsIsAvailable = true;
					break;
				}
			}
			if (debugUtilsIsAvailable == true)
			{
				extensionsToUse.push_back(Constants::debugUtilsExtensionName.data());
			}
		}
		returnVal.debugUtilsEnabled = debugUtilsIsAvailable;

		// Add validation layers if debug utils is available
		std::vector<const char*> validationLayersToUse{};
		if (debugUtilsIsAvailable == true)
		{
			validationLayersToUse.reserve(5);


			uint32_t availableLayerCount = 0;
			vkResult = vk::enumerateInstanceLayerProperties(&availableLayerCount, (vk::LayerProperties*)nullptr);
			assert(vkResult == vk::Result::eSuccess || vkResult == vk::Result::eIncomplete);
			std::array<vk::LayerProperties, 20> availableLayers{};
			assert(availableLayerCount <= availableLayers.size());
			vk::enumerateInstanceLayerProperties(&availableLayerCount, availableLayers.data());

			for (const auto& wantedLayer : Constants::preferredValidationLayers)
			{
				bool layerIsAvailable = false;
				for (size_t i = 0; i < (size_t)availableLayerCount; i++)
				{
					const char* availableLayerName = availableLayers[i].layerName;
					if (std::strcmp(wantedLayer, availableLayerName) == 0)
					{
						layerIsAvailable = true;
						break;
					}
				}
				if (layerIsAvailable == true)
				{
					validationLayersToUse.push_back(wantedLayer);
				}
			}
		}

		vk::InstanceCreateInfo instanceInfo{};
		instanceInfo.enabledExtensionCount = (std::uint32_t)extensionsToUse.size();
		instanceInfo.ppEnabledExtensionNames = extensionsToUse.data();
		instanceInfo.enabledLayerCount = (std::uint32_t)validationLayersToUse.size();
		instanceInfo.ppEnabledLayerNames = validationLayersToUse.data();

		vk::ApplicationInfo appInfo{};
		appInfo.apiVersion = VK_MAKE_VERSION(1, 1, 0);
		appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
		appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
		appInfo.pApplicationName = "DidgyImguiVulkanTest";
		appInfo.pEngineName = "Nothing special";
		instanceInfo.pApplicationInfo = &appInfo;

		vk::Instance vkInstance{};
		vkResult = vk::createInstance(&instanceInfo, nullptr, &vkInstance);
		assert(vkResult == vk::Result::eSuccess || vkInstance);

		returnVal.handle = vkInstance;

		return returnVal;
	}

	vk::DebugUtilsMessengerEXT CreateVkDebugUtilsMessenger(vk::Instance instanceHandle)
	{
		assert(instanceHandle);
		auto createDebugUtilsMessenger = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instanceHandle, "vkCreateDebugUtilsMessengerEXT");
		assert(createDebugUtilsMessenger);
		vk::DebugUtilsMessengerCreateInfoEXT debugMessengerInfo{};
		debugMessengerInfo.messageSeverity =
			vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
			vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning;
		//vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
		//vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose;
		debugMessengerInfo.messageType =
			vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
			vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
			vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation;
		debugMessengerInfo.pfnUserCallback = VulkanDebugCallback;

		vk::DebugUtilsMessengerEXT debugMessenger{};
		vk::Result createDebugMessengerResult = (vk::Result)createDebugUtilsMessenger(instanceHandle, (VkDebugUtilsMessengerCreateInfoEXT*)&debugMessengerInfo, nullptr, (VkDebugUtilsMessengerEXT*)&debugMessenger);
		assert(createDebugMessengerResult == vk::Result::eSuccess && debugMessenger);
		return debugMessenger;
	}

	PhysDeviceInfo LoadPhysDevice(
		vk::Instance instance,
		vk::SurfaceKHR surface)
	{
		PhysDeviceInfo physDeviceInfo{};
		vk::Result vkResult{};

		uint32_t physicalDeviceCount = 0;
		vkResult = instance.enumeratePhysicalDevices(&physicalDeviceCount, (vk::PhysicalDevice*)nullptr);
		assert(vkResult == vk::Result::eSuccess || vkResult == vk::Result::eIncomplete);
		assert(physicalDeviceCount != 0);
		std::array<vk::PhysicalDevice, 10> physDevices{};
		assert(physicalDeviceCount <= physDevices.size());
		instance.enumeratePhysicalDevices(&physicalDeviceCount, physDevices.data());

		physDeviceInfo.handle = physDevices[0];

		// Find preferred queues
		uint32_t queueFamilyPropertyCount = 0;
		physDeviceInfo.handle.getQueueFamilyProperties(&queueFamilyPropertyCount, nullptr);
		assert(queueFamilyPropertyCount != 0);
		std::array<vk::QueueFamilyProperties, 10> availableQueueFamilies{};
		assert(queueFamilyPropertyCount <= availableQueueFamilies.size());
		physDeviceInfo.handle.getQueueFamilyProperties(&queueFamilyPropertyCount, availableQueueFamilies.data());

		// Find graphics queue
		for (uint32_t i = 0; i < queueFamilyPropertyCount; i++)
		{
			const auto& queueFamily = availableQueueFamilies[i];
			if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
			{
				physDeviceInfo.preferredQueues.graphics.familyIndex = i;
				physDeviceInfo.preferredQueues.graphics.queueIndex = 0;
				break;
			}
		}
		assert(physDeviceInfo.preferredQueues.graphics.familyIndex != QueueInfo::invalidIndex);
		// Find transfer queue, prefer a queue on a different family than graphics
		for (uint32_t i = 0; i < (size_t)queueFamilyPropertyCount; i++)
		{
			if (i == physDeviceInfo.preferredQueues.graphics.familyIndex)
				continue;

			const auto& queueFamily = availableQueueFamilies[i];
			if (queueFamily.queueFlags & vk::QueueFlagBits::eTransfer)
			{
				physDeviceInfo.preferredQueues.transfer.familyIndex = i;
				physDeviceInfo.preferredQueues.transfer.queueIndex = 0;
				break;
			}
		}
		if (physDeviceInfo.preferredQueues.transfer.familyIndex == QueueInfo::invalidIndex)
			physDeviceInfo.preferredQueues.transfer.familyIndex = physDeviceInfo.preferredQueues.graphics.familyIndex;

		// Check presentation support
		vk::Bool32 presentSupport = 0;
		vkResult = physDeviceInfo.handle.getSurfaceSupportKHR(0, surface, &presentSupport);
		assert(vkResult == vk::Result::eSuccess);
		assert(presentSupport == 1);

		physDeviceInfo.handle.getProperties(&physDeviceInfo.properties);

		physDeviceInfo.handle.getMemoryProperties(&physDeviceInfo.memProperties);

		// Find device-local memory
		for (uint32_t i = 0; i < physDeviceInfo.memProperties.memoryTypeCount; i++)
		{
			if (physDeviceInfo.memProperties.memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal)
			{
				physDeviceInfo.memInfo.deviceLocal = i;
				break;
			}
		}
		if (physDeviceInfo.memInfo.deviceLocal == MemoryTypes::invalidIndex)
		{
			//Core::LogDebugMessage("Error. Found no device local memory.");
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
		if (physDeviceInfo.memInfo.hostVisible == MemoryTypes::invalidIndex)
		{
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

		return physDeviceInfo;
	}

	vk::Device CreateDevice(const PhysDeviceInfo& physDevice)
	{
		vk::Result vkResult{};

		// Create logical device
		vk::DeviceCreateInfo createInfo{};

		// Feature configuration

		vk::PhysicalDeviceFeatures physDeviceFeatures{};
		physDevice.handle.getFeatures(&physDeviceFeatures);

		vk::PhysicalDeviceFeatures featuresToUse{};
		if (physDeviceFeatures.robustBufferAccess == 1)
			featuresToUse.robustBufferAccess = true;
		if (physDeviceFeatures.sampleRateShading == 1)
			featuresToUse.sampleRateShading = true;

		createInfo.pEnabledFeatures = &featuresToUse;

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

		uint32_t deviceExtensionCount = 0;
		vkResult = physDevice.handle.enumerateDeviceExtensionProperties(nullptr, &deviceExtensionCount, (vk::ExtensionProperties*)nullptr);
		assert(vkResult == vk::Result::eSuccess || vkResult == vk::Result::eIncomplete);
		assert(deviceExtensionCount != 0);
		std::array<vk::ExtensionProperties, 25> deviceExtensionsAvailable{};
		assert(deviceExtensionCount <= deviceExtensionsAvailable.size());
		physDevice.handle.enumerateDeviceExtensionProperties(nullptr, &deviceExtensionCount, deviceExtensionsAvailable.data());
		// TODO: Check if all required device extensions are available
		createInfo.ppEnabledExtensionNames = Constants::requiredDeviceExtensions.data();
		createInfo.enabledExtensionCount = uint32_t(Constants::requiredDeviceExtensions.size());

		// Device layers have been DEPRECATED
		//auto deviceLayersAvailable = data.physDevice.handle.enumerateDeviceLayerProperties();
		//createInfo.enabledLayerCount = uint32_t(Vulkan::requiredDeviceLayers.size());
		//createInfo.ppEnabledLayerNames = Vulkan::requiredDeviceLayers.data();

		vk::Device vkDevice{};
		vkResult = physDevice.handle.createDevice(&createInfo, nullptr, &vkDevice);
		assert(vkResult == vk::Result::eSuccess);
		assert(vkDevice);

		return vkDevice;
	}

	// Get settings for building swapchain
	SwapchainSettings GetSwapchainSettings(vk::PhysicalDevice device, vk::SurfaceKHR surface, SurfaceCapabilities* surfaceCaps)
	{
		vk::Result vkResult{};

		SwapchainSettings settings{};

		settings.surface = surface;

		// Query surface capabilities
		vk::SurfaceCapabilitiesKHR capabilities{};
		vkResult = device.getSurfaceCapabilitiesKHR(surface, &capabilities);
		assert(vkResult == vk::Result::eSuccess);
		settings.capabilities = capabilities;
		settings.extents.width = capabilities.currentExtent.width;
		settings.extents.height = capabilities.currentExtent.height;

		// Handle swapchain length
		std::uint32_t swapchainLength = Constants::preferredSwapchainLength;
		if (swapchainLength > capabilities.maxImageCount&& capabilities.maxImageCount != 0)
			swapchainLength = capabilities.maxImageCount;
		if (swapchainLength < 2)
		{
			std::cout << "Error. Can't make a swapchain with less than 2 images." << std::endl;
			std::abort();
		}
		settings.numImages = swapchainLength;

		// Find supported formats, find the preferred present-mode
		// If not found, fallback to FIFO, it's guaranteed to be supported.
		std::array<vk::PresentModeKHR, 8> supportedPresentModes{};
		for (auto& item : supportedPresentModes)
			item = (vk::PresentModeKHR)std::numeric_limits<std::underlying_type_t<vk::PresentModeKHR>>::max();
		std::uint32_t supportedPresentModeCount = 0;
		vkResult = device.getSurfacePresentModesKHR(surface, &supportedPresentModeCount, (vk::PresentModeKHR*)nullptr);
		assert(vkResult == vk::Result::eSuccess || vkResult == vk::Result::eIncomplete);
		assert(supportedPresentModeCount != 0);
		device.getSurfacePresentModesKHR(surface, &supportedPresentModeCount, supportedPresentModes.data());
		vk::PresentModeKHR preferredPresentMode = Constants::preferredPresentMode;
		vk::PresentModeKHR presentModeToUse{};
		bool preferredPresentModeFound = false;
		for (size_t i = 0; i < supportedPresentModeCount; i++)
		{
			if (supportedPresentModes[i] == preferredPresentMode)
			{
				preferredPresentModeFound = true;
				presentModeToUse = supportedPresentModes[i];
				break;
			}
		}
		if (preferredPresentModeFound == false)
		{
			presentModeToUse = vk::PresentModeKHR::eFifo;
		}
		settings.presentMode = presentModeToUse;

		// Handle formats
		uint32_t surfaceFormatCount = 0;
		vkResult = device.getSurfaceFormatsKHR(surface, &surfaceFormatCount, (vk::SurfaceFormatKHR*)nullptr);
		assert(vkResult == vk::Result::eSuccess || vkResult == vk::Result::eIncomplete);
		assert(surfaceFormatCount != 0);
		std::array<vk::SurfaceFormatKHR, 25> formats{};
		assert(surfaceFormatCount <= formats.size());
		device.getSurfaceFormatsKHR(surface, &surfaceFormatCount, formats.data());
		vk::SurfaceFormatKHR formatToUse = vk::SurfaceFormatKHR{};
		bool foundPreferredFormat = false;
		for (const auto& preferredFormat : Constants::preferredSurfaceFormats)
		{
			for (size_t i = 0; i < (size_t)surfaceFormatCount; i++)
			{
				const auto format = formats[i];
				if (format.format == preferredFormat)
				{
					formatToUse.format = preferredFormat;
					foundPreferredFormat = true;
					break;
				}
			}
			if (foundPreferredFormat)
			{
				break;
			}
		}
		if (!foundPreferredFormat)
		{
			std::cout << "Found no usable surface formats." << std::endl;
			std::abort();
		}
		assert(formatToUse.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear);
		settings.surfaceFormat = formatToUse;

		// Store the capabilities of the surface.
		if (surfaceCaps != nullptr)
		{
			surfaceCaps->surfaceHandle = settings.surface;
			for (size_t i = 0; i < supportedPresentModeCount; i++)
			{
				surfaceCaps->supportedPresentModes[i] = supportedPresentModes[i];
			}
		}

		return settings;
	}

	bool TransitionSwapchainImages(vk::Device vkDevice, vk::Queue vkQueue, Span<const vk::Image> images)
	{
		vk::Result vkResult{};

		vk::CommandPoolCreateInfo cmdPoolInfo{};
		vk::CommandPool cmdPool{};
		vkResult = vkDevice.createCommandPool(&cmdPoolInfo, nullptr, &cmdPool);
		assert(vkResult == vk::Result::eSuccess);

		vk::CommandBufferAllocateInfo cmdBufferAllocInfo{};
		cmdBufferAllocInfo.commandBufferCount = 1;
		cmdBufferAllocInfo.commandPool = cmdPool;
		cmdBufferAllocInfo.level = vk::CommandBufferLevel::ePrimary;
		vk::CommandBuffer cmdBuffer{};
		vk::Result allocCmdBufferResult = vkDevice.allocateCommandBuffers(&cmdBufferAllocInfo, &cmdBuffer);
		assert(allocCmdBufferResult == vk::Result::eSuccess);

		// Record commandbuffer
		{
			vk::CommandBufferBeginInfo cmdBeginInfo{};
			cmdBeginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
			vkResult = cmdBuffer.begin(&cmdBeginInfo);
			assert(vkResult == vk::Result::eSuccess);

			for (size_t i = 0; i < images.Size(); i++)
			{
				vk::ImageMemoryBarrier imgBarrier{};
				imgBarrier.image = images[i];
				imgBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
				imgBarrier.subresourceRange.layerCount = 1;
				imgBarrier.subresourceRange.levelCount = 1;
				imgBarrier.oldLayout = vk::ImageLayout::eUndefined;
				imgBarrier.newLayout = vk::ImageLayout::ePresentSrcKHR;

				cmdBuffer.pipelineBarrier(
					vk::PipelineStageFlagBits::eTopOfPipe, 
					vk::PipelineStageFlagBits::eBottomOfPipe, 
					vk::DependencyFlags{},
					0, nullptr, 
					0, nullptr,
					1, &imgBarrier);
			}

			cmdBuffer.end();
		}

		vk::FenceCreateInfo fenceInfo{};
		vk::Fence fence{};
		vkResult = vkDevice.createFence(&fenceInfo, nullptr, &fence);
		assert(vkResult == vk::Result::eSuccess);

		vk::SubmitInfo submitInfo{};
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &cmdBuffer;
		vkResult = vkQueue.submit(1, &submitInfo, fence);
		assert(vkResult == vk::Result::eSuccess);

		vkResult = vkDevice.waitForFences(1, &fence, vk::Bool32(1), std::numeric_limits<uint64_t>::max());
		assert(vkResult == vk::Result::eSuccess);

		vkDevice.destroyFence(fence, nullptr);
		vkDevice.destroyCommandPool(cmdPool, nullptr);

		return true;
	}

	void RecordCopyRenderedImgCmdBuffers(const SwapchainData& swapchainData, vk::Image srcImg)
	{
		vk::Result vkResult{};

		const Span cmdBuffers = Span(swapchainData.cmdBuffers.data(), swapchainData.imageCount);
		for (size_t i = 0; i < cmdBuffers.Size(); i++)
		{
			vk::CommandBuffer cmdBuffer = cmdBuffers[i];

			vk::CommandBufferBeginInfo beginInfo{};
			beginInfo.flags = vk::CommandBufferUsageFlagBits::eSimultaneousUse;
			vkResult = cmdBuffer.begin(&beginInfo);
			assert(vkResult == vk::Result::eSuccess);

				// The renderpass handles transitioning the rendered image into transfer-src-optimal
				vk::ImageMemoryBarrier imguiPreTransferImgBarrier{};
				imguiPreTransferImgBarrier.image = srcImg;
				imguiPreTransferImgBarrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
				imguiPreTransferImgBarrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
				imguiPreTransferImgBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
				imguiPreTransferImgBarrier.subresourceRange.layerCount = 1;
				imguiPreTransferImgBarrier.subresourceRange.levelCount = 1;
				imguiPreTransferImgBarrier.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
				imguiPreTransferImgBarrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;
				cmdBuffer.pipelineBarrier(
					vk::PipelineStageFlagBits::eColorAttachmentOutput,
					vk::PipelineStageFlagBits::eTransfer,
					vk::DependencyFlags{},
					0, nullptr,
					0, nullptr,
					1, &imguiPreTransferImgBarrier);


				vk::ImageMemoryBarrier preTransferSwapchainImageBarrier{};
				preTransferSwapchainImageBarrier.image = swapchainData.images[i];
				preTransferSwapchainImageBarrier.oldLayout = vk::ImageLayout::ePresentSrcKHR;
				preTransferSwapchainImageBarrier.newLayout = vk::ImageLayout::eTransferDstOptimal;
				preTransferSwapchainImageBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
				preTransferSwapchainImageBarrier.subresourceRange.layerCount = 1;
				preTransferSwapchainImageBarrier.subresourceRange.levelCount = 1;
				preTransferSwapchainImageBarrier.srcAccessMask = vk::AccessFlagBits::eMemoryRead;
				preTransferSwapchainImageBarrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

				cmdBuffer.pipelineBarrier(
					vk::PipelineStageFlagBits::eTopOfPipe,
					vk::PipelineStageFlagBits::eTransfer,
					vk::DependencyFlags{},
					0, nullptr,
					0, nullptr,
					1, &preTransferSwapchainImageBarrier);


				vk::ImageCopy copyRegion{};
				copyRegion.extent = vk::Extent3D{ swapchainData.extents.width, swapchainData.extents.height, 1 };
				copyRegion.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
				copyRegion.srcSubresource.layerCount = 1;
				copyRegion.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
				copyRegion.dstSubresource.layerCount = 1;
				cmdBuffer.copyImage(
					srcImg,
					vk::ImageLayout::eTransferSrcOptimal,
					swapchainData.images[i],
					vk::ImageLayout::eTransferDstOptimal,
					1, &copyRegion);


				vk::ImageMemoryBarrier postTransferSwapchainImageBarrier{};
				postTransferSwapchainImageBarrier.image = swapchainData.images[i];
				postTransferSwapchainImageBarrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
				postTransferSwapchainImageBarrier.newLayout = vk::ImageLayout::ePresentSrcKHR;
				postTransferSwapchainImageBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
				postTransferSwapchainImageBarrier.subresourceRange.layerCount = 1;
				postTransferSwapchainImageBarrier.subresourceRange.levelCount = 1;
				postTransferSwapchainImageBarrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
				postTransferSwapchainImageBarrier.dstAccessMask = vk::AccessFlags{};
				cmdBuffer.pipelineBarrier(
					vk::PipelineStageFlagBits::eTransfer,
					vk::PipelineStageFlagBits::eBottomOfPipe,
					vk::DependencyFlags{},
					0, nullptr,
					0, nullptr,
					1, &postTransferSwapchainImageBarrier);

				// RenderPass handles transitioning rendered image back to color attachment output

			vkResult = cmdBuffer.end();
			assert(vkResult == vk::Result::eSuccess);
		}
	}

	bool CreateSwapchain2(SwapchainData& swapchainData)
	{
		return false;
	}

	SwapchainData CreateSwapchain(vk::Device vkDevice, vk::Queue vkQueue, const SwapchainSettings& settings)
	{
		vk::Result vkResult{};

		SwapchainData swapchain;
		swapchain.extents.width = settings.capabilities.currentExtent.width;
		swapchain.extents.height = settings.capabilities.currentExtent.height;
		swapchain.surfaceFormat = settings.surfaceFormat;
		swapchain.surface = settings.surface;

		vk::SwapchainCreateInfoKHR swapchainCreateInfo{};
		swapchainCreateInfo.imageArrayLayers = 1;
		swapchainCreateInfo.imageExtent = settings.capabilities.currentExtent;
		swapchainCreateInfo.imageFormat = settings.surfaceFormat.format;
		swapchainCreateInfo.imageColorSpace = settings.surfaceFormat.colorSpace;
		swapchainCreateInfo.imageSharingMode = vk::SharingMode::eExclusive;
		swapchainCreateInfo.presentMode = settings.presentMode;
		swapchainCreateInfo.surface = settings.surface;
		swapchainCreateInfo.preTransform = settings.capabilities.currentTransform;
		swapchainCreateInfo.clipped = 1;
		swapchainCreateInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
		swapchainCreateInfo.imageUsage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eColorAttachment;
		swapchainCreateInfo.minImageCount = settings.numImages;
		swapchainCreateInfo.oldSwapchain = vk::SwapchainKHR{};

		vk::SwapchainKHR swapchainHandle{};
		vkResult = vkDevice.createSwapchainKHR(&swapchainCreateInfo, nullptr, &swapchainHandle);
		assert(vkResult == vk::Result::eSuccess);

		swapchain.handle = swapchainHandle;
		swapchain.presentMode = settings.presentMode;

		std::uint32_t swapchainImageCount = 0;
		vkResult = vkDevice.getSwapchainImagesKHR(swapchain.handle, &swapchainImageCount, nullptr);
		assert(vkResult == vk::Result::eSuccess);
		assert(swapchainImageCount != 0);
		assert(swapchainImageCount == settings.numImages);
		assert(swapchainImageCount < swapchain.images.size());
		swapchain.imageCount = swapchainImageCount;
		vkDevice.getSwapchainImagesKHR(swapchain.handle, &swapchainImageCount, swapchain.images.data());
		// Transition new swapchain images
		TransitionSwapchainImages(vkDevice, vkQueue, Span(swapchain.images.data(), swapchain.imageCount));


		vk::CommandPoolCreateInfo cmdPoolInfo{};
		vkResult = vkDevice.createCommandPool(&cmdPoolInfo, nullptr, &swapchain.cmdPool);
		assert(vkResult == vk::Result::eSuccess);
		vk::CommandBufferAllocateInfo cmdBufferAllocInfo{};
		cmdBufferAllocInfo.commandPool = swapchain.cmdPool;
		cmdBufferAllocInfo.commandBufferCount = swapchain.imageCount;
		cmdBufferAllocInfo.level = vk::CommandBufferLevel::ePrimary;
		vkResult = vkDevice.allocateCommandBuffers(&cmdBufferAllocInfo, swapchain.cmdBuffers.data());
		assert(vkResult == vk::Result::eSuccess);

		vk::SemaphoreCreateInfo semaphoreInfo{};
		vkResult = vkDevice.createSemaphore(&semaphoreInfo, nullptr, &swapchain.imgCopyDoneSemaphore);
		assert(vkResult == vk::Result::eSuccess);

		return swapchain;
	}

	bool RecreateSwapchain(SwapchainData& swapchainData, vk::Device vkDevice, vk::Queue vkQueue, SwapchainSettings settings)
	{
		vk::Result vkResult{};

		vk::SwapchainCreateInfoKHR swapchainCreateInfo{};
		swapchainCreateInfo.clipped = 1;
		swapchainCreateInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
		swapchainCreateInfo.imageArrayLayers = 1;
		swapchainCreateInfo.imageColorSpace = swapchainData.surfaceFormat.colorSpace;
		swapchainCreateInfo.imageExtent = settings.extents;
		swapchainCreateInfo.imageFormat = swapchainData.surfaceFormat.format;
		swapchainCreateInfo.imageSharingMode = vk::SharingMode::eExclusive;
		swapchainCreateInfo.imageUsage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eColorAttachment;
		swapchainCreateInfo.minImageCount = swapchainData.imageCount;
		swapchainCreateInfo.oldSwapchain = swapchainData.handle;
		swapchainCreateInfo.surface = swapchainData.surface;
		swapchainCreateInfo.presentMode = swapchainData.presentMode;
		swapchainCreateInfo.preTransform = settings.capabilities.currentTransform;

		vk::SwapchainKHR newSwapchainHandle{};
		vkResult = vkDevice.createSwapchainKHR(&swapchainCreateInfo, nullptr, &newSwapchainHandle);
		assert(vkResult == vk::Result::eSuccess);

		swapchainData.handle = newSwapchainHandle;
		swapchainData.extents = settings.extents;

		std::uint32_t swapchainImageCount = 0;
		vk::Result getSwapchainImagesResult = vkDevice.getSwapchainImagesKHR(swapchainData.handle, &swapchainImageCount, (vk::Image*)nullptr);
		assert(getSwapchainImagesResult == vk::Result::eSuccess);
		assert(swapchainImageCount != 0);
		assert(swapchainImageCount == swapchainData.imageCount);
		assert(swapchainImageCount < swapchainData.images.size());
		swapchainData.imageCount = swapchainImageCount;
		swapchainData.images.fill(vk::Image{});
		vkDevice.getSwapchainImagesKHR(swapchainData.handle, &swapchainImageCount, (vk::Image*)swapchainData.images.data());
		// Transition new swapchain images
		Test::TransitionSwapchainImages(vkDevice, vkQueue, Test::Span(swapchainData.images.data(), swapchainData.imageCount));

		vkDevice.freeCommandBuffers(swapchainData.cmdPool, swapchainData.imageCount, swapchainData.cmdBuffers.data());
		// Clear previous copy-cmdbuffer handles
		swapchainData.cmdBuffers.fill(vk::CommandBuffer{});
		vk::CommandBufferAllocateInfo cmdBufferAllocInfo{};
		cmdBufferAllocInfo.commandPool = swapchainData.cmdPool;
		cmdBufferAllocInfo.commandBufferCount = swapchainData.imageCount;
		cmdBufferAllocInfo.level = vk::CommandBufferLevel::ePrimary;
		vkResult = vkDevice.allocateCommandBuffers(&cmdBufferAllocInfo, swapchainData.cmdBuffers.data());
		assert(vkResult == vk::Result::eSuccess);

		return true;
	}

	ImGuiRenderTarget CreateImGuiRenderTarget(
		vk::Device vkDevice,
		vk::Queue vkQueue,
		vk::Extent2D swapchainDimensions,
		vk::Format swapchainFormat)
	{
		vk::Result vkResult{};

		ImGuiRenderTarget returnVal{};

		// Create the renderpass for imgui
		vk::AttachmentDescription colorAttachment{};
		colorAttachment.initialLayout = vk::ImageLayout::eTransferSrcOptimal;
		colorAttachment.finalLayout = vk::ImageLayout::eTransferSrcOptimal;
		colorAttachment.format = swapchainFormat;
		colorAttachment.samples = vk::SampleCountFlagBits::e1;
		colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
		colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
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

		vkResult = vkDevice.createRenderPass(&createInfo, nullptr, &returnVal.renderPass);
		assert(vkResult == vk::Result::eSuccess);

		// Allocate the rendertarget
		{
			vk::ImageCreateInfo imgInfo{};
			imgInfo.arrayLayers = 1;
			imgInfo.extent = vk::Extent3D{ 2560, 1440, 1 };
			imgInfo.flags = vk::ImageCreateFlagBits{};
			imgInfo.format = swapchainFormat;
			imgInfo.imageType = vk::ImageType::e2D;
			imgInfo.initialLayout = vk::ImageLayout::eUndefined;
			imgInfo.mipLevels = 1;
			imgInfo.samples = vk::SampleCountFlagBits::e1;
			imgInfo.sharingMode = vk::SharingMode::eExclusive;
			imgInfo.tiling = vk::ImageTiling::eOptimal;
			imgInfo.usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc;
			vk::Image tempImg{};
			vkResult = vkDevice.createImage(&imgInfo, nullptr, &tempImg);
			assert(vkResult == vk::Result::eSuccess);

			vk::Image myImage{};
			vkResult = vkDevice.createImage(&imgInfo, nullptr, &myImage);
			assert(vkResult == vk::Result::eSuccess);

			vk::MemoryRequirements memReqs{};
			vkDevice.getImageMemoryRequirements(tempImg, &memReqs);

			vk::MemoryAllocateInfo memAllocInfo{};
			memAllocInfo.allocationSize = memReqs.size;
			memAllocInfo.memoryTypeIndex = 0;
			vkResult = vkDevice.allocateMemory(&memAllocInfo, nullptr, &returnVal.memory);
			assert(vkResult == vk::Result::eSuccess);

			vkDevice.destroyImage(tempImg, nullptr);

			imgInfo = vk::ImageCreateInfo{};
			imgInfo.arrayLayers = 1;
			imgInfo.extent = vk::Extent3D{ swapchainDimensions.width, swapchainDimensions.height, 1 };
			imgInfo.flags = vk::ImageCreateFlagBits{};
			imgInfo.format = swapchainFormat;
			imgInfo.imageType = vk::ImageType::e2D;
			imgInfo.initialLayout = vk::ImageLayout::eUndefined;
			imgInfo.mipLevels = 1;
			imgInfo.samples = vk::SampleCountFlagBits::e1;
			imgInfo.sharingMode = vk::SharingMode::eExclusive;
			imgInfo.tiling = vk::ImageTiling::eOptimal;
			imgInfo.usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc;

			vkResult = vkDevice.createImage(&imgInfo, nullptr, &returnVal.img);
			assert(vkResult == vk::Result::eSuccess);
			returnVal.format = swapchainFormat;

			vkDevice.bindImageMemory(returnVal.img, returnVal.memory, 0);

			vk::ImageViewCreateInfo imgViewInfo{};
			imgViewInfo.components = vk::ComponentMapping{};
			imgViewInfo.flags = vk::ImageViewCreateFlagBits{};
			imgViewInfo.format = swapchainFormat;
			imgViewInfo.image = returnVal.img;
			imgViewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
			imgViewInfo.subresourceRange.baseArrayLayer = 0;
			imgViewInfo.subresourceRange.baseMipLevel = 0;
			imgViewInfo.subresourceRange.layerCount = 1;
			imgViewInfo.subresourceRange.levelCount = 1;
			imgViewInfo.viewType = vk::ImageViewType::e2D;

			vkResult = vkDevice.createImageView(&imgViewInfo, nullptr, &returnVal.imgView);
			assert(vkResult == vk::Result::eSuccess);
		}
		// Allocate the rendertarget end


		vk::FramebufferCreateInfo fbInfo{};
		fbInfo.attachmentCount = 1;
		fbInfo.flags = vk::FramebufferCreateFlagBits{};
		fbInfo.width = swapchainDimensions.width;
		fbInfo.height = swapchainDimensions.height;
		fbInfo.layers = 1;
		fbInfo.pAttachments = &returnVal.imgView;
		fbInfo.renderPass = returnVal.renderPass;
		vkResult = vkDevice.createFramebuffer(&fbInfo, nullptr, &returnVal.framebuffer);
		assert(vkResult == vk::Result::eSuccess);

		// Transition the image
		vk::CommandPoolCreateInfo cmdPoolInfo{};
		cmdPoolInfo.queueFamilyIndex = 0;
		cmdPoolInfo.flags = vk::CommandPoolCreateFlagBits{};
		vk::CommandPool cmdPool{};
		vkResult = vkDevice.createCommandPool(&cmdPoolInfo, nullptr, &cmdPool);
		assert(vkResult == vk::Result::eSuccess);
		vk::CommandBufferAllocateInfo cmdBufferAllocInfo{};
		cmdBufferAllocInfo.commandPool = cmdPool;
		cmdBufferAllocInfo.level = vk::CommandBufferLevel::ePrimary;
		cmdBufferAllocInfo.commandBufferCount = 1;
		vk::CommandBuffer cmdBuffer{};
		vkResult = vkDevice.allocateCommandBuffers(&cmdBufferAllocInfo, &cmdBuffer);
		assert(vkResult == vk::Result::eSuccess);
		vk::CommandBufferBeginInfo cmdBufferBeginInfo{};
		cmdBufferBeginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

		vkResult = cmdBuffer.begin(&cmdBufferBeginInfo);
		assert(vkResult == vk::Result::eSuccess);

			vk::ImageMemoryBarrier imgMemoryBarrier{};
			imgMemoryBarrier.image = returnVal.img;
			imgMemoryBarrier.oldLayout = vk::ImageLayout::eUndefined;
			imgMemoryBarrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
			imgMemoryBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
			imgMemoryBarrier.subresourceRange.baseArrayLayer = 0;
			imgMemoryBarrier.subresourceRange.layerCount = 1;
			imgMemoryBarrier.subresourceRange.baseMipLevel = 0;
			imgMemoryBarrier.subresourceRange.levelCount = 1;
			imgMemoryBarrier.srcAccessMask = vk::AccessFlagBits{};
			imgMemoryBarrier.dstAccessMask = vk::AccessFlagBits{};

			cmdBuffer.pipelineBarrier(
				vk::PipelineStageFlagBits::eTopOfPipe,
				vk::PipelineStageFlagBits::eBottomOfPipe,
				vk::DependencyFlags{},
				0, nullptr,
				0, nullptr,
				1, &imgMemoryBarrier);

		cmdBuffer.end();


		vk::FenceCreateInfo fenceInfo{};
		vk::Fence tempFence{};
		vkResult = vkDevice.createFence(&fenceInfo, nullptr, &tempFence);
		assert(vkResult == vk::Result::eSuccess);

		vk::SubmitInfo submitInfo{};
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &cmdBuffer;
		vkResult = vkQueue.submit(1, &submitInfo, tempFence);
		assert(vkResult == vk::Result::eSuccess);


		vkResult = vkDevice.waitForFences(1, &tempFence, 1, std::numeric_limits<uint64_t>::max());
		assert(vkResult == vk::Result::eSuccess);

		vkDevice.destroyFence(tempFence, nullptr);
		vkDevice.destroyCommandPool(cmdPool, nullptr);

		return returnVal;
	}

	void RecreateImGuiRenderTarget(ImGuiRenderTarget& data, vk::Device vkDevice, vk::Queue vkQueue, vk::Extent2D newExtents)
	{
		vk::Result vkResult{};

		vkDevice.destroyFramebuffer(data.framebuffer, nullptr);
		data.framebuffer = vk::Framebuffer{};
		vkDevice.destroyImageView(data.imgView, nullptr);
		data.imgView = vk::ImageView{};
		vkDevice.destroyImage(data.img, nullptr);
		data.img = vk::Image{};


		vk::ImageCreateInfo imgInfo{};
		imgInfo.arrayLayers = 1;
		imgInfo.extent = vk::Extent3D{ newExtents.width, newExtents.height, 1 };
		imgInfo.flags = vk::ImageCreateFlagBits{};
		imgInfo.format = data.format;
		imgInfo.imageType = vk::ImageType::e2D;
		imgInfo.initialLayout = vk::ImageLayout::eUndefined;
		imgInfo.mipLevels = 1;
		imgInfo.samples = vk::SampleCountFlagBits::e1;
		imgInfo.sharingMode = vk::SharingMode::eExclusive;
		imgInfo.tiling = vk::ImageTiling::eOptimal;
		imgInfo.usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc;

		vkResult = vkDevice.createImage(&imgInfo, nullptr, &data.img);
		assert(vkResult == vk::Result::eSuccess);
		vkDevice.bindImageMemory(data.img, data.memory, 0);


		vk::ImageViewCreateInfo imgViewInfo{};
		imgViewInfo.components = vk::ComponentMapping{};
		imgViewInfo.flags = vk::ImageViewCreateFlagBits{};
		imgViewInfo.format = data.format;
		imgViewInfo.image = data.img;
		imgViewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		imgViewInfo.subresourceRange.baseArrayLayer = 0;
		imgViewInfo.subresourceRange.baseMipLevel = 0;
		imgViewInfo.subresourceRange.layerCount = 1;
		imgViewInfo.subresourceRange.levelCount = 1;
		imgViewInfo.viewType = vk::ImageViewType::e2D;

		vkResult = vkDevice.createImageView(&imgViewInfo, nullptr, &data.imgView);
		assert(vkResult == vk::Result::eSuccess);


		vk::FramebufferCreateInfo fbInfo{};
		fbInfo.attachmentCount = 1;
		fbInfo.flags = vk::FramebufferCreateFlagBits{};
		fbInfo.width = newExtents.width;
		fbInfo.height = newExtents.height;
		fbInfo.layers = 1;
		fbInfo.pAttachments = &data.imgView;
		fbInfo.renderPass = data.renderPass;

		vkResult = vkDevice.createFramebuffer(&fbInfo, nullptr, &data.framebuffer);
		assert(vkResult == vk::Result::eSuccess);


		// Transition the image


		vk::CommandPoolCreateInfo cmdPoolInfo{};
		cmdPoolInfo.queueFamilyIndex = 0;
		cmdPoolInfo.flags = vk::CommandPoolCreateFlagBits{};
		vk::CommandPool cmdPool{};
		vkResult = vkDevice.createCommandPool(&cmdPoolInfo, nullptr, &cmdPool);
		assert(vkResult == vk::Result::eSuccess);
		vk::CommandBufferAllocateInfo cmdBufferAllocInfo{};
		cmdBufferAllocInfo.commandPool = cmdPool;
		cmdBufferAllocInfo.level = vk::CommandBufferLevel::ePrimary;
		cmdBufferAllocInfo.commandBufferCount = 1;
		vk::CommandBuffer cmdBuffer{};
		vkResult = vkDevice.allocateCommandBuffers(&cmdBufferAllocInfo, &cmdBuffer);
		assert(vkResult == vk::Result::eSuccess);

		vk::CommandBufferBeginInfo cmdBufferBeginInfo{};
		cmdBufferBeginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
		vkResult = cmdBuffer.begin(&cmdBufferBeginInfo);
		assert(vkResult == vk::Result::eSuccess);

			vk::ImageMemoryBarrier imgMemoryBarrier{};
			imgMemoryBarrier.image = data.img;
			imgMemoryBarrier.oldLayout = vk::ImageLayout::eUndefined;
			imgMemoryBarrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
			imgMemoryBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
			imgMemoryBarrier.subresourceRange.baseArrayLayer = 0;
			imgMemoryBarrier.subresourceRange.layerCount = 1;
			imgMemoryBarrier.subresourceRange.baseMipLevel = 0;
			imgMemoryBarrier.subresourceRange.levelCount = 1;
			imgMemoryBarrier.srcAccessMask = vk::AccessFlagBits{};
			imgMemoryBarrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

			cmdBuffer.pipelineBarrier(
				vk::PipelineStageFlagBits::eTopOfPipe,
				vk::PipelineStageFlagBits::eColorAttachmentOutput,
				vk::DependencyFlags{},
				0, nullptr,
				0, nullptr,
				1, &imgMemoryBarrier);

		cmdBuffer.end();

		vk::FenceCreateInfo tempFenceInfo{};
		vk::Fence tempFence{}; 
		vkResult = vkDevice.createFence(&tempFenceInfo, nullptr, &tempFence);
		assert(vkResult == vk::Result::eSuccess);

		vk::SubmitInfo submitInfo{};
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &cmdBuffer;
		vkResult = vkQueue.submit(1, &submitInfo, tempFence);
		assert(vkResult == vk::Result::eSuccess);

		vkResult = vkDevice.waitForFences(1, &tempFence, 1, std::numeric_limits<uint64_t>::max());
		assert(vkResult == vk::Result::eSuccess);

		vkDevice.destroyFence(tempFence, nullptr);
		vkDevice.destroyCommandPool(cmdPool, nullptr);
	}

	vk::DescriptorPool CreateDescrPoolForImgui(vk::Device vkDevice)
	{
		VkDescriptorPoolSize pool_sizes[] =
		{
			{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
		};
		VkDescriptorPoolCreateInfo pool_info = {};
		pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		//pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
		//pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
		pool_info.pPoolSizes = pool_sizes;

		VkDescriptorPool descrPoolHandle{};
		auto result = (vk::Result)vkCreateDescriptorPool(vkDevice, &pool_info, nullptr, &descrPoolHandle);
		assert(result == vk::Result::eSuccess);
		return (vk::DescriptorPool)descrPoolHandle;
	}

	void InitImguiStuff(
		SDL_Window* sdlWindow,
		vk::Instance vkInstance,
		vk::PhysicalDevice physDevice,
		vk::Device vkDevice,
		vk::Queue vkQueue,
		vk::RenderPass rPass)
	{
		vk::Result vkResult{};
		/*
		// IMGUI INIT STUFF
		IMGUI_CHECKVERSION();
		ImGuiContext* imguiContext = ImGui::CreateContext();
		ImGuiIO& imguiIo = ImGui::GetIO(); (void)imguiIo;
		imguiIo.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
		//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
		imguiIo.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
		//imguiIo.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
		//io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoTaskBarIcons;
		//io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoMerge;
		// Setup Dear ImGui style
		ImGui::StyleColorsDark();
		// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
		ImGuiStyle& style = ImGui::GetStyle();
		if (imguiIo.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}
		// Init Vulkan stuff
		ImGui_ImplSDL2_InitForVulkan(sdlWindow);
		ImGui_ImplVulkan_InitInfo init_info = {};
		init_info.Instance = vkInstance;
		init_info.PhysicalDevice = physDevice;
		init_info.Device = vkDevice;
		init_info.QueueFamily = 0;
		init_info.Queue = vkQueue;
		init_info.PipelineCache = {};
		init_info.DescriptorPool = Test::CreateDescrPoolForImgui(vkDevice);
		init_info.Allocator = {};
		init_info.MinImageCount = 3;
		init_info.ImageCount = 3;
		init_info.CheckVkResultFn = Test::AssertVkResult;
		ImGui_ImplVulkan_Init(&init_info, rPass);
		// Upload Fonts
		{
			vk::CommandPoolCreateInfo cmdPoolInfo{};
			cmdPoolInfo.queueFamilyIndex = 0;
			cmdPoolInfo.flags = vk::CommandPoolCreateFlagBits{};
			vk::CommandPool cmdPool{};
			vkResult = vkDevice.createCommandPool(&cmdPoolInfo, nullptr, &cmdPool);
			assert(vkResult == vk::Result::eSuccess);
			vk::CommandBufferAllocateInfo cmdBufferAllocInfo{};
			cmdBufferAllocInfo.commandPool = cmdPool;
			cmdBufferAllocInfo.level = vk::CommandBufferLevel::ePrimary;
			cmdBufferAllocInfo.commandBufferCount = 1;
			vk::CommandBuffer cmdBuffer{};
			vkResult = vkDevice.allocateCommandBuffers(&cmdBufferAllocInfo, &cmdBuffer);

			vk::CommandBufferBeginInfo cmdBufferBeginInfo{};
			cmdBufferBeginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
			vkResult = cmdBuffer.begin(&cmdBufferBeginInfo);
			assert(vkResult == vk::Result::eSuccess);

				ImGui_ImplVulkan_CreateFontsTexture(cmdBuffer);

			cmdBuffer.end();

			vk::FenceCreateInfo tempFenceInfo{};
			vk::Fence tempFence{};
			vkResult = vkDevice.createFence(&tempFenceInfo, nullptr, &tempFence);
			assert(vkResult == vk::Result::eSuccess);

			vk::SubmitInfo submitInfo{};
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &cmdBuffer;
			vkResult = vkQueue.submit(1, &submitInfo, tempFence);
			assert(vkResult == vk::Result::eSuccess);

			vkResult = vkDevice.waitForFences(1, &tempFence, 1, std::numeric_limits<uint64_t>::max());
			assert(vkResult == vk::Result::eSuccess);

			ImGui_ImplVulkan_DestroyFontUploadObjects();

			vkDevice.destroyCommandPool(cmdPool, nullptr);
			vkDevice.destroyFence(tempFence, nullptr);
		}
		*/
	}

	ImguiRenderCmdBuffers MakeImguiRenderCmdBuffers(vk::Device vkDevice, uint32_t resourceSetCount)
	{
		vk::Result vkResult{};

		ImguiRenderCmdBuffers returnVal{};

		std::array<vk::CommandBuffer, Test::Constants::maxResourceSets> imguiRenderCmdBuffers{};
		vk::CommandPoolCreateInfo imguiRenderCmdPoolInfo{};
		imguiRenderCmdPoolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
		vk::CommandPool imguiRenderCmdPool{};
		vkResult = vkDevice.createCommandPool(&imguiRenderCmdPoolInfo, nullptr, &imguiRenderCmdPool);
		assert(vkResult == vk::Result::eSuccess);
		vk::CommandBufferAllocateInfo imguiRenderCmdBufferAllocInfo{};
		imguiRenderCmdBufferAllocInfo.commandPool = imguiRenderCmdPool;
		imguiRenderCmdBufferAllocInfo.level = vk::CommandBufferLevel::ePrimary;
		imguiRenderCmdBufferAllocInfo.commandBufferCount = resourceSetCount;
		vkResult = vkDevice.allocateCommandBuffers(&imguiRenderCmdBufferAllocInfo, imguiRenderCmdBuffers.data());
		assert(vkResult == vk::Result::eSuccess);

		std::array<vk::Fence, Constants::maxResourceSets> fences{};
		for (size_t i = 0; i < resourceSetCount; i++)
		{
			vk::FenceCreateInfo fenceInfo{};
			fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;
			vkResult = vkDevice.createFence(&fenceInfo, nullptr, &fences[i]);
			assert(vkResult == vk::Result::eSuccess);
		}

		returnVal.cmdPool = imguiRenderCmdPool;
		returnVal.cmdBuffers = imguiRenderCmdBuffers;
		returnVal.fences = fences;

		return returnVal;
	}
}