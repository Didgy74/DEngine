#include "DEngine/Gfx/Vk/Vk.hpp"
#include "DEngine/Gfx/VkInterface.hpp"
#include "DEngine/Gfx/Assert.hpp"

#include "DEngine/Int.hpp"
#include "DEngine/Containers/Span.hpp"

#include <cstring>
#include <vector>
#include <fstream>

namespace DEngine::Gfx::Vk
{
	VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(
		vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		vk::DebugUtilsMessageTypeFlagsEXT messageType,
		const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData)
	{
		const ILog* logger = reinterpret_cast<const ILog*>(pUserData);

		if (logger != nullptr)
			logger->log(pCallbackData->pMessage);

		return 0;
	}

	bool CreateVkInstance(APIData& apiData, Cont::Span<const char*> requiredExtensions, const ILog* logger);

	vk::DebugUtilsMessengerEXT CreateVkDebugUtilsMessenger(vk::Instance instanceHandle, const void* userData);

	PhysDeviceInfo LoadPhysDevice(
		vk::Instance instance,
		vk::SurfaceKHR surface,
		const ILog* logger);

	vk::Device CreateDevice(const PhysDeviceInfo& physDevice);

	SwapchainSettings GetSwapchainSettings(
		vk::PhysicalDevice device,
		vk::SurfaceKHR surface,
		SurfaceCapabilities* surfaceCaps,
		const ILog* logger);

	SwapchainData CreateSwapchain(vk::Device vkDevice, vk::Queue vkQueue, const SwapchainSettings& settings);

	bool RecreateSwapchain(SwapchainData& swapchainData, vk::Device vkDevice, vk::Queue vkQueue, SwapchainSettings settings);

	bool TransitionSwapchainImages(vk::Device vkDevice, vk::Queue vkQueue, Cont::Span<const vk::Image> images);

	void RecordCopyRenderedImgCmdBuffers(const SwapchainData& swapchainData, vk::Image srcImg);

	ImGuiRenderTarget CreateImGuiRenderTarget(
		vk::Device vkDevice,
		vk::Queue vkQueue,
		vk::Extent2D swapchainDimensions,
		vk::Format swapchainFormat);

	ImguiRenderCmdBuffers MakeImguiRenderCmdBuffers(vk::Device vkDevice, uint32_t resourceSetCount);

	void TestImageBleh(APIData& apiData);

	void Test(APIData& apiData);
}

DEngine::uSize DEngine::Gfx::Vk::GetAPIDataSize()
{
	return sizeof(Vk::APIData);
}

void DEngine::Gfx::Vk::Draw(Data& gfxData, void* apiDataBuffer, float scale)
{
	APIData& apiData = *reinterpret_cast<APIData*>(apiDataBuffer);
	vk::Result vkResult{};

	// Record and submit rendering workload
	{
		const u32 resourceSet = apiData.currentResourceSet;
		vkResult = apiData.device.waitForFences(1, &apiData.imguiRenderCmdBuffers.fences[resourceSet], vk::Bool32{ 1 }, std::numeric_limits<uint64_t>::max());
		DENGINE_GFX_ASSERT(vkResult == vk::Result::eSuccess);
		apiData.device.resetFences(1, &apiData.imguiRenderCmdBuffers.fences[resourceSet]);

		{
			// Update the scale value
			float* ptr = (float*)apiData.bufferData;

			ptr[resourceSet * 4] = scale;

			vk::MappedMemoryRange memRange{};
			memRange.memory = apiData.testMem;
			memRange.offset = vk::DeviceSize(resourceSet) * vk::DeviceSize(4);
			memRange.size = sizeof(float) * 4;
			//apiData.device.flushMappedMemoryRanges(1, &memRange);
		}


		vk::CommandBuffer cmdBuffer = apiData.imguiRenderCmdBuffers.cmdBuffers[resourceSet];

		vk::CommandBufferBeginInfo beginInfo{};
		beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
		vkResult = cmdBuffer.begin(&beginInfo);
		DENGINE_GFX_ASSERT(vkResult == vk::Result::eSuccess);
		{
			vk::RenderPassBeginInfo rpBeginInfo{};
			rpBeginInfo.renderPass = apiData.imguiRenderTarget.renderPass;
			rpBeginInfo.renderArea.extent = apiData.swapchain.extents;
			rpBeginInfo.framebuffer = apiData.imguiRenderTarget.framebuffer;
			rpBeginInfo.clearValueCount = 1;
			vk::ClearColorValue clearColorVal{};
			clearColorVal.setFloat32({ 0.25f, 0.0f, 0.0f, 0.f });
			vk::ClearValue clearVal{};
			clearVal.color = clearColorVal;
			rpBeginInfo.pClearValues = &clearVal;
			cmdBuffer.beginRenderPass(&rpBeginInfo, vk::SubpassContents::eInline);
			{
				cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, apiData.testPipeline);
				cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, apiData.testPipelineLayout, 0, 1, &apiData.descrSets[resourceSet], 0, nullptr);
				cmdBuffer.draw(4, 1, 0, 0);
			}
			cmdBuffer.endRenderPass();
		}
		cmdBuffer.end();

		vk::SubmitInfo imguiRenderSubmitInfo{};
		imguiRenderSubmitInfo.commandBufferCount = 1;
		imguiRenderSubmitInfo.pCommandBuffers = &cmdBuffer;
		apiData.tempQueue.submit(1, &imguiRenderSubmitInfo, apiData.imguiRenderCmdBuffers.fences[resourceSet]);

		apiData.currentResourceSet = (apiData.currentResourceSet + 1) % apiData.resourceSetCount;
	}

	u32 nextSwapchainImgIndex = Constants::invalidIndex;
	vkResult = apiData.device.acquireNextImageKHR(
		apiData.swapchain.handle, 
		std::numeric_limits<u64>::max(), 
		apiData.swapchain.imgCopyDoneSemaphore, 
		vk::Fence(), 
		&nextSwapchainImgIndex);
	DENGINE_GFX_ASSERT(vkResult == vk::Result::eSuccess);
	DENGINE_GFX_ASSERT(nextSwapchainImgIndex != Constants::invalidIndex);
	
	{
		// We don't need semaphore (I think) between this cmd buffer and the rendering one, it's handled by barriers
		vk::SubmitInfo copyImageSubmitInfo{};
		copyImageSubmitInfo.commandBufferCount = 1;
		copyImageSubmitInfo.pCommandBuffers = &apiData.swapchain.cmdBuffers[nextSwapchainImgIndex];
		const vk::PipelineStageFlags imgCopyDoneStage = vk::PipelineStageFlagBits::eTransfer;
		copyImageSubmitInfo.waitSemaphoreCount = 1;
		copyImageSubmitInfo.pWaitSemaphores = &apiData.swapchain.imgCopyDoneSemaphore;
		copyImageSubmitInfo.pWaitDstStageMask = &imgCopyDoneStage;
		apiData.tempQueue.submit(1, &copyImageSubmitInfo, vk::Fence());
	}

	{
		vk::PresentInfoKHR presentInfo{};
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &apiData.swapchain.handle;
		presentInfo.pImageIndices = &nextSwapchainImgIndex;

		vkResult = apiData.tempQueue.presentKHR(&presentInfo);
		DENGINE_GFX_ASSERT(vkResult == vk::Result::eSuccess);
	}
}

bool DEngine::Gfx::Vk::InitializeBackend(Data& gfxData, const InitInfo& initInfo, void* apiDataBuffer)
{
	APIData& apiData = *(new(apiDataBuffer) APIData);
	bool result = false;

	result = CreateVkInstance(apiData, initInfo.requiredVkInstanceExtensions, initInfo.iLog);
	DENGINE_GFX_ASSERT(result == true);

	if (apiData.debugUtilsEnabled)
	{
		apiData.debugMessenger = CreateVkDebugUtilsMessenger(apiData.instance, initInfo.iLog);
	}

	// Create the VkSurface using the callback;
	vk::SurfaceKHR surface{};
	result = initInfo.createVkSurfacePFN((u64)(VkInstance)apiData.instance, initInfo.createVkSurfaceUserData, reinterpret_cast<u64*>(&surface));
	DENGINE_GFX_ASSERT(result == true);
	apiData.surface = surface;

	apiData.physDeviceInfo = LoadPhysDevice(apiData.instance, apiData.surface, initInfo.iLog);

	apiData.device = CreateDevice(apiData.physDeviceInfo);

	SwapchainSettings swapchainSettings = GetSwapchainSettings(apiData.physDeviceInfo.handle, apiData.surface, nullptr, initInfo.iLog);

	apiData.device.getQueue(0, 0, &apiData.tempQueue);
	DENGINE_GFX_ASSERT(apiData.tempQueue);

	apiData.imguiRenderTarget = CreateImGuiRenderTarget(apiData.device, apiData.tempQueue, swapchainSettings.extents, swapchainSettings.surfaceFormat.format);

	apiData.swapchain = CreateSwapchain(apiData.device, apiData.tempQueue, swapchainSettings);

	RecordCopyRenderedImgCmdBuffers(apiData.swapchain, apiData.imguiRenderTarget.img);

	sizeof(vk::ExtensionProperties);

	apiData.resourceSetCount = 3;
	apiData.currentResourceSet = 0;

	apiData.imguiRenderCmdBuffers = MakeImguiRenderCmdBuffers(apiData.device, apiData.resourceSetCount);

	TestImageBleh(apiData);

	Test(apiData);

	return true;
}

bool DEngine::Gfx::Vk::CreateVkInstance(APIData& apiData, Cont::Span<const char*> requiredExtensions, const ILog* logger)
{
	vk::Result vkResult{};

	// Build what extensions we are going to use
	Cont::Array<const char*, 10> totalRequiredExtensions{};
	u8 totalRequiredExtensionCount = 0;
	// First copy all required instance extensions
	DENGINE_GFX_ASSERT(requiredExtensions.size() <= totalRequiredExtensions.size());
	for (uSize i = 0; i < requiredExtensions.size(); i++)
	{
		totalRequiredExtensions[i] = requiredExtensions[i];
	}
	totalRequiredExtensionCount = (u8)requiredExtensions.size();

	// Next add extensions required by renderer, don't add duplicates
	for (const char* requiredExtension : Constants::requiredInstanceExtensions)
	{
		bool extensionAlreadyPresent = false;
		for (const auto& existingExtension : totalRequiredExtensions)
		{
			if (std::strcmp(requiredExtension, existingExtension) == 0)
			{
				extensionAlreadyPresent = true;
				break;
			}
		}
		if (extensionAlreadyPresent == false)
		{
			DENGINE_GFX_ASSERT(((uSize)totalRequiredExtensionCount)+(uSize)1 < totalRequiredExtensions.size());
			totalRequiredExtensionCount++;
			totalRequiredExtensions[totalRequiredExtensionCount - 1] = requiredExtension;
		}
	}

	// Gather all available instance extensions
	u32 availableInstanceExtensionCount = 0;
	vkResult = vk::enumerateInstanceExtensionProperties((const char*)nullptr, &availableInstanceExtensionCount, nullptr);
	DENGINE_GFX_ASSERT(vkResult == vk::Result::eSuccess || vkResult == vk::Result::eIncomplete);
	Cont::Array<vk::ExtensionProperties, 20> availableExtensions{};
	DENGINE_GFX_ASSERT(availableInstanceExtensionCount <= availableExtensions.size());
	vk::enumerateInstanceExtensionProperties(nullptr, &availableInstanceExtensionCount, availableExtensions.data());

	// Check that all total required extensions are available
	for (uSize i = 0; i < (uSize)totalRequiredExtensionCount; i++)
	{
		const char* required = totalRequiredExtensions[i];
		bool requiredExtensionIsAvailable = false;
		for (uSize j = 0; j < (uSize)availableInstanceExtensionCount; j++)
		{
			const char* available = availableExtensions[j].extensionName;
			if (std::strcmp(required, available) == 0)
			{
				requiredExtensionIsAvailable = true;
				break;
			}
		}
		if (requiredExtensionIsAvailable == false)
		{
			if (logger != nullptr)
			{
				logger->log("Error. Required extension not found.");
			}
			std::abort();
		}
	}

	// Add debug utils if it's present
	if constexpr (Constants::enableDebugUtils == true)
	{
		bool debugUtilsIsAvailable = false;
		const char* debugUtilsName = Constants::debugUtilsExtensionName.data();
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
			DENGINE_GFX_ASSERT((uSize)totalRequiredExtensionCount + (uSize)1 < totalRequiredExtensions.size());
			totalRequiredExtensionCount++;
			totalRequiredExtensions[totalRequiredExtensionCount - 1] = debugUtilsName;

			apiData.debugUtilsEnabled = debugUtilsIsAvailable;
		}
	}


	// Add validation layers if debug utils is available
	Cont::Array<const char*, 5> layersToUse{};
	u8 layersToUseCount = 0;
	if (apiData.debugUtilsEnabled == true)
	{
		u32 availableLayerCount = 0;
		vkResult = vk::enumerateInstanceLayerProperties(&availableLayerCount, (vk::LayerProperties*)nullptr);
		DENGINE_GFX_ASSERT(vkResult == vk::Result::eSuccess || vkResult == vk::Result::eIncomplete);
		Cont::Array<vk::LayerProperties, 20> availableLayers{};
		DENGINE_GFX_ASSERT(availableLayerCount <= availableLayers.size());
		vk::enumerateInstanceLayerProperties(&availableLayerCount, availableLayers.data());

		for (const auto& wantedLayer : Constants::preferredValidationLayers)
		{
			bool layerIsAvailable = false;
			for (uSize i = 0; i < (uSize)availableLayerCount; i++)
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
				DENGINE_GFX_ASSERT((uSize)layersToUseCount+(uSize)1 < layersToUse.size());
				layersToUseCount++;
				layersToUse[layersToUseCount - 1] = wantedLayer;
			}
		}
	}

	vk::InstanceCreateInfo instanceInfo{};
	instanceInfo.enabledExtensionCount = (u32)totalRequiredExtensionCount;
	instanceInfo.ppEnabledExtensionNames = totalRequiredExtensions.data();
	instanceInfo.enabledLayerCount = (u32)layersToUseCount;
	instanceInfo.ppEnabledLayerNames = layersToUse.data();

	vk::ApplicationInfo appInfo{};
	appInfo.apiVersion = VK_MAKE_VERSION(1, 1, 0);
	appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
	appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
	appInfo.pApplicationName = "DidgyImguiVulkanTest";
	appInfo.pEngineName = "Nothing special";
	instanceInfo.pApplicationInfo = &appInfo;

	vk::Instance vkInstance{};
	vkResult = vk::createInstance(&instanceInfo, nullptr, &vkInstance);
	DENGINE_GFX_ASSERT(vkResult == vk::Result::eSuccess || vkInstance);

	apiData.instance = vkInstance;

	return true;
}

vk::DebugUtilsMessengerEXT DEngine::Gfx::Vk::CreateVkDebugUtilsMessenger(vk::Instance instance, const void* userData)
{
	auto createDebugUtilsMessengerPFN = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	DENGINE_GFX_ASSERT(createDebugUtilsMessengerPFN);
	vk::DebugUtilsMessengerCreateInfoEXT debugMessengerInfo{};
	debugMessengerInfo.messageSeverity =
		vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
		vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
		//vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo;
		vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose;
	debugMessengerInfo.messageType =
		vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
		vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
		vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation;
	debugMessengerInfo.pfnUserCallback = (PFN_vkDebugUtilsMessengerCallbackEXT)VulkanDebugCallback;
	debugMessengerInfo.pUserData = const_cast<void*>(userData);

	VkDebugUtilsMessengerCreateInfoEXT* infoPtr = (VkDebugUtilsMessengerCreateInfoEXT*)&debugMessengerInfo;

	vk::DebugUtilsMessengerEXT debugMessenger{};
	vk::Result createDebugMessengerResult = (vk::Result)createDebugUtilsMessengerPFN(instance, infoPtr, nullptr, (VkDebugUtilsMessengerEXT*)&debugMessenger);
	DENGINE_GFX_ASSERT(createDebugMessengerResult == vk::Result::eSuccess && debugMessenger);
	return debugMessenger;
}

DEngine::Gfx::Vk::PhysDeviceInfo DEngine::Gfx::Vk::LoadPhysDevice(
	vk::Instance instance,
	vk::SurfaceKHR surface,
	const ILog* logger)
{
	PhysDeviceInfo physDeviceInfo{};
	vk::Result vkResult{};

	u32 physicalDeviceCount = 0;
	vkResult = instance.enumeratePhysicalDevices(&physicalDeviceCount, (vk::PhysicalDevice*)nullptr);
	DENGINE_GFX_ASSERT(vkResult == vk::Result::eSuccess || vkResult == vk::Result::eIncomplete);
	DENGINE_GFX_ASSERT(physicalDeviceCount != 0);
	Cont::Array<vk::PhysicalDevice, 10> physDevices{};
	DENGINE_GFX_ASSERT(physicalDeviceCount <= physDevices.size());
	instance.enumeratePhysicalDevices(&physicalDeviceCount, physDevices.data());

	physDeviceInfo.handle = physDevices[0];

	// Find preferred queues
	u32 queueFamilyPropertyCount = 0;
	physDeviceInfo.handle.getQueueFamilyProperties(&queueFamilyPropertyCount, nullptr);
	DENGINE_GFX_ASSERT(queueFamilyPropertyCount != 0);
	Cont::Array<vk::QueueFamilyProperties, 10> availableQueueFamilies{};
	DENGINE_GFX_ASSERT(queueFamilyPropertyCount <= availableQueueFamilies.size());
	physDeviceInfo.handle.getQueueFamilyProperties(&queueFamilyPropertyCount, availableQueueFamilies.data());

	// Find graphics queue
	for (u32 i = 0; i < queueFamilyPropertyCount; i++)
	{
		const auto& queueFamily = availableQueueFamilies[i];
		if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
		{
			physDeviceInfo.preferredQueues.graphics.familyIndex = i;
			physDeviceInfo.preferredQueues.graphics.queueIndex = 0;
			break;
		}
	}
	DENGINE_GFX_ASSERT(physDeviceInfo.preferredQueues.graphics.familyIndex != QueueInfo::invalidIndex);
	// Find transfer queue, prefer a queue on a different family than graphics
	for (u32 i = 0; i < queueFamilyPropertyCount; i++)
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
	DENGINE_GFX_ASSERT(vkResult == vk::Result::eSuccess);
	DENGINE_GFX_ASSERT(presentSupport == 1);

	physDeviceInfo.handle.getProperties(&physDeviceInfo.properties);

	physDeviceInfo.handle.getMemoryProperties(&physDeviceInfo.memProperties);

	// Find device-local memory
	for (u32 i = 0; i < physDeviceInfo.memProperties.memoryTypeCount; i++)
	{
		if (physDeviceInfo.memProperties.memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal)
		{
			physDeviceInfo.memInfo.deviceLocal = i;
			break;
		}
	}
	if (physDeviceInfo.memInfo.deviceLocal == MemoryTypes::invalidIndex)
	{
		if (logger != nullptr)
		{
			logger->log("Error. Found no device local memory.");
		}
		std::abort();
	}

	// Find host-visible memory
	for (u32 i = 0; i < physDeviceInfo.memProperties.memoryTypeCount; i++)
	{
		if (physDeviceInfo.memProperties.memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eHostVisible)
		{
			physDeviceInfo.memInfo.hostVisible = i;
			break;
		}
	}
	if (physDeviceInfo.memInfo.hostVisible == MemoryTypes::invalidIndex)
	{
		if (logger != nullptr)
		{
			logger->log("Error. Found no host visible memory.");
		}
		std::abort();
	}

	// Find host-visible | device local memory
	for (u32 i = 0; i < physDeviceInfo.memProperties.memoryTypeCount; i++)
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
	DENGINE_GFX_ASSERT(physDeviceInfo.properties.limits.framebufferColorSampleCounts == physDeviceInfo.properties.limits.framebufferDepthSampleCounts);
	if (physDeviceInfo.properties.limits.framebufferColorSampleCounts & vk::SampleCountFlagBits::e1)
		physDeviceInfo.maxFramebufferSamples = vk::SampleCountFlagBits::e1;
	if (physDeviceInfo.properties.limits.framebufferColorSampleCounts & vk::SampleCountFlagBits::e2)
		physDeviceInfo.maxFramebufferSamples = vk::SampleCountFlagBits::e2;
	if (physDeviceInfo.properties.limits.framebufferColorSampleCounts & vk::SampleCountFlagBits::e4)
		physDeviceInfo.maxFramebufferSamples = vk::SampleCountFlagBits::e4;

	return physDeviceInfo;
}

vk::Device DEngine::Gfx::Vk::CreateDevice(const PhysDeviceInfo& physDevice)
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
	f32 priority[3] = { 1.f, 1.f, 1.f };
	Cont::Array<vk::DeviceQueueCreateInfo, 3> queueCreateInfos{};
	u8 queueCreateInfoCount = 0;

	vk::DeviceQueueCreateInfo queueCreateInfo{};

	// Add graphics queue
	queueCreateInfo.pQueuePriorities = priority;
	queueCreateInfo.queueCount = 1;
	queueCreateInfo.queueFamilyIndex = physDevice.preferredQueues.graphics.familyIndex;
	DENGINE_GFX_ASSERT(queueCreateInfoCount <= queueCreateInfos.size());
	queueCreateInfos[queueCreateInfoCount] = queueCreateInfo;
	queueCreateInfoCount++;

	// Add transfer queue if there is a separate one from graphics queue
	if (physDevice.preferredQueues.TransferIsSeparate())
	{
		queueCreateInfo.pQueuePriorities = priority;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.queueFamilyIndex = physDevice.preferredQueues.transfer.familyIndex;

		DENGINE_GFX_ASSERT(queueCreateInfoCount <= queueCreateInfos.size());
		queueCreateInfos[queueCreateInfoCount] = queueCreateInfo;
		queueCreateInfoCount++;
	}

	createInfo.queueCreateInfoCount = (u32)(queueCreateInfoCount);
	createInfo.pQueueCreateInfos = queueCreateInfos.data();

	uint32_t deviceExtensionCount = 0;
	vkResult = physDevice.handle.enumerateDeviceExtensionProperties(nullptr, &deviceExtensionCount, (vk::ExtensionProperties*)nullptr);
	DENGINE_GFX_ASSERT(vkResult == vk::Result::eSuccess || vkResult == vk::Result::eIncomplete);
	DENGINE_GFX_ASSERT(deviceExtensionCount != 0);
	std::vector<vk::ExtensionProperties> deviceExtensionsAvailable(deviceExtensionCount);
	physDevice.handle.enumerateDeviceExtensionProperties(nullptr, &deviceExtensionCount, deviceExtensionsAvailable.data());
	// TODO: Check if all required device extensions are available
	createInfo.ppEnabledExtensionNames = Constants::requiredDeviceExtensions.data();
	createInfo.enabledExtensionCount = uint32_t(Constants::requiredDeviceExtensions.size());

	vk::Device vkDevice{};
	vkResult = physDevice.handle.createDevice(&createInfo, nullptr, &vkDevice);
	DENGINE_GFX_ASSERT(vkResult == vk::Result::eSuccess);
	DENGINE_GFX_ASSERT(vkDevice);

	return vkDevice;
}

DEngine::Gfx::Vk::SwapchainSettings DEngine::Gfx::Vk::GetSwapchainSettings(
	vk::PhysicalDevice device, 
	vk::SurfaceKHR surface,
	SurfaceCapabilities* surfaceCaps,
	const ILog* logger)
{
	vk::Result vkResult{};

	SwapchainSettings settings{};

	settings.surface = surface;

	// Query surface capabilities
	vk::SurfaceCapabilitiesKHR capabilities{};
	vkResult = device.getSurfaceCapabilitiesKHR(surface, &capabilities);
	DENGINE_GFX_ASSERT(vkResult == vk::Result::eSuccess);
	settings.capabilities = capabilities;
	settings.extents.width = capabilities.currentExtent.width;
	settings.extents.height = capabilities.currentExtent.height;

	// Handle swapchain length
	std::uint32_t swapchainLength = Constants::preferredSwapchainLength;
	if (swapchainLength > capabilities.maxImageCount&& capabilities.maxImageCount != 0)
		swapchainLength = capabilities.maxImageCount;
	if (swapchainLength < 2)
	{
		if (logger != nullptr)
			logger->log("Error. Can't make a Vulkan swapchain with fileLength beneath 2.");
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
	DENGINE_GFX_ASSERT(vkResult == vk::Result::eSuccess || vkResult == vk::Result::eIncomplete);
	DENGINE_GFX_ASSERT(supportedPresentModeCount != 0);
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
	DENGINE_GFX_ASSERT(vkResult == vk::Result::eSuccess || vkResult == vk::Result::eIncomplete);
	DENGINE_GFX_ASSERT(surfaceFormatCount != 0);
	std::array<vk::SurfaceFormatKHR, 25> formats{};
	DENGINE_GFX_ASSERT(surfaceFormatCount <= formats.size());
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
		if (logger != nullptr)
			logger->log("Found no usable surface formats.");
		std::abort();
	}
	DENGINE_GFX_ASSERT(formatToUse.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear);
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

DEngine::Gfx::Vk::SwapchainData DEngine::Gfx::Vk::CreateSwapchain(
	vk::Device vkDevice, 
	vk::Queue vkQueue, 
	const SwapchainSettings& settings)
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
	DENGINE_GFX_ASSERT(vkResult == vk::Result::eSuccess);

	swapchain.handle = swapchainHandle;
	swapchain.presentMode = settings.presentMode;

	std::uint32_t swapchainImageCount = 0;
	vkResult = vkDevice.getSwapchainImagesKHR(swapchain.handle, &swapchainImageCount, nullptr);
	DENGINE_GFX_ASSERT(vkResult == vk::Result::eSuccess);
	DENGINE_GFX_ASSERT(swapchainImageCount != 0);
	DENGINE_GFX_ASSERT(swapchainImageCount == settings.numImages);
	DENGINE_GFX_ASSERT(swapchainImageCount < swapchain.images.size());
	swapchain.imageCount = swapchainImageCount;
	vkDevice.getSwapchainImagesKHR(swapchain.handle, &swapchainImageCount, swapchain.images.data());
	// Transition new swapchain images
	TransitionSwapchainImages(vkDevice, vkQueue, Cont::Span(swapchain.images.data(), swapchain.imageCount));


	vk::CommandPoolCreateInfo cmdPoolInfo{};
	vkResult = vkDevice.createCommandPool(&cmdPoolInfo, nullptr, &swapchain.cmdPool);
	DENGINE_GFX_ASSERT(vkResult == vk::Result::eSuccess);
	vk::CommandBufferAllocateInfo cmdBufferAllocInfo{};
	cmdBufferAllocInfo.commandPool = swapchain.cmdPool;
	cmdBufferAllocInfo.commandBufferCount = swapchain.imageCount;
	cmdBufferAllocInfo.level = vk::CommandBufferLevel::ePrimary;
	vkResult = vkDevice.allocateCommandBuffers(&cmdBufferAllocInfo, swapchain.cmdBuffers.data());
	DENGINE_GFX_ASSERT(vkResult == vk::Result::eSuccess);

	vk::SemaphoreCreateInfo semaphoreInfo{};
	vkResult = vkDevice.createSemaphore(&semaphoreInfo, nullptr, &swapchain.imgCopyDoneSemaphore);
	DENGINE_GFX_ASSERT(vkResult == vk::Result::eSuccess);

	return swapchain;
}

bool DEngine::Gfx::Vk::RecreateSwapchain(SwapchainData& swapchainData, vk::Device vkDevice, vk::Queue vkQueue, SwapchainSettings settings)
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
	DENGINE_GFX_ASSERT(vkResult == vk::Result::eSuccess);

	swapchainData.handle = newSwapchainHandle;
	swapchainData.extents = settings.extents;

	std::uint32_t swapchainImageCount = 0;
	vk::Result getSwapchainImagesResult = vkDevice.getSwapchainImagesKHR(swapchainData.handle, &swapchainImageCount, (vk::Image*)nullptr);
	DENGINE_GFX_ASSERT(getSwapchainImagesResult == vk::Result::eSuccess);
	DENGINE_GFX_ASSERT(swapchainImageCount != 0);
	DENGINE_GFX_ASSERT(swapchainImageCount == swapchainData.imageCount);
	DENGINE_GFX_ASSERT(swapchainImageCount < swapchainData.images.size());
	swapchainData.imageCount = swapchainImageCount;
	swapchainData.images.fill(vk::Image{});
	vkDevice.getSwapchainImagesKHR(swapchainData.handle, &swapchainImageCount, (vk::Image*)swapchainData.images.data());
	// Transition new swapchain images
	TransitionSwapchainImages(vkDevice, vkQueue, Cont::Span(swapchainData.images.data(), swapchainData.imageCount));

	vkDevice.freeCommandBuffers(swapchainData.cmdPool, swapchainData.imageCount, swapchainData.cmdBuffers.data());
	// Clear previous copy-cmdbuffer handles
	swapchainData.cmdBuffers.fill(vk::CommandBuffer{});
	vk::CommandBufferAllocateInfo cmdBufferAllocInfo{};
	cmdBufferAllocInfo.commandPool = swapchainData.cmdPool;
	cmdBufferAllocInfo.commandBufferCount = swapchainData.imageCount;
	cmdBufferAllocInfo.level = vk::CommandBufferLevel::ePrimary;
	vkResult = vkDevice.allocateCommandBuffers(&cmdBufferAllocInfo, swapchainData.cmdBuffers.data());
	DENGINE_GFX_ASSERT(vkResult == vk::Result::eSuccess);

	return true;
}

bool DEngine::Gfx::Vk::TransitionSwapchainImages(vk::Device vkDevice, vk::Queue vkQueue, Cont::Span<const vk::Image> images)
{
	vk::Result vkResult{};

	vk::CommandPoolCreateInfo cmdPoolInfo{};
	vk::CommandPool cmdPool{};
	vkResult = vkDevice.createCommandPool(&cmdPoolInfo, nullptr, &cmdPool);
	DENGINE_GFX_ASSERT(vkResult == vk::Result::eSuccess);

	vk::CommandBufferAllocateInfo cmdBufferAllocInfo{};
	cmdBufferAllocInfo.commandBufferCount = 1;
	cmdBufferAllocInfo.commandPool = cmdPool;
	cmdBufferAllocInfo.level = vk::CommandBufferLevel::ePrimary;
	vk::CommandBuffer cmdBuffer{};
	vk::Result allocCmdBufferResult = vkDevice.allocateCommandBuffers(&cmdBufferAllocInfo, &cmdBuffer);
	DENGINE_GFX_ASSERT(allocCmdBufferResult == vk::Result::eSuccess);

	// Record commandbuffer
	{
		vk::CommandBufferBeginInfo cmdBeginInfo{};
		cmdBeginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
		vkResult = cmdBuffer.begin(&cmdBeginInfo);
		DENGINE_GFX_ASSERT(vkResult == vk::Result::eSuccess);

		for (size_t i = 0; i < images.size(); i++)
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
	DENGINE_GFX_ASSERT(vkResult == vk::Result::eSuccess);

	vk::SubmitInfo submitInfo{};
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuffer;
	vkResult = vkQueue.submit(1, &submitInfo, fence);
	DENGINE_GFX_ASSERT(vkResult == vk::Result::eSuccess);

	vkResult = vkDevice.waitForFences(1, &fence, vk::Bool32(1), std::numeric_limits<uint64_t>::max());
	DENGINE_GFX_ASSERT(vkResult == vk::Result::eSuccess);

	vkDevice.destroyFence(fence, nullptr);
	vkDevice.destroyCommandPool(cmdPool, nullptr);

	return true;
}

void DEngine::Gfx::Vk::RecordCopyRenderedImgCmdBuffers(const SwapchainData& swapchainData, vk::Image srcImg)
{
	vk::Result vkResult{};

	const Cont::Span cmdBuffers = Cont::Span(swapchainData.cmdBuffers.data(), swapchainData.imageCount);
	for (size_t i = 0; i < cmdBuffers.size(); i++)
	{
		vk::CommandBuffer cmdBuffer = cmdBuffers[i];

		vk::CommandBufferBeginInfo beginInfo{};
		beginInfo.flags = vk::CommandBufferUsageFlagBits::eSimultaneousUse;
		vkResult = cmdBuffer.begin(&beginInfo);
		DENGINE_GFX_ASSERT(vkResult == vk::Result::eSuccess);

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
		DENGINE_GFX_ASSERT(vkResult == vk::Result::eSuccess);
	}
}

DEngine::Gfx::Vk::ImGuiRenderTarget DEngine::Gfx::Vk::CreateImGuiRenderTarget(
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
	DENGINE_GFX_ASSERT(vkResult == vk::Result::eSuccess);

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
		DENGINE_GFX_ASSERT(vkResult == vk::Result::eSuccess);

		vk::Image myImage{};
		vkResult = vkDevice.createImage(&imgInfo, nullptr, &myImage);
		DENGINE_GFX_ASSERT(vkResult == vk::Result::eSuccess);

		vk::MemoryRequirements memReqs{};
		vkDevice.getImageMemoryRequirements(tempImg, &memReqs);

		vk::MemoryAllocateInfo memAllocInfo{};
		memAllocInfo.allocationSize = memReqs.size;
		memAllocInfo.memoryTypeIndex = 0;
		vkResult = vkDevice.allocateMemory(&memAllocInfo, nullptr, &returnVal.memory);
		DENGINE_GFX_ASSERT(vkResult == vk::Result::eSuccess);

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
		DENGINE_GFX_ASSERT(vkResult == vk::Result::eSuccess);
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
		DENGINE_GFX_ASSERT(vkResult == vk::Result::eSuccess);
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
	DENGINE_GFX_ASSERT(vkResult == vk::Result::eSuccess);

	// Transition the image
	vk::CommandPoolCreateInfo cmdPoolInfo{};
	cmdPoolInfo.queueFamilyIndex = 0;
	cmdPoolInfo.flags = vk::CommandPoolCreateFlagBits{};
	vk::CommandPool cmdPool{};
	vkResult = vkDevice.createCommandPool(&cmdPoolInfo, nullptr, &cmdPool);
	DENGINE_GFX_ASSERT(vkResult == vk::Result::eSuccess);
	vk::CommandBufferAllocateInfo cmdBufferAllocInfo{};
	cmdBufferAllocInfo.commandPool = cmdPool;
	cmdBufferAllocInfo.level = vk::CommandBufferLevel::ePrimary;
	cmdBufferAllocInfo.commandBufferCount = 1;
	vk::CommandBuffer cmdBuffer{};
	vkResult = vkDevice.allocateCommandBuffers(&cmdBufferAllocInfo, &cmdBuffer);
	DENGINE_GFX_ASSERT(vkResult == vk::Result::eSuccess);
	vk::CommandBufferBeginInfo cmdBufferBeginInfo{};
	cmdBufferBeginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

	vkResult = cmdBuffer.begin(&cmdBufferBeginInfo);
	DENGINE_GFX_ASSERT(vkResult == vk::Result::eSuccess);

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
	DENGINE_GFX_ASSERT(vkResult == vk::Result::eSuccess);

	vk::SubmitInfo submitInfo{};
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuffer;
	vkResult = vkQueue.submit(1, &submitInfo, tempFence);
	DENGINE_GFX_ASSERT(vkResult == vk::Result::eSuccess);


	vkResult = vkDevice.waitForFences(1, &tempFence, 1, std::numeric_limits<uint64_t>::max());
	DENGINE_GFX_ASSERT(vkResult == vk::Result::eSuccess);

	vkDevice.destroyFence(tempFence, nullptr);
	vkDevice.destroyCommandPool(cmdPool, nullptr);

	return returnVal;
}

DEngine::Gfx::Vk::ImguiRenderCmdBuffers DEngine::Gfx::Vk::MakeImguiRenderCmdBuffers(vk::Device vkDevice, uint32_t resourceSetCount)
{
	vk::Result vkResult{};

	ImguiRenderCmdBuffers returnVal{};

	Cont::Array<vk::CommandBuffer, Constants::maxResourceSets> imguiRenderCmdBuffers{};
	vk::CommandPoolCreateInfo imguiRenderCmdPoolInfo{};
	imguiRenderCmdPoolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
	vk::CommandPool imguiRenderCmdPool{};
	vkResult = vkDevice.createCommandPool(&imguiRenderCmdPoolInfo, nullptr, &imguiRenderCmdPool);
	DENGINE_GFX_ASSERT(vkResult == vk::Result::eSuccess);
	vk::CommandBufferAllocateInfo imguiRenderCmdBufferAllocInfo{};
	imguiRenderCmdBufferAllocInfo.commandPool = imguiRenderCmdPool;
	imguiRenderCmdBufferAllocInfo.level = vk::CommandBufferLevel::ePrimary;
	imguiRenderCmdBufferAllocInfo.commandBufferCount = resourceSetCount;
	vkResult = vkDevice.allocateCommandBuffers(&imguiRenderCmdBufferAllocInfo, imguiRenderCmdBuffers.data());
	DENGINE_GFX_ASSERT(vkResult == vk::Result::eSuccess);

	Cont::Array<vk::Fence, Constants::maxResourceSets> fences{};
	for (size_t i = 0; i < resourceSetCount; i++)
	{
		vk::FenceCreateInfo fenceInfo{};
		fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;
		vkResult = vkDevice.createFence(&fenceInfo, nullptr, &fences[i]);
		DENGINE_GFX_ASSERT(vkResult == vk::Result::eSuccess);
	}

	returnVal.cmdPool = imguiRenderCmdPool;
	returnVal.cmdBuffers = imguiRenderCmdBuffers;
	returnVal.fences = fences;

	return returnVal;
}

#include "Texas/Texas.hpp"
#include "Texas/Tools.hpp"
#include "Texas/VkFormats.hpp"
#include <iostream>
void DEngine::Gfx::Vk::TestImageBleh(APIData& apiData)
{
	vk::Result vkResult{};


	std::ifstream filestream("data/02.ktx", std::fstream::ate | std::fstream::binary);
	DENGINE_GFX_ASSERT(filestream.is_open());
	uSize fileLength = filestream.tellg();
	filestream.seekg(0);
	std::vector<std::byte> fileVector;
	fileVector.resize(fileLength);
	filestream.read((char*)fileVector.data(), fileLength);


	// Parses the buffer containing the filestream
	Texas::ConstByteSpan fileBuffer = Texas::ConstByteSpan(fileVector.data(), fileVector.size());

	Texas::LoadResult<Texas::MemReqs> texasLoad = Texas::getMemReqs(fileBuffer);
	if (!texasLoad.isSuccessful())
	{
		std::cout << "Texas error: " << texasLoad.errorMessage() << std::endl;
		std::abort();
	}

	Texas::MemReqs texMemReqs = texasLoad.value();
	vk::Buffer tempBuffer{};
	{
		// Here we just allocate a new buffer in Vulkan.
		vk::BufferCreateInfo tempBufferInfo{};
		tempBufferInfo.sharingMode = vk::SharingMode::eExclusive;
		tempBufferInfo.size = texMemReqs.memoryRequired();
		tempBufferInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;
		vkResult = apiData.device.createBuffer(&tempBufferInfo, nullptr, &tempBuffer);
		DENGINE_GFX_ASSERT(vkResult == vk::Result::eSuccess);
		vk::MemoryRequirements bufferMemReqs{};
		apiData.device.getBufferMemoryRequirements(tempBuffer, &bufferMemReqs);
		vk::MemoryAllocateInfo memAllocInfo{};
		memAllocInfo.allocationSize = bufferMemReqs.size;
		memAllocInfo.memoryTypeIndex = 3;
		vk::DeviceMemory bufferMem{};
		vkResult = apiData.device.allocateMemory(&memAllocInfo, nullptr, &bufferMem);
		DENGINE_GFX_ASSERT(vkResult == vk::Result::eSuccess);
		vkResult = apiData.device.bindBufferMemory(tempBuffer, bufferMem, 0);
		DENGINE_GFX_ASSERT(vkResult == vk::Result::eSuccess);

		// imgData is our pointer into the Vulkan buffer.
		std::byte* imgData = nullptr;
		vkResult = apiData.device.mapMemory(bufferMem, 0, tempBufferInfo.size, vk::MemoryMapFlags(), (void**)&imgData);
		DENGINE_GFX_ASSERT(vkResult == vk::Result::eSuccess);

		Texas::ByteSpan dstBuffer = Texas::ByteSpan(imgData, tempBufferInfo.size);
		Texas::ByteSpan workingMem;
		std::size_t requiredAmountWorkingMemory = texMemReqs.workingMemoryRequired();
		if (requiredAmountWorkingMemory > 0)
		{
			workingMem = Texas::ByteSpan(new std::byte[requiredAmountWorkingMemory], requiredAmountWorkingMemory);
			for (size_t i = 0; i < requiredAmountWorkingMemory; i++)
				((std::uint8_t*)workingMem.data())[i] = 0;
		}
		// Loads imagedata onto the provided pointer (imgData)
		Texas::Result texasLoad2 = Texas::loadImageData(texMemReqs, dstBuffer, workingMem);
		if (!texasLoad2.isSuccessful())
		{
			std::cout << "Texas error: " << texasLoad2.errorMessage() << std::endl;
			std::abort();
		}

		apiData.device.unmapMemory(bufferMem);
	}


	vk::ImageCreateInfo imageInfo{};
	imageInfo.arrayLayers = (uint32_t)texMemReqs.metaData().arrayLayerCount;
	imageInfo.extent = Texas::toVkExtent3D(texMemReqs.baseDimensions());
	imageInfo.format = (vk::Format)Texas::toVkFormat(texMemReqs.metaData());
	imageInfo.imageType = (vk::ImageType)Texas::toVkImageType(texMemReqs.metaData().textureType);
	imageInfo.initialLayout = vk::ImageLayout::eUndefined;
	imageInfo.mipLevels = (uint32_t)texMemReqs.metaData().mipLevelCount;
	imageInfo.samples = vk::SampleCountFlagBits::e1;
	imageInfo.sharingMode = vk::SharingMode::eExclusive;
	imageInfo.tiling = vk::ImageTiling::eOptimal;
	imageInfo.usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst;;
	
	vk::Image vkImage{};
	vkResult = apiData.device.createImage(&imageInfo, nullptr, &vkImage);
	DENGINE_GFX_ASSERT(vkResult == vk::Result::eSuccess);

	vk::MemoryRequirements imgMemReqs{};
	apiData.device.getImageMemoryRequirements(vkImage, &imgMemReqs);

	vk::MemoryAllocateInfo memAllocInfo{};
	memAllocInfo.allocationSize = imgMemReqs.size;
	memAllocInfo.memoryTypeIndex = 0;
	vk::DeviceMemory imgMem{};
	vkResult = apiData.device.allocateMemory(&memAllocInfo, nullptr, &imgMem);
	DENGINE_GFX_ASSERT(vkResult == vk::Result::eSuccess);

	vkResult = apiData.device.bindImageMemory(vkImage, imgMem, 0);
	DENGINE_GFX_ASSERT(vkResult == vk::Result::eSuccess);

	vk::ImageViewCreateInfo imgViewInfo{};
	imgViewInfo.components.r = vk::ComponentSwizzle::eIdentity;
	imgViewInfo.components.g = vk::ComponentSwizzle::eIdentity;
	imgViewInfo.components.b = vk::ComponentSwizzle::eIdentity;
	imgViewInfo.components.a = vk::ComponentSwizzle::eIdentity;
	imgViewInfo.flags = {};
	imgViewInfo.format = imageInfo.format;
	imgViewInfo.image = vkImage;
	imgViewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	imgViewInfo.subresourceRange.layerCount = (uint32_t)texMemReqs.metaData().arrayLayerCount;
	imgViewInfo.subresourceRange.levelCount = (uint32_t)texMemReqs.metaData().mipLevelCount;
	imgViewInfo.viewType = (vk::ImageViewType)Texas::toVkImageViewType(texMemReqs.metaData().textureType);

	vk::ImageView imgView{};
	vkResult = apiData.device.createImageView(&imgViewInfo, nullptr, &imgView);
	DENGINE_GFX_ASSERT(vkResult == vk::Result::eSuccess);
	apiData.testImgView = imgView;


	{
		// Transition and copy the image
		vk::CommandPoolCreateInfo cmdPoolInfo{};
		cmdPoolInfo.queueFamilyIndex = 0;
		cmdPoolInfo.flags = vk::CommandPoolCreateFlagBits{};
		vk::CommandPool cmdPool{};
		vkResult = apiData.device.createCommandPool(&cmdPoolInfo, nullptr, &cmdPool);
		DENGINE_GFX_ASSERT(vkResult == vk::Result::eSuccess);
		vk::CommandBufferAllocateInfo cmdBufferAllocInfo{};
		cmdBufferAllocInfo.commandPool = cmdPool;
		cmdBufferAllocInfo.level = vk::CommandBufferLevel::ePrimary;
		cmdBufferAllocInfo.commandBufferCount = 1;
		vk::CommandBuffer cmdBuffer{};
		vkResult = apiData.device.allocateCommandBuffers(&cmdBufferAllocInfo, &cmdBuffer);
		DENGINE_GFX_ASSERT(vkResult == vk::Result::eSuccess);

		vk::CommandBufferBeginInfo cmdBufferBeginInfo{};
		cmdBufferBeginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
		vkResult = cmdBuffer.begin(&cmdBufferBeginInfo);
		DENGINE_GFX_ASSERT(vkResult == vk::Result::eSuccess);

		{
			vk::ImageMemoryBarrier imgMemoryBarrier{};
			imgMemoryBarrier.image = vkImage;
			imgMemoryBarrier.oldLayout = vk::ImageLayout::eUndefined;
			imgMemoryBarrier.newLayout = vk::ImageLayout::eTransferDstOptimal;
			imgMemoryBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
			imgMemoryBarrier.subresourceRange.baseArrayLayer = 0;
			imgMemoryBarrier.subresourceRange.layerCount = (uint32_t)texMemReqs.metaData().arrayLayerCount;
			imgMemoryBarrier.subresourceRange.baseMipLevel = 0;
			imgMemoryBarrier.subresourceRange.levelCount = (uint32_t)texMemReqs.metaData().mipLevelCount;
			imgMemoryBarrier.srcAccessMask = vk::AccessFlagBits{};
			imgMemoryBarrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

			cmdBuffer.pipelineBarrier(
				vk::PipelineStageFlagBits::eTopOfPipe,
				vk::PipelineStageFlagBits::eTransfer,
				vk::DependencyFlags{},
				0, nullptr,
				0, nullptr,
				1, & imgMemoryBarrier);

			Cont::Array<vk::BufferImageCopy, 12> regionArray{};

			for (u32 mipIndex = 0; mipIndex < texMemReqs.metaData().mipLevelCount; mipIndex++)
			{
				vk::BufferImageCopy& region = regionArray[mipIndex];
				region.bufferImageHeight = 0; // These can be zero because the data is tightly packed
				region.bufferRowLength = 0; // These can be zero because the data is tightly packed
				region.bufferOffset = Texas::Tools::calcMipOffset(texMemReqs.metaData(), mipIndex).value();
				auto mipDims = Texas::Tools::calcMipmapDimensions(texMemReqs.metaData().baseDimensions, mipIndex);
				region.imageExtent = vk::Extent3D(mipDims.width, mipDims.height, mipDims.depth);
				region.imageOffset = vk::Offset3D();
				region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
				region.imageSubresource.baseArrayLayer = 0;
				region.imageSubresource.layerCount = texMemReqs.metaData().arrayLayerCount;
				region.imageSubresource.mipLevel = mipIndex;

				
			}
			cmdBuffer.copyBufferToImage(tempBuffer, vkImage, vk::ImageLayout::eTransferDstOptimal, texMemReqs.metaData().mipLevelCount, regionArray.data());


			//vk::ImageMemoryBarrier imgMemoryBarrier{};
			imgMemoryBarrier.image = vkImage;
			imgMemoryBarrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
			imgMemoryBarrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
			imgMemoryBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
			imgMemoryBarrier.subresourceRange.baseArrayLayer = 0;
			imgMemoryBarrier.subresourceRange.layerCount = texMemReqs.metaData().arrayLayerCount;
			imgMemoryBarrier.subresourceRange.baseMipLevel = 0;
			imgMemoryBarrier.subresourceRange.levelCount = texMemReqs.metaData().mipLevelCount;
			imgMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
			imgMemoryBarrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

			cmdBuffer.pipelineBarrier(
				vk::PipelineStageFlagBits::eTransfer,
				vk::PipelineStageFlagBits::eFragmentShader,
				vk::DependencyFlags{},
				0, nullptr,
				0, nullptr,
				1, & imgMemoryBarrier);
		}


		cmdBuffer.end();


		vk::FenceCreateInfo fenceInfo{};
		vk::Fence tempFence{};
		vkResult = apiData.device.createFence(&fenceInfo, nullptr, &tempFence);
		DENGINE_GFX_ASSERT(vkResult == vk::Result::eSuccess);

		vk::SubmitInfo submitInfo{};
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &cmdBuffer;
		vkResult = apiData.tempQueue.submit(1, &submitInfo, tempFence);
		DENGINE_GFX_ASSERT(vkResult == vk::Result::eSuccess);


		vkResult = apiData.device.waitForFences(1, &tempFence, 1, std::numeric_limits<uint64_t>::max());
		DENGINE_GFX_ASSERT(vkResult == vk::Result::eSuccess);

		apiData.device.destroyFence(tempFence, nullptr);
		apiData.device.destroyCommandPool(cmdPool, nullptr);
	}

	// Make the VkSampler
	{
		vk::SamplerCreateInfo samplerInfo{};
		samplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
		samplerInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
		samplerInfo.addressModeW = vk::SamplerAddressMode::eRepeat;
		samplerInfo.anisotropyEnable = 0;
		samplerInfo.maxAnisotropy = 0.f;
		samplerInfo.borderColor = vk::BorderColor::eFloatOpaqueWhite;
		samplerInfo.magFilter = vk::Filter::eNearest;
		samplerInfo.maxLod = 100.f;
		samplerInfo.minFilter = vk::Filter::eNearest;
		samplerInfo.minLod = 0.f;
		samplerInfo.mipLodBias = 0.f;
		samplerInfo.mipmapMode = vk::SamplerMipmapMode::eNearest;
		samplerInfo.unnormalizedCoordinates = 0;

		vk::Sampler sampler{};
		vkResult = apiData.device.createSampler(&samplerInfo, nullptr, &sampler);
		DENGINE_GFX_ASSERT(vkResult == vk::Result::eSuccess);

		apiData.testSampler = sampler;
	}
}

void DEngine::Gfx::Vk::Test(APIData& apiData)
{
	vk::Result vkResult{};
	vk::DescriptorSetLayoutBinding bufferLayoutBinding{};
	bufferLayoutBinding.binding = 0;
	bufferLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;
	bufferLayoutBinding.descriptorCount = 1;
	bufferLayoutBinding.descriptorType = vk::DescriptorType::eUniformBuffer;

	vk::DescriptorSetLayoutBinding imageLayoutBinding{};
	imageLayoutBinding.binding = 1;
	imageLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;
	imageLayoutBinding.descriptorCount = 1;
	imageLayoutBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;

	Cont::Array<vk::DescriptorSetLayoutBinding, 2> layoutBindings = { bufferLayoutBinding, imageLayoutBinding };

	vk::DescriptorSetLayoutCreateInfo descrLayoutInfo{};
	descrLayoutInfo.bindingCount = (u32)layoutBindings.size();
	descrLayoutInfo.pBindings = layoutBindings.data();
	vk::DescriptorSetLayout descrLayout{};
	vkResult = apiData.device.createDescriptorSetLayout(&descrLayoutInfo, nullptr, &descrLayout);
	DENGINE_GFX_ASSERT(vkResult == vk::Result::eSuccess);

	// Create buffers and descriptor sets
	{
		vk::BufferCreateInfo bufferInfo{};
		bufferInfo.sharingMode = vk::SharingMode::eExclusive;
		bufferInfo.size = sizeof(f32) * 4 * apiData.resourceSetCount;
		bufferInfo.usage = vk::BufferUsageFlagBits::eUniformBuffer;

		vk::Buffer scaleBuffer{};
		vkResult = apiData.device.createBuffer(&bufferInfo, nullptr, &scaleBuffer);
		DENGINE_GFX_ASSERT(vkResult == vk::Result::eSuccess);

		vk::MemoryRequirements buffMemReqs{};
		apiData.device.getBufferMemoryRequirements(scaleBuffer, &buffMemReqs);

		vk::MemoryAllocateInfo memAllocInfo{};
		memAllocInfo.allocationSize = buffMemReqs.size;
		memAllocInfo.memoryTypeIndex = 2;

		vk::DeviceMemory bufferMem{};
		vkResult = apiData.device.allocateMemory(&memAllocInfo, nullptr, &bufferMem);
		DENGINE_GFX_ASSERT(vkResult == vk::Result::eSuccess);

		apiData.testMem = bufferMem;

		vkResult = apiData.device.bindBufferMemory(scaleBuffer, bufferMem, 0);
		DENGINE_GFX_ASSERT(vkResult == vk::Result::eSuccess);

		void* dataPtr = nullptr;
		vkResult = apiData.device.mapMemory(bufferMem, 0, bufferInfo.size, vk::MemoryMapFlags{}, &dataPtr);
		DENGINE_GFX_ASSERT(vkResult == vk::Result::eSuccess);

		apiData.bufferData = dataPtr;

		// There are resourceSetCount * 4 floats in the memory
		for (uSize i = 0; i < (uSize)apiData.resourceSetCount * 4; i++)
		{
			((float*)dataPtr)[i] = 0.5f;
		}
		vk::MappedMemoryRange memRange{};
		memRange.memory = bufferMem;
		memRange.offset = 0;
		memRange.size = bufferInfo.size;
		vkResult = apiData.device.flushMappedMemoryRanges(1, &memRange);
		DENGINE_GFX_ASSERT(vkResult == vk::Result::eSuccess);

		vk::DescriptorPoolSize bufferPoolSize{};
		bufferPoolSize.type = vk::DescriptorType::eUniformBuffer;
		bufferPoolSize.descriptorCount = apiData.resourceSetCount;

		vk::DescriptorPoolSize imagePoolSize{};
		imagePoolSize.type = vk::DescriptorType::eCombinedImageSampler;
		imagePoolSize.descriptorCount = apiData.resourceSetCount;

		Cont::Array<vk::DescriptorPoolSize, 2> poolSizes = { bufferPoolSize, imagePoolSize };

		vk::DescriptorPoolCreateInfo descrPoolInfo{};
		descrPoolInfo.maxSets = apiData.resourceSetCount;
		descrPoolInfo.poolSizeCount = (u32)poolSizes.size();
		descrPoolInfo.pPoolSizes = poolSizes.data();

		vk::DescriptorPool descrPool{};
		vkResult = apiData.device.createDescriptorPool(&descrPoolInfo, nullptr, &descrPool);
		DENGINE_GFX_ASSERT(vkResult == vk::Result::eSuccess);


		Cont::Array<vk::DescriptorSetLayout, Constants::maxResourceSets> descrLayouts{};
		for (uSize i = 0; i < apiData.resourceSetCount; i++)
			descrLayouts[i] = descrLayout;
		vk::DescriptorSetAllocateInfo descrSetAllocInfo{};
		descrSetAllocInfo.descriptorPool = descrPool;
		descrSetAllocInfo.descriptorSetCount = apiData.resourceSetCount;
		descrSetAllocInfo.pSetLayouts = descrLayouts.data();

		Cont::Array<vk::DescriptorSet, Constants::maxResourceSets> descrSets{};
		vkResult = apiData.device.allocateDescriptorSets(&descrSetAllocInfo, descrSets.data());
		DENGINE_GFX_ASSERT(vkResult == vk::Result::eSuccess);

		Cont::Array<vk::WriteDescriptorSet, Constants::maxResourceSets> bufferDescrWrites{};
		Cont::Array<vk::DescriptorBufferInfo, Constants::maxResourceSets> descrBufferInfos{};
		
		for (uSize i = 0; i < apiData.resourceSetCount; i++)
		{ 
			vk::WriteDescriptorSet& descrWrite = bufferDescrWrites[i];
			descrWrite.dstSet = descrSets[i];
			descrWrite.dstBinding = 0;
			descrWrite.descriptorType = vk::DescriptorType::eUniformBuffer;

			vk::DescriptorBufferInfo& bufferInfo = descrBufferInfos[i];
			bufferInfo.buffer = scaleBuffer;
			bufferInfo.offset = sizeof(f32) * 4 * i;
			bufferInfo.range = sizeof(f32) * 4;

			descrWrite.descriptorCount = 1;
			descrWrite.pBufferInfo = &bufferInfo;
		}
		Cont::Array<vk::WriteDescriptorSet, Constants::maxResourceSets> imageDescrWrites{};
		Cont::Array<vk::DescriptorImageInfo, Constants::maxResourceSets> descrImageInfos{};
		for (uSize i = 0; i < apiData.resourceSetCount; i++)
		{
			vk::WriteDescriptorSet& descrWrite = imageDescrWrites[i];
			descrWrite.dstSet = descrSets[i];
			descrWrite.dstBinding = 1;
			descrWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;

			vk::DescriptorImageInfo& imageInfo = descrImageInfos[i];
			imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
			imageInfo.imageView = apiData.testImgView;
			imageInfo.sampler = apiData.testSampler;

			descrWrite.descriptorCount = 1;
			descrWrite.pImageInfo = &imageInfo;
		}

		Cont::Array<vk::WriteDescriptorSet, Constants::maxResourceSets * 2> descrWrites{};
		u32 i = 0;
		for (; i < apiData.resourceSetCount; i++)
			descrWrites[i] = bufferDescrWrites[i];
		for (; i < apiData.resourceSetCount * 2; i++)
			descrWrites[i] = imageDescrWrites[i - apiData.resourceSetCount];

		apiData.device.updateDescriptorSets(apiData.resourceSetCount * 2, descrWrites.data(), 0, nullptr);

		apiData.descrSets = descrSets;
	}

	vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &descrLayout;
	vk::PipelineLayout pipelineLayout{};
	vkResult = apiData.device.createPipelineLayout(&pipelineLayoutInfo, nullptr, &pipelineLayout);
	DENGINE_GFX_ASSERT(vkResult == vk::Result::eSuccess);

	apiData.testPipelineLayout = pipelineLayout;

	// Create vertex shader module
	std::ifstream vertFile("data/Shader/vert.spv", std::ios::ate | std::ios::binary);
	DENGINE_GFX_ASSERT_MSG(vertFile.is_open(), "Could not open vertex shader");
	std::vector<u8> vertCode(vertFile.tellg());
	vertFile.seekg(0);
	vertFile.read(reinterpret_cast<char*>(vertCode.data()), vertCode.size());
	vertFile.close();

	vk::ShaderModuleCreateInfo vertModCreateInfo{};
	vertModCreateInfo.codeSize = vertCode.size();
	vertModCreateInfo.pCode = reinterpret_cast<const u32*>(vertCode.data());

	vk::ShaderModule vertModule{};
	vkResult = apiData.device.createShaderModule(&vertModCreateInfo, nullptr, &vertModule);
	DENGINE_GFX_ASSERT(vkResult == vk::Result::eSuccess);

	vk::PipelineShaderStageCreateInfo vertStageInfo{};
	vertStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
	vertStageInfo.module = vertModule;
	vertStageInfo.pName = "main";



	std::ifstream fragFile("data/Shader/frag.spv", std::ios::ate | std::ios::binary);
	DENGINE_GFX_ASSERT_MSG(fragFile.is_open(), "Could not open fragment shader");

	std::vector<u8> fragCode(fragFile.tellg());
	fragFile.seekg(0);
	fragFile.read(reinterpret_cast<char*>(fragCode.data()), fragCode.size());
	fragFile.close();

	vk::ShaderModuleCreateInfo fragModInfo{};
	fragModInfo.codeSize = fragCode.size();
	fragModInfo.pCode = reinterpret_cast<const u32*>(fragCode.data());

	vk::ShaderModule fragModule{};
	vkResult = apiData.device.createShaderModule(&fragModInfo, nullptr, &fragModule);

	vk::PipelineShaderStageCreateInfo fragStageInfo{};
	fragStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
	fragStageInfo.module = fragModule;
	fragStageInfo.pName = "main";

	Cont::Array<vk::PipelineShaderStageCreateInfo, 2> shaderStages = { vertStageInfo, fragStageInfo };

	vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};

	vk::PipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.topology = vk::PrimitiveTopology::eTriangleStrip;

	vk::Viewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (f32)1280.f;
	viewport.height = (f32)720.f;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vk::Rect2D scissor{};
	scissor.offset = vk::Offset2D{ 0, 0 };
	scissor.extent = vk::Extent2D{ 1280, 720 };
	vk::PipelineViewportStateCreateInfo viewportState{};
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	vk::PipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.lineWidth = 1.f;
	rasterizer.polygonMode = vk::PolygonMode::eFill;
	rasterizer.frontFace = vk::FrontFace::eCounterClockwise;
	//rasterizer.rasterizerDiscardEnable = 0;

	vk::PipelineDepthStencilStateCreateInfo depthStencilInfo{};
	depthStencilInfo.depthTestEnable = 0;
	depthStencilInfo.depthCompareOp = vk::CompareOp::eLess;
	depthStencilInfo.stencilTestEnable = 0;
	depthStencilInfo.depthWriteEnable = 0;
	depthStencilInfo.minDepthBounds = 0.f;
	depthStencilInfo.maxDepthBounds = 1.f;

	vk::PipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sampleShadingEnable = 0;
	multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;

	vk::PipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

	vk::PipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;

	vk::GraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = apiData.imguiRenderTarget.renderPass;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pMultisampleState = &multisampling;
	//pipelineInfo.pMultisampleState = nullptr;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pColorBlendState = &colorBlending;
	//pipelineInfo.pDepthStencilState = &depthStencilInfo;
	pipelineInfo.pDepthStencilState = nullptr;
	pipelineInfo.stageCount = (u32)shaderStages.size();
	pipelineInfo.pStages = shaderStages.data();

	vk::Pipeline pipeline{};
	vkResult = apiData.device.createGraphicsPipelines(vk::PipelineCache{}, 1, &pipelineInfo, nullptr, &pipeline);
	DENGINE_GFX_ASSERT(vkResult == vk::Result::eSuccess);

	apiData.testPipeline = pipeline;

	apiData.device.destroyShaderModule(vertModule, nullptr);
	apiData.device.destroyShaderModule(fragModule, nullptr);
}