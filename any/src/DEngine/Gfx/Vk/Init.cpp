#include "Init.hpp"

#include "VulkanIncluder.hpp"

#include "DynamicDispatch.hpp"
#include "SurfaceInfo.hpp"

#include <DEngine/Gfx/impl/Assert.hpp>

#include <format>
#include <string>
#include <iostream>

import DEngine.Std.Vec;

namespace DEngine::Gfx::Vk
{
	[[nodiscard]] std::string DebugUtilsMessageSeverityFlagBitToString(
		vk::DebugUtilsMessageSeverityFlagBitsEXT in)
	{
		if (in == vk::DebugUtilsMessageSeverityFlagBitsEXT::eError)
			return "Error";
		else if (in == vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning)
			return "Warning";
		else if (in == vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo)
			return "Info";
		else if (in == vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose)
			return "Verbose";
		return "Unknown severity";
	}

	[[nodiscard]] std::string DebugUtilsMessageTypeFlagBitToString(
		vk::DebugUtilsMessageTypeFlagsEXT in)
	{
		if (in & vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral)
			return "General";
		if (in & vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance)
			return "Performance";
		if (in & vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation)
			return "Validation";
		return "Unknown type";
	}

	VKAPI_ATTR vk::Bool32 VulkanDebugCallback(
		vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		vk::DebugUtilsMessageTypeFlagsEXT messageType,
		vk::DebugUtilsMessengerCallbackDataEXT const* pCallbackData,
		void* pUserData)
	{
		DENGINE_IMPL_GFX_ASSERT(pUserData != nullptr);
		auto* logger = static_cast<LogInterface*>(pUserData);

		if (logger == nullptr) {
			std::cerr << "Received Vulkan validation layer message but no logger attached.";
		} else {
			std::string msg;
			msg.reserve(512);

			if (messageSeverity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eError) {
				int i = 0;
			}

			msg = std::format("[{}, {}]: {}",
				DebugUtilsMessageSeverityFlagBitToString(messageSeverity),
				DebugUtilsMessageTypeFlagBitToString(messageType),
				pCallbackData->pMessage);
			logger->Log(LogInterface::Level::Fatal, { msg.data(), msg.size() });
		}

		return 0;
	}
}

using namespace DEngine;
using namespace DEngine::Gfx;
using namespace DEngine::Gfx::Vk;

Vk::Init::CreateVkInstance_Return Vk::Init::CreateVkInstance(
	Std::Span<char const*> requiredExtensionsIn,
	bool enableLayers,
	BaseDispatch const& baseDispatch,
	Std::AllocRef const& transientAlloc,
	LogInterface* logger)
{
	vk::Result vkResult = {};
	CreateVkInstance_Return returnValue = {};

	// Build what extensions we are going to use
	auto extensionsToUse = Std::NewVec<char const*>(transientAlloc);
	extensionsToUse.Reserve(requiredExtensionsIn.Size() + Constants::requiredInstanceExtensions.size());
	// First copy all required instance extensions
	for (auto const& item : requiredExtensionsIn)
		extensionsToUse.PushBack(item);

	// Next add extensions required by renderer, don't add duplicates
	for (auto requiredExtension : Constants::requiredInstanceExtensions)
	{
		bool extensionAlreadyPresent = false;
		for (auto existingExtension : extensionsToUse)
		{
			if (std::strcmp(requiredExtension, existingExtension) == 0)
			{
				extensionAlreadyPresent = true;
				break;
			}
		}
		if (!extensionAlreadyPresent)
			extensionsToUse.PushBack(requiredExtension);
	}

	// Check if all the required extensions are also available
	u32 instanceExtensionCount = 0;
	vkResult = baseDispatch.EnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, nullptr);
	if (vkResult != vk::Result::eSuccess && vkResult != vk::Result::eIncomplete)
		throw std::runtime_error("Vulkan: Unable to enumerate available instance extension properties.");
	auto availableExtensions = Std::NewVec<vk::ExtensionProperties>(transientAlloc);
	availableExtensions.Resize(instanceExtensionCount);
	vkResult = baseDispatch.EnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, availableExtensions.Data());
	if (vkResult != vk::Result::eSuccess) {
		throw std::runtime_error("Vulkan: Unable to enumerate available instance extension properties.");
	}
	for (const char* required : extensionsToUse) {
		bool requiredExtensionIsAvailable = false;
		for (const auto& available : availableExtensions) {
			if (std::strcmp(required, available.extensionName) == 0) {
				requiredExtensionIsAvailable = true;
				break;
			}
		}
		if (!requiredExtensionIsAvailable)
			throw std::runtime_error("Required Vulkan instance extension is not available.");
	}

	Std::StackVec<const char*, 5> layersToUse = {};
	if constexpr (Constants::enableDebugUtils)
	{
		if (enableLayers) {
			// Check if debug utils is available through global list.
			bool debugUtilsIsAvailable = false;
			for (const auto& ext : availableExtensions) {
				if (std::strcmp(ext.extensionName, Constants::debugUtilsExtensionName) == 0) {
					debugUtilsIsAvailable = true;
					break;
				}
			}

			u32 availableLayerCount = 0;
			vkResult = baseDispatch.EnumerateInstanceLayerProperties(&availableLayerCount, nullptr);
			if (vkResult != vk::Result::eSuccess && vkResult != vk::Result::eIncomplete)
				throw std::runtime_error("Failed to enumerate instance layer properties during Vulkan instance creation.");

			auto availableLayers = Std::NewVec<vk::LayerProperties>(transientAlloc);
			availableLayers.Resize(availableLayerCount);
			vkResult = baseDispatch.EnumerateInstanceLayerProperties(&availableLayerCount, availableLayers.Data());
			if (vkResult != vk::Result::eSuccess) {
				throw std::runtime_error("Failed to enumerate instance layer properties during Vulkan instance creation.");
			}

			if (!debugUtilsIsAvailable) {
				// Debug utils is not confirmed to be available yet,
				// We look if we find the KHRONOS layer is available,
				// it guarantees debug-utils to be availab.
				for (const auto& availableLayer : availableLayers) {
					char const* khronosLayerName = Constants::khronosLayerName;
					char const* availableLayerName = availableLayer.layerName;
					if (std::strcmp(khronosLayerName, availableLayerName) == 0)
					{
						// If the layer is available, we know it implements debug utils.
						debugUtilsIsAvailable = true;
						break;
					}
				}
			}

			if (debugUtilsIsAvailable) {
				// Add all preferred layers that are also available.
				for (auto const& availableLayer : availableLayers)
				{
					for (auto const& preferredLayerName : Constants::preferredLayerNames)
					{
						if (std::strcmp(availableLayer.layerName, preferredLayerName) == 0)
							layersToUse.PushBack(preferredLayerName);
					}
				}

				extensionsToUse.PushBack(Constants::debugUtilsExtensionName);
				returnValue.debugUtilsEnabled = true;
			}
		}
	}

	vk::InstanceCreateInfo instanceInfo{};
	instanceInfo.enabledExtensionCount = (u32)extensionsToUse.Size();
	instanceInfo.ppEnabledExtensionNames = extensionsToUse.Data();
	instanceInfo.enabledLayerCount = (u32)layersToUse.Size();
	instanceInfo.ppEnabledLayerNames = layersToUse.Data();

#ifdef __APPLE__
	instanceInfo.flags = vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
#endif

	auto test = baseDispatch.EnumerateInstanceVersion();
	auto major = VK_VERSION_MAJOR(test);
	auto minor = VK_VERSION_MINOR(test);

	vk::ApplicationInfo appInfo{};
	appInfo.apiVersion = VK_MAKE_VERSION(1, 1, 0);
	appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
	appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
	appInfo.pApplicationName = "DEngine";
	appInfo.pEngineName = "DEngine";
	instanceInfo.pApplicationInfo = &appInfo;

	// If we want debug utils functionality when constructing the VkInstance,
	// we must pass it in the pNext member. The actual DebugUtilsMessenger object
	// is created after the VkInstance.
	vk::DebugUtilsMessengerCreateInfoEXT messengerCreateInfo = {};
	if constexpr (Constants::enableDebugUtils) {
		messengerCreateInfo.messageSeverity =
			vk::DebugUtilsMessageSeverityFlagBitsEXT::eError
			| vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning;
			//vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
			//vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose;
		messengerCreateInfo.messageType =
			vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
			| vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance
			| vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation;
		messengerCreateInfo.pfnUserCallback = VulkanDebugCallback;
		messengerCreateInfo.pUserData = static_cast<void*>(logger);
		instanceInfo.pNext = &messengerCreateInfo;
	}

	vk::Instance instance = baseDispatch.CreateInstance(instanceInfo);

	returnValue.instanceHandle = instance;

	return returnValue;
}

vk::DebugUtilsMessengerEXT Vk::Init::CreateLayerMessenger(
	vk::Instance instanceHandle,
	DebugUtilsDispatch const* debugUtilsOpt,
	void* userData)
{
	vk::DebugUtilsMessengerCreateInfoEXT debugMessengerInfo = {};
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
	debugMessengerInfo.pUserData = userData;

	return debugUtilsOpt->createDebugUtilsMessengerEXT(instanceHandle, debugMessengerInfo);
}

void Vk::SurfaceInfo::BuildInPlace(
	SurfaceInfo& surfaceInfo,
	vk::SurfaceKHR initialSurface,
	InstanceDispatch const& instance,
	vk::PhysicalDevice physDevice)
{
	vk::Result vkResult = {};

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
	vk::PresentModeKHR presentModeToUse = {};
	bool preferredPresentModeFound = false;
	for (auto const availableMode : surfaceInfo.supportedPresentModes) {
		if (availableMode == Constants::preferredPresentMode) {
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
	Std::Opt<vk::SurfaceFormatKHR> formatToUse;
	for (const auto& preferredFormat : Constants::preferredSurfaceFormats) {
		for (const auto& availableFormat : surfaceInfo.supportedSurfaceFormats) {
			if (availableFormat == preferredFormat) {
				formatToUse = preferredFormat;
				break;
			}
		}
		if (formatToUse.Has())
			break;
	}
	if (!formatToUse.Has())
		throw std::runtime_error("Vulkan: Found no suitable surface format when querying VkSurfaceKHR.");
	surfaceInfo.surfaceFormatToUse = formatToUse.Value();

	auto surfaceCaps = instance.getPhysicalDeviceSurfaceCapabilitiesKHR(physDevice, initialSurface);

	if (surfaceCaps.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::eOpaque)
		surfaceInfo.compositeAlphaToUse = vk::CompositeAlphaFlagBitsKHR::eOpaque;
	else if (surfaceCaps.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::eInherit)
		surfaceInfo.compositeAlphaToUse = vk::CompositeAlphaFlagBitsKHR::eInherit;
	else
		throw std::runtime_error("DEngine - Renderer: : Found no suitable compostive alpha flag bit for swapchain.");
}

Vk::PhysDeviceInfo Vk::Init::LoadPhysDevice(
	InstanceDispatch const& instance,
	vk::SurfaceKHR surface,
	Std::AllocRef const& transientAlloc)
{
	PhysDeviceInfo physDevice = {};
	vk::Result vkResult = {};

	u32 physicalDeviceCount = 0;
	vkResult = instance.enumeratePhysicalDevices(&physicalDeviceCount, nullptr);
	if (vkResult != vk::Result::eSuccess && vkResult != vk::Result::eIncomplete)
		throw std::runtime_error("Vulkan: Unable to enumerate physical devices.");
	if (physicalDeviceCount == 0)
		throw std::runtime_error("Vulkan: Host machine has no Vulkan-capable devices.");
	auto physDevices = Std::NewVec<vk::PhysicalDevice>(transientAlloc);
	physDevices.Resize(physicalDeviceCount);
	vkResult = instance.enumeratePhysicalDevices(&physicalDeviceCount, physDevices.Data());
	if (vkResult != vk::Result::eSuccess)
		throw std::runtime_error("DEngine-Gfx-Vulkan: Unable to enumerate physical devices.");

	// For now we just select the first physDevice we find.
	physDevice.handle = physDevices[0];

	// Find preferred queues
	u32 queueFamilyPropertyCount = 0;
	instance.getPhysicalDeviceQueueFamilyProperties(
		physDevice.handle,
		&queueFamilyPropertyCount,
		nullptr);
	auto availableQueueFamilies = Std::NewVec<vk::QueueFamilyProperties>(transientAlloc);
	availableQueueFamilies.Resize(queueFamilyPropertyCount);
	instance.getPhysicalDeviceQueueFamilyProperties(
		physDevice.handle, 
		&queueFamilyPropertyCount, 
		availableQueueFamilies.Data());

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
	PhysDeviceInfo const& physDevice,
	Std::AllocRef const& transientAlloc)
{
	vk::Result vkResult = {};

	// CreateJob logical physDevice
	vk::DeviceCreateInfo createInfo = {};

	// Feature configuration
	auto physDeviceFeatures = instance.getPhysicalDeviceFeatures(physDevice.handle);

	vk::PhysicalDeviceFeatures featuresToUse = {};
	// We turn on robust buffer access only if supported and only if debug-utils is enabled.
	if (physDeviceFeatures.robustBufferAccess == 1) {
		featuresToUse.robustBufferAccess = static_cast<vk::Bool32>(Constants::enableDebugUtils);
	}

	//if (physDeviceFeatures.sampleRateShading == 1)
			//featuresToUse.sampleRateShading = true;

	createInfo.pEnabledFeatures = &featuresToUse;

	// Queue configuration
	f32 priority[3] = { 1.f, 1.f, 1.f };
	Std::StackVec<vk::DeviceQueueCreateInfo, 10> queueCreateInfos = {};
	vk::DeviceQueueCreateInfo tempQueueCreateInfo = {};
	// Add graphics queue
	tempQueueCreateInfo.pQueuePriorities = priority;
	tempQueueCreateInfo.queueCount = 1;
	tempQueueCreateInfo.queueFamilyIndex = physDevice.queueIndices.graphics.familyIndex;
	queueCreateInfos.PushBack(tempQueueCreateInfo);

	// Add transfer queue if there is a separate one from graphics queue
	if (physDevice.queueIndices.transfer.familyIndex != invalidIndex) {
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
	if (vkResult != vk::Result::eSuccess && vkResult != vk::Result::eIncomplete) {
		throw std::runtime_error("Vulkan: Unable to enumerate device extensions.");
	}
	auto availableExtensions = Std::NewVec<vk::ExtensionProperties>(transientAlloc);
	availableExtensions.Resize(deviceExtensionCount);
	vkResult = instance.enumeratePhysicalDeviceExtensionProperties(physDevice.handle, &deviceExtensionCount, availableExtensions.Data());
	// Check if all required extensions are present
	Std::StackVec<const char*, 10> requiredExtensions;
	for (auto const& required : Constants::requiredDeviceExtensions) {
		requiredExtensions.PushBack(required);
	}
#ifdef __APPLE__
	requiredExtensions.PushBack("VK_KHR_portability_subset");
#endif

	for (const char* required : requiredExtensions) {
		bool foundExtension = false;
		for (const auto& available : availableExtensions) {
			if (std::strcmp(required, available.extensionName) == 0) {
				foundExtension = true;
				break;
			}
		}
		if (!foundExtension) {
			throw std::runtime_error("Not all required physDevice extensions were available during Vulkan initialization.");
		}
	}

	createInfo.ppEnabledExtensionNames = requiredExtensions.Data();
	createInfo.enabledExtensionCount = (u32)requiredExtensions.Size();

	vk::Device vkDevice = instance.createDevice(physDevice.handle, createInfo);
	return vkDevice;
}

Std::StackVec<BoxVkFence, Vk::Constants::maxInFlightCount> Vk::Init::CreateMainFences(
	DevDispatch const& device, 
	u8 resourceSetCount,
	DebugUtilsDispatch const* debugUtils)
{
	Std::StackVec<BoxVkFence, Constants::maxInFlightCount> returnVal = {};
	returnVal.Resize(resourceSetCount);
	for (uSize i = 0; i < returnVal.Size(); i += 1) {
		vk::FenceCreateInfo info = {};
		info.flags = vk::FenceCreateFlagBits::eSignaled;
		returnVal[i] = device.CreateBox(info);
		if (debugUtils) {
			auto name = std::format("Main Fence #{}", i);
			debugUtils->Helper_SetObjectName(
				device.handle,
				returnVal[i].Handle(),
				name.c_str());
		}
	}

	return returnVal;
}

static Std::Array<vk::SubpassDependency, 2> BuildSharedUiSubpassDependencies()
{
	Std::Array<vk::SubpassDependency, 2> dependencies = {};

	// External -> subpass 0.
	dependencies[0].dependencyFlags = vk::DependencyFlags();
	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcStageMask =
		vk::PipelineStageFlagBits::eColorAttachmentOutput
		| vk::PipelineStageFlagBits::eFragmentShader
		| vk::PipelineStageFlagBits::eEarlyFragmentTests
		| vk::PipelineStageFlagBits::eLateFragmentTests;
	dependencies[0].srcAccessMask = vk::AccessFlagBits::eShaderRead;
	dependencies[0].dstStageMask =
		vk::PipelineStageFlagBits::eColorAttachmentOutput
		| vk::PipelineStageFlagBits::eEarlyFragmentTests
		| vk::PipelineStageFlagBits::eLateFragmentTests;
	dependencies[0].dstAccessMask =
		vk::AccessFlagBits::eColorAttachmentWrite
		| vk::AccessFlagBits::eDepthStencilAttachmentRead
		| vk::AccessFlagBits::eDepthStencilAttachmentWrite;

	// Subpass 0 -> external.
	dependencies[1].dependencyFlags = vk::DependencyFlags();
	dependencies[1].srcSubpass = 0;
	dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask =
		vk::PipelineStageFlagBits::eColorAttachmentOutput
		| vk::PipelineStageFlagBits::eEarlyFragmentTests
		| vk::PipelineStageFlagBits::eLateFragmentTests;
	dependencies[1].srcAccessMask =
		vk::AccessFlagBits::eColorAttachmentWrite
		| vk::AccessFlagBits::eDepthStencilAttachmentWrite;
	dependencies[1].dstStageMask =
		vk::PipelineStageFlagBits::eFragmentShader
		| vk::PipelineStageFlagBits::eBottomOfPipe;
	dependencies[1].dstAccessMask =
		vk::AccessFlagBits::eShaderRead
		| vk::AccessFlagBits::eMemoryRead;

	return dependencies;
}

vk::RenderPass Vk::Init::BuildMainGfxRenderPass(
	DeviceDispatch const& device,
	vk::Format renderTargetFormat,
	bool useEditorPipeline,
	DebugUtilsDispatch const* debugUtils)
{
	if (!useEditorPipeline)
		DENGINE_IMPL_GFX_UNREACHABLE(); // We haven't implemented this yet.

	vk::AttachmentDescription colorAttachment = {};
	colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
	colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
	colorAttachment.samples = vk::SampleCountFlagBits::e1;
	colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
	colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
	colorAttachment.format = renderTargetFormat;
	colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
	// We want to sample from the finalized image into the editor GUI
	colorAttachment.finalLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

	vk::AttachmentDescription stencilAttachment = {};
	stencilAttachment.format = vk::Format::eS8Uint;
	stencilAttachment.samples = vk::SampleCountFlagBits::e1;
	stencilAttachment.initialLayout = vk::ImageLayout::eUndefined;
	stencilAttachment.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
	stencilAttachment.loadOp = vk::AttachmentLoadOp::eDontCare;
	stencilAttachment.storeOp = vk::AttachmentStoreOp::eDontCare;
	stencilAttachment.stencilLoadOp = vk::AttachmentLoadOp::eClear;
	stencilAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;

	Std::Array<vk::AttachmentDescription, 2> attachments {
		colorAttachment,
		stencilAttachment };

	// We want to render into the graphics viewport on subpass 0.
	vk::AttachmentReference colorAttachRef = {};
	colorAttachRef.attachment = 0;
	colorAttachRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

	vk::AttachmentReference stencilAttachmentRef = {};
	stencilAttachmentRef.attachment = 1;
	stencilAttachmentRef.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

	vk::SubpassDescription subpassDescr = {};
	subpassDescr.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
	subpassDescr.colorAttachmentCount = 1;
	subpassDescr.pColorAttachments = &colorAttachRef;
	subpassDescr.pDepthStencilAttachment = &stencilAttachmentRef;

	// This graphics framebuffer is rendered into, and then sampled from in the GUI rendering pass.
	// These dependencies are shared with the GUI render pass to keep the two render passes
	// compatible, so that the UI pipelines may be used in both. See BuildSharedUiSubpassDependencies.
	auto dependencies = BuildSharedUiSubpassDependencies();

	vk::RenderPassCreateInfo rpInfo = {};
	rpInfo.attachmentCount = attachments.Size();
	rpInfo.pAttachments = attachments.Data();
	rpInfo.subpassCount = 1;
	rpInfo.pSubpasses = &subpassDescr;
	rpInfo.dependencyCount = dependencies.Size();
	rpInfo.pDependencies = dependencies.Data();

	vk::RenderPass renderPass = device.createRenderPass(rpInfo);
	if (debugUtils) {
		debugUtils->Helper_SetObjectName(
			device.handle,
			renderPass,
			"Main Gfx RenderPass");
	}

	return renderPass;
}

vk::RenderPass Vk::Init::CreateGuiRenderPass(
	DeviceDispatch const& device,
	vk::Format guiTargetFormat,
	DebugUtilsDispatch const* debugUtils)
{
	vk::AttachmentDescription colorAttachment{};
	colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
	// We want to present the image after we're done rendering to it.
	colorAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;
	colorAttachment.format = guiTargetFormat;
	colorAttachment.samples = vk::SampleCountFlagBits::e1;
	colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
	colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
	colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
	colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;

	vk::AttachmentDescription stencilAttachment = {};
	stencilAttachment.initialLayout = vk::ImageLayout::eUndefined;
	stencilAttachment.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
	stencilAttachment.format = vk::Format::eS8Uint;
	stencilAttachment.samples = vk::SampleCountFlagBits::e1;
	stencilAttachment.loadOp = vk::AttachmentLoadOp::eDontCare;
	stencilAttachment.storeOp = vk::AttachmentStoreOp::eDontCare;
	stencilAttachment.stencilLoadOp = vk::AttachmentLoadOp::eClear;
	stencilAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;

	Std::Array<vk::AttachmentDescription, 2> attachments {
		colorAttachment,
		stencilAttachment };

	// We want to render into the GUI in subpass 0.
	vk::AttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

	vk::AttachmentReference stencilAttachmentRef = {};
	stencilAttachmentRef.attachment = 1;
	stencilAttachmentRef.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

	vk::SubpassDescription subpassDescription{};
	subpassDescription.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
	subpassDescription.colorAttachmentCount = 1;
	subpassDescription.pColorAttachments = &colorAttachmentRef;
	subpassDescription.pDepthStencilAttachment = &stencilAttachmentRef;

	// Shared with the Main Gfx render pass to keep the two render passes compatible, so that the
	// UI pipelines may be used in both. The external->subpass dependency chains with the
	// acquire-image semaphore (waited at color-attachment-output) so the attachment layout
	// transition is ordered after presentation finishes reading the swapchain image.
	// See BuildSharedUiSubpassDependencies.
	auto dependencies = BuildSharedUiSubpassDependencies();

	// Set up render pass
	vk::RenderPassCreateInfo createInfo = {};
	createInfo.attachmentCount = attachments.Size();
	createInfo.pAttachments = attachments.Data();
	createInfo.subpassCount = 1;
	createInfo.pSubpasses = &subpassDescription;
	createInfo.dependencyCount = dependencies.Size();
	createInfo.pDependencies = dependencies.Data();

	vk::RenderPass renderPass = device.createRenderPass(createInfo);
	if (debugUtils != nullptr) {
		debugUtils->Helper_SetObjectName(
			device.handle,
			renderPass,
			"GUI RenderPass");
	}

	return renderPass;
}

auto Init::CreateMainCmdBuffers(
	DeviceDispatch const& device,
	int queueFamilyIndex,
	int inFlightCount,
	DebugUtilsDispatch const* debugUtils)
	-> CreateMainCmdBuffers_ReturnT
{
	CreateMainCmdBuffers_ReturnT returnVal = {};

	returnVal.cmdPools.Resize(inFlightCount);
	returnVal.cmdBuffers.Resize(inFlightCount);

	for (uSize i = 0; i < inFlightCount; i += 1) {
		vk::CommandPoolCreateInfo cmdPoolInfo = {};
		//cmdPoolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
		cmdPoolInfo.queueFamilyIndex = queueFamilyIndex;
		auto cmdPool = device.CreateBox(cmdPoolInfo);
		returnVal.cmdPools[i] = Std::Move(cmdPool);
		if (debugUtils) {
			auto name = std::format("Main CmdPool #{}", i);
			debugUtils->Helper_SetObjectName(
				device.handle,
				returnVal.cmdPools[i].Handle(),
				name.c_str());
		}

		vk::CommandBufferAllocateInfo cmdBufferAllocInfo = {};
		cmdBufferAllocInfo.commandBufferCount = 1;
		cmdBufferAllocInfo.commandPool = returnVal.cmdPools[i].Handle();
		cmdBufferAllocInfo.level = vk::CommandBufferLevel::ePrimary;
		auto vkResult = device.allocateCommandBuffers(cmdBufferAllocInfo, &returnVal.cmdBuffers[i]);
		if (vkResult != vk::Result::eSuccess)
			throw std::runtime_error("DEngine - Vulkan: Failed to initialize main commandbuffers.");
		// We don't give the command buffers debug names here,
		// because we need to rename them everytime we re-record anyways.
	}

	return returnVal;
}
