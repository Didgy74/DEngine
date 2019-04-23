#include "Vulkan.hpp"

#include "Volk/volk.h"
#include <vulkan/vulkan.hpp>

#include "VulkanExtensionConfig.hpp"

#include <cassert>
#include <cstdint>
#include <array>

namespace DRenderer::Vulkan
{
	constexpr std::array requiredValidLayers
	{
		"VK_LAYER_LUNARG_standard_validation"
	};

	struct APIData;
	void APIDataDeleter(void*& ptr);
	APIData& GetAPIData();
}

struct DRenderer::Vulkan::APIData
{
	struct Version
	{
		uint32_t major;
		uint32_t minor;
		uint32_t patch;
	};
	Version apiVersion{};

	std::vector<vk::ExtensionProperties> instanceExtensionProperties;
	std::vector<vk::LayerProperties> instanceLayerProperties;
};

void DRenderer::Vulkan::APIDataDeleter(void*& ptr)
{
	APIData* data = static_cast<APIData*>(ptr);
	ptr = nullptr;
	delete data;
}

DRenderer::Vulkan::APIData& DRenderer::Vulkan::GetAPIData()
{
	return *static_cast<APIData*>(Engine::Renderer::Core::GetAPIData());
}

namespace DRenderer::Vulkan
{
	bool Init_LoadAPIVersion(APIData::Version& version);
	bool Init_LoadInstanceExtensionProperties(std::vector<vk::ExtensionProperties>& vecRef);
	bool Init_LoadInstanceLayerProperties(std::vector<vk::LayerProperties>& vecRef);
	bool Init_CreateInstance(vk::Instance& target, 
							 const std::vector<vk::ExtensionProperties>& extensions, 
							 const std::vector<vk::LayerProperties>& layers,
							 vk::ApplicationInfo appInfo);
}

bool DRenderer::Vulkan::Init_LoadAPIVersion(APIData::Version& version)
{
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

	// Check if all required extensions are available.
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
		if (foundExtension == false)
			return false;
	}

	vecRef = std::move(activeExts);

	return true;
}

bool DRenderer::Vulkan::Init_LoadInstanceLayerProperties(std::vector<vk::LayerProperties>& vecRef)
{
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
											vk::ApplicationInfo appInfo)
{
	vk::InstanceCreateInfo createInfo{};

	std::vector<const char*> extensionList;
	extensionList.reserve(extensions.size());
	for (const auto& extension : extensions)
		extensionList.push_back(extension.extensionName);
	createInfo.enabledExtensionCount = extensionList.size();
	createInfo.ppEnabledExtensionNames = extensionList.data();

	if constexpr (debugConfig == true)
	{
		std::vector<const char*> layerList;
		layerList.reserve(layers.size());
		for (const auto& layer : layers)
			layerList.push_back(layer.layerName);
		createInfo.enabledLayerCount = layerList.size();
		createInfo.ppEnabledLayerNames = layerList.data();
	}

	createInfo.pApplicationInfo = &appInfo;
}

void DRenderer::Vulkan::Initialize(Core::APIDataPointer& apiData, InitInfo& initInfo)
{
	if (initInfo.test() == nullptr)
		std::abort();

	volkInitializeCustom((PFN_vkGetInstanceProcAddr)initInfo.test());

	apiData.data = new APIData();
	apiData.deleterPfn = &APIDataDeleter;
	APIData& data = *static_cast<APIData*>(apiData.data);

	Init_LoadAPIVersion(data.apiVersion);
	Init_LoadInstanceExtensionProperties(data.instanceExtensionProperties);
	Init_LoadInstanceLayerProperties(data.instanceLayerProperties);

	
}
