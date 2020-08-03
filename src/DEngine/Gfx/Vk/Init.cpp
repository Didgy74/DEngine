#include "Init.hpp"

#include "VulkanIncluder.hpp"

#include "DynamicDispatch.hpp"
#include "SurfaceInfo.hpp"

#include <DEngine/Gfx/detail/Assert.hpp>

#include <string>

#undef max

namespace DEngine::Gfx::Vk
{
	VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverityIn,
		VkDebugUtilsMessageTypeFlagsEXT messageTypeIn,
		VkDebugUtilsMessengerCallbackDataEXT const* pCallbackDataIn,
		void* pUserData)
	{
		auto messageSeverity = static_cast<vk::DebugUtilsMessageSeverityFlagBitsEXT>(messageSeverityIn);
		auto messageType = static_cast<vk::DebugUtilsMessageTypeFlagsEXT>(messageTypeIn);
		auto pCallbackData = reinterpret_cast<vk::DebugUtilsMessengerCallbackDataEXT const*>(pCallbackDataIn);

		LogInterface* logger = static_cast<LogInterface*>(pUserData);

		if (logger != nullptr)
		{
			std::string msg;
			msg.reserve(512);

			if (messageSeverity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eError)
				msg += "[Error";
			else if (messageSeverity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning)
				msg += "[Warning";
			else if (messageSeverity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo)
				msg += "[Info";
			else if (messageSeverity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose)
				msg += "[Verbose";

			if (messageType & vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral)
				msg += ", General";
			if (messageType & vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance)
				msg += ", Performance";
			else if (messageType & vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation)
				msg += ", Validation";


			msg += "] - ";

			msg += pCallbackData->pMessage;

			logger->log(LogInterface::Level::Fatal, msg.data());
		}


		return 0;
	}
}

using namespace DEngine;
using namespace DEngine::Gfx;

Vk::Init::CreateVkInstance_Return Vk::Init::CreateVkInstance(
	Std::Span<char const*> requiredExtensions,
	bool enableLayers,
	BaseDispatch const& baseDispatch,
	LogInterface* logger)
{
	vk::Result vkResult{};
	CreateVkInstance_Return returnValue{};

	// Build what extensions we are going to use
	std::vector<char const*> totalRequiredExtensions;
	totalRequiredExtensions.reserve(requiredExtensions.Size() + Constants::requiredInstanceExtensions.size());
	// First copy all required instance extensions
	for (uSize i = 0; i < requiredExtensions.Size(); i++)
		totalRequiredExtensions.push_back(requiredExtensions[i]);

	// Next add extensions required by renderer, don't add duplicates
	for (const char* requiredExtension : Constants::requiredInstanceExtensions)
	{
		bool extensionAlreadyPresent = false;
		for (const char* existingExtension : totalRequiredExtensions)
		{
			if (std::strcmp(requiredExtension, existingExtension) == 0)
			{
				extensionAlreadyPresent = true;
				break;
			}
		}
		if (!extensionAlreadyPresent)
			totalRequiredExtensions.push_back(requiredExtension);
	}

	// Check if all the required extensions are also available
	u32 instanceExtensionCount = 0;
	vkResult = baseDispatch.enumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, nullptr);
	if (vkResult != vk::Result::eSuccess && vkResult != vk::Result::eIncomplete)
		throw std::runtime_error("Vulkan: Unable to enumerate available instance extension properties.");
	std::vector<vk::ExtensionProperties> availableExtensions(instanceExtensionCount);
	vkResult = baseDispatch.enumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, availableExtensions.data());
	for (const char* required : totalRequiredExtensions)
	{
		bool requiredExtensionIsAvailable = false;
		for (const auto& available : availableExtensions)
		{
			if (std::strcmp(required, available.extensionName) == 0)
			{
				requiredExtensionIsAvailable = true;
				break;
			}
		}
		if (!requiredExtensionIsAvailable)
			throw std::runtime_error("Required Vulkan instance extension is not available.");
	}

	Std::StaticVector<const char*, 5> layersToUse{};
	if constexpr (Constants::enableDebugUtils)
	{
		if (enableLayers)
		{
			// Check if debug utils is available through global list.
			bool debugUtilsIsAvailable = false;
			for (const auto& ext : availableExtensions)
			{
				if (std::strcmp(ext.extensionName, Constants::debugUtilsExtensionName) == 0)
				{
					debugUtilsIsAvailable = true;
					break;
				}
			}


			// First check if the Khronos validation layer is present
			u32 availableLayerCount = 0;
			vkResult = baseDispatch.enumerateInstanceLayerProperties(&availableLayerCount, nullptr);
			if (vkResult != vk::Result::eSuccess && vkResult != vk::Result::eIncomplete)
				throw std::runtime_error("Failed to enumerate instance layer properties during Vulkan instance creation.");
			std::vector<vk::LayerProperties> availableLayers;
			availableLayers.resize(availableLayerCount);
			vkResult = baseDispatch.enumerateInstanceLayerProperties(&availableLayerCount, availableLayers.data());
			bool layerIsAvailable = false;
			for (const auto& availableLayer : availableLayers)
			{
				char const* wantedLayerName = Constants::khronosLayerName;
				char const* availableLayerName = availableLayer.layerName;
				if (std::strcmp(wantedLayerName, availableLayerName) == 0)
				{
					layerIsAvailable = true;
					// If the layer is available, we know it implements debug utils.
					debugUtilsIsAvailable = true;
					break;
				}
			}
			if (debugUtilsIsAvailable && layerIsAvailable)
			{
				totalRequiredExtensions.push_back(Constants::debugUtilsExtensionName);
				layersToUse.PushBack(Constants::khronosLayerName);

				returnValue.debugUtilsEnabled = true;
			}
		}
	}

	vk::InstanceCreateInfo instanceInfo{};
	instanceInfo.enabledExtensionCount = (u32)totalRequiredExtensions.size();
	instanceInfo.ppEnabledExtensionNames = totalRequiredExtensions.data();
	instanceInfo.enabledLayerCount = (u32)layersToUse.Size();
	instanceInfo.ppEnabledLayerNames = layersToUse.Data();

	vk::ApplicationInfo appInfo{};
	appInfo.apiVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
	appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
	appInfo.pApplicationName = "DidgyImguiVulkanTest";
	appInfo.pEngineName = "Nothing special";
	instanceInfo.pApplicationInfo = &appInfo;

	vk::Instance instance = baseDispatch.createInstance(instanceInfo);

	returnValue.instanceHandle = instance;

	return returnValue;
}

vk::DebugUtilsMessengerEXT DEngine::Gfx::Vk::Init::CreateLayerMessenger(
	vk::Instance instanceHandle,
	DebugUtilsDispatch const* debugUtilsOpt,
	const void* userData)
{
	vk::DebugUtilsMessengerCreateInfoEXT debugMessengerInfo{};
	debugMessengerInfo.messageSeverity =
		vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
		vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
		//vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
		vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose;
	debugMessengerInfo.messageType =
		vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
		vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
		vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation;
	debugMessengerInfo.pfnUserCallback = VulkanDebugCallback;
	debugMessengerInfo.pUserData = const_cast<void*>(userData);

	return debugUtilsOpt->createDebugUtilsMessengerEXT(instanceHandle, debugMessengerInfo);
}

void Vk::SurfaceInfo::BuildInPlace(
	SurfaceInfo& surfaceInfo,
	vk::SurfaceKHR initialSurface,
	InstanceDispatch const& instance,
	vk::PhysicalDevice physDevice)
{
	vk::Result vkResult{};

	u32 presentModeCount = 0;
	vkResult = instance.getPhysicalDeviceSurfacePresentModesKHR(physDevice, initialSurface, &presentModeCount, nullptr);
	if ((vkResult != vk::Result::eSuccess && vkResult != vk::Result::eIncomplete) || presentModeCount == 0)
		throw std::runtime_error("DEngine - Renderer: Unable to query present modes for Vulkan surface.");
	surfaceInfo.supportedPresentModes.resize(presentModeCount);
	vkResult = instance.getPhysicalDeviceSurfacePresentModesKHR(
		physDevice,
		initialSurface,
		&presentModeCount,
		surfaceInfo.supportedPresentModes.data());
	// Select present mode to use
	// If not found, fallback to FIFO, it's guaranteed to be supported.
	vk::PresentModeKHR presentModeToUse{};
	bool preferredPresentModeFound = false;
	for (auto const availableMode : surfaceInfo.supportedPresentModes)
	{
		if (availableMode == Constants::preferredPresentMode)
		{
			preferredPresentModeFound = true;
			presentModeToUse = availableMode;
			break;
		}
	}
	// FIFO is guaranteed to exist, so we fallback to that one if we didn't find the one we wanted.
	if (!preferredPresentModeFound)
		presentModeToUse = vk::PresentModeKHR::eFifo;
	surfaceInfo.presentModeToUse = presentModeToUse;

	// Grab surface formats
	u32 surfaceFormatCount = 0;
	vkResult = instance.getPhysicalDeviceSurfaceFormatsKHR(
		physDevice,
		initialSurface,
		&surfaceFormatCount,
		nullptr);
	if ((vkResult != vk::Result::eSuccess && vkResult != vk::Result::eIncomplete) || surfaceFormatCount == 0)
		throw std::runtime_error("DEngine - Renderer: Unable to query surface formats for Vulkan surface.");
	surfaceInfo.supportedSurfaceFormats.resize(surfaceFormatCount);
	vkResult = instance.getPhysicalDeviceSurfaceFormatsKHR(
		physDevice,
		initialSurface,
		&surfaceFormatCount,
		surfaceInfo.supportedSurfaceFormats.data());
	// Select format to use
	vk::SurfaceFormatKHR formatToUse = vk::SurfaceFormatKHR{};
	bool foundPreferredFormat = false;
	for (const auto& preferredFormat : Constants::preferredSurfaceFormats)
	{
		for (const auto& availableFormat : surfaceInfo.supportedSurfaceFormats)
		{
			if (availableFormat == preferredFormat)
			{
				formatToUse = preferredFormat;
				foundPreferredFormat = true;
				break;
			}
		}
		if (foundPreferredFormat)
			break;
	}
	if (!foundPreferredFormat)
		std::runtime_error("Vulkan: Found no suitable surface format when querying VkSurfaceKHR.");
	surfaceInfo.surfaceFormatToUse = formatToUse;


	vk::SurfaceCapabilitiesKHR surfaceCaps = instance.getPhysicalDeviceSurfaceCapabilitiesKHR(physDevice, initialSurface);


	if (surfaceCaps.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::eOpaque)
		surfaceInfo.compositeAlphaToUse = vk::CompositeAlphaFlagBitsKHR::eOpaque;
	else if (surfaceCaps.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::eInherit)
		surfaceInfo.compositeAlphaToUse = vk::CompositeAlphaFlagBitsKHR::eInherit;
	else
		throw std::runtime_error("DEngine - Renderer: : Found no suitable compostive alpha flag bit for swapchain.");
}

Vk::PhysDeviceInfo Vk::Init::LoadPhysDevice(
	InstanceDispatch const& instance,
	vk::SurfaceKHR surface)
{
	PhysDeviceInfo physDevice{};
	vk::Result vkResult{};

	u32 physicalDeviceCount = 0;
	vkResult = instance.enumeratePhysicalDevices(&physicalDeviceCount, nullptr);
	if (vkResult != vk::Result::eSuccess && vkResult != vk::Result::eIncomplete)
		throw std::runtime_error("Vulkan: Unable to enumerate physical devices.");
	if (physicalDeviceCount == 0)
		throw std::runtime_error("Vulkan: Host machine has no Vulkan-capable devices.");
	std::vector<vk::PhysicalDevice> physDevices;
	physDevices.resize(physicalDeviceCount);
	vkResult = instance.enumeratePhysicalDevices(&physicalDeviceCount, physDevices.data());

	// For now we just select the first physDevice we find.
	physDevice.handle = physDevices[0];

	// Find preferred queues
	u32 queueFamilyPropertyCount = 0;
	instance.getPhysicalDeviceQueueFamilyProperties(
			physDevice.handle,
			&queueFamilyPropertyCount,
			nullptr);
	std::vector<vk::QueueFamilyProperties> availableQueueFamilies;
	availableQueueFamilies.resize(queueFamilyPropertyCount);
	instance.getPhysicalDeviceQueueFamilyProperties(
		physDevice.handle, 
		&queueFamilyPropertyCount, 
		availableQueueFamilies.data());

	// Find graphics queue
	for (u32 i = 0; i < queueFamilyPropertyCount; i++)
	{
		const auto& queueFamily = availableQueueFamilies[i];
		if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
		{
			physDevice.queueIndices.graphics.familyIndex = i;
			physDevice.queueIndices.graphics.queueIndex = 0;
			break;
		}
	}
	if (physDevice.queueIndices.graphics.familyIndex == invalidIndex)
		throw std::runtime_error("DEngine - Vulkan: Unable to find a graphics queue on VkPhysicalDevice.");
	// Find transfer queue, prefer a queue on a different family than graphics. 
	for (u32 i = 0; i < queueFamilyPropertyCount; i++)
	{
		if (i == physDevice.queueIndices.graphics.familyIndex)
			continue;

		const auto& queueFamily = availableQueueFamilies[i];
		if (queueFamily.queueFlags & vk::QueueFlagBits::eTransfer)
		{
			physDevice.queueIndices.transfer.familyIndex = i;
			physDevice.queueIndices.transfer.queueIndex = 0;
			break;
		}
	}

	// Check presentation support
	bool presentSupport = instance.getPhysicalDeviceSurfaceSupportKHR(
			physDevice.handle,
			physDevice.queueIndices.graphics.familyIndex,
			surface);
	if (!presentSupport)
		throw std::runtime_error("DEngine - Vulkan: No surface present support.");

	physDevice.properties = instance.getPhysicalDeviceProperties(physDevice.handle);

	physDevice.memProperties = instance.getPhysicalDeviceMemoryProperties(physDevice.handle);

	// Find physDevice-local memory
	for (u32 i = 0; i < physDevice.memProperties.memoryTypeCount; i++)
	{
		if (physDevice.memProperties.memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal)
		{
			physDevice.memInfo.deviceLocal = i;
			break;
		}
	}
	if (physDevice.memInfo.deviceLocal == invalidIndex)
		throw std::runtime_error("Unable to find any physDevice-local memory during Vulkan initialization.");

	// Find host-visible memory
	for (u32 i = 0; i < physDevice.memProperties.memoryTypeCount; i++)
	{
		if (physDevice.memProperties.memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eHostVisible)
		{
			physDevice.memInfo.hostVisible = i;
			break;
		}
	}
	if (physDevice.memInfo.hostVisible == invalidIndex)
		throw std::runtime_error("Unable to find any physDevice-local memory during Vulkan initialization.");

	// Find host-visible | physDevice local memory
	for (u32 i = 0; i < physDevice.memProperties.memoryTypeCount; i++)
	{
		vk::MemoryPropertyFlags searchFlags = vk::MemoryPropertyFlagBits::eDeviceLocal | vk::MemoryPropertyFlagBits::eHostVisible;
		if ((physDevice.memProperties.memoryTypes[i].propertyFlags & searchFlags) == searchFlags)
		{
			if (physDevice.memProperties.memoryHeaps[i].flags & vk::MemoryHeapFlagBits::eDeviceLocal)
			{
				physDevice.memInfo.deviceLocalAndHostVisible = i;
				break;
			}
		}
	}

	return physDevice;
}

vk::Device Vk::Init::CreateDevice(
	InstanceDispatch const& instance,
	PhysDeviceInfo const& physDevice)
{
	vk::Result vkResult{};

	// CreateJob logical physDevice
	vk::DeviceCreateInfo createInfo{};

	// Feature configuration
	vk::PhysicalDeviceFeatures physDeviceFeatures = instance.getPhysicalDeviceFeatures(physDevice.handle);

	vk::PhysicalDeviceFeatures featuresToUse{};
	if (physDeviceFeatures.robustBufferAccess == 1)
		featuresToUse.robustBufferAccess = true;
	//if (physDeviceFeatures.sampleRateShading == 1)
			//featuresToUse.sampleRateShading = true;

	createInfo.pEnabledFeatures = &featuresToUse;

	// Queue configuration
	f32 priority[3] = { 1.f, 1.f, 1.f };
	Std::StaticVector<vk::DeviceQueueCreateInfo, 10> queueCreateInfos{};

	vk::DeviceQueueCreateInfo tempQueueCreateInfo{};

	// Add graphics queue
	tempQueueCreateInfo.pQueuePriorities = priority;
	tempQueueCreateInfo.queueCount = 1;
	tempQueueCreateInfo.queueFamilyIndex = physDevice.queueIndices.graphics.familyIndex;
	queueCreateInfos.PushBack(tempQueueCreateInfo);

	// Add transfer queue if there is a separate one from graphics queue
	if (physDevice.queueIndices.transfer.familyIndex != invalidIndex)
	{
		tempQueueCreateInfo = vk::DeviceQueueCreateInfo{};
		tempQueueCreateInfo.pQueuePriorities = priority;
		tempQueueCreateInfo.queueCount = 1;
		tempQueueCreateInfo.queueFamilyIndex = physDevice.queueIndices.transfer.familyIndex;

		queueCreateInfos.PushBack(tempQueueCreateInfo);
	}

	createInfo.queueCreateInfoCount = (u32)queueCreateInfos.Size();
	createInfo.pQueueCreateInfos = queueCreateInfos.Data();

	u32 deviceExtensionCount = 0;
	vkResult = instance.enumeratePhysicalDeviceExtensionProperties(physDevice.handle, &deviceExtensionCount, nullptr);
	if (vkResult != vk::Result::eSuccess && vkResult != vk::Result::eIncomplete)
		throw std::runtime_error("Vulkan: Unable to enumerate device extensions.");
	std::vector<vk::ExtensionProperties> availableExtensions(deviceExtensionCount);
	vkResult = instance.enumeratePhysicalDeviceExtensionProperties(physDevice.handle, &deviceExtensionCount, availableExtensions.data());
	// Check if all required extensions are present
	for (const char* required : Constants::requiredDeviceExtensions)
	{
		bool foundExtension = false;
		for (const auto& available : availableExtensions)
		{
			if (std::strcmp(required, available.extensionName) == 0)
			{
				foundExtension = true;
				break;
			}
		}
		if (foundExtension == false)
			throw std::runtime_error("Not all required physDevice extensions were available during Vulkan initialization.");
	}

	createInfo.ppEnabledExtensionNames = Constants::requiredDeviceExtensions.data();
	createInfo.enabledExtensionCount = u32(Constants::requiredDeviceExtensions.size());

	vk::Device vkDevice = instance.createDevice(physDevice.handle, createInfo);
	return vkDevice;
}

Std::StaticVector<vk::Fence, Vk::Constants::maxInFlightCount> Vk::Init::CreateMainFences(
	DevDispatch const& device, 
	u8 resourceSetCount,
	DebugUtilsDispatch const* debugUtils)
{
	Std::StaticVector<vk::Fence, Constants::maxInFlightCount> returnVal{};
	returnVal.Resize(resourceSetCount);

	for (uSize i = 0; i < returnVal.Size(); i += 1)
	{
		vk::FenceCreateInfo info{};
		info.flags = vk::FenceCreateFlagBits::eSignaled;
		returnVal[i] = device.createFence(info);

		if (debugUtils)
		{
			std::string name = std::string("Main Fence #") + std::to_string(i);

			vk::DebugUtilsObjectNameInfoEXT nameInfo{};
			nameInfo.objectHandle = (u64)(VkFence)returnVal[i];
			nameInfo.objectType = returnVal[i].objectType;
			nameInfo.pObjectName = name.data();

			debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
		}
	}

	return returnVal;
}

vk::RenderPass Vk::Init::BuildMainGfxRenderPass(
	DeviceDispatch const& device,
	bool useEditorPipeline,
	DebugUtilsDispatch const* debugUtils)
{
	vk::AttachmentDescription colorAttach{};
	colorAttach.loadOp = vk::AttachmentLoadOp::eClear;
	colorAttach.storeOp = vk::AttachmentStoreOp::eStore;
	colorAttach.samples = vk::SampleCountFlagBits::e1;
	colorAttach.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
	colorAttach.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
	colorAttach.format = vk::Format::eR8G8B8A8Srgb;
	if (useEditorPipeline)
	{
		// We want to sample from the finalized image into the editor GUI
		colorAttach.initialLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		colorAttach.finalLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
	}
	else
	{
		// We want to transfer it to a swapchain image later
		colorAttach.initialLayout = vk::ImageLayout::eTransferSrcOptimal;
		colorAttach.finalLayout = vk::ImageLayout::eTransferSrcOptimal;
	}

	vk::AttachmentReference colorAttachRef{};
	colorAttachRef.attachment = 0;
	colorAttachRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

	vk::SubpassDescription subpass{};
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachRef;

	vk::RenderPassCreateInfo rpInfo{};
	rpInfo.attachmentCount = 1;
	rpInfo.pAttachments = &colorAttach;
	rpInfo.subpassCount = 1;
	rpInfo.pSubpasses = &subpass;

	vk::RenderPass renderPass = device.createRenderPass(rpInfo);
	if (debugUtils)
	{
		vk::DebugUtilsObjectNameInfoEXT nameInfo{};
		nameInfo.objectHandle = (u64)(VkRenderPass)renderPass;
		nameInfo.objectType = renderPass.objectType;
		nameInfo.pObjectName = "Main Rendering RenderPass";
		debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
	}

	return renderPass;
}

vk::RenderPass Vk::Init::CreateGuiRenderPass(
	DeviceDispatch const& device,
	vk::Format guiTargetFormat,
	DebugUtilsDispatch const* debugUtils)
{
	vk::AttachmentDescription colorAttachment{};
	colorAttachment.initialLayout = vk::ImageLayout::eTransferSrcOptimal;
	colorAttachment.finalLayout = vk::ImageLayout::eTransferSrcOptimal;
	colorAttachment.format = guiTargetFormat;
	colorAttachment.samples = vk::SampleCountFlagBits::e1;
	colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
	colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
	colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
	colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
	vk::AttachmentDescription attachments[1] = { colorAttachment };
	vk::AttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;
	vk::SubpassDescription subpassDescription{};
	subpassDescription.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
	subpassDescription.colorAttachmentCount = 1;
	subpassDescription.pColorAttachments = &colorAttachmentRef;
	// Set up render pass
	vk::RenderPassCreateInfo createInfo{};
	createInfo.attachmentCount = 1;
	createInfo.pAttachments = attachments;
	createInfo.subpassCount = 1;
	createInfo.pSubpasses = &subpassDescription;

	vk::RenderPass renderPass = device.createRenderPass(createInfo);
	if (debugUtils != nullptr)
	{
		vk::DebugUtilsObjectNameInfoEXT nameInfo{};
		nameInfo.objectHandle = (u64)(VkRenderPass)renderPass;
		nameInfo.objectType = renderPass.objectType;
		nameInfo.pObjectName = "GUI RenderPass";
		debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
	}

	return renderPass;
}
