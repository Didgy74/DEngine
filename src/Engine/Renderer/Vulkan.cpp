#include "Renderer.hpp"
#include "Vulkan.hpp"

#include "../Asset.hpp"
#include "../Application.hpp"
#include "../Engine.hpp"

#include "vulkan/vulkan.hpp"

#include <array>
#include <set>
#include <vector>
#include <fstream>
#include <queue>
#include <iostream>
#include <map>
#include <thread>
#include <unordered_map>

namespace Engine
{
	namespace Renderer
	{
		namespace Vulkan
		{
		#ifdef NDEBUG
			constexpr bool enableDebug = false;
		#else
			constexpr bool enableDebug = true;
		#endif

			constexpr bool preferTripleBuffering = true;

			constexpr std::string_view createDebugReportCallbackName = "vkCreateDebugReportCallbackEXT";
			constexpr std::string_view destroyDebugReportCallbackName = "vkDestroyDebugReportCallbackEXT";

			constexpr std::string_view vkKHRSurfaceExtensionName = VK_KHR_SURFACE_EXTENSION_NAME;
			constexpr std::string_view vkEXTDebugReportExtensionName = VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
			constexpr char vkKHRSwapchainExtensionName[] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;

			constexpr std::array requiredValidationLayers
			{
				"VK_LAYER_LUNARG_standard_validation"
			};

			constexpr std::array requiredInstanceExtensions
			{
				vkKHRSurfaceExtensionName
			};

			constexpr std::array requiredDeviceExtensions
			{
				vkKHRSwapchainExtensionName
			};

			struct PhysicalDevice;

			struct VertexBufferObject;
			using VBO = VertexBufferObject;

			struct ImageBufferObject;
			using IBO = ImageBufferObject;

			struct ImageHandle;
			struct PerInFlightSyncObject;

			struct Swapchain;

			struct DeletionQueues;

			struct Data;
			Data& GetData() { return *static_cast<Data*>(Core::GetAPIData()); }

			void LoadInstanceExtensions(Data& data);
			bool CheckValidationLayerSupport(const std::vector<vk::LayerProperties>& availableLayers);
			void CreateInstance(Data& data);
			void SetUpDebugCallback(Data& data);
			void CreateSurface(Data& data);
			void PickPhysicalDevice(Data& data);
			void CreateLogicalDevice(Data& data);
			void CreateSwapchain(Data& data);
			void CreateSwapchainImageViews(Data& data);
			void CreateDepthResources(Data& data);
			void CreateSyncObjects(Data& data);
			void CreateEssentialUniforms(Data& data);
			void CreateDeletionQueues(Data& data);
			void CreateRenderPass(Data& data);
			void CreateSwapchainFramebuffers(Data& data);
			void CreateCommandPool(Data& data);
			void CreateSampler(Data& data);
			void HandleGraphicsCommandBuffers();


			void UpdateEssentialUniforms(Data& data, uint8_t inFlightIndex);
			void MemoryCleanup(Data& data, uint8_t inFlightIndex);

			void UpdateVBOs(Data& data, const std::vector<MeshID>& loadQueue);

			uint32_t FindMemoryType(PhysicalDevice& device, uint32_t typeFilter, vk::MemoryPropertyFlags properties);
			std::pair<vk::Buffer, vk::DeviceMemory> MakeAndBindBuffer(vk::Device& device, vk::DeviceSize bufferSize, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags memoryProperties);



			void CreatePipeline(Data& data);
		}
	}
}


struct Engine::Renderer::Vulkan::PerInFlightSyncObject
{
	vk::Semaphore imageAvailable = nullptr;
	vk::Semaphore renderFinished = nullptr;
	vk::Fence inFlight = nullptr;
};

struct Engine::Renderer::Vulkan::ImageHandle
{
	vk::Image image = nullptr;
	vk::ImageView imageView = nullptr;
	vk::DeviceMemory deviceMemory = nullptr;
};

struct Engine::Renderer::Vulkan::PhysicalDevice
{
	vk::PhysicalDevice handle = nullptr;
	uint32_t graphicsQueueFamilyIndex = std::numeric_limits<uint32_t>::max();
	uint32_t transferQueueFamilyIndex = std::numeric_limits<uint32_t>::max();
	vk::SurfaceCapabilitiesKHR capabilities;
	std::vector<vk::SurfaceFormatKHR> surfaceFormats;
	vk::PhysicalDeviceMemoryProperties memoryProperties;
	vk::PhysicalDeviceProperties properties;
	std::vector<vk::PresentModeKHR> presentModes;
};

struct Engine::Renderer::Vulkan::VertexBufferObject
{
	enum class Attribute
	{
		Position,
		TexCoord,
		Normal,
		Index,
		COUNT
	};

	vk::Buffer buffer = nullptr;
	vk::DeviceMemory deviceMemory = nullptr;
	std::array<vk::DeviceSize, static_cast<size_t>(Attribute::COUNT)> attributeSizes;

	vk::DeviceSize indexCount = std::numeric_limits<vk::DeviceSize>::max();
	vk::IndexType indexType;

	size_t GetByteOffset(Attribute attribute) const;
};

size_t Engine::Renderer::Vulkan::VBO::GetByteOffset(Attribute attribute) const
{
	size_t offset = 0;
	for (size_t i = 0; i < static_cast<size_t>(attribute); i++)
		offset += static_cast<size_t>(attributeSizes[i]);
	return offset;
}

struct Engine::Renderer::Vulkan::ImageBufferObject
{
	vk::Image image = nullptr;
	vk::ImageView imageView = nullptr;
	vk::DeviceMemory deviceMemory = nullptr;
};

struct Engine::Renderer::Vulkan::Swapchain
{
	uint8_t numImages = std::numeric_limits<uint8_t>::max();
	uint8_t numInFlight = std::numeric_limits<uint8_t>::max();

	vk::SwapchainKHR handle = nullptr;
	vk::SwapchainKHR oldHandle = nullptr;

	struct ImageHandle
	{
		vk::Image image = nullptr;
		vk::ImageView imageView = nullptr;
		vk::Framebuffer framebuffer = nullptr;
	};
	std::vector<ImageHandle> handles;

	Vulkan::ImageHandle depthImage;

	vk::SurfaceFormatKHR surfaceFormat;
	vk::PresentModeKHR presentMode;
	vk::Extent2D extent;
};

struct Engine::Renderer::Vulkan::DeletionQueues
{
	struct DeletionJob
	{
		vk::Buffer buffer;
		vk::DeviceMemory memory;
	};
	std::array<std::vector<DeletionJob>, 2> deletionJobs;
};

struct Engine::Renderer::Vulkan::Data
{
	Data() = default;
	Data(const Data&) = delete;
	Data(Data&&) = delete;

	uint8_t currentInFlight = std::numeric_limits<uint8_t>::max();
	uint8_t currentFrame = std::numeric_limits<uint8_t>::max();

	vk::Instance instance = nullptr;

	PFN_vkCreateDebugReportCallbackEXT createDebugReportCallbackPtr = nullptr;
	PFN_vkDestroyDebugReportCallbackEXT destroyDebugReportCallbackPtr = nullptr;
	vk::DebugReportCallbackEXT debugReportCallback = nullptr;

	vk::SurfaceKHR surface = nullptr;

	PhysicalDevice physDevice;
	vk::Device device = nullptr;
	bool hostVisibleIsDeviceLocal = false;

	vk::Queue graphicsQueue = nullptr;
	vk::Queue transferQueue = nullptr;

	Swapchain swapchain;
	std::vector<PerInFlightSyncObject> perInFlightSyncObjects;

	struct EssentialUniforms
	{
		vk::Buffer buffer = nullptr;
		vk::DeviceMemory deviceMemory = nullptr;
		std::byte* mappedMemory = nullptr;
		vk::DeviceSize cameraTransformSize = std::numeric_limits<vk::DeviceSize>::max();
		vk::DeviceSize modelTransformSize = std::numeric_limits<vk::DeviceSize>::max();
		vk::DeviceSize modelCount = std::numeric_limits<vk::DeviceSize>::max();
		vk::DescriptorSetLayout descriptorSetLayout = nullptr;
		vk::DescriptorPool descriptorPool = nullptr;
		vk::DescriptorSet descSet = nullptr;
	};
	EssentialUniforms essentialUniforms;

	vk::RenderPass renderPass = nullptr;
	vk::PipelineLayout pipelineLayout = nullptr;
	vk::Pipeline pipeline = nullptr;

	vk::CommandPool commandPool = nullptr;
	struct PrimaryCommandBuffer
	{
		vk::CommandBuffer handle;
		bool upToDate = false;
	};
	std::vector<PrimaryCommandBuffer> primaryCommandBuffers;

	vk::Sampler sampler = nullptr;

	std::unordered_map<MeshID, VBO> vboDatabase;
	std::unordered_map<SpriteID, IBO> iboDatabase;
	std::vector<DeletionQueues> deletionQueues;

	std::vector<const char*> availableInstanceExtensions;
	std::vector<const char*> activeInstanceExtensions;
	std::vector<vk::LayerProperties> availableValidationLayers;
};

uint32_t Engine::Renderer::Vulkan::FindMemoryType(PhysicalDevice& device, uint32_t typeFilter, vk::MemoryPropertyFlags properties)
{
	for (uint32_t i = 0; i < device.memoryProperties.memoryTypeCount; i++)
	{
		bool memoryTypeCanBeUsed = typeFilter & (1 << i);
		if (memoryTypeCanBeUsed && (device.memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
			return i;
	}

	return std::numeric_limits<uint32_t>::max();
}

std::pair<vk::Buffer, vk::DeviceMemory> Engine::Renderer::Vulkan::MakeAndBindBuffer(vk::Device& device, vk::DeviceSize bufferSize, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags memoryProperties)
{
	vk::BufferCreateInfo bufferInfo;
	bufferInfo.size = bufferSize;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = vk::SharingMode::eExclusive;
	//std::array<uint32_t, 1> queueFamilyIndices = { VkVar::PhysicalDevice::queueFamilyIndices.graphicsFamily };
	//bufferInfo.queueFamilyIndexCount = queueFamilyIndices.size();
	//bufferInfo.pQueueFamilyIndices = queueFamilyIndices.data();

	vk::Buffer newBuffer = device.createBuffer(bufferInfo);
	assert(newBuffer);

	vk::MemoryRequirements memReq = device.getBufferMemoryRequirements(newBuffer);
	vk::MemoryAllocateInfo allocInfo;
	allocInfo.allocationSize = memReq.size;
	allocInfo.memoryTypeIndex = FindMemoryType(GetData().physDevice, memReq.memoryTypeBits, memoryProperties);
	vk::DeviceMemory newDeviceMemory = device.allocateMemory(allocInfo);
	assert(newDeviceMemory);

	device.bindBufferMemory(newBuffer, newDeviceMemory, 0);

	return { newBuffer, newDeviceMemory };
}

void Engine::Renderer::Vulkan::LoadInstanceExtensions(Data& data)
{
	auto availableInstanceExtensions = vk::enumerateInstanceExtensionProperties();
	data.availableInstanceExtensions.reserve(availableInstanceExtensions.size());
	for (const auto& item : availableInstanceExtensions)
		GetData().availableInstanceExtensions.push_back(item.extensionName);

	Renderer::Viewport& viewport = Renderer::GetViewport(0);

	// Grabs extensions required by SDL.
	auto requiredExtensions = Engine::Application::Core::Vulkan_GetInstanceExtensions(viewport.GetSurfaceHandle());

	// Combines extensions required by application and extensions required by SDL, without making duplicates.
	for (const auto& item1 : requiredInstanceExtensions)
	{
		bool extensionFound = false;
		for (const auto& item2 : requiredExtensions)
		{
			if (std::strcmp(item1.data(), item2) == 0)
			{
				extensionFound = true;
				break;
			}
		}
		if (extensionFound == false)
			requiredExtensions.push_back(item1.data());
	}

	if constexpr (enableDebug)
		requiredExtensions.push_back(vkEXTDebugReportExtensionName.data());

	// Checks if all the required extensions are available.
	for (const auto& item1 : requiredExtensions)
	{
		bool extensionFound = false;
		for (const auto& item2 : GetData().availableInstanceExtensions)
		{
			if (std::strcmp(item1, item2) == 0)
			{
				extensionFound = true;
				break;
			}
		}
		assert(extensionFound);
	}
	GetData().activeInstanceExtensions = std::move(requiredExtensions);
}

bool Engine::Renderer::Vulkan::CheckValidationLayerSupport(const std::vector<vk::LayerProperties>& availableLayers)
{
	for (const auto& layerName : requiredValidationLayers)
	{
		bool layerFound = false;
		for (const auto& layerProperties : availableLayers)
		{
			if (strcmp(layerName, layerProperties.layerName) == 0)
			{
				layerFound = true;
				break;
			}
		}
		if (!layerFound)
			return false;
	}
	return true;
}

void Engine::Renderer::Vulkan::CreateInstance(Data& data)
{
	vk::InstanceCreateInfo instanceCreateInfo;

	vk::ApplicationInfo appInfo;
	appInfo.pApplicationName = "Some application";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "Some Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_1;
	instanceCreateInfo.pApplicationInfo = &appInfo;

	instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(data.activeInstanceExtensions.size());
	instanceCreateInfo.ppEnabledExtensionNames = data.activeInstanceExtensions.data();

	if constexpr (enableDebug)
	{
		instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(requiredValidationLayers.size());
		instanceCreateInfo.ppEnabledLayerNames = requiredValidationLayers.data();
	}

	// Create instance
	data.instance = vk::createInstance(instanceCreateInfo);
	assert(data.instance);
}

#pragma warning( push )
#pragma warning( disable : 4100 )
VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback
(
	VkDebugReportFlagsEXT flags,
	VkDebugReportObjectTypeEXT objType,
	uint64_t obj,
	size_t location,
	int32_t code,
	const char* layerPrefix,
	const char* msg,
	void* userData)
{
	std::cerr << "Vulkan Validation Layer:\n" << msg << '\n' << std::endl;

	return VK_FALSE;
}
#pragma warning( pop )

void Engine::Renderer::Vulkan::SetUpDebugCallback(Data& data)
{
	vk::DebugReportCallbackCreateInfoEXT createInfo;
	createInfo.flags = vk::DebugReportFlagBitsEXT::eError | vk::DebugReportFlagBitsEXT::eWarning | vk::DebugReportFlagBitsEXT::ePerformanceWarning;
	createInfo.pfnCallback = VulkanDebugCallback;

	// Bind vkCreateDebugReportCallbackEXT and create debug report callback.
	data.createDebugReportCallbackPtr = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(data.instance.getProcAddr(createDebugReportCallbackName.data()));
	assert(data.createDebugReportCallbackPtr);

	VkDebugReportCallbackCreateInfoEXT createInfoTemp = createInfo;
	VkDebugReportCallbackEXT callback;
	data.createDebugReportCallbackPtr(static_cast<VkInstance>(data.instance), &createInfoTemp, nullptr, &callback);
	data.debugReportCallback = static_cast<vk::DebugReportCallbackEXT>(callback);

	// Bind vkDestroyDebugReportCallbackEXT for destruction upon Vulkan termination.
	data.destroyDebugReportCallbackPtr = reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(data.instance.getProcAddr(destroyDebugReportCallbackName.data()));
	assert(data.destroyDebugReportCallbackPtr);
}

void Engine::Renderer::Vulkan::CreateSurface(Data& data)
{
	Renderer::Viewport& viewport = Renderer::GetViewport(0);

	VkSurfaceKHR surface;
	Engine::Application::Core::Vulkan_CreateSurface(viewport.GetSurfaceHandle(), &data.instance, &surface);
	data.surface = static_cast<vk::SurfaceKHR>(surface);

	data.swapchain.surfaceFormat = { vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear };

	data.swapchain.presentMode = vk::PresentModeKHR::eImmediate;
	data.swapchain.extent = vk::Extent2D(viewport.GetDimensions().width, viewport.GetDimensions().height);
}

void Engine::Renderer::Vulkan::PickPhysicalDevice(Data& data)
{
	auto physicalDevices = data.instance.enumeratePhysicalDevices();
	assert(physicalDevices.size() > 0);
	data.physDevice.handle = physicalDevices[0];
	assert(data.physDevice.handle);

	auto queueFamilies = data.physDevice.handle.getQueueFamilyProperties();
	assert(queueFamilies.size() > 0);

	if ((queueFamilies[0].queueFlags & vk::QueueFlagBits::eGraphics) == vk::QueueFlagBits::eGraphics)
		data.physDevice.graphicsQueueFamilyIndex = 0;
	else
		assert(false);

	auto surfaceSupportTest = data.physDevice.handle.getSurfaceSupportKHR(0, data.surface);
	assert(surfaceSupportTest == VK_TRUE);

	data.physDevice.capabilities = data.physDevice.handle.getSurfaceCapabilitiesKHR(data.surface);

	data.physDevice.surfaceFormats = data.physDevice.handle.getSurfaceFormatsKHR(data.surface);
	data.physDevice.presentModes = data.physDevice.handle.getSurfacePresentModesKHR(data.surface);


	data.physDevice.memoryProperties = data.physDevice.handle.getMemoryProperties();

	data.physDevice.properties = data.physDevice.handle.getProperties();

	// Checks if host-visible memory is device-local.
	size_t hostVisible = FindMemoryType(data.physDevice, std::numeric_limits<uint32_t>::max(), vk::MemoryPropertyFlagBits::eHostVisible);
	size_t hostVisibleHeapIndex = data.physDevice.memoryProperties.memoryTypes[hostVisible].heapIndex;
	size_t deviceLocal = FindMemoryType(data.physDevice, std::numeric_limits<uint32_t>::max(), vk::MemoryPropertyFlagBits::eDeviceLocal);
	size_t deviceLocalHeapIndex = data.physDevice.memoryProperties.memoryTypes[deviceLocal].heapIndex;
	if (hostVisibleHeapIndex == deviceLocalHeapIndex)
		data.hostVisibleIsDeviceLocal = true;
	else
		data.hostVisibleIsDeviceLocal = false;




	/*if ((queueFamilies[1].queueFlags & vk::QueueFlagBits::eTransfer) == vk::QueueFlagBits::eTransfer)
		data.physDevice.transferQueueFamilyIndex = 1;
	else
		assert(false);*/
}

void Engine::Renderer::Vulkan::CreateLogicalDevice(Data& data)
{
	vk::DeviceCreateInfo createInfo;

	std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;

	std::set<uint32_t> uniqueQueueFamilies{ data.physDevice.graphicsQueueFamilyIndex };

	queueCreateInfos.reserve(uniqueQueueFamilies.size());

	float queuePriority = 1.0f;
	for (const auto& queueFamily : uniqueQueueFamilies)
	{
		vk::DeviceQueueCreateInfo queueCreateInfo;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();

	vk::PhysicalDeviceFeatures deviceFeatures;
	createInfo.pEnabledFeatures = &deviceFeatures;

	// Pass info about enabled device extensions.
	createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredDeviceExtensions.size());
	createInfo.ppEnabledExtensionNames = requiredDeviceExtensions.data();

	if constexpr (enableDebug)
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>(requiredValidationLayers.size());
		createInfo.ppEnabledLayerNames = requiredValidationLayers.data();
	}

	data.device = data.physDevice.handle.createDevice(createInfo);
	assert(data.device);

	data.graphicsQueue = data.device.getQueue(data.physDevice.graphicsQueueFamilyIndex, 0);
	//data.transferQueue = data.device.getQueue(data.physDevice.transferQueueFamilyIndex, 0);
}

void Engine::Renderer::Vulkan::CreateSwapchain(Data& data)
{
	Renderer::Viewport& viewport = Renderer::GetViewport(0);

	vk::SwapchainCreateInfoKHR createInfo;
	createInfo.surface = data.surface;
	createInfo.minImageCount = preferTripleBuffering ? 3 : 2;
	createInfo.imageFormat = data.swapchain.surfaceFormat.format;
	createInfo.imageColorSpace = data.swapchain.surfaceFormat.colorSpace;
	createInfo.imageExtent = data.swapchain.extent;
	createInfo.presentMode = data.swapchain.presentMode;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;

	
	// What the fuck even is this for?
	//QueueFamilyIndices indices = VkVar::PhysicalDevice::queueFamilyIndices;
	//std::vector<uint32_t> queueFamilyIndices{ static_cast<uint32_t>(indices.graphicsFamily), static_cast<uint32_t>(indices.presentFamily) };
	//if (indices.graphicsFamily != indices.presentFamily)
	//{
		//createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
		//createInfo.queueFamilyIndexCount = static_cast<uint32_t>(queueFamilyIndices.size());
		//createInfo.pQueueFamilyIndices = queueFamilyIndices.data();
	//}
	createInfo.imageSharingMode = vk::SharingMode::eExclusive;


	createInfo.preTransform = data.physDevice.capabilities.currentTransform;

	createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;

	// Setting clipped to true means you can't read the pixels from it.
	createInfo.clipped = VK_TRUE;

	createInfo.oldSwapchain = data.swapchain.handle;
	data.swapchain.oldHandle = data.swapchain.handle;
	data.swapchain.handle = data.device.createSwapchainKHR(createInfo);
	assert(data.swapchain.handle);

	auto imageHandles = data.device.getSwapchainImagesKHR(data.swapchain.handle);

	data.swapchain.numImages = static_cast<uint8_t>(imageHandles.size());
	data.swapchain.numInFlight = data.swapchain.numImages - 1;
	assert(data.swapchain.numImages <= 3);

	data.swapchain.handles.resize(data.swapchain.numImages);
	for (size_t i = 0; i < data.swapchain.numImages; i++)
		data.swapchain.handles[i].image = imageHandles[i];

	// Schedule image layout transition to presentation
}

void Engine::Renderer::Vulkan::CreateSwapchainImageViews(Data& data)
{
	for (size_t i = 0; i < data.swapchain.numImages; i++)
	{
		vk::ImageViewCreateInfo createInfo;
		createInfo.image = data.swapchain.handles[i].image;
		createInfo.viewType = vk::ImageViewType::e2D;
		createInfo.format = data.swapchain.surfaceFormat.format;
		createInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		data.swapchain.handles[i].imageView = data.device.createImageView(createInfo);
	}
}

void Engine::Renderer::Vulkan::CreateDepthResources(Data& data)
{
	vk::ImageCreateInfo imageInfo;
	imageInfo.imageType = vk::ImageType::e2D;
	imageInfo.extent = vk::Extent3D(data.swapchain.extent.width, data.swapchain.extent.height, 1);
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = vk::Format::eD32Sfloat;
	imageInfo.tiling = vk::ImageTiling::eOptimal;
	imageInfo.initialLayout = vk::ImageLayout::eUndefined;
	imageInfo.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment;
	imageInfo.samples = vk::SampleCountFlagBits::e1;
	imageInfo.sharingMode = vk::SharingMode::eExclusive;

	data.swapchain.depthImage.image = data.device.createImage(imageInfo);
	assert(data.swapchain.depthImage.image);

	vk::MemoryRequirements memRequirements = data.device.getImageMemoryRequirements(data.swapchain.depthImage.image);

	vk::MemoryAllocateInfo allocInfo;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = FindMemoryType(data.physDevice, memRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);

	data.swapchain.depthImage.deviceMemory = data.device.allocateMemory(allocInfo);

	data.device.bindImageMemory(data.swapchain.depthImage.image, data.swapchain.depthImage.deviceMemory, 0);

	vk::ImageViewCreateInfo imageViewInfo;
	imageViewInfo.image = data.swapchain.depthImage.image;
	imageViewInfo.viewType = vk::ImageViewType::e2D;
	imageViewInfo.format = vk::Format::eD32Sfloat;
	imageViewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
	imageViewInfo.subresourceRange.baseMipLevel = 0;
	imageViewInfo.subresourceRange.levelCount = 1;
	imageViewInfo.subresourceRange.baseArrayLayer = 0;
	imageViewInfo.subresourceRange.layerCount = 1;

	data.swapchain.depthImage.imageView = data.device.createImageView(imageViewInfo);

	// Schedule image layout transition to presentation
}

void Engine::Renderer::Vulkan::CreateSyncObjects(Data& data)
{
	size_t length = data.swapchain.numInFlight;

	data.perInFlightSyncObjects.resize(length);
	for (auto& item : data.perInFlightSyncObjects)
	{
		item.imageAvailable = data.device.createSemaphore(vk::SemaphoreCreateInfo());
		item.renderFinished = data.device.createSemaphore(vk::SemaphoreCreateInfo());

		vk::FenceCreateInfo createInfo;
		createInfo.flags = vk::FenceCreateFlagBits::eSignaled;
		item.inFlight = data.device.createFence(createInfo);
	}
}

void Engine::Renderer::Vulkan::CreateEssentialUniforms(Data& data)
{
	size_t inFlightCount = data.swapchain.numInFlight;

	auto& cameraTransformSize = data.essentialUniforms.cameraTransformSize;
	cameraTransformSize = sizeof(float) * 16;
	if (cameraTransformSize < data.physDevice.properties.limits.minUniformBufferOffsetAlignment)
		cameraTransformSize = data.physDevice.properties.limits.minUniformBufferOffsetAlignment;

	auto& modelTransformSize = data.essentialUniforms.modelTransformSize;
	modelTransformSize = sizeof(float) * 16;
	if (modelTransformSize < data.physDevice.properties.limits.minUniformBufferOffsetAlignment)
		modelTransformSize = data.physDevice.properties.limits.minUniformBufferOffsetAlignment;

	auto& modelCount = data.essentialUniforms.modelCount;
	modelCount = 1000;

	auto totalBufferSize = static_cast<size_t>((cameraTransformSize + modelTransformSize * modelCount) * inFlightCount);
	{
		auto temp = MakeAndBindBuffer(data.device, totalBufferSize, vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible);

		data.essentialUniforms.buffer = temp.first;
		data.essentialUniforms.deviceMemory = temp.second;
		auto mappedMemory = data.device.mapMemory(temp.second, 0, totalBufferSize);
		data.essentialUniforms.mappedMemory = static_cast<std::byte*>(mappedMemory);
	}
	
	// Create descriptor set layout binding for camera + models uniforms
	{
		std::vector<vk::DescriptorSetLayoutBinding> layoutBindings(2);
		// Camera
		layoutBindings[0].binding = 0;
		layoutBindings[0].descriptorCount = 1;
		layoutBindings[0].descriptorType = vk::DescriptorType::eUniformBufferDynamic;
		layoutBindings[0].stageFlags = vk::ShaderStageFlagBits::eVertex;
		// Models
		layoutBindings[1].binding = 1;
		layoutBindings[1].descriptorCount = 1;
		layoutBindings[1].descriptorType = vk::DescriptorType::eUniformBufferDynamic;
		layoutBindings[1].stageFlags = vk::ShaderStageFlagBits::eVertex;

		vk::DescriptorSetLayoutCreateInfo layoutInfo;
		layoutInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
		layoutInfo.pBindings = layoutBindings.data();

		data.essentialUniforms.descriptorSetLayout = data.device.createDescriptorSetLayout(layoutInfo);
		assert(data.essentialUniforms.descriptorSetLayout);
	}
	
	// Create descriptor pool for essential uniforms (camera + models)
	{
		std::vector<vk::DescriptorPoolSize> poolSizes(1);
		poolSizes[0].type = vk::DescriptorType::eUniformBufferDynamic;
		poolSizes[0].descriptorCount = 2;

		vk::DescriptorPoolCreateInfo poolInfo;
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = 1;
		data.essentialUniforms.descriptorPool = data.device.createDescriptorPool(poolInfo);
		assert(data.essentialUniforms.descriptorPool);
	}

	// Allocates descriptor set
	{
		std::vector<vk::DescriptorSetLayout> layouts(1, data.essentialUniforms.descriptorSetLayout);
		vk::DescriptorSetAllocateInfo allocInfo;
		allocInfo.descriptorPool = data.essentialUniforms.descriptorPool;
		allocInfo.descriptorSetCount = static_cast<uint32_t>(layouts.size());
		allocInfo.pSetLayouts = layouts.data();

		auto temp = data.device.allocateDescriptorSets(allocInfo);
		assert(temp.size() > 0);
		data.essentialUniforms.descSet = temp[0];
	}
	
	// Updates descriptorset
	{
		std::vector<vk::WriteDescriptorSet> writes(2);

		vk::DescriptorBufferInfo cameraBufferInfo{};
		cameraBufferInfo.buffer = data.essentialUniforms.buffer;
		//cameraBufferInfo.range = totalBufferSize - (modelTransformSize * modelCount);
		cameraBufferInfo.range = cameraTransformSize;

		writes[0].dstSet = data.essentialUniforms.descSet;
		writes[0].dstBinding = 0;
		writes[0].descriptorCount = 1;
		writes[0].descriptorType = vk::DescriptorType::eUniformBufferDynamic;
		writes[0].pBufferInfo = &cameraBufferInfo;

		vk::DescriptorBufferInfo modelsBufferInfo{};
		modelsBufferInfo.buffer = data.essentialUniforms.buffer;
		modelsBufferInfo.offset = cameraTransformSize;
		//modelsBufferInfo.range = totalBufferSize - cameraTransformSize;
		modelsBufferInfo.range = modelTransformSize;

		writes[1].dstSet = data.essentialUniforms.descSet;
		writes[1].dstBinding = 1;
		writes[1].descriptorCount = 1;
		writes[1].descriptorType = vk::DescriptorType::eUniformBufferDynamic;
		writes[1].pBufferInfo = &modelsBufferInfo;

		data.device.updateDescriptorSets(writes, nullptr);
	}
}

void Engine::Renderer::Vulkan::CreateDeletionQueues(Data& data)
{
	data.deletionQueues.resize(data.swapchain.numInFlight);
	for (auto& perFrameDeletionQueue : data.deletionQueues)
	{
		for (auto& buffer : perFrameDeletionQueue.deletionJobs)
			buffer.reserve(20);
	}
}

void Engine::Renderer::Vulkan::CreateRenderPass(Data& data)
{
	vk::AttachmentDescription colorAttachment;
	colorAttachment.format = data.swapchain.surfaceFormat.format;
	colorAttachment.samples = vk::SampleCountFlagBits::e1;
	colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
	colorAttachment.storeOp = vk::AttachmentStoreOp::eDontCare;
	colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
	colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
	colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
	colorAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;

	vk::AttachmentReference colorAttachmentRef;
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

	vk::AttachmentDescription depthAttachment;
	depthAttachment.format = vk::Format::eD32Sfloat;
	depthAttachment.samples = vk::SampleCountFlagBits::e1;
	depthAttachment.loadOp = vk::AttachmentLoadOp::eClear;
	depthAttachment.storeOp = vk::AttachmentStoreOp::eDontCare;
	depthAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
	depthAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
	depthAttachment.initialLayout = vk::ImageLayout::eUndefined;
	depthAttachment.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

	vk::AttachmentReference depthAttachmentRef;
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

	vk::SubpassDescription subpass;
	subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	std::array attachments = { colorAttachment, depthAttachment };

	vk::RenderPassCreateInfo createInfo;
	createInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	createInfo.pAttachments = attachments.data();
	createInfo.subpassCount = 1;
	createInfo.pSubpasses = &subpass;

	data.renderPass = data.device.createRenderPass(createInfo);
	assert(data.renderPass);
}

void Engine::Renderer::Vulkan::CreateSwapchainFramebuffers(Data& data)
{
	for (size_t i = 0; i < data.swapchain.numImages; i++)
	{
		std::array attachments
		{
			data.swapchain.handles[i].imageView,
			data.swapchain.depthImage.imageView,
		};

		vk::FramebufferCreateInfo createInfo;
		createInfo.renderPass = data.renderPass;
		createInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		createInfo.pAttachments = attachments.data();
		createInfo.width = data.swapchain.extent.width;
		createInfo.height = data.swapchain.extent.height;
		createInfo.layers = 1;

		data.swapchain.handles[i].framebuffer = data.device.createFramebuffer(createInfo);
		assert(data.swapchain.handles[i].framebuffer);
	}
}

void Engine::Renderer::Vulkan::CreateCommandPool(Data& data)
{
	vk::CommandPoolCreateInfo graphicsCreateInfo;
	graphicsCreateInfo.queueFamilyIndex = data.physDevice.graphicsQueueFamilyIndex;
	//graphicsCreateInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;

	data.commandPool = data.device.createCommandPool(graphicsCreateInfo);
	assert(data.commandPool);

	vk::CommandBufferAllocateInfo allocInfo;
	allocInfo.commandPool = data.commandPool;
	allocInfo.level = vk::CommandBufferLevel::ePrimary;
	allocInfo.commandBufferCount = static_cast<uint32_t>(data.swapchain.numImages);

	auto commandBuffers = data.device.allocateCommandBuffers(allocInfo);
	data.primaryCommandBuffers.resize(commandBuffers.size());
	for (size_t i = 0; i < data.primaryCommandBuffers.size(); i++)
		data.primaryCommandBuffers[i].handle = commandBuffers[i];
}

void Engine::Renderer::Vulkan::CreateSampler(Data& data)
{
	vk::SamplerCreateInfo samplerInfo;
	samplerInfo.magFilter = vk::Filter::eLinear;
	samplerInfo.minFilter = vk::Filter::eLinear;
	samplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
	samplerInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
	samplerInfo.addressModeW = vk::SamplerAddressMode::eRepeat;
	samplerInfo.anisotropyEnable = VK_FALSE;
	samplerInfo.maxAnisotropy = 1;
	samplerInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = vk::CompareOp::eAlways;
	samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	data.sampler = data.device.createSampler(samplerInfo);
}

void Engine::Renderer::Vulkan::HandleGraphicsCommandBuffers()
{
	Data& data = GetData();

	const auto& cmdBuffer = data.primaryCommandBuffers[data.currentFrame].handle;

	vk::CommandBufferBeginInfo beginInfo;
	beginInfo.flags = vk::CommandBufferUsageFlagBits::eRenderPassContinue;

	cmdBuffer.begin(beginInfo);

	vk::RenderPassBeginInfo renderPassInfo;
	renderPassInfo.renderPass = data.renderPass;
	renderPassInfo.framebuffer = data.swapchain.handles[data.currentFrame].framebuffer;
	renderPassInfo.renderArea.offset = vk::Offset2D{ 0, 0 };
	renderPassInfo.renderArea.extent = data.swapchain.extent;

	std::array<vk::ClearValue, 2> clearValues;
	clearValues[0].color.setFloat32({ 0.0f, 0.0f, 0.0f, 1.0f });
	clearValues[1].depthStencil = vk::ClearDepthStencilValue{ 1.0f, 0 };

	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	cmdBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

	const auto& sprites = std::vector<SpriteID>();
	if (sprites.size() > 0)
	{
		cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, data.pipeline);

		// Load VBO for spriteplane
		const auto& vbo = data.vboDatabase[static_cast<MeshID>(Asset::Mesh::SpritePlane)];
		std::array<vk::Buffer, 3> vertexBuffers{ vbo.buffer, vbo.buffer, vbo.buffer };
		std::array<vk::DeviceSize, 3> vboOffsets
		{
			vbo.GetByteOffset(VBO::Attribute::Position),
			vbo.GetByteOffset(VBO::Attribute::TexCoord),
			vbo.GetByteOffset(VBO::Attribute::Normal) 
		};
		cmdBuffer.bindVertexBuffers(0, vertexBuffers, vboOffsets);
		cmdBuffer.bindIndexBuffer(vbo.buffer, vbo.GetByteOffset(VBO::Attribute::Index), vbo.indexType);

		std::vector<vk::DescriptorSet> descriptorSets{ data.essentialUniforms.descSet };
		std::vector<uint32_t> uniformOffsets(2);
		uniformOffsets[0] = (data.essentialUniforms.cameraTransformSize + data.essentialUniforms.modelTransformSize * data.essentialUniforms.modelCount) * data.currentInFlight;

		size_t i = 0;
		for (const auto& spriteID : sprites)
		{
			uniformOffsets[1] = data.essentialUniforms.modelTransformSize * i;
			if (data.currentInFlight > 0)
				uniformOffsets[1] += (data.essentialUniforms.cameraTransformSize + data.essentialUniforms.modelTransformSize * data.essentialUniforms.modelCount);

			//const auto& uniformSize = VkVar::MainUniforms::modelsUniformSize;
			//const auto& offset = frameIndex * 100 * uniformSize + uniformSize * i;

			cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, data.pipelineLayout, 0, descriptorSets, uniformOffsets);

			cmdBuffer.drawIndexed(vbo.indexCount, 1, 0, 0, 0);

			i++;
		}
	}

	cmdBuffer.endRenderPass();

	cmdBuffer.end();
}

void Engine::Renderer::Vulkan::Initialize(void*& apiData)
{
	apiData = new Data();
	auto& data = *static_cast<Data*>(apiData);


	// Can't be multi-threaded start
	if constexpr (enableDebug)
	{
		GetData().availableValidationLayers = vk::enumerateInstanceLayerProperties();
		assert(CheckValidationLayerSupport(GetData().availableValidationLayers));
	}

	LoadInstanceExtensions(data);
	CreateInstance(data);

	if constexpr (enableDebug)
		SetUpDebugCallback(data);
	CreateSurface(data);

	PickPhysicalDevice(data);
	CreateLogicalDevice(data);
	// Can't be multi-threaded end

	//CreateSampler(data);

	CreateSwapchain(data);
	// Only dependant on swapchain, can be multi-threaded
	CreateSyncObjects(data);
	CreateSwapchainImageViews(data);
	CreateDepthResources(data);
	CreateEssentialUniforms(data);
	CreateRenderPass(data);
	CreateCommandPool(data);
	CreateDeletionQueues(data);

	// Framebuffers are dependant on renderpass
	CreateSwapchainFramebuffers(data);

	CreatePipeline(data);

	data.currentInFlight = 0;
	auto imageAvailableSemaphore = data.perInFlightSyncObjects[data.currentInFlight].imageAvailable;
	auto frameIndexOptional = data.device.acquireNextImageKHR(data.swapchain.handle, std::numeric_limits<uint64_t>::max(), imageAvailableSemaphore, nullptr);
	assert(frameIndexOptional.result == vk::Result::eSuccess);
	data.currentFrame = static_cast<uint8_t>(frameIndexOptional.value);
}

void Engine::Renderer::Vulkan::Terminate(void*& apiData)
{
	Data& data = *static_cast<Data*>(apiData);

	data.device.waitIdle();

	// Cleanup VBOs
	{
		for (auto& item : data.vboDatabase)
		{
			data.device.destroyBuffer(item.second.buffer);
			data.device.freeMemory(item.second.deviceMemory);
		}
	}

	data.device.destroyCommandPool(data.commandPool);

	data.device.destroyRenderPass(data.renderPass);

	// Destroy essential uniforms
	{
		data.device.destroyBuffer(data.essentialUniforms.buffer);
		data.device.freeMemory(data.essentialUniforms.deviceMemory);
		data.device.destroyDescriptorPool(data.essentialUniforms.descriptorPool);
		data.device.destroyDescriptorSetLayout(data.essentialUniforms.descriptorSetLayout);
	}


	// Destroy sync objects
	{
		for (auto& item : data.perInFlightSyncObjects)
		{
			data.device.destroySemaphore(item.imageAvailable);
			data.device.destroySemaphore(item.renderFinished);
			data.device.destroyFence(item.inFlight);
		}
	}


	// Destroy swapchain
	{
		// Destroy depth image resources
		data.device.destroyImageView(data.swapchain.depthImage.imageView);
		data.device.destroyImage(data.swapchain.depthImage.image);
		data.device.freeMemory(data.swapchain.depthImage.deviceMemory);

		// Destroy image handles (except images, which are handled automatically by destroying the swapchain itself).
		for (auto& handle : data.swapchain.handles)
		{
			data.device.destroyFramebuffer(handle.framebuffer);
			data.device.destroyImageView(handle.imageView);
		}
		data.device.destroySwapchainKHR(data.swapchain.handle);
	}
	

	// Destroy debug report callback
	if constexpr (enableDebug)
		data.destroyDebugReportCallbackPtr(
				static_cast<VkInstance>(data.instance),
				static_cast<VkDebugReportCallbackEXT>(data.debugReportCallback),
				nullptr);

	// Destroy logical device
	data.device.destroy();

	// Destroy vkSurface
	data.instance.destroySurfaceKHR(data.surface);

	// Destroy Vulkan instance
	data.instance.destroy();

	delete static_cast<Data*>(apiData);
	apiData = nullptr;
}

void Engine::Renderer::Vulkan::UpdateVBOs(Data& data, const std::vector<MeshID>& loadQueue)
{
	for (auto& item : loadQueue)
	{
		using namespace Asset;

		MeshDocument document = Asset::LoadMeshDocument(static_cast<Mesh>(item));

		VBO vbo{};

		vk::BufferUsageFlags bufferUsageFlags;

		// If Host-Visible memory is also Device-Local, then we don't need to schedule any transfer to device-local memory.
		//if (data.hostVisibleIsDeviceLocal == true)
			bufferUsageFlags = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eIndexBuffer;
		//else
		//	bufferUsageFlags = vk::BufferUsageFlagBits::eTransferSrc;

		auto temp = MakeAndBindBuffer(data.device, document.GetTotalByteLength(), bufferUsageFlags, vk::MemoryPropertyFlagBits::eHostVisible);
		
		auto mappedMemory = static_cast<std::byte*>(data.device.mapMemory(temp.second, 0, document.GetTotalByteLength()));

		vbo.attributeSizes[static_cast<size_t>(VBO::Attribute::Position)] = document.GetData(MeshDocument::Attribute::Position).byteLength;
		vbo.attributeSizes[static_cast<size_t>(VBO::Attribute::TexCoord)] = document.GetData(MeshDocument::Attribute::TexCoord).byteLength;
		vbo.attributeSizes[static_cast<size_t>(VBO::Attribute::Normal)] = document.GetData(MeshDocument::Attribute::Normal).byteLength;
		vbo.attributeSizes[static_cast<size_t>(VBO::Attribute::Index)] = document.GetData(MeshDocument::Attribute::Index).byteLength;
		vbo.indexCount = document.GetIndexCount();
		vbo.indexType = document.GetIndexType() == MeshDocument::IndexType::Uint16 ? vk::IndexType::eUint16 : vk::IndexType::eUint32;

		std::memcpy(mappedMemory + vbo.GetByteOffset(VBO::Attribute::Position), document.GetDataPtr(MeshDocument::Attribute::Position), document.GetData(MeshDocument::Attribute::Position).byteLength);
		std::memcpy(mappedMemory + vbo.GetByteOffset(VBO::Attribute::TexCoord), document.GetDataPtr(MeshDocument::Attribute::TexCoord), document.GetData(MeshDocument::Attribute::TexCoord).byteLength);
		std::memcpy(mappedMemory + vbo.GetByteOffset(VBO::Attribute::Normal), document.GetDataPtr(MeshDocument::Attribute::Normal), document.GetData(MeshDocument::Attribute::Normal).byteLength);
		std::memcpy(mappedMemory + vbo.GetByteOffset(VBO::Attribute::Index), document.GetDataPtr(MeshDocument::Attribute::Index), document.GetData(MeshDocument::Attribute::Index).byteLength);

		data.device.unmapMemory(temp.second);

		//if (data.hostVisibleIsDeviceLocal == true)
		//{
			vbo.buffer = temp.first;
			vbo.deviceMemory = temp.second;
		//}
		//else
		//{
		//	assert(false);
		//}

		data.vboDatabase.insert({ item, vbo });
	}
}

void Engine::Renderer::Vulkan::PrepareRendering()
{
	bool updateRequired = false;
}

void Engine::Renderer::Vulkan::UpdateEssentialUniforms(Data& data, uint8_t inFlightIndex)
{
	auto ptr = data.essentialUniforms.mappedMemory;

	// Offset pointer to correct inFlight set
	ptr = ptr + (data.essentialUniforms.cameraTransformSize + data.essentialUniforms.modelTransformSize * data.essentialUniforms.modelCount) * inFlightIndex;

	auto& viewport = GetViewport(0);

	auto temp = Math::Matrix4x4();
	std::memcpy(ptr, &temp, sizeof(temp));

	ptr = ptr + data.essentialUniforms.cameraTransformSize;
}

void Engine::Renderer::Vulkan::MemoryCleanup(Data& data, uint8_t inFlightIndex)
{
	std::swap(data.deletionQueues[data.currentInFlight].deletionJobs[0], data.deletionQueues[data.currentInFlight].deletionJobs[1]);
	auto& deletionQueue = data.deletionQueues[data.currentInFlight].deletionJobs[0];
	for (auto& item : deletionQueue)
	{
		data.device.destroyBuffer(item.buffer);
		data.device.freeMemory(item.memory);
	}
	deletionQueue.clear();
}

void Engine::Renderer::Vulkan::Draw()
{
	Data& data = GetData();

	auto imageAvailableSemaphore = data.perInFlightSyncObjects[data.currentInFlight].imageAvailable;
	auto renderFinishedSemaphore = data.perInFlightSyncObjects[data.currentInFlight].renderFinished;
	auto inFlightFence = data.perInFlightSyncObjects[data.currentInFlight].inFlight;

	// Wait for ready image
	vk::ArrayProxy<const vk::Fence> waitFences = { inFlightFence };
	data.device.waitForFences(waitFences, VK_TRUE, std::numeric_limits<uint64_t>::max());
	data.device.resetFences(waitFences);

	// Update render uniforms
	UpdateEssentialUniforms(data, data.currentInFlight);

	MemoryCleanup(data, data.currentInFlight);
	
	
	auto newImageAvailableSemaphore = data.perInFlightSyncObjects[data.currentInFlight].imageAvailable;
	auto frameIndexOptional = data.device.acquireNextImageKHR(data.swapchain.handle, std::numeric_limits<uint64_t>::max(), newImageAvailableSemaphore, nullptr);
	assert(frameIndexOptional.result == vk::Result::eSuccess);
	data.currentFrame = static_cast<uint8_t>(frameIndexOptional.value);

	// Re-build command buffers (if needed).
	HandleGraphicsCommandBuffers();


	// Submit render
	vk::SubmitInfo renderSubmitInfo;
	renderSubmitInfo.waitSemaphoreCount = 1;
	renderSubmitInfo.pWaitSemaphores = &imageAvailableSemaphore;
	vk::PipelineStageFlags renderWaitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
	renderSubmitInfo.pWaitDstStageMask = renderWaitStages;
	renderSubmitInfo.signalSemaphoreCount = 1;
	renderSubmitInfo.pSignalSemaphores = &renderFinishedSemaphore;
	renderSubmitInfo.commandBufferCount = 1;
	renderSubmitInfo.pCommandBuffers = &data.primaryCommandBuffers[data.currentFrame].handle;
	data.graphicsQueue.submit({ renderSubmitInfo }, data.perInFlightSyncObjects[data.currentInFlight].inFlight);

	// Submit presentation
	uint32_t swapchainFrameIndex = data.currentFrame;
	vk::PresentInfoKHR presentInfo;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &renderFinishedSemaphore;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &data.swapchain.handle;
	presentInfo.pImageIndices = &swapchainFrameIndex;
	auto presentResult = data.graphicsQueue.presentKHR(presentInfo);
	assert(presentResult == vk::Result::eSuccess);

	data.currentInFlight = (data.currentInFlight + uint8_t(1)) % data.swapchain.numInFlight;
}




std::vector<char> readFile(const std::string& filename) 
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		throw std::runtime_error("failed to open file!");
	}

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();

	return buffer;
}

vk::ShaderModule createShaderModule(const std::vector<char>& code)
{
	vk::ShaderModuleCreateInfo createInfo{};
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	vk::ShaderModule shaderModule = Engine::Renderer::Vulkan::GetData().device.createShaderModule(createInfo);
	assert(shaderModule);
	return shaderModule;
}

std::array<vk::VertexInputBindingDescription, 3> getBindingDescription()
{
	std::array<vk::VertexInputBindingDescription, 3> bindingDescriptions;
	bindingDescriptions[0].binding = 0;
	bindingDescriptions[0].stride = sizeof(float) * 3;
	bindingDescriptions[0].inputRate = vk::VertexInputRate::eVertex;

	bindingDescriptions[1].binding = 1;
	bindingDescriptions[1].stride = sizeof(float) * 2;
	bindingDescriptions[1].inputRate = vk::VertexInputRate::eVertex;

	bindingDescriptions[2].binding = 2;
	bindingDescriptions[2].stride = sizeof(float) * 3;
	bindingDescriptions[2].inputRate = vk::VertexInputRate::eVertex;

	return bindingDescriptions;
}

std::array<vk::VertexInputAttributeDescription, 3> getAttributeDescriptions() 
{
	std::array<vk::VertexInputAttributeDescription, 3> attributeDescriptions;

	attributeDescriptions[0].binding = 0;
	attributeDescriptions[0].location = 0;
	attributeDescriptions[0].format = vk::Format::eR32G32B32A32Sfloat;

	attributeDescriptions[1].binding = 1;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format = vk::Format::eR32G32Sfloat;

	attributeDescriptions[2].binding = 2;
	attributeDescriptions[2].location = 2;
	attributeDescriptions[2].format = vk::Format::eR32G32B32A32Sfloat;

	return attributeDescriptions;
}

void Engine::Renderer::Vulkan::CreatePipeline(Data& data)
{
	vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &data.essentialUniforms.descriptorSetLayout;

	data.pipelineLayout = data.device.createPipelineLayout(pipelineLayoutInfo);
	assert(data.pipelineLayout);

	auto vertShaderCode = readFile("Data/Shaders/Mesh/Vulkan/vert.spv");
	auto fragShaderCode = readFile("Data/Shaders/Mesh/Vulkan/frag.spv");

	vk::ShaderModule vertShaderModule = createShaderModule(vertShaderCode);
	vk::ShaderModule fragShaderModule = createShaderModule(fragShaderCode);

	vk::PipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";

	vk::PipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";

	vk::PipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	
	

	vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};
	auto bindingDescription = getBindingDescription();
	vertexInputInfo.vertexBindingDescriptionCount = bindingDescription.size();
	vertexInputInfo.pVertexBindingDescriptions = bindingDescription.data();
	auto attributeDescriptions = getAttributeDescriptions();
	vertexInputInfo.vertexAttributeDescriptionCount = attributeDescriptions.size();
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	vk::PipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	vk::Viewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(data.swapchain.extent.width);
	viewport.height = static_cast<float>(data.swapchain.extent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	vk::Rect2D scissor {};
	scissor.offset = vk::Offset2D{ 0, 0 };
	scissor.extent = data.swapchain.extent;

	vk::PipelineViewportStateCreateInfo viewportState{};
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	vk::PipelineRasterizationStateCreateInfo rasterizer;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = vk::PolygonMode::eFill;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = vk::CullModeFlagBits::eNone;
	rasterizer.frontFace = vk::FrontFace::eClockwise;
	rasterizer.depthBiasEnable = VK_FALSE;

	vk::PipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;

	vk::PipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
	colorBlendAttachment.blendEnable = VK_FALSE;

	vk::PipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = vk::LogicOp::eCopy;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	vk::PipelineDepthStencilStateCreateInfo depthStencil = {};
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = vk::CompareOp::eLess;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.stencilTestEnable = VK_FALSE;

	

	vk::GraphicsPipelineCreateInfo pipelineInfo;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.layout = data.pipelineLayout;
	pipelineInfo.renderPass = data.renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = nullptr;

	pipelineInfo.pDepthStencilState = &depthStencil;

	data.pipeline = data.device.createGraphicsPipeline(nullptr, pipelineInfo);
	assert(data.pipeline);

	data.device.destroyShaderModule(fragShaderModule);
	data.device.destroyShaderModule(vertShaderModule);
}