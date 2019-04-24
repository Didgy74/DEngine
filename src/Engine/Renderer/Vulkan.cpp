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
		"VK_KHR_swapchain",
	};

	constexpr std::array requiredDeviceLayers
	{
		"VK_LAYER_LUNARG_standard_validation"
	};

	struct APIData;
	void APIDataDeleter(void*& ptr);
	APIData& GetAPIData();

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

	vk::PhysicalDevice physicalDevice = nullptr;
	bool hostMemoryIsDeviceLocal = false;

	vk::Device device = nullptr;

	vk::Queue graphicsQueue = nullptr;
	vk::Queue presentQueue = nullptr;
	vk::Queue transferQueue = nullptr;
	vk::Queue computeQueue = nullptr;
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

namespace DRenderer::Vulkan
{
	bool Init_LoadAPIVersion(APIData::Version& version);
	bool Init_LoadInstanceExtensionProperties(std::vector<vk::ExtensionProperties>& vecRef);
	bool Init_LoadInstanceLayerProperties(std::vector<vk::LayerProperties>& vecRef);
	bool Init_CreateInstance(vk::Instance& target, 
							 const std::vector<vk::ExtensionProperties>& extensions, 
							 const std::vector<vk::LayerProperties>& layers,
							 vk::ApplicationInfo appInfo,
							 const std::vector<std::string_view>& extensions2);
	bool Init_CreateDebugMessenger(vk::Instance instance, vk::DebugUtilsMessengerEXT& messengerRef);
	bool Init_CreateSurface(vk::Instance instance, void* hwnd, vk::SurfaceKHR& targetSurface);
}

bool DRenderer::Vulkan::Init_LoadAPIVersion(APIData::Version& version)
{
	// Query the highest API version available.
	uint32_t apiVersion = 0;
	auto enumerateVersionResult = vk::enumerateInstanceVersion(&apiVersion);
	assert(enumerateVersionResult == vk::Result::eSuccess);
	version.major = VK_VERSION_MAJOR(apiVersion);
	version.minor = VK_VERSION_MINOR(apiVersion);
	version.patch = VK_VERSION_PATCH(apiVersion);

	return true;
}

bool DRenderer::Vulkan::Init_LoadInstanceExtensionProperties(std::vector<vk::ExtensionProperties>& vecRef)
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
			return false;
	}

	vecRef = std::move(activeExts);

	return true;
}

bool DRenderer::Vulkan::Init_LoadInstanceLayerProperties(std::vector<vk::LayerProperties>& vecRef)
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
				return false;
		}

		vecRef = std::move(activeLayers);
	}

	return true;
}

bool DRenderer::Vulkan::Init_CreateInstance(vk::Instance& target,
											const std::vector<vk::ExtensionProperties>& extensions,
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
		return false;

	target = result;

	return true;
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

bool DRenderer::Vulkan::Init_CreateDebugMessenger(vk::Instance instance, vk::DebugUtilsMessengerEXT& messengerRef)
{
	if constexpr (debugConfig == true)
	{
		vk::DebugUtilsMessengerCreateInfoEXT createInfo{};
		createInfo.messageSeverity =
			vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
			vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning;
		createInfo.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation;
		createInfo.pfnUserCallback = (PFN_vkDebugUtilsMessengerCallbackEXT)&Callback;

		vk::DebugUtilsMessengerEXT temp = instance.createDebugUtilsMessengerEXT(createInfo);
		if (temp)
			messengerRef = temp;
		else
			return false;
	}

	return true;
}

bool DRenderer::Vulkan::Init_CreateSurface(vk::Instance instance, void* hwnd, vk::SurfaceKHR& targetSurface)
{
	vk::SurfaceKHR tempSurfaceHandle;
	bool surfaceCreationTest = GetAPIData().initInfo.test(&instance, hwnd, &tempSurfaceHandle);
	assert(surfaceCreationTest == true);
	targetSurface = tempSurfaceHandle;
	return true;
}

void DRenderer::Vulkan::Initialize(Core::APIDataPointer& apiData, InitInfo& initInfo)
{
	Volk::InitializeCustom((PFN_vkGetInstanceProcAddr)initInfo.getInstanceProcAddr());

	apiData.data = new APIData();
	apiData.deleterPfn = &APIDataDeleter;
	APIData& data = *static_cast<APIData*>(apiData.data);
	data.initInfo = initInfo;

	// Start initialization
	Init_LoadAPIVersion(data.apiVersion);
	Init_LoadInstanceExtensionProperties(data.instanceExtensionProperties);
	Init_LoadInstanceLayerProperties(data.instanceLayerProperties);
	
	vk::ApplicationInfo appInfo{};
	appInfo.apiVersion = VK_MAKE_VERSION(data.apiVersion.major, data.apiVersion.minor, data.apiVersion.patch);
	appInfo.pApplicationName = "DEngine";
	appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
	appInfo.pEngineName = "DEngine";
	appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
	auto wsiRequiredExtensions = initInfo.getRequiredInstanceExtensions();
	Init_CreateInstance(data.instance, data.instanceExtensionProperties, data.instanceLayerProperties, appInfo, wsiRequiredExtensions);

	Volk::LoadInstance(data.instance);

	Init_CreateDebugMessenger(data.instance, data.debugMessenger);
	void* hwnd = Engine::Renderer::GetViewport(0).GetSurfaceHandle();
	Init_CreateSurface(data.instance, hwnd, data.surface);


	





	auto devices = data.instance.enumeratePhysicalDevices();
	assert(devices.size() > 0);
	data.physicalDevice = devices[0];
	auto queues = data.physicalDevice.getQueueFamilyProperties();
	bool presentSupport = data.physicalDevice.getSurfaceSupportKHR(2, data.surface);


	size_t graphicsFamily = 0;
	size_t graphicsQueue = 0;

	size_t transferFamily = 2;
	size_t transferQueue = 0;

	size_t computeFamily = 1;
	size_t computeQueue = 0;

	size_t presentFamily = 2;
	size_t presentQueue = 1;


	vk::DeviceCreateInfo createInfo{};

	// Feature configuration
	auto physDeviceFeatures = data.physicalDevice.getFeatures();

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
	secondQueueCreateInfo.queueCount = 1;
	secondQueueCreateInfo.queueFamilyIndex = 1;
	queueCreateInfos.push_back(secondQueueCreateInfo);

	vk::DeviceQueueCreateInfo thirdQueueCreateInfo;
	thirdQueueCreateInfo.pQueuePriorities = priority;
	thirdQueueCreateInfo.queueCount = 2;
	thirdQueueCreateInfo.queueFamilyIndex = 2;
	queueCreateInfos.push_back(thirdQueueCreateInfo);

	createInfo.queueCreateInfoCount = uint32_t(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();


	auto deviceExtensionsAvailable = data.physicalDevice.enumerateDeviceExtensionProperties();
	// Check if all required device extensions are available
	createInfo.ppEnabledExtensionNames = Vulkan::requiredDeviceExtensions.data();
	createInfo.enabledExtensionCount = uint32_t(Vulkan::requiredDeviceExtensions.size());

	auto deviceLayersAvailable = data.physicalDevice.enumerateDeviceLayerProperties();
	createInfo.enabledLayerCount = uint32_t(Vulkan::requiredDeviceLayers.size());
	createInfo.ppEnabledLayerNames = Vulkan::requiredDeviceLayers.data();

	auto result = data.physicalDevice.createDevice(createInfo);
	if (!result)
		std::abort();
	data.device = result;

	data.graphicsQueue = data.device.getQueue(0, 0);
	data.presentQueue = data.device.getQueue(2, 1);
	data.transferQueue = data.device.getQueue(2, 0);
	data.computeQueue = data.device.getQueue(1, 0);




	// Query surface capabilities
	auto capabilities = data.physicalDevice.getSurfaceCapabilitiesKHR(data.surface);
	auto presentModes = data.physicalDevice.getSurfacePresentModesKHR(data.surface);
	auto formats = data.physicalDevice.getSurfaceFormatsKHR(data.surface);



}
