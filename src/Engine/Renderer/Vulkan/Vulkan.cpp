#include "Vulkan.hpp"

#include "DRenderer/MeshDocument.hpp"

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
	auto data = static_cast<APIData*>(ptr);
	ptr = nullptr;
	delete data;
}

DRenderer::Vulkan::APIData& DRenderer::Vulkan::GetAPIData()
{
	return *static_cast<APIData*>(DRenderer::Core::GetAPIData());
}

size_t DRenderer::Vulkan::VertexBufferObject::GetByteOffset(VertexBufferObject::Attribute attribute) const
{
	size_t offset = 0;
	for (size_t i = 0; i < static_cast<size_t>(attribute); i++)
		offset += static_cast<size_t>(attributeSizes[i]);
	return offset;
}

size_t& DRenderer::Vulkan::VertexBufferObject::GetAttrSize(Attribute attr)
{
	return attributeSizes[static_cast<size_t>(attr)];
}

size_t DRenderer::Vulkan::VertexBufferObject::GetAttrSize(Attribute attr) const
{
	return attributeSizes[static_cast<size_t>(attr)];
}

uint8_t* DRenderer::Vulkan::APIData::MainUniforms::GetObjectDataResourceSet(uint32_t resourceSet)
{
	return objectDataMappedMem + GetObjectDataResourceSetOffset(resourceSet);
}

size_t DRenderer::Vulkan::APIData::MainUniforms::GetObjectDataResourceSetOffset(uint32_t resourceSet) const
{
	return GetObjectDataResourceSetSize() * resourceSet;
}

size_t DRenderer::Vulkan::APIData::MainUniforms::GetObjectDataResourceSetSize() const
{
	return objectDataResourceSetSize;
}

size_t DRenderer::Vulkan::APIData::MainUniforms::GetObjectDataDynamicOffset(size_t modelDataIndex) const
{
	assert(modelDataIndex < objectDataResourceSetSize);
	return objectDataSize * modelDataIndex;
}

uint8_t* DRenderer::Vulkan::APIData::MainUniforms::GetCameraBufferResourceSet(uint32_t resourceSet)
{
	return cameraMemoryMap + GetCameraResourceSetOffset(resourceSet);
}

size_t DRenderer::Vulkan::APIData::MainUniforms::GetCameraResourceSetOffset(uint32_t resourceSet) const
{
	return GetCameraResourceSetSize() * resourceSet;
}

size_t DRenderer::Vulkan::APIData::MainUniforms::GetCameraResourceSetSize() const
{
	return cameraDataResourceSetSize;
}

size_t DRenderer::Vulkan::APIData::MainUniforms::GetObjectDataSize() const
{
	return objectDataSize;
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
	void MakePipeline();
	void MakeVBO();
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


	data.device = Init::CreateDevice(data.physDevice.handle);

	// Load function pointers for our device.
	Volk::LoadDevice(data.device);

	data.descriptorSetLayout = Init::CreatePrimaryDescriptorSetLayout(data.device);

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

	auto [descPool, descSets] = Init::AllocatePrimaryDescriptorSets(data.device, data.descriptorSetLayout, data.resourceSetCount);
	data.descriptorSetPool = descPool;
	data.descriptorSets = std::move(descSets);

	// Build main uniforms
	data.mainUniforms = Init::BuildMainUniforms(data.device, data.physDevice.memProperties, data.physDevice.properties.limits, data.resourceSetCount);

	Init::ConfigurePrimaryDescriptors();

	// Set up presentation cmd buffers
	Init::SetupPresentCmdBuffers(data.device, data.renderTarget, data.surfaceExtents, data.swapchain.images, data.presentCmdPool, data.presentCmdBuffer);

	// Allocate rendering cmd buffers.
	Init::SetupRenderingCmdBuffers(data.device, data.swapchain.length, data.renderCmdPool, data.renderCmdBuffer);

	// Request our graphics queue
	data.graphicsQueue = data.device.getQueue(0, 0);

	// Transition layouts of our render target and swapchain
	Init::TransitionRenderTargetAndSwapchain(data.device, data.graphicsQueue, data.renderTarget, data.swapchain.images);

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
	MakeVBO();
}

void DRenderer::Vulkan::PrepareRenderingEarly(const Core::PrepareRenderingEarlyParams& in)
{
	APIData& data = GetAPIData();

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

	// Bind VBO
	VBO& vbo = data.testVBO;
	std::array<vk::Buffer, 3> vertexBuffers{ vbo.buffer, vbo.buffer, vbo.buffer };
	std::array<vk::DeviceSize, 3> vboOffsets
	{
		vbo.GetByteOffset(VBO::Attribute::Position),
		vbo.GetByteOffset(VBO::Attribute::TexCoord),
		vbo.GetByteOffset(VBO::Attribute::Normal)
	};
	cmdBuffer.bindVertexBuffers(0, vertexBuffers, vboOffsets);
	cmdBuffer.bindIndexBuffer(vbo.buffer, vbo.GetByteOffset(VBO::Attribute::Index), vbo.indexType);

	for (size_t i = 0; i < drendererData.renderGraph.meshes.size(); i++)
	{
		cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, data.pipelineLayout, 0, data.descriptorSets[data.currentResourceSet], data.mainUniforms.GetObjectDataDynamicOffset(i));
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

	vk::SubmitInfo renderImageSubmit{};
	renderImageSubmit.commandBufferCount = 1;
	renderImageSubmit.pCommandBuffers = &data.renderCmdBuffer[data.swapchain.currentImage];
	data.graphicsQueue.submit(renderImageSubmit, data.resourceSetAvailable[data.currentResourceSet]);

	vk::SubmitInfo resolveImageSubmit{};
	resolveImageSubmit.commandBufferCount = 1;
	resolveImageSubmit.pCommandBuffers = &data.presentCmdBuffer[data.swapchain.currentImage];

	resolveImageSubmit.waitSemaphoreCount = 1;
	resolveImageSubmit.pWaitSemaphores = &data.swapchainImageAvailable[data.imageAvailableActiveIndex];
	vk::PipelineStageFlags semaphoreWaitFlag = vk::PipelineStageFlagBits::eBottomOfPipe;
	resolveImageSubmit.pWaitDstStageMask = &semaphoreWaitFlag;
	data.graphicsQueue.submit(resolveImageSubmit, {});



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
	pipelineLayoutCreateInfo.pSetLayouts = &data.descriptorSetLayout;

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
}

void DRenderer::Vulkan::MakeVBO()
{
	APIData& data = GetAPIData();

	auto meshDocumentOpt = Core::GetData().assetLoadData.meshLoader(0);

	assert(meshDocumentOpt.has_value());

	MeshDocument& meshDoc = meshDocumentOpt.value();

	vk::BufferCreateInfo bufferInfo{};
	bufferInfo.usage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eIndexBuffer;
	bufferInfo.sharingMode = vk::SharingMode::eExclusive;
	bufferInfo.size = meshDoc.GetTotalSizeRequired();

	vk::Buffer buffer = data.device.createBuffer(bufferInfo);

	vk::MemoryRequirements memReqs = data.device.getBufferMemoryRequirements(buffer);

	vk::MemoryAllocateInfo allocInfo{};
	allocInfo.allocationSize = memReqs.size;
	allocInfo.memoryTypeIndex = std::numeric_limits<uint32_t>::max();

	// Find host-visible memory we can allocate to
	for (uint32_t i = 0; i < data.physDevice.memProperties.memoryTypeCount; i++)
	{
		if (data.physDevice.memProperties.memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eHostVisible)
		{
			allocInfo.memoryTypeIndex = i;
			break;
		}
	}

	vk::DeviceMemory mem = data.device.allocateMemory(allocInfo);

	data.device.bindBufferMemory(buffer, mem, 0);

	VBO vbo;
	vbo.buffer = buffer;
	vbo.deviceMemory = mem;
	vbo.indexType = meshDoc.GetIndexType() == MeshDoc::IndexType::UInt32 ? vk::IndexType::eUint32 : vk::IndexType::eUint16;
	vbo.indexCount = meshDoc.GetIndexCount();


	uint8_t* ptr = static_cast<uint8_t*>(data.device.mapMemory(mem, 0, bufferInfo.size));

	using MeshDocAttr = MeshDoc::Attribute;
	using VBOAttr = VBO::Attribute;

	// Copy position data
	std::memcpy(ptr, meshDoc.GetDataPtr(MeshDocAttr::Position), meshDoc.GetByteLength(MeshDocAttr::Position));
	vbo.GetAttrSize(VBOAttr::Position) = meshDoc.GetByteLength(MeshDocAttr::Position);

	ptr += vbo.GetAttrSize(VBOAttr::Position);

	// Copy UV data
	std::memcpy(ptr, meshDoc.GetDataPtr(MeshDocAttr::TexCoord), meshDoc.GetByteLength(MeshDocAttr::TexCoord));
	vbo.GetAttrSize(VBOAttr::TexCoord) = meshDoc.GetByteLength(MeshDocAttr::TexCoord);

	ptr += vbo.GetAttrSize(VBOAttr::TexCoord);

	// Copy normal data
	std::memcpy(ptr, meshDoc.GetDataPtr(MeshDocAttr::Normal), meshDoc.GetByteLength(MeshDocAttr::Normal));
	vbo.GetAttrSize(VBOAttr::Normal) = meshDoc.GetByteLength(MeshDocAttr::Normal);

	ptr += vbo.GetAttrSize(VBOAttr::Normal);

	// Copy index data
	std::memcpy(ptr, meshDoc.GetDataPtr(MeshDocAttr::Index), meshDoc.GetByteLength(MeshDocAttr::Index));
	vbo.GetAttrSize(VBOAttr::Index) = meshDoc.GetByteLength(MeshDocAttr::Index);

	data.device.unmapMemory(mem);

	data.testVBO = vbo;
}