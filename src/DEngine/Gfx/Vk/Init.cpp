#include "Init.hpp"

#include "DEngine/Gfx/Assert.hpp"

#include "ImGui/imgui_impl_vulkan.h"

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

        ILog* logger = static_cast<ILog*>(pUserData);

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

            logger->log(msg.data());
        }
            

        return 0;
    }
}

DEngine::Gfx::Vk::Init::CreateVkInstance_Return DEngine::Gfx::Vk::Init::CreateVkInstance(
    Cont::Span<char const*> const requiredExtensions,
    bool const enableLayers,
    BaseDispatch const& baseDispatch,
    ILog* logger)
{
    vk::Result vkResult{};
    CreateVkInstance_Return returnValue{};

    // Build what extensions we are going to use
    Cont::FixedVector<const char*, Constants::maxRequiredInstanceExtensions> totalRequiredExtensions;
    if (requiredExtensions.size() > totalRequiredExtensions.maxSize())
    {
        logger->log("Application requires more instance extensions than is maximally allocated during Vulkan instance creation. Increase it in the backend.");
        std::abort();
    }
    // First copy all required instance extensions
    for (uSize i = 0; i < requiredExtensions.size(); i++)
        totalRequiredExtensions.pushBack(requiredExtensions[i]);

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
        if (extensionAlreadyPresent == false)
        {
            if (totalRequiredExtensions.canPushBack() == false)
            {
                logger->log("Could not fit all required instance extensions in allocated memory during Vulkan instance creation. Increase it in the backend.");
                std::abort();
            }
            totalRequiredExtensions.pushBack(requiredExtension);
        }
    }

    {
        // Check if all the required extensions are also available
        u32 instanceExtensionCount = 0;
        vkResult = baseDispatch.enumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, nullptr);
        if (vkResult != vk::Result::eSuccess && vkResult != vk::Result::eIncomplete)
            throw std::runtime_error("Vulkan: Unable to enumerate available instance extension properties.");
        std::vector<vk::ExtensionProperties> availableExtensions(instanceExtensionCount);
        baseDispatch.enumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, availableExtensions.data());
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
            if (requiredExtensionIsAvailable == false)
                throw std::runtime_error("Required Vulkan instance extension is not available.");
        }
    }

    // Add Khronos validation layer if both it and debug_utils is available
    Cont::FixedVector<const char*, 5> layersToUse{};
    if constexpr (Constants::enableDebugUtils)
    {
        if (enableLayers)
        {
            // First check if the Khronos validation layer is present
            u32 availableLayerCount = 0;
            vkResult = baseDispatch.enumerateInstanceLayerProperties(&availableLayerCount, nullptr);
            if (vkResult != vk::Result::eSuccess && vkResult != vk::Result::eIncomplete)
                throw std::runtime_error("Failed to enumerate instance layer properties during Vulkan instance creation.");
            Cont::FixedVector<vk::LayerProperties, Constants::maxAvailableInstanceLayers> availableLayers;
            if (availableLayerCount > availableLayers.maxSize())
                throw std::runtime_error("Could not fit available layers in allocated memory during Vulkan instance creation.");
            availableLayers.resize(availableLayerCount);
            baseDispatch.enumerateInstanceLayerProperties(&availableLayerCount, availableLayers.data());

            bool layerIsAvailable = false;
            for (const auto& availableLayer : availableLayers)
            {
                char const* wantedLayerName = Constants::preferredValidationLayer.data();
                char const* availableLayerName = availableLayer.layerName;
                if (std::strcmp(wantedLayerName, availableLayerName) == 0)
                {
                    layerIsAvailable = true;
                    break;
                }
            }
            if (layerIsAvailable == true)
            {
                // If the layer is available, we check if it implements VK_EXT_debug_utils
                u32 layerExtensionCount = 0;
                vkResult = baseDispatch.enumerateInstanceExtensionProperties(Constants::preferredValidationLayer.data(), &layerExtensionCount, nullptr);
                if (vkResult != vk::Result::eSuccess && vkResult != vk::Result::eIncomplete)
                    throw std::runtime_error("Vulkan: Unable to enumerate available instance extension properties.");
                Cont::FixedVector<vk::ExtensionProperties, Constants::maxAvailableLayerExtensions> availableExtensions;
                if (layerExtensionCount > availableExtensions.maxSize())
                    throw std::runtime_error("Vulkan: Unable to load layer's extensions into allocated memory.");
                availableExtensions.resize(layerExtensionCount);
                baseDispatch.enumerateInstanceExtensionProperties(Constants::preferredValidationLayer.data(), &layerExtensionCount, availableExtensions.data());

                bool debugUtilsIsAvailable = false;
                for (auto const& availableLayerExtension : availableExtensions)
                {
                    if (std::strcmp(availableLayerExtension.extensionName, Constants::debugUtilsExtensionName.data()) == 0)
                    {
                        debugUtilsIsAvailable = true;
                        break;
                    }
                }

                if (debugUtilsIsAvailable)
                {
                    // Debug utils and the Khronos validation layer is available.
                    // Push them onto the vectors
                    if (!totalRequiredExtensions.canPushBack())
                        throw std::runtime_error("Vulkan: Unable to push debug-utils on extensions to use, fixed-vector too small.");
                    totalRequiredExtensions.pushBack(Constants::debugUtilsExtensionName.data());
                    if (!layersToUse.canPushBack())
                        throw std::runtime_error("Vulkan: Unable to push Khronos validation layer onto layers to use, fixed-vector too small.");
                    layersToUse.pushBack(Constants::preferredValidationLayer.data());

                    returnValue.debugUtilsEnabled = true;
                }
            }
        }
    }

    vk::InstanceCreateInfo instanceInfo{};
    instanceInfo.enabledExtensionCount = (u32)totalRequiredExtensions.size();
    instanceInfo.ppEnabledExtensionNames = totalRequiredExtensions.data();
    instanceInfo.enabledLayerCount = (u32)layersToUse.size();
    instanceInfo.ppEnabledLayerNames = layersToUse.data();

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

    auto infoPtr = reinterpret_cast<VkDebugUtilsMessengerCreateInfoEXT const*>(&debugMessengerInfo);

    return debugUtilsOpt->createDebugUtilsMessengerEXT(instanceHandle, debugMessengerInfo);
}

DEngine::Gfx::Vk::PhysDeviceInfo DEngine::Gfx::Vk::Init::LoadPhysDevice(
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
    Cont::FixedVector<vk::PhysicalDevice, Constants::maxAvailablePhysicalDevices> physDevices;
    if (physicalDeviceCount > physDevices.maxSize())
        throw std::out_of_range("Vulkan: Could not fit all Vulkan physical physDevice handles in allocated memory during Vulkan physDevice creation.");
    physDevices.resize(physicalDeviceCount);
    instance.enumeratePhysicalDevices(&physicalDeviceCount, physDevices.data());

    // For now we just select the first physDevice we find.
    physDevice.handle = physDevices[0];

    // Find preferred queues
    u32 queueFamilyPropertyCount = 0;
    instance.getPhysicalDeviceQueueFamilyProperties(physDevice.handle, &queueFamilyPropertyCount, nullptr);
    Cont::FixedVector<vk::QueueFamilyProperties, Constants::maxAvailableQueueFamilies> availableQueueFamilies;
    if (queueFamilyPropertyCount > availableQueueFamilies.maxSize())
        throw std::runtime_error("Failed to fit all VkQueueFamilyProperties inside allocated memory during Vulkan initialization.");
    availableQueueFamilies.resize(queueFamilyPropertyCount);
    instance.getPhysicalDeviceQueueFamilyProperties(physDevice.handle, &queueFamilyPropertyCount, availableQueueFamilies.data());

    // Find graphics queue
    for (u32 i = 0; i < queueFamilyPropertyCount; i++)
    {
        const auto& queueFamily = availableQueueFamilies[i];
        if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
        {
            physDevice.preferredQueues.graphics.familyIndex = i;
            physDevice.preferredQueues.graphics.queueIndex = 0;
            break;
        }
    }
    DENGINE_GFX_ASSERT(isValidIndex(physDevice.preferredQueues.graphics.familyIndex));
    // Find transfer queue, prefer a queue on a different family than graphics. 
    for (u32 i = 0; i < queueFamilyPropertyCount; i++)
    {
        if (i == physDevice.preferredQueues.graphics.familyIndex)
            continue;

        const auto& queueFamily = availableQueueFamilies[i];
        if (queueFamily.queueFlags & vk::QueueFlagBits::eTransfer)
        {
            physDevice.preferredQueues.transfer.familyIndex = i;
            physDevice.preferredQueues.transfer.queueIndex = 0;
            break;
        }
    }

    // Check presentation support
    bool presentSupport = instance.getPhysicalDeviceSurfaceSupportKHR(physDevice.handle, physDevice.preferredQueues.graphics.familyIndex, surface);
    if (presentSupport == false)
        throw std::runtime_error("Vulkan: No surface present support.");

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
    if (isValidIndex(physDevice.memInfo.deviceLocal) == false)
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
    if (isValidIndex(physDevice.memInfo.hostVisible) == false)
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

DEngine::Gfx::Vk::SurfaceInfo DEngine::Gfx::Vk::Init::BuildSurfaceInfo(
    InstanceDispatch const& instance,
    vk::PhysicalDevice physDevice,
    vk::SurfaceKHR surface,
    ILog* logger)
{
    vk::Result vkResult{};

    SurfaceInfo returnVal{};

    returnVal.handle = surface;

    returnVal.capabilities = instance.getPhysicalDeviceSurfaceCapabilitiesKHR(physDevice, surface);

    // Grab present modes
    u32 presentModeCount = 0;
    vkResult = instance.getPhysicalDeviceSurfacePresentModesKHR(physDevice, surface, &presentModeCount, nullptr);
    if ((vkResult != vk::Result::eSuccess && vkResult != vk::Result::eIncomplete) || presentModeCount == 0)
    {
        logger->log("Vulkan: Unable to query present modes for Vulkan surface.");
        std::abort();
    }
    if (presentModeCount > returnVal.supportedPresentModes.maxSize())
    {
        logger->log("Vulkan: Unable to fit all supported present modes in allocated memory.");
        std::abort();
    }
    returnVal.supportedPresentModes.resize(presentModeCount);
    instance.getPhysicalDeviceSurfacePresentModesKHR(physDevice, surface, &presentModeCount, returnVal.supportedPresentModes.data());

    // Grab surface formats
    u32 surfaceFormatCount = 0;
    vkResult = instance.getPhysicalDeviceSurfaceFormatsKHR(physDevice, surface, &surfaceFormatCount, (vk::SurfaceFormatKHR*)nullptr);
    if ((vkResult != vk::Result::eSuccess && vkResult != vk::Result::eIncomplete) || surfaceFormatCount == 0)
    {
        logger->log("Vulkan: Unable to query surface formats for Vulkan surface.");
        std::abort();
    }
    if (surfaceFormatCount > returnVal.supportedSurfaceFormats.maxSize())
    {
        logger->log("Vulkan: Unable to fit all supported surface formats in allocated memory.");
        std::abort();
    }
    returnVal.supportedSurfaceFormats.resize(surfaceFormatCount);
    instance.getPhysicalDeviceSurfaceFormatsKHR(physDevice, surface, &surfaceFormatCount, returnVal.supportedSurfaceFormats.data());

    if (returnVal.capabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::eOpaque)
        returnVal.compositeAlphaFlag = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    else if (returnVal.capabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::eInherit)
        returnVal.compositeAlphaFlag = vk::CompositeAlphaFlagBitsKHR::eInherit;
    else
    {
        logger->log("Vulkan: Found no suitable compostive alpha flag bit for swapchain.");
        std::abort();
    }

    return returnVal;
}

void DEngine::Gfx::Vk::Init::RebuildSurfaceInfo(
    InstanceDispatch const& instance,
    PhysDeviceInfo const& physDevice,
    vk::SurfaceKHR newSurface,
    SurfaceInfo& outSurfaceInfo)
{
    instance.destroySurface(outSurfaceInfo.handle);
    outSurfaceInfo.handle = newSurface;

    outSurfaceInfo.capabilities = instance.getPhysicalDeviceSurfaceCapabilitiesKHR(physDevice.handle, outSurfaceInfo.handle);

    // Check presentation support
    bool presentSupport = instance.getPhysicalDeviceSurfaceSupportKHR(physDevice.handle, physDevice.preferredQueues.graphics.familyIndex, outSurfaceInfo.handle);
    if (presentSupport == false)
        throw std::runtime_error("Vulkan: No presentation support for new surface");
}

DEngine::Gfx::Vk::SwapchainSettings DEngine::Gfx::Vk::Init::BuildSwapchainSettings(
    InstanceDispatch const& instance,
    vk::PhysicalDevice physDevice,
    SurfaceInfo const& surfaceInfo,
    ILog* logger)
{
    vk::Result vkResult{};

    SwapchainSettings settings{};
    settings.surface = surfaceInfo.handle;
    settings.capabilities = surfaceInfo.capabilities;
    settings.extents = settings.capabilities.currentExtent;
    settings.compositeAlphaFlag = surfaceInfo.compositeAlphaFlag;

    // Handle swapchainData length
    u32 swapchainLength = Constants::preferredSwapchainLength;
    // If we need to, clamp the swapchainData length.
    // Upper clamp only applies if maxImageCount != 0
    if (settings.capabilities.maxImageCount != 0 && swapchainLength > settings.capabilities.maxImageCount)
        swapchainLength = settings.capabilities.maxImageCount;
    if (swapchainLength < 2)
    {
        logger->log("Vulkan: Vulkan backend doesn't support swapchainData length of 1.");
        std::abort();
    }

    settings.numImages = swapchainLength;

    // Find supported formats, find the preferred present-mode
    // If not found, fallback to FIFO, it's guaranteed to be supported.
    vk::PresentModeKHR preferredPresentMode = Constants::preferredPresentMode;
    vk::PresentModeKHR presentModeToUse{};
    bool preferredPresentModeFound = false;
    for (auto const availableMode : surfaceInfo.supportedPresentModes)
    {
        if (availableMode == preferredPresentMode)
        {
            preferredPresentModeFound = true;
            presentModeToUse = availableMode;
            break;
        }
    }
    // FIFO is guaranteed to exist, so we fallback to that one if we didn't find the one we wanted.
    if (!preferredPresentModeFound)
        presentModeToUse = vk::PresentModeKHR::eFifo;
    settings.presentMode = presentModeToUse;

    // Handle formats
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
    {
        logger->log("Vulkan: Found no suitable surface format when querying VkSurfaceKHR.");
        std::abort();
    }
    settings.surfaceFormat = formatToUse;

    return settings;
}

vk::Device DEngine::Gfx::Vk::Init::CreateDevice(
    InstanceDispatch const& instance,
    PhysDeviceInfo const& physDevice)
{
    vk::Result vkResult{};

    // Create logical physDevice
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
    Cont::FixedVector<vk::DeviceQueueCreateInfo, 10> queueCreateInfos{};

    vk::DeviceQueueCreateInfo tempQueueCreateInfo{};

    // Add graphics queue
    tempQueueCreateInfo.pQueuePriorities = priority;
    tempQueueCreateInfo.queueCount = 1;
    tempQueueCreateInfo.queueFamilyIndex = physDevice.preferredQueues.graphics.familyIndex;
    queueCreateInfos.pushBack(tempQueueCreateInfo);

    // Add transfer queue if there is a separate one from graphics queue
    if (physDevice.preferredQueues.dedicatedTransfer())
    {
        tempQueueCreateInfo = vk::DeviceQueueCreateInfo{};
        tempQueueCreateInfo.pQueuePriorities = priority;
        tempQueueCreateInfo.queueCount = 1;
        tempQueueCreateInfo.queueFamilyIndex = physDevice.preferredQueues.transfer.familyIndex;

        queueCreateInfos.pushBack(tempQueueCreateInfo);
    }

    createInfo.queueCreateInfoCount = (u32)queueCreateInfos.size();
    createInfo.pQueueCreateInfos = queueCreateInfos.data();

    u32 deviceExtensionCount = 0;
    vkResult = instance.enumeratePhysicalDeviceExtensionProperties(physDevice.handle, &deviceExtensionCount, nullptr);
    if (vkResult != vk::Result::eSuccess && vkResult != vk::Result::eIncomplete)
        throw std::runtime_error("Vulkan: Unable to enumerate device extensions.");
    std::vector<vk::ExtensionProperties> availableExtensions(deviceExtensionCount);
    instance.enumeratePhysicalDeviceExtensionProperties(physDevice.handle, &deviceExtensionCount, availableExtensions.data());
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

DEngine::Gfx::Vk::SwapchainData DEngine::Gfx::Vk::Init::CreateSwapchain(
    Vk::DeviceDispatch const& device,
    vk::Queue vkQueue,
    SwapchainSettings const& settings,
    DebugUtilsDispatch const* debugUtils)
{
    vk::Result vkResult{};

    SwapchainData swapchain;
    swapchain.uid = 0;
    swapchain.extents = settings.extents;
    swapchain.surfaceFormat = settings.surfaceFormat;
    swapchain.presentMode = settings.presentMode;

    vk::SwapchainCreateInfoKHR swapchainCreateInfo{};
    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.imageExtent = settings.extents;
    swapchainCreateInfo.imageFormat = settings.surfaceFormat.format;
    swapchainCreateInfo.imageColorSpace = settings.surfaceFormat.colorSpace;
    swapchainCreateInfo.imageSharingMode = vk::SharingMode::eExclusive;
    swapchainCreateInfo.presentMode = settings.presentMode;
    swapchainCreateInfo.surface = settings.surface;
    swapchainCreateInfo.preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
    swapchainCreateInfo.clipped = 1;
    swapchainCreateInfo.compositeAlpha = settings.compositeAlphaFlag;
    swapchainCreateInfo.imageUsage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eColorAttachment;
    swapchainCreateInfo.minImageCount = settings.numImages;
    swapchainCreateInfo.oldSwapchain = vk::SwapchainKHR{};

    swapchain.handle = device.createSwapchainKHR(swapchainCreateInfo);
    // Make name for the swapchainData
    if (debugUtils != nullptr)
    {
        vk::DebugUtilsObjectNameInfoEXT nameInfo{};
        nameInfo.objectHandle = (u64)(VkSwapchainKHR)swapchain.handle;
        nameInfo.objectType = swapchain.handle.objectType;
        std::string objectName = std::string("Swapchain #") + std::to_string(0);
        nameInfo.pObjectName = objectName.data();
        debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
    }

    u32 swapchainImageCount = 0;
    vkResult = device.getSwapchainImagesKHR(swapchain.handle, &swapchainImageCount, (vk::Image*)nullptr);
    if (vkResult != vk::Result::eSuccess && vkResult != vk::Result::eIncomplete)
        throw std::runtime_error("Unable to grab swapchainData images from VkSwapchainKHR object.");
    DENGINE_GFX_ASSERT(swapchainImageCount != 0);
    DENGINE_GFX_ASSERT(swapchainImageCount == settings.numImages);
    if (swapchainImageCount >= swapchain.images.maxSize())
        throw std::runtime_error("Unable to fit swapchainData image handles in allocated memory.");
    swapchain.images.resize(swapchainImageCount);
    device.getSwapchainImagesKHR(swapchain.handle, &swapchainImageCount, swapchain.images.data());

    // Make names for the swapchainData images
    if (debugUtils != nullptr)
    {
        for (uSize i = 0; i < swapchain.images.size(); i++)
        {
            vk::DebugUtilsObjectNameInfoEXT nameInfo{};
            nameInfo.objectHandle = (u64)(VkImage)swapchain.images[i];
            nameInfo.objectType = swapchain.images[i].objectType;
            std::string objectName = std::string("Swapchain #0 - Image #") + std::to_string(i);
            nameInfo.pObjectName = objectName.data();
            debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
        }
    }

    // Transition new swapchainData images
    TransitionSwapchainImages(device, vkQueue, swapchain.images);

    vk::CommandPoolCreateInfo cmdPoolInfo{};
    swapchain.cmdPool = device.createCommandPool(cmdPoolInfo);
    if (debugUtils != nullptr)
    {
        vk::DebugUtilsObjectNameInfoEXT nameInfo{};
        nameInfo.objectHandle = (u64)(VkCommandPool)swapchain.cmdPool;
        nameInfo.objectType = swapchain.cmdPool.objectType;
        std::string objectName = std::string("Swapchain #0 - Copy image CmdPool");
        nameInfo.pObjectName = objectName.data();
        debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
    }
    vk::CommandBufferAllocateInfo cmdBufferAllocInfo{};
    cmdBufferAllocInfo.commandPool = swapchain.cmdPool;
    cmdBufferAllocInfo.commandBufferCount = (u32)swapchain.images.size();
    cmdBufferAllocInfo.level = vk::CommandBufferLevel::ePrimary;
    swapchain.cmdBuffers.resize(swapchain.images.size());
    vkResult = device.allocateCommandBuffers(&cmdBufferAllocInfo, swapchain.cmdBuffers.data());
    if (vkResult != vk::Result::eSuccess)
        throw std::runtime_error("Failed to allocate VkCommandBuffers for copy-to-swapchainData.");
    // Give names to swapchainData command cmdBuffers
    if (debugUtils != nullptr)
    {
        for (uSize i = 0; i < swapchain.cmdBuffers.size(); i++)
        {
            vk::DebugUtilsObjectNameInfoEXT nameInfo{};
            nameInfo.objectHandle = (u64)(VkCommandBuffer)swapchain.cmdBuffers[i];
            nameInfo.objectType = swapchain.cmdBuffers[i].objectType;
            std::string objectName = std::string("Swapchain #0 - Copy cmd buffer #0") + std::to_string(i);
            nameInfo.pObjectName = objectName.data();
            debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
        }
    }

    vk::SemaphoreCreateInfo semaphoreInfo{};
    swapchain.imageAvailableSemaphore = device.createSemaphore(semaphoreInfo);
    // Give name to the semaphore
    if (debugUtils != nullptr)
    {
        vk::DebugUtilsObjectNameInfoEXT nameInfo{};
        nameInfo.objectHandle = (u64)(VkSemaphore)swapchain.imageAvailableSemaphore;
        nameInfo.objectType = swapchain.imageAvailableSemaphore.objectType;
        std::string objectName = std::string("Swapchain #0 - Copy done semaphore");
        nameInfo.pObjectName = objectName.data();
        debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
    }

    return swapchain;
}

void DEngine::Gfx::Vk::Init::RecreateSwapchain(
    InstanceDispatch const& instance,
    DevDispatch const& device,
    DebugUtilsDispatch const* debugUtils,
    vk::Queue queue,
    vk::PhysicalDevice physDevice,
    SurfaceInfo& surface,
    SwapchainData& swapchain)
{
    // Query surface for new details
    vk::Result vkResult{};

    vk::SurfaceCapabilitiesKHR capabilities = instance.getPhysicalDeviceSurfaceCapabilitiesKHR(physDevice, surface.handle);
    surface.capabilities = capabilities;

    SwapchainSettings settings = {};
    settings.surface = surface.handle;
    settings.extents = surface.capabilities.currentExtent;
    settings.numImages = static_cast<std::uint32_t>(swapchain.images.size());
    settings.presentMode = swapchain.presentMode;
    settings.surfaceFormat = swapchain.surfaceFormat;
    settings.capabilities = surface.capabilities;
    settings.transform = surface.capabilities.currentTransform;
    settings.compositeAlphaFlag = surface.compositeAlphaFlag;

    // We have figured out the settings to build the new swapchainData, now we actually make it

    swapchain.extents = settings.extents;
    swapchain.surfaceFormat = settings.surfaceFormat;
    swapchain.presentMode = settings.presentMode;

    vk::SwapchainCreateInfoKHR swapchainCreateInfo{};
    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.imageExtent = settings.extents;
    swapchainCreateInfo.imageFormat = settings.surfaceFormat.format;
    swapchainCreateInfo.imageColorSpace = settings.surfaceFormat.colorSpace;
    swapchainCreateInfo.imageSharingMode = vk::SharingMode::eExclusive;
    swapchainCreateInfo.presentMode = settings.presentMode;
    swapchainCreateInfo.surface = settings.surface;
    swapchainCreateInfo.preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
    swapchainCreateInfo.clipped = 1;
    swapchainCreateInfo.compositeAlpha = settings.compositeAlphaFlag;
    swapchainCreateInfo.imageUsage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eColorAttachment;
    swapchainCreateInfo.minImageCount = settings.numImages;
    swapchainCreateInfo.oldSwapchain = swapchain.handle;

    swapchain.handle = device.createSwapchainKHR(swapchainCreateInfo);
    // Make name for the swapchainData
    if (debugUtils != nullptr)
    {
        vk::DebugUtilsObjectNameInfoEXT nameInfo{};
        nameInfo.objectHandle = (u64)(VkSwapchainKHR)swapchain.handle;
        nameInfo.objectType = swapchain.handle.objectType;
        std::string objectName = std::string("Swapchain #0");
        nameInfo.pObjectName = objectName.data();
        debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
    }

    u32 swapchainImageCount = 0;
    vkResult = device.getSwapchainImagesKHR(swapchain.handle, &swapchainImageCount, nullptr);
    if (vkResult != vk::Result::eSuccess && vkResult != vk::Result::eIncomplete)
        throw std::runtime_error("Unable to grab swapchainData images from VkSwapchainKHR object.");
    device.getSwapchainImagesKHR(swapchain.handle, &swapchainImageCount, swapchain.images.data());
    // Make names for the swapchainData images
    if (debugUtils != nullptr)
    {
        for (uSize i = 0; i < swapchain.images.size(); i++)
        {
            vk::DebugUtilsObjectNameInfoEXT nameInfo{};
            nameInfo.objectHandle = (u64)(VkImage)swapchain.images[i];
            nameInfo.objectType = swapchain.images[i].objectType;
            std::string objectName = std::string("Swapchain #0") + "- Image #0" + std::to_string(i);
            nameInfo.pObjectName = objectName.data();
            debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
        }
    }

    // Transition new swapchainData images
    Init::TransitionSwapchainImages(device, queue, swapchain.images);

    // Command cmdBuffers are not resettable, we deallocate the old ones and allocate new ones
    device.freeCommandBuffers(swapchain.cmdPool, { static_cast<std::uint32_t>(swapchain.cmdBuffers.size()), swapchain.cmdBuffers.data() });

    vk::CommandBufferAllocateInfo cmdBufferAllocInfo{};
    cmdBufferAllocInfo.commandPool = swapchain.cmdPool;
    cmdBufferAllocInfo.commandBufferCount = (u32)swapchain.images.size();
    cmdBufferAllocInfo.level = vk::CommandBufferLevel::ePrimary;
    vkResult = device.allocateCommandBuffers(&cmdBufferAllocInfo, swapchain.cmdBuffers.data());
    if (vkResult != vk::Result::eSuccess)
        throw std::runtime_error("Vulkan: Failed to allocate VkCommandBuffers for copy-to-swapchainData.");
    // Give names to swapchainData command cmdBuffers
    if (debugUtils != nullptr)
    {
        for (uSize i = 0; i < swapchain.cmdBuffers.size(); i++)
        {
            vk::DebugUtilsObjectNameInfoEXT nameInfo{};
            nameInfo.objectHandle = (u64)(VkCommandBuffer)swapchain.cmdBuffers[i];
            nameInfo.objectType = swapchain.cmdBuffers[i].objectType;
            std::string objectName = std::string("Swapchain #0 - Copy CmdBuffer #") + std::to_string(i);
            nameInfo.pObjectName = objectName.data();
            debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
        }
    }
}

bool DEngine::Gfx::Vk::Init::TransitionSwapchainImages(
    DeviceDispatch const& device,
    vk::Queue vkQueue,
    Cont::Span<const vk::Image> images)
{
    vk::Result vkResult{};

    vk::CommandPoolCreateInfo cmdPoolInfo{};
    vk::CommandPool cmdPool = device.createCommandPool(cmdPoolInfo);

    vk::CommandBufferAllocateInfo cmdBufferAllocInfo{};
    cmdBufferAllocInfo.commandBufferCount = 1;
    cmdBufferAllocInfo.commandPool = cmdPool;
    cmdBufferAllocInfo.level = vk::CommandBufferLevel::ePrimary;
    vk::CommandBuffer cmdBuffer{};
    vkResult = device.allocateCommandBuffers(&cmdBufferAllocInfo, &cmdBuffer);
    if (vkResult != vk::Result::eSuccess)
        throw std::runtime_error("Vulkan: Unable to allocate command buffer when transitioning swapchainData images.");

    // Record commandbuffer
    {
        vk::CommandBufferBeginInfo cmdBeginInfo{};
        cmdBeginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
        device.beginCommandBuffer(cmdBuffer, cmdBeginInfo);

        for (size_t i = 0; i < images.size(); i++)
        {
            vk::ImageMemoryBarrier imgBarrier{};
            imgBarrier.image = images[i];
            imgBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
            imgBarrier.subresourceRange.layerCount = 1;
            imgBarrier.subresourceRange.levelCount = 1;
            imgBarrier.oldLayout = vk::ImageLayout::eUndefined;
            imgBarrier.newLayout = vk::ImageLayout::ePresentSrcKHR;
            imgBarrier.srcAccessMask = {};
            // We are going to be transferring onto the swapchain images.
            imgBarrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

            device.cmdPipelineBarrier(
                cmdBuffer,
                vk::PipelineStageFlagBits::eTopOfPipe,
                vk::PipelineStageFlagBits::eTransfer,
                vk::DependencyFlags{},
                0, nullptr,
                0, nullptr,
                1, &imgBarrier);
        }

        device.endCommandBuffer(cmdBuffer);
    }

    vk::FenceCreateInfo fenceInfo{};
    vk::Fence fence = device.createFence(fenceInfo);

    vk::SubmitInfo submitInfo{};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdBuffer;
    device.queueSubmit(vkQueue, { submitInfo }, fence);

    vkResult = device.waitForFences({ fence }, true, std::numeric_limits<uint64_t>::max());
    DENGINE_GFX_ASSERT(vkResult == vk::Result::eSuccess);

    device.destroyFence(fence);
    device.destroyCommandPool(cmdPool);

    return true;
}

DEngine::Gfx::Vk::GUIRenderTarget DEngine::Gfx::Vk::Init::CreateGUIRenderTarget(
    DeviceDispatch const& device,
    vk::Queue vkQueue,
    vk::Extent2D swapchainDimensions,
    vk::Format swapchainFormat,
    vk::RenderPass renderPass,
    DebugUtilsDispatch const* debugUtils)
{
    vk::Result vkResult{};

    GUIRenderTarget returnVal{};
    returnVal.format = swapchainFormat;
    returnVal.extent = swapchainDimensions;

    // Allocate the rendertarget
    {
        vk::ImageCreateInfo imgInfo{};
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

        returnVal.img = device.createImage(imgInfo);
        if (debugUtils != nullptr)
        {
            vk::DebugUtilsObjectNameInfoEXT nameInfo{};
            nameInfo.objectHandle = (u64)(VkImage)returnVal.img;
            nameInfo.objectType = returnVal.img.objectType;
            nameInfo.pObjectName = "GUI Image";
            debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
        }

        vk::MemoryRequirements memReqs = device.getImageMemoryRequirements(returnVal.img);
        vk::MemoryAllocateInfo memAllocInfo{};
        memAllocInfo.allocationSize = memReqs.size;
        memAllocInfo.memoryTypeIndex = 0;
        returnVal.memory = device.allocateMemory(memAllocInfo);
        returnVal.memorySize = memAllocInfo.allocationSize;
        if (debugUtils != nullptr)
        {
            vk::DebugUtilsObjectNameInfoEXT nameInfo{};
            nameInfo.objectHandle = (u64)(VkDeviceMemory)returnVal.memory;
            nameInfo.objectType = returnVal.memory.objectType;
            nameInfo.pObjectName = "GUI ImageMemory";
            debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
        }

        device.bindImageMemory(returnVal.img, returnVal.memory, 0);

        vk::ImageViewCreateInfo imgViewInfo{};
        imgViewInfo.format = swapchainFormat;
        imgViewInfo.image = returnVal.img;
        imgViewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        imgViewInfo.subresourceRange.baseArrayLayer = 0;
        imgViewInfo.subresourceRange.baseMipLevel = 0;
        imgViewInfo.subresourceRange.layerCount = 1;
        imgViewInfo.subresourceRange.levelCount = 1;
        imgViewInfo.viewType = vk::ImageViewType::e2D;

        returnVal.imgView = device.createImageView(imgViewInfo);

        if (debugUtils != nullptr)
        {
            vk::DebugUtilsObjectNameInfoEXT nameInfo{};
            nameInfo.objectHandle = (u64)(VkImageView)returnVal.imgView;
            nameInfo.objectType = returnVal.imgView.objectType;
            nameInfo.pObjectName = "GUI ImageView";
            debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
        }
    }

    vk::FramebufferCreateInfo fbInfo{};
    fbInfo.attachmentCount = 1;
    fbInfo.width = swapchainDimensions.width;
    fbInfo.height = swapchainDimensions.height;
    fbInfo.layers = 1;
    fbInfo.pAttachments = &returnVal.imgView;
    fbInfo.renderPass = renderPass;
    returnVal.framebuffer = device.createFramebuffer(fbInfo);
    if (debugUtils != nullptr)
    {
        vk::DebugUtilsObjectNameInfoEXT nameInfo{};
        nameInfo.objectHandle = (u64)(VkFramebuffer)returnVal.framebuffer;
        nameInfo.objectType = returnVal.framebuffer.objectType;
        nameInfo.pObjectName = "GUI Framebuffer";
        debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
    }


    // Transition the image
    vk::CommandPoolCreateInfo cmdPoolInfo{};
    cmdPoolInfo.queueFamilyIndex = 0;
    vk::CommandPool cmdPool = device.createCommandPool(cmdPoolInfo);
    vk::CommandBufferAllocateInfo cmdBufferAllocInfo{};
    cmdBufferAllocInfo.commandPool = cmdPool;
    cmdBufferAllocInfo.level = vk::CommandBufferLevel::ePrimary;
    cmdBufferAllocInfo.commandBufferCount = 1;
    vk::CommandBuffer cmdBuffer{};
    vkResult = device.allocateCommandBuffers(&cmdBufferAllocInfo, &cmdBuffer);
    DENGINE_GFX_ASSERT(vkResult == vk::Result::eSuccess);

    vk::CommandBufferBeginInfo cmdBufferBeginInfo{};
    cmdBufferBeginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
    device.beginCommandBuffer(cmdBuffer, cmdBufferBeginInfo);

    vk::ImageMemoryBarrier imgMemoryBarrier{};
    imgMemoryBarrier.image = returnVal.img;
    imgMemoryBarrier.oldLayout = vk::ImageLayout::eUndefined;
    imgMemoryBarrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
    imgMemoryBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    imgMemoryBarrier.subresourceRange.layerCount = 1;
    imgMemoryBarrier.subresourceRange.levelCount = 1;
    imgMemoryBarrier.srcAccessMask = vk::AccessFlagBits{};
    imgMemoryBarrier.dstAccessMask = vk::AccessFlagBits{};

    device.cmdPipelineBarrier(
        cmdBuffer,
        vk::PipelineStageFlagBits::eTopOfPipe,
        vk::PipelineStageFlagBits::eBottomOfPipe,
        vk::DependencyFlags{},
        {},
        {},
        { imgMemoryBarrier });

    device.endCommandBuffer(cmdBuffer);

    vk::FenceCreateInfo fenceInfo{};
    vk::Fence tempFence = device.createFence(fenceInfo);

    vk::SubmitInfo submitInfo{};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdBuffer;
    device.queueSubmit(vkQueue, { submitInfo }, tempFence);

    vkResult = device.waitForFences({ tempFence }, true, std::numeric_limits<uint64_t>::max());
    if (vkResult != vk::Result::eSuccess)
        throw std::runtime_error("Vulkan: Could not wait for fences after transitioning GUI RenderTarget Image");

    device.destroyFence(tempFence, nullptr);
    device.destroyCommandPool(cmdPool, nullptr);

    return returnVal;
}

void DEngine::Gfx::Vk::Init::RecreateGUIRenderTarget(
    DevDispatch const& device,
    vk::Extent2D swapchainDimensions,
    vk::Queue vkQueue,
    vk::RenderPass renderPass,
    GUIRenderTarget& renderTarget,
    DebugUtilsDispatch const* debugUtils)
{
    vk::Result vkResult{};

    renderTarget.extent = swapchainDimensions;

    device.destroyImageView(renderTarget.imgView);
    device.destroyImage(renderTarget.img);

    // Allocate the new rendertarget
    {
        vk::ImageCreateInfo imgInfo{};
        imgInfo.arrayLayers = 1;
        imgInfo.extent = vk::Extent3D{ swapchainDimensions.width, swapchainDimensions.height, 1 };
        imgInfo.flags = vk::ImageCreateFlagBits{};
        imgInfo.format = renderTarget.format;
        imgInfo.imageType = vk::ImageType::e2D;
        imgInfo.initialLayout = vk::ImageLayout::eUndefined;
        imgInfo.mipLevels = 1;
        imgInfo.samples = vk::SampleCountFlagBits::e1;
        imgInfo.sharingMode = vk::SharingMode::eExclusive;
        imgInfo.tiling = vk::ImageTiling::eOptimal;
        imgInfo.usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc;

        renderTarget.img = device.createImage(imgInfo);
        if (debugUtils != nullptr)
        {
            vk::DebugUtilsObjectNameInfoEXT nameInfo{};
            nameInfo.objectHandle = (u64)(VkImage)renderTarget.img;
            nameInfo.objectType = renderTarget.img.objectType;
            nameInfo.pObjectName = "GUI Image";
            debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
        }

        vk::MemoryRequirements memReqs = device.getImageMemoryRequirements(renderTarget.img);
        if (memReqs.size > renderTarget.memorySize)
        {
            vk::MemoryAllocateInfo memAllocInfo{};
            memAllocInfo.allocationSize = memReqs.size;
            memAllocInfo.memoryTypeIndex = 0;
            renderTarget.memory = device.allocateMemory(memAllocInfo);
            if (debugUtils != nullptr)
            {
                vk::DebugUtilsObjectNameInfoEXT nameInfo{};
                nameInfo.objectHandle = (u64)(VkDeviceMemory)renderTarget.memory;
                nameInfo.objectType = renderTarget.memory.objectType;
                nameInfo.pObjectName = "GUI ImageMemory";
                debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
            }
        }

        device.bindImageMemory(renderTarget.img, renderTarget.memory, 0);

        vk::ImageViewCreateInfo imgViewInfo{};
        imgViewInfo.format = renderTarget.format;
        imgViewInfo.image = renderTarget.img;
        imgViewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        imgViewInfo.subresourceRange.baseArrayLayer = 0;
        imgViewInfo.subresourceRange.baseMipLevel = 0;
        imgViewInfo.subresourceRange.layerCount = 1;
        imgViewInfo.subresourceRange.levelCount = 1;
        imgViewInfo.viewType = vk::ImageViewType::e2D;

        renderTarget.imgView = device.createImageView(imgViewInfo);
        if (debugUtils != nullptr)
        {
            vk::DebugUtilsObjectNameInfoEXT nameInfo{};
            nameInfo.objectHandle = (u64)(VkImageView)renderTarget.imgView;
            nameInfo.objectType = renderTarget.imgView.objectType;
            nameInfo.pObjectName = "GUI ImageView";
            debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
        }
    }

    vk::FramebufferCreateInfo fbInfo{};
    fbInfo.attachmentCount = 1;
    fbInfo.width = swapchainDimensions.width;
    fbInfo.height = swapchainDimensions.height;
    fbInfo.layers = 1;
    fbInfo.pAttachments = &renderTarget.imgView;
    fbInfo.renderPass = renderPass;
    renderTarget.framebuffer = device.createFramebuffer(fbInfo);
    if (debugUtils != nullptr)
    {
        vk::DebugUtilsObjectNameInfoEXT nameInfo{};
        nameInfo.objectHandle = (u64)(VkFramebuffer)renderTarget.framebuffer;
        nameInfo.objectType = renderTarget.framebuffer.objectType;
        nameInfo.pObjectName = "GUI Framebuffer";
        debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
    }


    // Transition the image
    {
        vk::CommandPoolCreateInfo cmdPoolInfo{};
        cmdPoolInfo.queueFamilyIndex = 0;
        vk::CommandPool cmdPool = device.createCommandPool(cmdPoolInfo);
        vk::CommandBufferAllocateInfo cmdBufferAllocInfo{};
        cmdBufferAllocInfo.commandPool = cmdPool;
        cmdBufferAllocInfo.level = vk::CommandBufferLevel::ePrimary;
        cmdBufferAllocInfo.commandBufferCount = 1;
        vk::CommandBuffer cmdBuffer{};
        vkResult = device.allocateCommandBuffers(&cmdBufferAllocInfo, &cmdBuffer);
        if (vkResult != vk::Result::eSuccess)
            throw std::runtime_error("Vulkan: Unable to allocate command cmdBuffers when transitioning GUI image.");

        vk::CommandBufferBeginInfo cmdBufferBeginInfo{};
        cmdBufferBeginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
        device.beginCommandBuffer(cmdBuffer, cmdBufferBeginInfo);

        vk::ImageMemoryBarrier imgMemoryBarrier{};
        imgMemoryBarrier.image = renderTarget.img;
        imgMemoryBarrier.oldLayout = vk::ImageLayout::eUndefined;
        imgMemoryBarrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
        imgMemoryBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        imgMemoryBarrier.subresourceRange.layerCount = 1;
        imgMemoryBarrier.subresourceRange.levelCount = 1;
        imgMemoryBarrier.srcAccessMask = vk::AccessFlagBits{};
        imgMemoryBarrier.dstAccessMask = vk::AccessFlagBits{};

        device.cmdPipelineBarrier(
            cmdBuffer,
            vk::PipelineStageFlagBits::eTopOfPipe,
            vk::PipelineStageFlagBits::eBottomOfPipe,
            vk::DependencyFlags{},
            {},
            {},
            { imgMemoryBarrier });

        device.endCommandBuffer(cmdBuffer);

        vk::FenceCreateInfo fenceInfo{};
        vk::Fence tempFence = device.createFence(fenceInfo);

        vk::SubmitInfo submitInfo{};
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmdBuffer;
        device.queueSubmit(vkQueue, { submitInfo }, tempFence);

        vkResult = device.waitForFences({ tempFence }, true, std::numeric_limits<uint64_t>::max());
        if (vkResult != vk::Result::eSuccess)
            throw std::runtime_error("Vulkan: Unable to wait for fence after transitioning GUI image.");

        device.destroyFence(tempFence, nullptr);
        device.destroyCommandPool(cmdPool, nullptr);
    }
}

DEngine::Gfx::Vk::GUIData DEngine::Gfx::Vk::Init::CreateGUIData(
    DeviceDispatch const& device,
    vk::Format swapchainFormat,
    u8 resourceSetCount,
    vk::Extent2D swapchainDimensions,
    vk::Queue vkQueue,
    DebugUtilsDispatch const* debugUtils)
{
    vk::Result vkResult{};
    GUIData returnVal{};

    // Create the renderpass
    {
        vk::AttachmentDescription colorAttachment{};
        colorAttachment.initialLayout = vk::ImageLayout::eTransferSrcOptimal;
        colorAttachment.finalLayout = vk::ImageLayout::eTransferSrcOptimal;
        colorAttachment.format = swapchainFormat;
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

        returnVal.renderPass = device.createRenderPass(createInfo);
        if (debugUtils != nullptr)
        {
            vk::DebugUtilsObjectNameInfoEXT nameInfo{};
            nameInfo.objectHandle = (u64)(VkRenderPass)returnVal.renderPass;
            nameInfo.objectType = returnVal.renderPass.objectType;
            nameInfo.pObjectName = "GUI RenderPass";
            debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
        }
    }

    // Create the commandbuffers
    {
        vk::CommandPoolCreateInfo cmdPoolInfo{};
        cmdPoolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
        returnVal.cmdPool = device.createCommandPool(cmdPoolInfo);
        if (debugUtils != nullptr)
        {
            vk::DebugUtilsObjectNameInfoEXT nameInfo{};
            nameInfo.objectHandle = (u64)(VkCommandPool)returnVal.cmdPool;
            nameInfo.objectType = returnVal.cmdPool.objectType;
            nameInfo.pObjectName = "GUI CmdPool";
            debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
        }

        vk::CommandBufferAllocateInfo cmdBufferAllocInfo{};
        cmdBufferAllocInfo.commandPool = returnVal.cmdPool;
        cmdBufferAllocInfo.level = vk::CommandBufferLevel::ePrimary;
        cmdBufferAllocInfo.commandBufferCount = resourceSetCount;
        returnVal.cmdBuffers.resize(cmdBufferAllocInfo.commandBufferCount);
        vkResult = device.allocateCommandBuffers(&cmdBufferAllocInfo, returnVal.cmdBuffers.data());
        if (vkResult != vk::Result::eSuccess)
            throw std::runtime_error("Vulkan: Unable to allocate command cmdBuffers for GUI rendering.");
    }

    returnVal.renderTarget = CreateGUIRenderTarget(device, vkQueue, swapchainDimensions, swapchainFormat, returnVal.renderPass, debugUtils);

    return returnVal;
}

void DEngine::Gfx::Vk::Init::RecordSwapchainCmdBuffers(
    DeviceDispatch const& device,
    SwapchainData const& swapchainData,
    vk::Image srcImage)
{
    vk::Result vkResult{};

    for (uSize i = 0; i < swapchainData.cmdBuffers.size(); i++)
    {
        vk::CommandBuffer cmdBuffer = swapchainData.cmdBuffers[i];
        vk::Image dstImage = swapchainData.images[i];

        vk::CommandBufferBeginInfo beginInfo{};
        beginInfo.flags = vk::CommandBufferUsageFlagBits::eSimultaneousUse;
        device.beginCommandBuffer(cmdBuffer, beginInfo);

        // The renderpass handles transitioning the rendered image into transfer-src-optimal
        vk::ImageMemoryBarrier preTransfer_GuiBarrier{};
        preTransfer_GuiBarrier.image = srcImage;
        preTransfer_GuiBarrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
        preTransfer_GuiBarrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
        preTransfer_GuiBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        preTransfer_GuiBarrier.subresourceRange.layerCount = 1;
        preTransfer_GuiBarrier.subresourceRange.levelCount = 1;
        preTransfer_GuiBarrier.srcAccessMask = {};
        preTransfer_GuiBarrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;
        device.cmdPipelineBarrier(
            cmdBuffer,
            vk::PipelineStageFlagBits::eColorAttachmentOutput,
            vk::PipelineStageFlagBits::eTransfer,
            vk::DependencyFlags{},
            {},
            {},
            { preTransfer_GuiBarrier });

        vk::ImageMemoryBarrier preTransfer_SwapchainBarrier{};
        preTransfer_SwapchainBarrier.image = dstImage;
        preTransfer_SwapchainBarrier.oldLayout = vk::ImageLayout::ePresentSrcKHR;
        preTransfer_SwapchainBarrier.newLayout = vk::ImageLayout::eTransferDstOptimal;
        preTransfer_SwapchainBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        preTransfer_SwapchainBarrier.subresourceRange.layerCount = 1;
        preTransfer_SwapchainBarrier.subresourceRange.levelCount = 1;
        preTransfer_SwapchainBarrier.srcAccessMask = {};
        preTransfer_SwapchainBarrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
        device.cmdPipelineBarrier(
            cmdBuffer,
            vk::PipelineStageFlagBits::eColorAttachmentOutput,
            vk::PipelineStageFlagBits::eTransfer,
            vk::DependencyFlags{},
            {},
            {},
            { preTransfer_SwapchainBarrier });

        vk::ImageCopy copyRegion{};
        copyRegion.extent = vk::Extent3D{ swapchainData.extents.width, swapchainData.extents.height, 1 };
        copyRegion.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        copyRegion.srcSubresource.layerCount = 1;
        copyRegion.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        copyRegion.dstSubresource.layerCount = 1;
        device.cmdCopyImage(
            cmdBuffer,
            srcImage,
            vk::ImageLayout::eTransferSrcOptimal,
            dstImage,
            vk::ImageLayout::eTransferDstOptimal,
            { copyRegion });

        vk::ImageMemoryBarrier postTransfer_SwapchainBarrier{};
        postTransfer_SwapchainBarrier.image = dstImage;
        postTransfer_SwapchainBarrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
        postTransfer_SwapchainBarrier.newLayout = vk::ImageLayout::ePresentSrcKHR;
        postTransfer_SwapchainBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        postTransfer_SwapchainBarrier.subresourceRange.layerCount = 1;
        postTransfer_SwapchainBarrier.subresourceRange.levelCount = 1;
        postTransfer_SwapchainBarrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
        postTransfer_SwapchainBarrier.dstAccessMask = vk::AccessFlags{};
        device.cmdPipelineBarrier(
            cmdBuffer,
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eColorAttachmentOutput,
            vk::DependencyFlags{},
            {},
            {},
            { postTransfer_SwapchainBarrier });

        // RenderPass handles transitioning rendered image back to color attachment output

        device.endCommandBuffer(cmdBuffer);
    }
}

vk::RenderPass DEngine::Gfx::Vk::Init::BuildMainGfxRenderPass(
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

void DEngine::Gfx::Vk::Init::TransitionGfxImage(
    DeviceDispatch const& device,
    vk::Image img,
    vk::Queue queue,
    bool useEditorPipeline
)
{
    vk::Result vkResult{};

    vk::CommandPoolCreateInfo cmdPoolInfo{};
    vk::CommandPool cmdPool = device.createCommandPool(cmdPoolInfo);

    vk::CommandBufferAllocateInfo cmdBufferAllocInfo{};
    cmdBufferAllocInfo.commandBufferCount = 1;
    cmdBufferAllocInfo.commandPool = cmdPool;
    cmdBufferAllocInfo.level = vk::CommandBufferLevel::ePrimary;
    vk::CommandBuffer cmdBuffer{};
    vkResult = device.allocateCommandBuffers(&cmdBufferAllocInfo, &cmdBuffer);
    if (vkResult != vk::Result::eSuccess)
        throw std::runtime_error("Unable to allocate command buffer when transitioning swapchainData images.");

    // Record commandbuffer
    {
        vk::CommandBufferBeginInfo cmdBeginInfo{};
        cmdBeginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
        device.beginCommandBuffer(cmdBuffer, cmdBeginInfo);
        vk::ImageMemoryBarrier imgBarrier{};
        imgBarrier.image = img;
        imgBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        imgBarrier.subresourceRange.layerCount = 1;
        imgBarrier.subresourceRange.levelCount = 1;
        imgBarrier.oldLayout = vk::ImageLayout::eUndefined;
        if (useEditorPipeline)
            imgBarrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        else
            imgBarrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
        imgBarrier.srcAccessMask = {};
        // We want to write to the image as a color-attachment
        imgBarrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

        vk::PipelineStageFlags srcStage = vk::PipelineStageFlagBits::eTopOfPipe;
        vk::PipelineStageFlags dstStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;

        device.cmdPipelineBarrier(
            cmdBuffer,
            srcStage,
            dstStage,
            vk::DependencyFlags{},
            0, nullptr,
            0, nullptr,
            1, &imgBarrier);

        device.endCommandBuffer(cmdBuffer);
    }

    vk::FenceCreateInfo fenceInfo{};
    vk::Fence fence = device.createFence(fenceInfo);

    vk::SubmitInfo submitInfo{};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdBuffer;
    device.queueSubmit(queue, { submitInfo }, fence);

    vkResult = device.waitForFences({ fence }, true, std::numeric_limits<uint64_t>::max());
    if (vkResult != vk::Result::eSuccess)
        throw std::runtime_error("Could not wait for fence after submitting command buffer to transition graphics render-target.");

    device.destroyFence(fence);
    device.destroyCommandPool(cmdPool);
}

DEngine::Gfx::Vk::GfxRenderTarget DEngine::Gfx::Vk::Init::InitializeGfxViewport(
    DeviceDispatch const& device,
    u8 viewportID,
    u32 deviceLocalMemType,
    vk::Extent2D viewportSize,
    vk::RenderPass renderPass,
    vk::Queue queue,
    bool useEditorPipeline,
    DebugUtilsDispatch const* debugUtils)
{
    vk::Result vkResult{};

    GfxRenderTarget returnVal{};
    returnVal.extent = viewportSize;

    // First we make a temp image that has max size, so we won't have to re-allocate memory later when resizing this image.
    vk::ImageCreateInfo imageInfo{};
    imageInfo.arrayLayers = 1;
    imageInfo.extent = vk::Extent3D{ viewportSize.width, viewportSize.height, 1 };
    imageInfo.format = vk::Format::eR8G8B8A8Srgb;
    imageInfo.imageType = vk::ImageType::e2D;
    imageInfo.initialLayout = vk::ImageLayout::eUndefined;
    imageInfo.mipLevels = 1;
    imageInfo.samples = vk::SampleCountFlagBits::e1;
    imageInfo.sharingMode = vk::SharingMode::eExclusive;
    imageInfo.tiling = vk::ImageTiling::eOptimal;
    imageInfo.usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc;
    if (useEditorPipeline)
    {
        // We want to sample from the image to show it in the editor.
        imageInfo.usage |= vk::ImageUsageFlagBits::eSampled;
    }

    returnVal.img = device.createImage(imageInfo);
    if (debugUtils)
    {
        vk::DebugUtilsObjectNameInfoEXT nameInfo{};
        nameInfo.objectHandle = (u64)(VkImage)returnVal.img;
        nameInfo.objectType = returnVal.img.objectType;
        std::string name = std::string("Graphics viewport #") + std::to_string(viewportID) + " - Image";
        nameInfo.pObjectName = name.data();
        debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
    }


    vk::MemoryRequirements memReqs = device.getImageMemoryRequirements(returnVal.img);

    vk::MemoryAllocateInfo allocInfo{};
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = deviceLocalMemType;

    vk::DeviceMemory memory = device.allocateMemory(allocInfo);
    returnVal.memory = memory;
    returnVal.memorySize = allocInfo.allocationSize;
    if (debugUtils)
    {
        vk::DebugUtilsObjectNameInfoEXT nameInfo{};
        nameInfo.objectHandle = (u64)(VkDeviceMemory)returnVal.memory;
        nameInfo.objectType = returnVal.memory.objectType;
        std::string name = std::string("Graphics viewport #") + std::to_string(viewportID) + " - Memory";
        nameInfo.pObjectName = name.data();
        debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
    }

    device.bindImageMemory(returnVal.img, memory, 0);

    // We have to transition this image
    TransitionGfxImage(device, returnVal.img, queue, useEditorPipeline);

    // Make the image view
    vk::ImageViewCreateInfo imgViewInfo{};
    imgViewInfo.components.r = vk::ComponentSwizzle::eIdentity;
    imgViewInfo.components.g = vk::ComponentSwizzle::eIdentity;
    imgViewInfo.components.b = vk::ComponentSwizzle::eIdentity;
    imgViewInfo.components.a = vk::ComponentSwizzle::eIdentity;
    imgViewInfo.format = vk::Format::eR8G8B8A8Srgb;
    imgViewInfo.image = returnVal.img;
    imgViewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    imgViewInfo.subresourceRange.baseArrayLayer = 0;
    imgViewInfo.subresourceRange.baseMipLevel = 0;
    imgViewInfo.subresourceRange.layerCount = 1;
    imgViewInfo.subresourceRange.levelCount = 1;
    imgViewInfo.viewType = vk::ImageViewType::e2D;

    returnVal.imgView = device.createImageView(imgViewInfo);
    if (debugUtils)
    {
        vk::DebugUtilsObjectNameInfoEXT nameInfo{};
        nameInfo.objectHandle = (u64)(VkImageView)returnVal.imgView;
        nameInfo.objectType = returnVal.imgView.objectType;
        std::string name = std::string("Graphics viewport #") + std::to_string(viewportID) + " - Image View";
        nameInfo.pObjectName = name.data();
        debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
    }

    vk::FramebufferCreateInfo fbInfo{};
    fbInfo.attachmentCount = 1;
    fbInfo.pAttachments = &returnVal.imgView;
    fbInfo.height = viewportSize.height;
    fbInfo.layers = 1;
    fbInfo.renderPass = renderPass;
    fbInfo.width = viewportSize.width;
    returnVal.framebuffer = device.createFramebuffer(fbInfo);
    if (debugUtils)
    {
        vk::DebugUtilsObjectNameInfoEXT nameInfo{};
        nameInfo.objectHandle = (u64)(VkFramebuffer)returnVal.framebuffer;
        nameInfo.objectType = returnVal.framebuffer.objectType;
        std::string name = std::string("Graphics viewport #") + std::to_string(viewportID) + " - Framebuffer";
        nameInfo.pObjectName = name.data();
        debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
    }

    return returnVal;
}

void DEngine::Gfx::Vk::Init::ResizeGfxViewport(
    DevDispatch const& device,
    vk::Queue queue,
    vk::RenderPass renderPass,
    u8 viewportID,
    void* imguiTextureID,
    vk::Extent2D newSize,
    bool useEditorPipeline,
    GfxRenderTarget& viewportRef,
    DebugUtilsDispatch const* debugUtils)
{
    if (newSize == viewportRef.extent)
        return;

    viewportRef.extent = newSize;

    device.destroyFramebuffer(viewportRef.framebuffer);
    device.destroyImageView(viewportRef.imgView);
    device.destroyImage(viewportRef.img);

    vk::ImageCreateInfo imageInfo{};
    imageInfo.arrayLayers = 1;
    imageInfo.extent = vk::Extent3D{ newSize.width, newSize.height, 1 };
    imageInfo.format = vk::Format::eR8G8B8A8Srgb;
    imageInfo.imageType = vk::ImageType::e2D;
    imageInfo.initialLayout = vk::ImageLayout::eUndefined;
    imageInfo.mipLevels = 1;
    imageInfo.samples = vk::SampleCountFlagBits::e1;
    imageInfo.sharingMode = vk::SharingMode::eExclusive;
    imageInfo.tiling = vk::ImageTiling::eOptimal;
    imageInfo.usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc;
    if (useEditorPipeline)
    {
        // We want to sample from the image to show it in the editor.
        imageInfo.usage |= vk::ImageUsageFlagBits::eSampled;
    }

    viewportRef.img = device.createImage(imageInfo);
    if (debugUtils)
    {
        vk::DebugUtilsObjectNameInfoEXT nameInfo{};
        nameInfo.objectHandle = (u64)(VkImage)viewportRef.img;
        nameInfo.objectType = viewportRef.img.objectType;
        std::string name = std::string("Graphics viewport #") + std::to_string(viewportID) + " - Image";
        nameInfo.pObjectName = name.data();
        debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
    }

    vk::MemoryRequirements memReqs = device.getImageMemoryRequirements(viewportRef.img);
    // Re-allocate if we need a larger buffer
    if (memReqs.size > viewportRef.memorySize)
    {
        device.freeMemory(viewportRef.memory);

        vk::MemoryAllocateInfo allocInfo{};
        allocInfo.allocationSize = memReqs.size;
        allocInfo.memoryTypeIndex = viewportRef.memoryTypeIndex;

        viewportRef.memory = device.allocateMemory(allocInfo);
        viewportRef.memorySize = memReqs.size;
    }

    device.bindImageMemory(viewportRef.img, viewportRef.memory, 0);

    TransitionGfxImage(device, viewportRef.img, queue, useEditorPipeline);

    vk::ImageViewCreateInfo imgViewInfo{};
    imgViewInfo.components.r = vk::ComponentSwizzle::eIdentity;
    imgViewInfo.components.g = vk::ComponentSwizzle::eIdentity;
    imgViewInfo.components.b = vk::ComponentSwizzle::eIdentity;
    imgViewInfo.components.a = vk::ComponentSwizzle::eIdentity;
    imgViewInfo.format = vk::Format::eR8G8B8A8Srgb;
    imgViewInfo.image = viewportRef.img;
    imgViewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    imgViewInfo.subresourceRange.baseArrayLayer = 0;
    imgViewInfo.subresourceRange.baseMipLevel = 0;
    imgViewInfo.subresourceRange.layerCount = 1;
    imgViewInfo.subresourceRange.levelCount = 1;
    imgViewInfo.viewType = vk::ImageViewType::e2D;

    viewportRef.imgView = device.createImageView(imgViewInfo);
    if (debugUtils)
    {
        vk::DebugUtilsObjectNameInfoEXT nameInfo{};
        nameInfo.objectHandle = (u64)(VkImageView)viewportRef.imgView;
        nameInfo.objectType = viewportRef.imgView.objectType;
        std::string name = std::string("Graphics viewport #") + std::to_string(viewportID) + " - Image View";
        nameInfo.pObjectName = name.data();
        debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
    }

    if (useEditorPipeline)
        ImGui_ImplVulkan_OverwriteTexture(imguiTextureID, (VkImageView)viewportRef.imgView, (VkImageLayout)vk::ImageLayout::eShaderReadOnlyOptimal);

    vk::FramebufferCreateInfo fbInfo{};
    fbInfo.attachmentCount = 1;
    fbInfo.pAttachments = &viewportRef.imgView;
    fbInfo.height = newSize.height;
    fbInfo.layers = 1;
    fbInfo.renderPass = renderPass;
    fbInfo.width = newSize.width;
    viewportRef.framebuffer = device.createFramebuffer(fbInfo);
    if (debugUtils)
    {
        vk::DebugUtilsObjectNameInfoEXT nameInfo{};
        nameInfo.objectHandle = (u64)(VkFramebuffer)viewportRef.framebuffer;
        nameInfo.objectType = viewportRef.framebuffer.objectType;
        std::string name = std::string("Graphics viewport #") + std::to_string(viewportID) + " - Framebuffer";
        nameInfo.pObjectName = name.data();
        debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
    }
}

namespace DEngine::Gfx::Vk
{
    void imguiCheckVkResult(VkResult result)
    {
        if (static_cast<vk::Result>(result) != vk::Result::eSuccess)
            throw std::runtime_error("");
    }
}

#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_vulkan.h"

void DEngine::Gfx::Vk::Init::InitializeImGui(
    APIData& apiData, 
    DevDispatch const& device,
    PFN_vkGetInstanceProcAddr instanceProcAddr, 
    DebugUtilsDispatch const* debugUtils)
{
    vk::Result vkResult{};

    // Create the descriptor pool for ImGui to use
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
    vk::DescriptorPoolCreateInfo poolInfo{};
    poolInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
    poolInfo.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
    poolInfo.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
    poolInfo.pPoolSizes = (vk::DescriptorPoolSize const*)pool_sizes;
    vk::DescriptorPool imguiDescrPool = device.createDescriptorPool(poolInfo);
    if (debugUtils)
    {
        char const* name = "Dear ImGui - Descriptor Pool";

        vk::DebugUtilsObjectNameInfoEXT nameInfo{};
        nameInfo.objectHandle = (u64)(VkDescriptorPool)imguiDescrPool;
        nameInfo.objectType = imguiDescrPool.objectType;
        nameInfo.pObjectName = name;
        debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
    }

    ImGui_ImplVulkan_InitInfo imguiInfo = {};
    imguiInfo.Instance = static_cast<VkInstance>(apiData.instance.handle);
    imguiInfo.PhysicalDevice = static_cast<VkPhysicalDevice>(apiData.physDevice.handle);
    imguiInfo.Device = static_cast<VkDevice>(apiData.device.handle);
    imguiInfo.QueueFamily = 0;
    imguiInfo.Queue = static_cast<VkQueue>(apiData.renderQueue);
    imguiInfo.PipelineCache = 0;
    imguiInfo.DescriptorPool = static_cast<VkDescriptorPool>(imguiDescrPool);
    imguiInfo.Allocator = 0;
    imguiInfo.MinImageCount = 2;
    imguiInfo.ImageCount = 2;
    imguiInfo.CheckVkResultFn = &Vk::imguiCheckVkResult;
    imguiInfo.MSAASamples = static_cast<VkSampleCountFlagBits>(vk::SampleCountFlagBits::e1);
    imguiInfo.pfnVkGetInstanceProcAddr = instanceProcAddr;
    if (debugUtils)
        imguiInfo.useDebugUtils = true;

    bool imguiInitSuccess = ImGui_ImplVulkan_Init(&imguiInfo, static_cast<VkRenderPass>(apiData.guiData.renderPass));
    if (!imguiInitSuccess)
        throw std::runtime_error("Could not initialize the ImGui Vulkan stuff.");

    vk::CommandPoolCreateInfo cmdPoolInfo{};
    vk::CommandPool cmdPool = device.createCommandPool(cmdPoolInfo);

    vk::CommandBufferAllocateInfo cmdBufferAllocInfo{};
    cmdBufferAllocInfo.commandBufferCount = 1;
    cmdBufferAllocInfo.commandPool = cmdPool;
    cmdBufferAllocInfo.level = vk::CommandBufferLevel::ePrimary;
    vk::CommandBuffer cmdBuffer{};
    vkResult = device.allocateCommandBuffers(&cmdBufferAllocInfo, &cmdBuffer);
    if (vkResult != vk::Result::eSuccess)
        throw std::runtime_error("Unable to allocate command buffer when initializing ImGui font texture");

    vk::CommandBufferBeginInfo beginInfo{};
    beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
    device.beginCommandBuffer(cmdBuffer, beginInfo);

    ImGui_ImplVulkan_CreateFontsTexture(static_cast<VkCommandBuffer>(cmdBuffer));

    device.endCommandBuffer(cmdBuffer);

    vk::Fence tempFence = apiData.device.createFence({});
    vk::SubmitInfo submitInfo{};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdBuffer;
    device.queueSubmit(apiData.renderQueue, { submitInfo }, tempFence);
    vkResult = apiData.device.waitForFences({ tempFence }, true, std::numeric_limits<std::uint64_t>::max());
    if (vkResult != vk::Result::eSuccess)
        throw std::runtime_error("Vulkan: Could not wait for fence after submitting ImGui create-fonts cmdBuffer.");
    device.destroyFence(tempFence);
    device.destroyCommandPool(cmdPool);
    ImGui_ImplVulkan_DestroyFontUploadObjects();
}