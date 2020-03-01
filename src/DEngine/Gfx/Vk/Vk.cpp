#include "Vk.hpp"
#include "../VkInterface.hpp"
#include "DEngine/Gfx/Assert.hpp"
#include "Init.hpp"

#include "DEngine/FixedWidthTypes.hpp"
#include "DEngine/Containers/Span.hpp"
#include "DEngine/Containers/FixedVector.hpp"

#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_vulkan.h"

#include <string>

//vk::DispatchLoaderDynamic vk::defaultDispatchLoaderDynamic;

namespace DEngine::Gfx::Vk
{
	void NewViewport(void* apiDataBuffer, u8& viewportID, void*& imguiTexID)
	{
		vk::Result vkResult{};
		APIData& apiData = *reinterpret_cast<APIData*>(apiDataBuffer);

		// Find next available viewport ID
		for (uSize i = 0; i < apiData.viewportDatas.Size(); i += 1)
		{
			ViewportVkData& vpData = apiData.viewportDatas[i];
			if (!vpData.inUse)
			{
				viewportID = (u8)i;
				break;
			}
		}

		ViewportVkData viewportData{};

		imguiTexID = ImGui_ImplVulkan_AddTexture(VkImageView(), VkImageLayout());

		viewportData.imguiTextureID = imguiTexID;
		viewportData.inUse = true;

		apiData.viewportDatas[viewportID] = std::move(viewportData);
	}

	namespace Init
	{
		void Test(APIData& apiData);
	}
}

bool DEngine::Gfx::Vk::InitializeBackend(Data& gfxData, InitInfo const& initInfo, void*& apiDataBuffer)
{
	apiDataBuffer = new APIData;
	APIData& apiData = *static_cast<APIData*>(apiDataBuffer);
	GlobUtils& globUtils = apiData.globUtils;

	vk::Result vkResult{};

	apiData.logger = initInfo.optional_iLog;
	apiData.wsiInterface = initInfo.iWsi;

	globUtils.resourceSetCount = 2;
	apiData.currentResourceSet = 0;
	globUtils.useEditorPipeline = true;



	PFN_vkGetInstanceProcAddr instanceProcAddr = Vk::loadInstanceProcAddressPFN();

	BaseDispatch baseDispatch = BaseDispatch::Build(instanceProcAddr);

	Init::CreateVkInstance_Return createVkInstanceResult = Init::CreateVkInstance(initInfo.requiredVkInstanceExtensions, true, baseDispatch, apiData.logger);
	InstanceDispatch instance = InstanceDispatch::Build(createVkInstanceResult.instanceHandle, instanceProcAddr);
	globUtils.instance = instance;

	// If we enabled debug utils, we build the debug utils dispatch table and the debug utils messenger.
	if (createVkInstanceResult.debugUtilsEnabled)
	{
		globUtils.debugUtils = DebugUtilsDispatch::Build(globUtils.instance.handle, instanceProcAddr);
		globUtils.debugMessenger = Init::CreateLayerMessenger(
			globUtils.instance.handle, 
			globUtils.DebugUtilsPtr(), 
			initInfo.optional_iLog);
	}

	// TODO: I don't think I like this code
	// Create the VkSurface using the callback
	vk::SurfaceKHR surface{};
	vk::Result surfaceCreateResult = (vk::Result)apiData.wsiInterface->createVkSurface((u64)(VkInstance)instance.handle, nullptr, reinterpret_cast<u64*>(&surface));
	if (surfaceCreateResult != vk::Result::eSuccess)
		throw std::runtime_error("Unable to create VkSurfaceKHR object during initialization.");


	// Pick our phys device and load the info for it
	apiData.globUtils.physDevice = Init::LoadPhysDevice(instance, surface);

	// Build the surface info now that we have our physical device.
	apiData.surface = Init::BuildSurfaceInfo(instance, globUtils.physDevice.handle, surface, apiData.logger);

	// Build the settings we will use to build the swapchain.
	SwapchainSettings swapchainSettings = Init::BuildSwapchainSettings(instance, globUtils.physDevice.handle, apiData.surface, apiData.logger);

	PFN_vkGetDeviceProcAddr deviceProcAddr = (PFN_vkGetDeviceProcAddr)instanceProcAddr((VkInstance)instance.handle, "vkGetDeviceProcAddr");

	// Create the device and the dispatch table for it.
	vk::Device deviceHandle = Init::CreateDevice(instance, globUtils.physDevice);
	globUtils.device.copy(DeviceDispatch::Build(deviceHandle, deviceProcAddr));

	QueueData::Initialize(globUtils.device, globUtils.physDevice.queueIndices, globUtils.queues);

	apiData.globUtils.vma = Init::InitializeVMA(instance, globUtils.device, globUtils.physDevice, globUtils.DebugUtilsPtr());
	apiData.globUtils.deletionQueue.Initialize(apiData.globUtils.device, apiData.globUtils.resourceSetCount);

	// Build our swapchain on our device
	apiData.swapchain = Init::CreateSwapchain(
		apiData.globUtils.device, 
		apiData.globUtils.queues,
		apiData.globUtils.deletionQueue,
		swapchainSettings,
		apiData.globUtils.DebugUtilsPtr());


	// Create our main fences
	apiData.mainFences = Init::CreateMainFences(
		apiData.globUtils.device,
		apiData.globUtils.resourceSetCount,
		apiData.globUtils.DebugUtilsPtr());


	// Create the resources for rendering GUI
	apiData.guiData = Init::CreateGUIData(
		apiData.globUtils.device,
		apiData.globUtils.vma,
		apiData.globUtils.deletionQueue,
		apiData.globUtils.queues,
		apiData.swapchain.surfaceFormat.format,
		apiData.globUtils.resourceSetCount,
		apiData.swapchain.extents,
		apiData.globUtils.DebugUtilsPtr());

	// Record the copy-image command cmdBuffers that go from render-target to swapchain
	Init::RecordSwapchainCmdBuffers(apiData.globUtils.device, apiData.swapchain, apiData.guiData.renderTarget.img);

	// Initialize ImGui stuff
	Init::InitializeImGui(apiData, apiData.globUtils.device, instanceProcAddr, apiData.globUtils.DebugUtilsPtr());

	// Create the main render stuff
	apiData.globUtils.gfxRenderPass = Init::BuildMainGfxRenderPass(apiData.globUtils.device, true, apiData.globUtils.DebugUtilsPtr());


	// Allocate memory for cameras
	{
		vk::DeviceSize elementSize = 64;
		if (apiData.globUtils.physDevice.properties.limits.minUniformBufferOffsetAlignment > elementSize)
			elementSize = apiData.globUtils.physDevice.properties.limits.minUniformBufferOffsetAlignment;
		apiData.test_camUboOffset = elementSize;

		vk::DeviceSize memSize = elementSize * Constants::maxResourceSets * Gfx::Constants::maxViewportCount;
		vk::BufferCreateInfo bufferInfo{};
		bufferInfo.sharingMode = vk::SharingMode::eExclusive;
		bufferInfo.size = memSize;
		bufferInfo.usage = vk::BufferUsageFlagBits::eUniformBuffer;
		vk::Buffer buffer = apiData.globUtils.device.createBuffer(bufferInfo);

		vk::MemoryRequirements memReqs = apiData.globUtils.device.getBufferMemoryRequirements(buffer);
		if ((memReqs.memoryTypeBits & (1 << apiData.globUtils.physDevice.memInfo.hostVisible)) == false)
			throw std::runtime_error("DEngine::Gfx Vulkan: Can't use host-visible memory for camera memory");

		vk::MemoryAllocateInfo allocInfo{};
		allocInfo.allocationSize = memSize;
		allocInfo.memoryTypeIndex = apiData.globUtils.physDevice.memInfo.hostVisible;
		vk::DeviceMemory mem = apiData.globUtils.device.allocateMemory(allocInfo);

		void* mappedMemory = apiData.globUtils.device.mapMemory(mem, 0, memSize, vk::MemoryMapFlags());
		apiData.test_mappedMem = mappedMemory;



		apiData.globUtils.device.bindBufferMemory(buffer, mem, 0);

		vk::DescriptorSetLayoutBinding binding{};
		binding.binding = 0;
		binding.descriptorCount = 1;
		binding.descriptorType = vk::DescriptorType::eUniformBuffer;
		binding.stageFlags = vk::ShaderStageFlagBits::eVertex;


		vk::DescriptorSetLayoutCreateInfo descrLayoutInfo{};
		descrLayoutInfo.bindingCount = 1;
		descrLayoutInfo.pBindings = &binding;

		apiData.test_cameraDescrLayout = apiData.globUtils.device.createDescriptorSetLayout(descrLayoutInfo);

		vk::DescriptorPoolSize poolSize{};
		poolSize.descriptorCount = Constants::maxResourceSets * Gfx::Constants::maxViewportCount;
		poolSize.type = vk::DescriptorType::eUniformBuffer;
		vk::DescriptorPoolCreateInfo poolInfo{};
		poolInfo.poolSizeCount = 1;
		poolInfo.pPoolSizes = &poolSize;
		poolInfo.maxSets = Constants::maxResourceSets * Gfx::Constants::maxViewportCount;
		vk::DescriptorPool descrPool = apiData.globUtils.device.createDescriptorPool(poolInfo);

		Cont::FixedVector<vk::DescriptorSetLayout, Constants::maxResourceSets * Gfx::Constants::maxViewportCount> descrLayouts{};
		descrLayouts.Resize(descrLayouts.Capacity());
		for (auto& item : descrLayouts)
			item = apiData.test_cameraDescrLayout;

		vk::DescriptorSetAllocateInfo setAllocInfo{};
		setAllocInfo.descriptorPool = descrPool;
		setAllocInfo.descriptorSetCount = (u32)descrLayouts.Size();
		setAllocInfo.pSetLayouts = descrLayouts.Data();

		Cont::FixedVector<vk::DescriptorSet, Constants::maxResourceSets * Gfx::Constants::maxViewportCount> descrSets{};
		descrSets.Resize(descrSets.Capacity());
		vkResult = apiData.globUtils.device.allocateDescriptorSets(setAllocInfo, descrSets.Data());
		if (vkResult != vk::Result::eSuccess)
			throw std::runtime_error("DEngine, Vulkan: Unable to allocate descriptor sets for camera-matrices.");

		// Update the descriptor sets to point at the camera-Data
		{

			Cont::FixedVector<vk::WriteDescriptorSet, Constants::maxResourceSets * Gfx::Constants::maxViewportCount> descrWrites{};
			descrWrites.Resize(descrWrites.Capacity());
			Cont::FixedVector<vk::DescriptorBufferInfo, descrWrites.Capacity()> bufferInfos{};
			bufferInfos.Resize(descrWrites.Size());

			for (uSize i = 0; i < descrSets.Size(); i++)
			{
				vk::DescriptorBufferInfo& bufferInfo = bufferInfos[i];
				bufferInfo.buffer = buffer;
				bufferInfo.offset = i * elementSize;
				bufferInfo.range = 64;

				vk::WriteDescriptorSet& setWrite = descrWrites[i];
				setWrite.descriptorCount = 1;
				setWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
				setWrite.dstArrayElement = 0;
				setWrite.dstBinding = 0;
				setWrite.dstSet = descrSets[i];
				setWrite.pBufferInfo = &bufferInfo;
			}

			apiData.globUtils.device.updateDescriptorSets({ (u32)descrWrites.Size(), descrWrites.Data() }, {});
		}

		apiData.test_cameraDescrSets = descrSets;
	}

	Init::Test(apiData);


	return true;
}

#include <fstream>
#include "DEngine/Containers/Array.hpp"

#include "SDL2/SDL.h"

void DEngine::Gfx::Vk::Init::Test(APIData& apiData)
{
	vk::Result vkResult{};

	vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &apiData.test_cameraDescrLayout;
	apiData.testPipelineLayout = apiData.globUtils.device.createPipelineLayout(pipelineLayoutInfo);

	SDL_RWops* vertFile = SDL_RWFromFile("Data/vert.spv", "rb");
	if (vertFile == nullptr)
		throw std::runtime_error("Could not open vertex shader file");
	Sint64 vertFileLength = SDL_RWsize(vertFile);
	if (vertFileLength <= 0)
		throw std::runtime_error("Could not grab Size of vertex shader file");
	// Create vertex shader module
	std::vector<u8> vertCode(vertFileLength);
	SDL_RWread(vertFile, vertCode.data(), 1, vertFileLength);
	SDL_RWclose(vertFile);

	vk::ShaderModuleCreateInfo vertModCreateInfo{};
	vertModCreateInfo.codeSize = vertCode.size();
	vertModCreateInfo.pCode = reinterpret_cast<const u32*>(vertCode.data());
	vk::ShaderModule vertModule = apiData.globUtils.device.createShaderModule(vertModCreateInfo);
	vk::PipelineShaderStageCreateInfo vertStageInfo{};
	vertStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
	vertStageInfo.module = vertModule;
	vertStageInfo.pName = "main";

	SDL_RWops* fragFile = SDL_RWFromFile("Data/frag.spv", "rb");
	if (fragFile == nullptr)
		throw std::runtime_error("Could not open fragment shader file");
	Sint64 fragFileLength = SDL_RWsize(fragFile);
	if (fragFileLength <= 0)
		throw std::runtime_error("Could not grab Size of fragment shader file");
	std::vector<u8> fragCode(fragFileLength);
	SDL_RWread(fragFile, fragCode.data(), 1, fragFileLength);
	SDL_RWclose(fragFile);

	vk::ShaderModuleCreateInfo fragModInfo{};
	fragModInfo.codeSize = fragCode.size();
	fragModInfo.pCode = reinterpret_cast<const u32*>(fragCode.data());
	vk::ShaderModule fragModule = apiData.globUtils.device.createShaderModule(fragModInfo);
	vk::PipelineShaderStageCreateInfo fragStageInfo{};
	fragStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
	fragStageInfo.module = fragModule;
	fragStageInfo.pName = "main";

	Cont::Array<vk::PipelineShaderStageCreateInfo, 2> shaderStages = { vertStageInfo, fragStageInfo };

	vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};

	vk::PipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.topology = vk::PrimitiveTopology::eTriangleStrip;

	vk::Viewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (f32)0.f;
	viewport.height = (f32)0.f;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vk::Rect2D scissor{};
	scissor.offset = vk::Offset2D{ 0, 0 };
	scissor.extent = vk::Extent2D{ 8192, 8192 };
	vk::PipelineViewportStateCreateInfo viewportState{};
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	vk::PipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.lineWidth = 1.f;
	rasterizer.polygonMode = vk::PolygonMode::eFill;
	rasterizer.frontFace = vk::FrontFace::eCounterClockwise;
	rasterizer.rasterizerDiscardEnable = 0;

	vk::PipelineDepthStencilStateCreateInfo depthStencilInfo{};
	depthStencilInfo.depthTestEnable = 0;
	depthStencilInfo.depthCompareOp = vk::CompareOp::eLess;
	depthStencilInfo.stencilTestEnable = 0;
	depthStencilInfo.depthWriteEnable = 0;
	depthStencilInfo.minDepthBounds = 0.f;
	depthStencilInfo.maxDepthBounds = 1.f;

	vk::PipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sampleShadingEnable = 0;
	multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;

	vk::PipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

	vk::PipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;

	vk::DynamicState temp = vk::DynamicState::eViewport;
	vk::PipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.dynamicStateCount = 1;
	dynamicState.pDynamicStates = &temp;

	vk::GraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.layout = apiData.testPipelineLayout;
	pipelineInfo.renderPass = apiData.globUtils.gfxRenderPass;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.stageCount = (u32)shaderStages.Size();
	pipelineInfo.pStages = shaderStages.Data();

	apiData.testPipeline = apiData.globUtils.device.createGraphicsPipeline(vk::PipelineCache{}, pipelineInfo);

	apiData.globUtils.device.destroyShaderModule(vertModule, nullptr);
	apiData.globUtils.device.destroyShaderModule(fragModule, nullptr);
}