#include "Vulkan.hpp"

#include "VulkanData.hpp"
#include "Init.hpp"
#include "AssetSystem.hpp"

#include "volk/volk.hpp"
#include <vulkan/vulkan.hpp>

#include <cassert>
#include <cstdint>
#include <array>
#include <vector>
#include <fstream>

void DRenderer::Vulkan::APIDataDeleter(void*& ptr)
{
	auto data = static_cast<APIData*>(ptr);
	ptr = nullptr;
	delete data;
}

DRenderer::Vulkan::APIData& DRenderer::Vulkan::GetAPIData()
{
	return *static_cast<APIData*>(DRenderer::Core::GetAPIData());
}

std::array<vk::VertexInputBindingDescription, 3> DRenderer::Vulkan::GetVertexBindings()
{
	std::array<vk::VertexInputBindingDescription, 3> bindingDescriptions{};
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

std::array<vk::VertexInputAttributeDescription, 3> DRenderer::Vulkan::GetVertexAttributes()
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

namespace DRenderer::Vulkan
{
	QueueInfo LoadQueueInfo(vk::Device device, const QueueInfo& queuesToLoad);
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
	data.apiVersion = Init::LoadAPIVersion();
	data.instanceExtensionProperties = Init::LoadInstanceExtensionProperties();
	assert(!data.instanceExtensionProperties.empty());
	if constexpr (Core::debugLevel >= 2)
		data.instanceLayerProperties = Init::LoadInstanceLayerProperties();

	// Create our instance
	vk::ApplicationInfo appInfo{};
	appInfo.apiVersion = VK_MAKE_VERSION(data.apiVersion.major, data.apiVersion.minor, data.apiVersion.patch);
	appInfo.pApplicationName = "DEngine";
	appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
	appInfo.pEngineName = "DEngine";
	appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
	auto wsiRequiredExtensions = data.initInfo.getRequiredInstanceExtensions();
	data.instance = Init::CreateInstance(data.instanceExtensionProperties, data.instanceLayerProperties, appInfo, wsiRequiredExtensions);

	// Load function pointers for our instance.
	Volk::LoadInstance(data.instance);

	// Enable debugging only if we are in debug config, and the user requested debugging.
	if constexpr (Core::debugLevel >= 2)
	{
		if (Core::GetData().debugData.useDebugging == true)
			data.debugMessenger = Init::CreateDebugMessenger(data.instance);
	}

	// Create our surface we will render onto
	void* hwnd = Engine::Renderer::GetViewport(0).GetSurfaceHandle();
	data.surface = Init::CreateSurface(data.instance, hwnd);

	// Find physical device
	data.physDevice = Init::LoadPhysDevice(data.instance, data.surface);

	// Get swapchain creation details
	SwapchainSettings swapchainSettings = Init::GetSwapchainSettings(data.physDevice.handle, data.surface);
	data.surfaceExtents = vk::Extent2D{ swapchainSettings.capabilities.currentExtent.width, swapchainSettings.capabilities.currentExtent.height };

	data.device = Init::CreateDevice(data.physDevice);

	// Load function pointers for our device.
	Volk::LoadDevice(data.device);

	MainUniforms::Init_CreateDescriptorSetAndPipelineLayout(data.mainUniforms, data.device);

	// Create our Render Target
	data.renderTarget = Init::CreateRenderTarget(data.device, data.surfaceExtents, swapchainSettings.surfaceFormat.format, data.physDevice.maxFramebufferSamples);

	// Set up main renderpass
	data.renderPass = Init::CreateMainRenderPass(data.device, data.renderTarget.sampleCount, swapchainSettings.surfaceFormat.format);

	data.renderTarget.framebuffer = Init::CreateRenderTargetFramebuffer(
			data.device,
			data.renderPass,
			data.surfaceExtents,
			data.renderTarget.colorImgView,
			data.renderTarget.depthImgView);

	// Set up swapchain
	data.swapchain = Init::CreateSwapchain(data.device, data.surface, swapchainSettings);
	data.resourceSetCount = data.swapchain.length - 1;

	// Setup deletion queue
	data.deletionQueue.device = data.device;

	MainUniforms::Init_AllocateDescriptorSets(data.mainUniforms, data.resourceSetCount);

	MainUniforms::Init_BuildBuffers(data.mainUniforms, data.physDevice.memInfo, data.physDevice.memProperties, data.physDevice.properties.limits);

	MainUniforms::Init_ConfigureDescriptors(data.mainUniforms);

	// Set up presentation cmd buffers
	Init::SetupPresentCmdBuffers(data.device, data.renderTarget, data.surfaceExtents, data.swapchain.images, data.presentCmdPool, data.presentCmdBuffer);

	// Allocate rendering cmd buffers.
	Init::SetupRenderingCmdBuffers(data.device, data.swapchain.length, data.renderCmdPool, data.renderCmdBuffer);

	// Load all queues
	data.queues = LoadQueueInfo(data.device, data.physDevice.preferredQueues);

	AssetSystem::Init(data.device, data.queues, data.resourceSetCount, data.assetSystem);

	// Transition layouts of our render target and swapchain
	Init::TransitionRenderTargetAndSwapchain(data.device, data.queues.graphics.handle, data.renderTarget, data.swapchain.images);

	data.resourceSetAvailable.resize(data.resourceSetCount);
	for (size_t i = 0; i < data.resourceSetCount; i++)
	{
		vk::FenceCreateInfo createInfo{};
		createInfo.flags = vk::FenceCreateFlagBits::eSignaled;
		data.resourceSetAvailable[i] = data.device.createFence(createInfo);
	}

	data.swapchainImageAvailable.resize(data.swapchain.length);
	for (size_t i = 0; i < data.swapchain.length; i++)
	{
		vk::SemaphoreCreateInfo info{};
		data.swapchainImageAvailable[i] = data.device.createSemaphore(info);
	}

	auto imageResult = data.device.acquireNextImageKHR(data.swapchain.handle, std::numeric_limits<uint64_t>::max(), data.swapchainImageAvailable[data.imageAvailableActiveIndex], {});
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

DRenderer::Vulkan::QueueInfo DRenderer::Vulkan::LoadQueueInfo(vk::Device device, const QueueInfo& queuesToLoad)
{
	APIData& data = GetAPIData();

	QueueInfo qInfo = queuesToLoad;

	qInfo.graphics.handle = device.getQueue(qInfo.graphics.familyIndex, qInfo.graphics.queueIndex);

	if (qInfo.TransferIsSeparate())
		qInfo.transfer.handle = device.getQueue(qInfo.transfer.familyIndex, qInfo.transfer.queueIndex);
	else
		qInfo.transfer.handle = qInfo.graphics.handle;

	if constexpr (Core::debugLevel >= 2)
	{
		if (Core::GetData().debugData.useDebugging)
		{
			// Name our render target frame
			vk::DebugUtilsObjectNameInfoEXT debugObjectNameInfo{};
			debugObjectNameInfo.objectType = vk::ObjectType::eQueue;
			debugObjectNameInfo.objectHandle = (uint64_t)(VkQueue)qInfo.graphics.handle;
			debugObjectNameInfo.pObjectName = "Render Queue";
			device.setDebugUtilsObjectNameEXT(debugObjectNameInfo);

			if (qInfo.TransferIsSeparate())
			{
				debugObjectNameInfo.objectType = vk::ObjectType::eQueue;
				debugObjectNameInfo.objectHandle = (uint64_t)(VkQueue)qInfo.transfer.handle;
				debugObjectNameInfo.pObjectName = "Transfer Queue";
				device.setDebugUtilsObjectNameEXT(debugObjectNameInfo);
			}
		}
	}

	return qInfo;
}

void DRenderer::Vulkan::PrepareRenderingEarly(const Core::PrepareRenderingEarlyParams& in)
{
	APIData& data = GetAPIData();
	data.deletionQueue.CleanupCurrentFrame();

	// Reconfigure object model matrix buffer if necessary
	MainUniforms::Update(data.mainUniforms, in.meshLoadQueue->size(), data.deletionQueue);

	// Load all assets
	AssetSystem::LoadAssets(data.assetSystem, data.physDevice.memInfo, data.queues, data.deletionQueue, in);

	const Core::Data& drendererData = Core::GetData();

	auto cmdBuffer = data.renderCmdBuffer[data.imageAvailableActiveIndex];

	vk::CommandBufferBeginInfo beginInfo{};
	beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

	cmdBuffer.begin(beginInfo);

	vk::RenderPassBeginInfo renderPassBeginInfo{};
	renderPassBeginInfo.renderPass = data.renderPass;
	renderPassBeginInfo.framebuffer = data.renderTarget.framebuffer;

	vk::ClearValue colorClearValue{};
	colorClearValue.color.float32[0] = 0.25f;
	colorClearValue.color.float32[1] = 0.f;
	colorClearValue.color.float32[2] = 0.f;
	colorClearValue.color.float32[3] = 1.f;

	vk::ClearValue depthClearValue{};
	depthClearValue.depthStencil.setDepth(1.f);

	std::array clearValues { colorClearValue, depthClearValue };

	renderPassBeginInfo.clearValueCount = uint32_t(clearValues.size());
	renderPassBeginInfo.pClearValues = clearValues.data();

	vk::Rect2D renderArea{};
	renderArea.extent = data.surfaceExtents;
	renderPassBeginInfo.renderArea = renderArea;

	cmdBuffer.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);

	cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, data.pipeline);

	for (size_t i = 0; i < drendererData.renderGraph.meshes.size(); i++)
	{
		// Bind VBO
		auto vboIter = data.assetSystem.vboDatabase.find(drendererData.renderGraph.meshes[i].meshID);

		if constexpr (Core::debugLevel >= 2)
		{
			if (Core::GetData().debugData.useDebugging)
			{
				if (vboIter == data.assetSystem.vboDatabase.end())
				{
					Core::LogDebugMessage("Fatal error. Couldn't find designated VBO in VBO database.");
					std::abort();
				}
			}
		}

		VBO& vbo = vboIter->second;
		std::array<vk::Buffer, 3> vertexBuffers{ vbo.buffer, vbo.buffer, vbo.buffer };
		std::array<vk::DeviceSize, 3> vboOffsets
		{
			vbo.GetByteOffset(VBO::Attribute::Position),
			vbo.GetByteOffset(VBO::Attribute::TexCoord),
			vbo.GetByteOffset(VBO::Attribute::Normal)
		};
		cmdBuffer.bindVertexBuffers(0, vertexBuffers, vboOffsets);
		cmdBuffer.bindIndexBuffer(vbo.buffer, vbo.GetByteOffset(VBO::Attribute::Index), vbo.indexType);

		cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, data.pipelineLayout, 0, data.mainUniforms.descrSets[data.currentResourceSet], data.mainUniforms.GetObjectDataDynamicOffset(i));
		cmdBuffer.drawIndexed(vbo.indexCount, 1, 0, 0, 0);
	}

	cmdBuffer.endRenderPass();

	cmdBuffer.end();
}

void DRenderer::Vulkan::PrepareRenderingLate()
{
	APIData& data = GetAPIData();

	data.device.waitForFences(data.resourceSetAvailable[data.currentResourceSet], 1, std::numeric_limits<uint64_t>::max());
	data.device.resetFences(data.resourceSetAvailable[data.currentResourceSet]);

	const auto& viewport = Engine::Renderer::GetViewport(0);
	const auto& renderGraphTransform = Engine::Renderer::Core::GetRenderGraphTransform();

	// Update camera UBO
	const auto& cameraInfo = Engine::Renderer::Core::GetCameraInfo();
	uint8_t* cameraBufferPtr = data.mainUniforms.GetCameraBufferResourceSet(data.currentResourceSet);
	auto cameraMatrix = cameraInfo.GetModel(viewport.GetDimensions().GetAspectRatio());
	std::memcpy(cameraBufferPtr, &cameraMatrix, sizeof(cameraMatrix));

	vk::MappedMemoryRange cameraRange{};
	cameraRange.memory = data.mainUniforms.cameraBuffersMem;
	cameraRange.offset = data.mainUniforms.GetCameraResourceSetOffset(data.currentResourceSet);
	cameraRange.size = data.mainUniforms.GetCameraResourceSetSize();


	uint8_t* modelUBOBufferPtr = data.mainUniforms.GetObjectDataResourceSet(data.currentResourceSet);

	for (size_t i = 0; i < renderGraphTransform.meshes.size(); i++)
	{
		uint8_t* uboPtr = modelUBOBufferPtr + data.mainUniforms.GetObjectDataDynamicOffset(i);
		const auto& transform = renderGraphTransform.meshes[i];

		std::memcpy(uboPtr, transform.data(), sizeof(transform));
	}

	vk::MappedMemoryRange modelUBORange{};
	modelUBORange.memory = data.mainUniforms.objectDataMemory;
	modelUBORange.size = data.mainUniforms.GetObjectDataResourceSetSize();
	modelUBORange.offset = data.mainUniforms.GetObjectDataResourceSetOffset(data.currentResourceSet);

	data.device.flushMappedMemoryRanges({ cameraRange, modelUBORange });
}

void DRenderer::Vulkan::Draw()
{
	APIData& data = GetAPIData();

	// Queue rendering
	
	vk::SubmitInfo renderImageSubmit{};
	renderImageSubmit.commandBufferCount = 1;
	renderImageSubmit.pCommandBuffers = &data.renderCmdBuffer[data.swapchain.currentImage];

	// Build wait semaphore array for render submit
	auto assetSystemWaits = AssetSystem::GetWaitSemaphores(data.assetSystem);
	renderImageSubmit.waitSemaphoreCount = uint32_t(assetSystemWaits.first.size());
	renderImageSubmit.pWaitSemaphores = assetSystemWaits.first.data();
	renderImageSubmit.pWaitDstStageMask = assetSystemWaits.second.data();

	data.queues.graphics.handle.submit(renderImageSubmit, data.resourceSetAvailable[data.currentResourceSet]);

	AssetSystem::SubmittedFrame(data.assetSystem);

	// Queue rendertarget to swapchain image copy
	vk::SubmitInfo resolveImageSubmit{};
	resolveImageSubmit.commandBufferCount = 1;
	resolveImageSubmit.pCommandBuffers = &data.presentCmdBuffer[data.swapchain.currentImage];

	resolveImageSubmit.waitSemaphoreCount = 1;
	resolveImageSubmit.pWaitSemaphores = &data.swapchainImageAvailable[data.imageAvailableActiveIndex];
	vk::PipelineStageFlags semaphoreWaitFlag = vk::PipelineStageFlagBits::eBottomOfPipe;
	resolveImageSubmit.pWaitDstStageMask = &semaphoreWaitFlag;

	data.queues.graphics.handle.submit(resolveImageSubmit, {});

	vk::PresentInfoKHR presentInfo{};
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &data.swapchain.handle;

	uint32_t swapchainIndex = data.swapchain.currentImage;
	presentInfo.pImageIndices = &swapchainIndex;

	vk::Result presentResult{};
	presentInfo.pResults = &presentResult;

	data.queues.graphics.handle.presentKHR(presentInfo);

	if constexpr (Core::debugLevel >= 2)
	{
		if (presentResult != vk::Result::eSuccess)
		{
			Core::LogDebugMessage("Presentation result of index " + std::to_string(swapchainIndex) + " was not success.");
			std::abort();
		}
	}

	// Linearly increment index-counters
	data.currentResourceSet = (data.currentResourceSet + 1) % data.resourceSetCount;
	data.imageAvailableActiveIndex = (data.imageAvailableActiveIndex + 1) % data.swapchain.length;

	// Acquire next image
	auto imageResult = data.device.acquireNextImageKHR(
			data.swapchain.handle,
			std::numeric_limits<uint64_t>::max(),
			data.swapchainImageAvailable[data.imageAvailableActiveIndex],
			{});
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
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pipelineLayoutCreateInfo.pSetLayouts = &data.mainUniforms.descrSetLayout;

	data.pipelineLayout = data.device.createPipelineLayout(pipelineLayoutCreateInfo);

	std::ifstream vertFile("data/Shaders/VulkanTest/vert.spv", std::ios::ate | std::ios::binary);
	if (!vertFile.is_open())
	{
		std::abort();
	}

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
	auto vertexBindingDescriptions = GetVertexBindings();
	vertexInputInfo.vertexBindingDescriptionCount = uint32_t(vertexBindingDescriptions.size());
	vertexInputInfo.pVertexBindingDescriptions = vertexBindingDescriptions.data();
	auto vertexAttributes = GetVertexAttributes();
	vertexInputInfo.vertexAttributeDescriptionCount = uint32_t(vertexAttributes.size());
	vertexInputInfo.pVertexAttributeDescriptions = vertexAttributes.data();

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
	rasterizer.frontFace = vk::FrontFace::eCounterClockwise;

	vk::PipelineDepthStencilStateCreateInfo depthStencilInfo{};
	depthStencilInfo.depthTestEnable = 1;
	depthStencilInfo.depthCompareOp = vk::CompareOp::eLess;
	depthStencilInfo.stencilTestEnable = 0;
	depthStencilInfo.depthWriteEnable = 1;
	depthStencilInfo.minDepthBounds = 0.f;
	depthStencilInfo.maxDepthBounds = 1.f;

	vk::PipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sampleShadingEnable = 1;
	multisampling.rasterizationSamples = data.renderTarget.sampleCount;

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
	pipelineInfo.pDepthStencilState = &depthStencilInfo;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages.data();

	data.pipeline = data.device.createGraphicsPipeline({}, pipelineInfo);

	data.device.destroyShaderModule(vertModule);
	data.device.destroyShaderModule(fragModule);
}

void DRenderer::Vulkan::Terminate(void*& apiData)
{
	APIData& data = *reinterpret_cast<APIData*>(apiData);

	data.device.waitIdle();

	MainUniforms::Terminate(data.mainUniforms);

	// Delete main semaphores
	for (auto item : data.swapchainImageAvailable)
		data.device.destroySemaphore(item);
	for (auto item : data.resourceSetAvailable)
		data.device.destroyFence(item);

	// Destroy all main command pools / buffers
	data.device.destroyCommandPool(data.renderCmdPool);
	data.device.destroyCommandPool(data.presentCmdPool);

	// Cleanup swapchain
	for (auto item : data.swapchain.imageViews)
		data.device.destroyImageView(item);
	data.device.destroySwapchainKHR(data.swapchain.handle);

	// Clean up rendertarget
	data.device.destroyImage(data.renderTarget.colorImg);
	data.device.destroyImageView(data.renderTarget.colorImgView);
	data.device.destroyImage(data.renderTarget.depthImg);
	data.device.destroyImageView(data.renderTarget.depthImgView);
	data.device.destroyFramebuffer(data.renderTarget.framebuffer);
	data.device.freeMemory(data.renderTarget.memory);

	// Clean up renderPass
	data.device.destroyRenderPass(data.renderPass);

	AssetSystem::Terminate(data.assetSystem);

	data.device.destroy();

	data.instance.destroyDebugUtilsMessengerEXT(data.debugMessenger);
	data.instance.destroy();

	delete reinterpret_cast<APIData*>(apiData);
	apiData = nullptr;
}