#include "Init.hpp"

#include "VulkanIncluder.hpp"

#include "DynamicDispatch.hpp"
#include "SurfaceInfo.hpp"

#include <DEngine/Gfx/impl/Assert.hpp>

#include <DEngine/Std/Containers/Vec.hpp>

#include <string>

namespace DEngine::Gfx::Vk
{
	VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverityIn,
		VkDebugUtilsMessageTypeFlagsEXT messageTypeIn,
		VkDebugUtilsMessengerCallbackDataEXT const* pCallbackDataIn,
		void* pUserData)
	{
		auto messageSeverity = (vk::DebugUtilsMessageSeverityFlagBitsEXT)messageSeverityIn;
		auto messageType = (vk::DebugUtilsMessageTypeFlagsEXT)messageTypeIn;
		auto* pCallbackData = reinterpret_cast<vk::DebugUtilsMessengerCallbackDataEXT const*>(pCallbackDataIn);

		auto* logger = static_cast<LogInterface*>(pUserData);




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
	for (const char* required : extensionsToUse)
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

			if (!debugUtilsIsAvailable)
			{
				// Debug utils is not confirmed to be available yet,
				// We look if we find the KHRONOS layer is available,
				// it guarantees debug-utils to be availab.
				for (const auto& availableLayer : availableLayers)
				{
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

			if (debugUtilsIsAvailable)
			{
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

	auto test = baseDispatch.EnumerateInstanceVersion();
	auto major = VK_VERSION_MAJOR(test);
	auto minor = VK_VERSION_MINOR(test);

	vk::ApplicationInfo appInfo{};
	appInfo.apiVersion = VK_MAKE_VERSION(1, 3, 0);
	appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
	appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
	appInfo.pApplicationName = "DidgyImguiVulkanTest";
	appInfo.pEngineName = "Nothing special";
	instanceInfo.pApplicationInfo = &appInfo;

	vk::DebugUtilsMessengerCreateInfoEXT messengerCreateInfo{};
	if constexpr (Constants::enableDebugUtils)
	{
		messengerCreateInfo.messageSeverity =
			vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
			vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning;
			//vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
			//vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose;
		messengerCreateInfo.messageType =
			vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
			vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
			vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation;
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
	debugMessengerInfo.pUserData = userData;

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
	vk::SurfaceKHR surface,
	Std::AllocRef const& transientAlloc)
{
	PhysDeviceInfo physDevice{};
	vk::Result vkResult{};

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
	Std::StackVec<vk::DeviceQueueCreateInfo, 10> queueCreateInfos{};

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
	auto availableExtensions = Std::NewVec<vk::ExtensionProperties>(transientAlloc);
	availableExtensions.Resize(deviceExtensionCount);
	vkResult = instance.enumeratePhysicalDeviceExtensionProperties(physDevice.handle, &deviceExtensionCount, availableExtensions.Data());
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
		if (!foundExtension)
			throw std::runtime_error("Not all required physDevice extensions were available during Vulkan initialization.");
	}

	createInfo.ppEnabledExtensionNames = Constants::requiredDeviceExtensions.data();
	createInfo.enabledExtensionCount = u32(Constants::requiredDeviceExtensions.size());

	vk::Device vkDevice = instance.createDevice(physDevice.handle, createInfo);
	return vkDevice;
}

Std::StackVec<vk::Fence, Vk::Constants::maxInFlightCount> Vk::Init::CreateMainFences(
	DevDispatch const& device, 
	u8 resourceSetCount,
	DebugUtilsDispatch const* debugUtils)
{
	Std::StackVec<vk::Fence, Constants::maxInFlightCount> returnVal{};
	returnVal.Resize(resourceSetCount);

	for (uSize i = 0; i < returnVal.Size(); i += 1)
	{
		vk::FenceCreateInfo info{};
		info.flags = vk::FenceCreateFlagBits::eSignaled;
		returnVal[i] = device.createFence(info);

		if (debugUtils)
		{
			std::string name = std::string("Main Fence #") + std::to_string(i);
			debugUtils->Helper_SetObjectName(
				device.handle,
				returnVal[i],
				name.c_str());
		}
	}

	return returnVal;
}

vk::RenderPass Vk::Init::BuildMainGfxRenderPass(
	DeviceDispatch const& device,
	bool useEditorPipeline,
	DebugUtilsDispatch const* debugUtils)
{
	if (!useEditorPipeline)
		DENGINE_IMPL_GFX_UNREACHABLE(); // We haven't implemented this yet.

	vk::AttachmentDescription colorAttach = {};
	colorAttach.loadOp = vk::AttachmentLoadOp::eClear;
	colorAttach.storeOp = vk::AttachmentStoreOp::eStore;
	colorAttach.samples = vk::SampleCountFlagBits::e1;
	colorAttach.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
	colorAttach.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
	colorAttach.format = vk::Format::eR8G8B8A8Srgb;
	colorAttach.initialLayout = vk::ImageLayout::eUndefined;
	// We want to sample from the finalized image into the editor GUI
	colorAttach.finalLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

	// We want to render into the graphics viewport on subpass 0.
	vk::AttachmentReference colorAttachRef = {};
	colorAttachRef.attachment = 0;
	colorAttachRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

	vk::SubpassDescription subpass = {};
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachRef;

	// This graphics framebuffer is rendered into, and the sampled from into the GUI rendering pass.
	vk::SubpassDependency dependencies[2] = {};
	dependencies[0].dependencyFlags = vk::DependencyFlags();
	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].srcAccessMask = vk::AccessFlagBits::eShaderRead;
	dependencies[0].srcStageMask = vk::PipelineStageFlagBits::eFragmentShader;
	dependencies[0].dstSubpass = 0;
	dependencies[0].dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
	dependencies[0].dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;

	dependencies[1].dependencyFlags = vk::DependencyFlags();
	dependencies[1].srcSubpass = 0;
	dependencies[1].srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
	dependencies[1].srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[1].dstAccessMask = vk::AccessFlagBits::eShaderRead;
	dependencies[1].dstStageMask = vk::PipelineStageFlagBits::eFragmentShader;

	vk::RenderPassCreateInfo rpInfo = {};
	rpInfo.attachmentCount = 1;
	rpInfo.pAttachments = &colorAttach;
	rpInfo.subpassCount = 1;
	rpInfo.pSubpasses = &subpass;
	rpInfo.dependencyCount = 2;
	rpInfo.pDependencies = dependencies;

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

	vk::AttachmentDescription attachments[1] = { colorAttachment };

	// We want to render into the GUI in subpass 0.
	vk::AttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

	vk::SubpassDescription subpassDescription{};
	subpassDescription.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
	subpassDescription.colorAttachmentCount = 1;
	subpassDescription.pColorAttachments = &colorAttachmentRef;

	// The framebuffer is the swapchain image. It gets rendered to as color-attachment, and then
	// it is presented to surface.
	vk::SubpassDependency dependencies[2] = {};
	dependencies[0].dependencyFlags = vk::DependencyFlags();
	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].srcAccessMask = vk::AccessFlagBits::eNoneKHR;
	dependencies[0].srcStageMask = vk::PipelineStageFlagBits::eTopOfPipe;
	dependencies[0].dstSubpass = 0;
	dependencies[0].dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
	dependencies[0].dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;

	dependencies[1].dependencyFlags = vk::DependencyFlags();
	dependencies[1].srcSubpass = 0;
	dependencies[1].srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
	dependencies[1].srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[1].dstAccessMask = vk::AccessFlagBits::eMemoryRead;
	dependencies[1].dstStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;

	// Set up render pass
	vk::RenderPassCreateInfo createInfo = {};
	createInfo.attachmentCount = 1;
	createInfo.pAttachments = attachments;
	createInfo.subpassCount = 1;
	createInfo.pSubpasses = &subpassDescription;
	createInfo.dependencyCount = 2;
	createInfo.pDependencies = dependencies;

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
		auto cmdPool = device.Create(cmdPoolInfo);
		if (cmdPool.result != vk::Result::eSuccess)
			throw std::runtime_error("Unable to make command pool");
		returnVal.cmdPools[i] = cmdPool.value;
		if (debugUtils) {
			std::string name = "Main CmdPool #";
			name += std::to_string(i);
			debugUtils->Helper_SetObjectName(
				device.handle,
				returnVal.cmdPools[i],
				name.c_str());
		}

		vk::CommandBufferAllocateInfo cmdBufferAllocInfo = {};
		cmdBufferAllocInfo.commandBufferCount = 1;
		cmdBufferAllocInfo.commandPool = returnVal.cmdPools[i];
		cmdBufferAllocInfo.level = vk::CommandBufferLevel::ePrimary;
		auto vkResult = device.allocateCommandBuffers(cmdBufferAllocInfo, &returnVal.cmdBuffers[i]);
		if (vkResult != vk::Result::eSuccess)
			throw std::runtime_error("DEngine - Vulkan: Failed to initialize main commandbuffers.");
		// We don't give the command buffers debug names here,
		// because we need to rename them everytime we re-record anyways.
	}

	return returnVal;
}