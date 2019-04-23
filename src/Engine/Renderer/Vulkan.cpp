#include "Vulkan.hpp"

#include <vulkan/vulkan.hpp>

void DRenderer::Vulkan::Initialize(std::any& apiData, InitInfo& createInfo)
{
	auto test = createInfo.test();
	if (test != nullptr)
	{
		auto ptr = (PFN_vkEnumerateInstanceExtensionProperties)test;
		uint32_t count = 0;
		ptr(nullptr, &count, nullptr);

		std::vector<VkExtensionProperties> extensionProperties(count);

		ptr(nullptr, &count, extensionProperties.data());

	}


}
