#include "Vulkan.hpp"

#include "VulkanData.hpp"
#include "Init.hpp"

#include "volk/volk.hpp"
#include <vulkan/vulkan.hpp>

#include <cassert>
#include <cstdint>
#include <array>
#include <vector>
#include <fstream>

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

DRenderer::Vulkan::SwapchainSettings DRenderer::Vulkan::GetSwapchainSettings(vk::PhysicalDevice device, vk::SurfaceKHR surface)
{
	SwapchainSettings settings{};

	// Query surface capabilities
	auto capabilities = device.getSurfaceCapabilitiesKHR(surface);
	settings.capabilities = capabilities;

	// Handle swapchain length
	constexpr uint8_t preferredSwapchainLength = 3;
	settings.numImages = preferredSwapchainLength;
	if (settings.numImages > capabilities.maxImageCount && capabilities.maxImageCount != 0)
		settings.numImages = capabilities.maxImageCount;
	if (settings.numImages < 2)
	{
		Core::LogDebugMessage("Error. Maximum swapchain-length cannot be less than 2.");
		std::abort();
	}

	auto presentModes = device.getSurfacePresentModesKHR(surface);
	vk::PresentModeKHR presentMode = vk::PresentModeKHR::eFifo;
	settings.presentMode = presentMode;

	// Handle formats
	constexpr std::array preferredFormats
	{
		vk::Format::eR8G8B8A8Unorm,
		vk::Format::eB8G8R8A8Unorm
	};
	auto formats = device.getSurfaceFormatsKHR(surface);
	vk::SurfaceFormatKHR formatToUse = { vk::Format::eUndefined, vk::ColorSpaceKHR::eSrgbNonlinear };
	bool foundPreferredFormat = false;
	for (const auto& preferredFormat : preferredFormats)
	{
		for (const auto& format : formats)
		{
			if (format.format == preferredFormat)
			{
				formatToUse.format = preferredFormat;
				foundPreferredFormat = true;
				break;
			}
		}
	}
	if constexpr (Core::debugLevel >= 2)
	{
		if (!foundPreferredFormat)
		{
			Core::LogDebugMessage("Found no usable surface formats.");
			std::abort();
		}
	}
	settings.surfaceFormat = formatToUse;

	return settings;
}

namespace DRenderer::Vulkan
{
	void MakePipeline();
}


void DRenderer::Vulkan::Initialize(Core::APIDataPointer& apiData, InitInfo& initInfo)
{
	// Load the first function pointers.
	Volk::InitializeCustom((PFN_vkGetInstanceProcAddr)initInfo.getInstanceProcAddr());

	apiData.data = new APIData();
	apiData.deleterPfn = &APIDataDeleter;
	APIData& data = *static_cast<APIData*>(apiData.data);
	data.initInfo = initInfo;

	// Start initialization
	data.apiVersion = Init_LoadAPIVersion();
	data.instanceExtensionProperties = Init_LoadInstanceExtensionProperties();
	if constexpr (Core::debugLevel >= 2)
		data.instanceLayerProperties = Init_LoadInstanceLayerProperties();

	// Create our instance
	vk::ApplicationInfo appInfo{};
	appInfo.apiVersion = VK_MAKE_VERSION(data.apiVersion.major, data.apiVersion.minor, data.apiVersion.patch);
	appInfo.pApplicationName = "DEngine";
	appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
	appInfo.pEngineName = "DEngine";
	appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
	auto wsiRequiredExtensions = data.initInfo.getRequiredInstanceExtensions();
	data.instance = Init_CreateInstance(data.instanceExtensionProperties, data.instanceLayerProperties, appInfo, wsiRequiredExtensions);

	// Load function pointers for our instance.
	Volk::LoadInstance(data.instance);

	// Enable debugging only if we are in debug config, and the user requested debugging.
	if constexpr (Core::debugLevel >= 2)
	{
		if (Core::GetData().debugData.useDebugging == true)
			data.debugMessenger = Init_CreateDebugMessenger(data.instance);
	}

	// Create our surface we will render onto
	void* hwnd = Engine::Renderer::GetViewport(0).GetSurfaceHandle();
	data.surface = Init_CreateSurface(data.instance, hwnd);

	// Find physical device
	data.physDevice = Init_LoadPhysDevice(data.instance, data.surface);

	// Get swapchain creation details
	SwapchainSettings swapchainSettings = GetSwapchainSettings(data.physDevice.handle, data.surface);
	data.surfaceExtents = vk::Extent2D{ swapchainSettings.capabilities.currentExtent.width, swapchainSettings.capabilities.currentExtent.height };


	// Create logical device
	auto queues = data.physDevice.handle.getQueueFamilyProperties();

	uint32_t graphicsFamily = 0;
	uint32_t graphicsQueue = 0;

	vk::DeviceCreateInfo createInfo{};

	// Feature configuration
	auto physDeviceFeatures = data.physDevice.handle.getFeatures();

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

	createInfo.queueCreateInfoCount = uint32_t(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();

	auto deviceExtensionsAvailable = data.physDevice.handle.enumerateDeviceExtensionProperties();
	// TODO: Check if all required device extensions are available
	createInfo.ppEnabledExtensionNames = Vulkan::requiredDeviceExtensions.data();
	createInfo.enabledExtensionCount = uint32_t(Vulkan::requiredDeviceExtensions.size());

	//auto deviceLayersAvailable = data.physDevice.handle.enumerateDeviceLayerProperties();
	//createInfo.enabledLayerCount = uint32_t(Vulkan::requiredDeviceLayers.size());
	//createInfo.ppEnabledLayerNames = Vulkan::requiredDeviceLayers.data();

	auto result = data.physDevice.handle.createDevice(createInfo);
	if (!result)
		std::abort();
	data.device = result;

	// Load function pointers for our device.
	Volk::LoadDevice(data.device);

	// Create our Render Target
	data.renderTarget = Init_CreateRenderTarget(data.device, data.surfaceExtents, swapchainSettings.surfaceFormat.format);

	// Set up main renderpass
	data.renderPass = Init_CreateMainRenderPass(data.device, vk::SampleCountFlagBits::e1, swapchainSettings.surfaceFormat.format);

	data.renderTarget.framebuffer = Init_CreateRenderTargetFramebuffer(data.device, data.surfaceExtents, data.renderTarget.imageView, data.renderPass);


	// Set up swapchain
	data.swapchain = Init_CreateSwapchain(data.device, data.surface, swapchainSettings);

	Init_SetupPresentCmdBuffers(data.device, data.renderTarget.image, data.surfaceExtents, data.swapchain.images, data.presentCmdPool, data.presentCmdBuffer);

	Init_SetupRenderingCmdBuffers(data.device, data.swapchain.length, data.renderCmdPool, data.renderCmdBuffer);

	data.graphicsQueue = data.device.getQueue(graphicsFamily, graphicsQueue);

	Init_TransitionRenderTargetAndSwapchain(data.device, data.graphicsQueue, data.renderTarget.image, data.swapchain.images);

	data.imageAvailableForPresentation.resize(data.swapchain.length);
	data.renderCmdBufferAvailable.resize(data.swapchain.length);
	for (size_t i = 0; i < data.swapchain.length; i++)
	{
		vk::FenceCreateInfo createInfo1{};
		createInfo1.flags = vk::FenceCreateFlagBits::eSignaled;
	
		data.renderCmdBufferAvailable[i] = data.device.createFence(createInfo1);

		data.imageAvailableForPresentation[i] = data.device.createFence({});
	}

	auto imageResult = data.device.acquireNextImageKHR(data.swapchain.handle, std::numeric_limits<uint64_t>::max(), vk::Semaphore(), data.imageAvailableForPresentation[data.imageAvailableActiveIndex]);
	if constexpr (Core::debugLevel >= 2)
	{
		if (imageResult.result != vk::Result::eSuccess)
		{
			Core::LogDebugMessage("Result of vkAcquireNextImageKHR was not successful.");
			std::abort();
		}
	}
	data.swapchain.currentImage = imageResult.value;

	MakePipeline();
}

void DRenderer::Vulkan::PrepareRenderingEarly(const Core::PrepareRenderingEarlyParams& in)
{
	APIData& data = GetAPIData();

	data.device.waitForFences(data.renderCmdBufferAvailable[data.swapchain.currentImage], 1, std::numeric_limits<uint64_t>::max());
	data.device.resetFences(data.renderCmdBufferAvailable[data.swapchain.currentImage]);

	auto cmdBuffer = data.renderCmdBuffer[data.swapchain.currentImage];

	vk::CommandBufferBeginInfo beginInfo{};
	beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

	cmdBuffer.begin(beginInfo);

	vk::RenderPassBeginInfo renderPassBeginInfo{};
	renderPassBeginInfo.renderPass = data.renderPass;
	renderPassBeginInfo.framebuffer = data.renderTarget.framebuffer;

	vk::ClearValue clearValue;
	clearValue.color.float32[0] = 0.25f;
	clearValue.color.float32[1] = 0.f;
	clearValue.color.float32[2] = 0.f;
	clearValue.color.float32[3] = 1.f;
	renderPassBeginInfo.clearValueCount = 1;
	renderPassBeginInfo.pClearValues = &clearValue;

	vk::Rect2D renderArea{};
	renderArea.extent = data.surfaceExtents;
	renderPassBeginInfo.renderArea = renderArea;

	cmdBuffer.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);

	cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, data.pipeline);

	cmdBuffer.draw(3, 1, 0, 0);

	cmdBuffer.endRenderPass();

	cmdBuffer.end();
}

void DRenderer::Vulkan::PrepareRenderingLate()
{
}

void DRenderer::Vulkan::Draw()
{
	APIData& data = GetAPIData();

	data.device.waitForFences(
			data.imageAvailableForPresentation[data.imageAvailableActiveIndex],
			1,
			std::numeric_limits<uint64_t>::max());
	data.device.resetFences(data.imageAvailableForPresentation[data.imageAvailableActiveIndex]);
	
	vk::SubmitInfo renderSubmitInfo{};

	vk::SubmitInfo submitInfo{};
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &data.renderCmdBuffer[data.swapchain.currentImage];
	data.graphicsQueue.submit(submitInfo, data.renderCmdBufferAvailable[data.swapchain.currentImage]);

	vk::SubmitInfo submitInfo2{};
	submitInfo2.commandBufferCount = 1;
	submitInfo2.pCommandBuffers = &data.presentCmdBuffer[data.swapchain.currentImage];
	data.graphicsQueue.submit(submitInfo2, {});

	vk::PresentInfoKHR presentInfo{};
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &data.swapchain.handle;

	uint32_t swapchainIndex = data.swapchain.currentImage;
	presentInfo.pImageIndices = &swapchainIndex;

	vk::Result presentResult{};
	presentInfo.pResults = &presentResult;

	data.graphicsQueue.presentKHR(presentInfo);

	if constexpr (Core::debugLevel >= 2)
	{
		if (presentResult != vk::Result::eSuccess)
		{
			Core::LogDebugMessage("Presentation result of index " + std::to_string(swapchainIndex) + " was not success.");
			std::abort();
		}
	}

	data.imageAvailableActiveIndex = (data.imageAvailableActiveIndex + 1) % data.swapchain.length;
	auto imageResult = data.device.acquireNextImageKHR(
			data.swapchain.handle,
			std::numeric_limits<uint64_t>::max(),
			{},
			data.imageAvailableForPresentation[data.imageAvailableActiveIndex]);
	if constexpr (Core::debugLevel >= 2)
	{
		if (imageResult.result != vk::Result::eSuccess)
		{
			Core::LogDebugMessage("Result of vkAcquireNextImageKHR was not successful.");
			std::abort();
		}
	}
	data.swapchain.currentImage = imageResult.value;
}

void DRenderer::Vulkan::MakePipeline()
{
	APIData& data = GetAPIData();

	vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
	data.pipelineLayout = data.device.createPipelineLayout(pipelineLayoutCreateInfo);

	std::ifstream vertFile("data/Shaders/VulkanTest/vert.spv", std::ios::ate | std::ios::binary);
	if (!vertFile.is_open())
		std::abort();

	std::vector<uint8_t> vertCode(vertFile.tellg());
	vertFile.seekg(0);
	vertFile.read(reinterpret_cast<char*>(vertCode.data()), vertCode.size());
	vertFile.close();

	vk::ShaderModuleCreateInfo vertModCreateInfo{};
	vertModCreateInfo.codeSize = vertCode.size();
	vertModCreateInfo.pCode = reinterpret_cast<const uint32_t*>(vertCode.data());

	vk::ShaderModule vertModule = data.device.createShaderModule(vertModCreateInfo);

	vk::PipelineShaderStageCreateInfo vertStageCreateInfo{};
	vertStageCreateInfo.stage = vk::ShaderStageFlagBits::eVertex;
	vertStageCreateInfo.module = vertModule;
	vertStageCreateInfo.pName = "main";

	std::ifstream fragFile("data/Shaders/VulkanTest/frag.spv", std::ios::ate | std::ios::binary);
	if (!fragFile.is_open())
		std::abort();

	std::vector<uint8_t> fragCode(fragFile.tellg());
	fragFile.seekg(0);
	fragFile.read(reinterpret_cast<char*>(fragCode.data()), fragCode.size());
	fragFile.close();

	vk::ShaderModuleCreateInfo fragModCreateInfo{};
	fragModCreateInfo.codeSize = fragCode.size();
	fragModCreateInfo.pCode = reinterpret_cast<const uint32_t*>(fragCode.data());

	vk::ShaderModule fragModule = data.device.createShaderModule(fragModCreateInfo);

	vk::PipelineShaderStageCreateInfo fragStageCreateInfo{};
	fragStageCreateInfo.stage = vk::ShaderStageFlagBits::eFragment;
	fragStageCreateInfo.module = fragModule;
	fragStageCreateInfo.pName = "main";

	std::array shaderStages = { vertStageCreateInfo, fragStageCreateInfo };


	vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};

	vk::PipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;


	vk::Viewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)data.surfaceExtents.width;
	viewport.height = (float)data.surfaceExtents.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vk::Rect2D scissor{};
	scissor.offset = vk::Offset2D{ 0, 0 };
	scissor.extent = data.surfaceExtents;
	vk::PipelineViewportStateCreateInfo viewportState{};
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	vk::PipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.lineWidth = 1.f;
	rasterizer.polygonMode = vk::PolygonMode::eFill;

	vk::PipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;

	vk::PipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

	vk::PipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;

	vk::GraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.layout = data.pipelineLayout;
	pipelineInfo.renderPass = data.renderPass;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages.data();


	data.pipeline = data.device.createGraphicsPipeline({}, pipelineInfo);
}
