#include "Vk.hpp"
#include <DEngine/Gfx/impl/Assert.hpp>
#include "Init.hpp"

#include "GizmoManager.hpp"

#include <DEngine/FixedWidthTypes.hpp>
#include <DEngine/Std/FrameAllocator.hpp>
#include <DEngine/Std/Containers/Span.hpp>
#include <DEngine/Std/Containers/StackVec.hpp>
#include <DEngine/Std/Containers/Vec.hpp>
#include <DEngine/Std/Utility.hpp>


// For file IO
#include <DEngine/Application.hpp>

#include <iostream>
#include <stdexcept>
#include <string>


//vk::DispatchLoaderDynamic vk::defaultDispatchLoaderDynamic;

namespace DEngine::Gfx::Vk
{
	APIDataBase* InitializeBackend(Context& gfxData, InitInfo const& initInfo);

	namespace Init
	{
		void InitTestPipeline(APIData& apiData, Std::FrameAllocator& frameAlloc);
	}
}

using namespace DEngine;
using namespace DEngine::Gfx;

Vk::APIData::APIData()
{
}

Vk::APIData::~APIData()
{
	APIData& apiData = *this;

	apiData.globUtils.device.waitIdle();
}

void Vk::APIData::NewViewport(ViewportID& viewportID)
{
	APIData& apiData = *this;

	ViewportManager::NewViewport(
		apiData.viewportManager,
		viewportID);
}

void Vk::APIData::DeleteViewport(ViewportID id)
{
	//vk::Result vkResult{};
	APIData& apiData = *this;

	ViewportManager::DeleteViewport(
		apiData.viewportManager,
		id);
}

void Vk::APIData::NewFontTexture(
	u32 id,
	u32 width,
	u32 height,
	u32 pitch,
	Std::Span<std::byte const> data)
{
	APIData& apiData = *this;

	GuiResourceManager::NewFontTexture(
		apiData.guiResourceManager,
		apiData.globUtils,
		id,
		width,
		height,
		pitch,
		data);
}

Vk::GlobUtils::GlobUtils()
{
}

namespace DEngine::Gfx::Vk
{
	[[noreturn]] void RenderingThreadEntryPoint(APIData* inApiData)
	{
		Std::NameThisThread("RenderingThread");

		APIData& apiData = *inApiData;

		while (true)
		{
			std::unique_lock lock{ apiData.drawParamsLock };
			apiData.drawParamsCondVarWorker.wait(lock, [&apiData]() -> bool { return apiData.drawParamsReady; });

			apiData.InternalDraw(apiData.drawParams);

			apiData.drawParamsReady = false;
			lock.unlock();
			apiData.drawParamsCondVarProducer.notify_one();
		}
	}
}

APIDataBase* Vk::InitializeBackend(Context& gfxData, InitInfo const& initInfo)
{
	auto* apiDataPtr = new APIData;

	APIData& apiData = *apiDataPtr;
	GlobUtils& globUtils = apiData.globUtils;

	apiData.frameAllocator = Std::FrameAllocator::PreAllocate(1024 * 1024).Value();
	auto& transientAlloc = apiData.frameAllocator;

	vk::Result vkResult{};
	bool boolResult = false;

	apiData.globUtils.logger = initInfo.optional_logger;
	apiData.test_textureAssetInterface = initInfo.texAssetInterface;
	globUtils.wsiInterface = initInfo.wsiConnection;

	globUtils.editorMode = true;
	globUtils.inFlightCount = Constants::preferredInFlightCount;

	// Make the VkInstance
	PFN_vkGetInstanceProcAddr instanceProcAddr = Vk::loadInstanceProcAddressPFN();
	BaseDispatch baseDispatch{};
	BaseDispatch::BuildInPlace(baseDispatch, instanceProcAddr);
	auto const createVkInstanceResult = Init::CreateVkInstance(
		initInfo.requiredVkInstanceExtensions, 
		Constants::enableDebugUtils,
		baseDispatch,
		transientAlloc,
		globUtils.logger);
	InstanceDispatch::BuildInPlace(
		globUtils.instance, 
		createVkInstanceResult.instanceHandle, 
		instanceProcAddr);
	auto& instance = globUtils.instance;

	// Enable DebugUtils functionality.
	// If we enabled debug utils, we build the debug utils dispatch table and the debug utils messenger.
	if (createVkInstanceResult.debugUtilsEnabled)
	{
		DebugUtilsDispatch::BuildInPlace(
			globUtils.debugUtils,
			instance.handle,
			instanceProcAddr);
		globUtils.debugMessenger = Init::CreateLayerMessenger(
			instance.handle,
			globUtils.DebugUtilsPtr(), 
			initInfo.optional_logger);
	}
	auto* debugUtils = globUtils.DebugUtilsPtr();

	// I fucking hate this code, but whatever
	vk::SurfaceKHR surface{};
	{
		auto surfaceCreateResult = globUtils.wsiInterface->CreateVkSurface(
			initInfo.initialWindow,
			(uSize)(VkInstance)instance.handle,
			nullptr);
		vkResult = (vk::Result)surfaceCreateResult.vkResult;
		if (vkResult != vk::Result::eSuccess)
			throw std::runtime_error("Unable to create VkSurfaceKHR object during initialization.");
		surface = (vk::SurfaceKHR)(VkSurfaceKHR)surfaceCreateResult.vkSurface;
	}

	// Pick our phys device and load the info for it
	globUtils.physDevice = Init::LoadPhysDevice(
		instance,
		surface,
		transientAlloc);
	auto& physDevice = globUtils.physDevice;

	SurfaceInfo::BuildInPlace(
		globUtils.surfaceInfo,
		surface,
		instance,
		physDevice.handle);

	PFN_vkGetDeviceProcAddr deviceProcAddr = (PFN_vkGetDeviceProcAddr)instanceProcAddr(
		(VkInstance)instance.handle,
		"vkGetDeviceProcAddr");
	// Create the device and the dispatch table for it.
	vk::Device deviceHandle = Init::CreateDevice(
		instance,
		physDevice,
		transientAlloc);
	DeviceDispatch::BuildInPlace(
		globUtils.device,
		deviceHandle,
		deviceProcAddr);
	QueueData::Init(
		globUtils.queues,
		globUtils.device,
		physDevice.queueIndices,
		debugUtils);
	globUtils.device.m_queueDataPtr = &globUtils.queues;
	auto& device = globUtils.device;
	auto& queues = globUtils.queues;

	boolResult = DeletionQueue::Init(
		globUtils.delQueue,
		globUtils.inFlightCount);
	if (!boolResult)
		throw std::runtime_error("DEngine - Vulkan: Failed to initialize DeletionQueue");

	// Init VMA
	apiData.vma_trackingData.deviceHandle = device.handle;
	apiData.vma_trackingData.debugUtils = debugUtils;
	auto const vmaResult = Init::InitializeVMA(
		instance,
		physDevice.handle,
		device,
		&apiData.vma_trackingData);
	if (vmaResult.result != vk::Result::eSuccess)
		throw std::runtime_error("DEngine - Vulkan: Failed to initialize VMA.");
	globUtils.vma = vmaResult.value;
	auto& vma = globUtils.vma;

	// Initialize main cmd buffer
	{
		apiData.mainCmdPools.Resize(globUtils.inFlightCount);
		for (uSize i = 0; i < globUtils.inFlightCount; i += 1)
		{
			vk::CommandPoolCreateInfo cmdPoolInfo = {};
			//cmdPoolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
			cmdPoolInfo.queueFamilyIndex = queues.graphics.FamilyIndex();
			auto cmdPool = device.createCommandPool(cmdPoolInfo);
			if (cmdPool.result != vk::Result::eSuccess)
				throw std::runtime_error("Unable to make command pool");
			apiData.mainCmdPools[i] = cmdPool.value;
			if (debugUtils)
			{
				std::string name = "Main CmdPool #";
				name += std::to_string(i);
				debugUtils->Helper_SetObjectName(
					device.handle,
					apiData.mainCmdPools[i],
					name.c_str());
			}

			vk::CommandBufferAllocateInfo cmdBufferAllocInfo = {};
			cmdBufferAllocInfo.commandBufferCount = 1;
			cmdBufferAllocInfo.commandPool = apiData.mainCmdPools[i];
			cmdBufferAllocInfo.level = vk::CommandBufferLevel::ePrimary;
			apiData.mainCmdBuffers.Resize(globUtils.inFlightCount);
			vkResult = device.allocateCommandBuffers(cmdBufferAllocInfo, &apiData.mainCmdBuffers[i]);
			if (vkResult != vk::Result::eSuccess)
				throw std::runtime_error("DEngine - Vulkan: Failed to initialize main commandbuffers.");
			// We don't give the command buffers debug names here, because we need to rename them everytime we re-record anyways.
		}
	}

	NativeWindowManager::InitInfo windowManInfo = {
		.manager = &apiData.nativeWindowManager,
		.initialWindow = initInfo.initialWindow,
		.device = &device,
		.queues = &queues,
		.optional_debugUtils = debugUtils };
	NativeWindowManager::Initialize(windowManInfo);

	boolResult = ViewportManager::Init(
		apiData.viewportManager,
		device,
		physDevice.properties.limits.minUniformBufferOffsetAlignment,
		debugUtils);
	if (!boolResult)
		throw std::runtime_error("DEngine - Vulkan: Failed to initialize ViewportManager.");

	TextureManager::Init(
		apiData.textureManager,
		device,
		queues,
		debugUtils);

	boolResult = ObjectDataManager::Init(
		apiData.objectDataManager,
		physDevice.properties.limits.minUniformBufferOffsetAlignment,
		globUtils.inFlightCount,
		vma,
		device,
		debugUtils);
	if (!boolResult)
		throw std::runtime_error("DEngine - Vulkan: Failed to initialize ObjectDataManager.");


	// Create our main fences
	apiData.mainFences = Init::CreateMainFences(
		device,
		globUtils.inFlightCount,
		debugUtils);

	
	globUtils.guiRenderPass = Init::CreateGuiRenderPass(
		device,
		globUtils.surfaceInfo.surfaceFormatToUse.format,
		debugUtils);

	// Create the main render stuff
	globUtils.gfxRenderPass = Init::BuildMainGfxRenderPass(
		device,
		true, 
		globUtils.DebugUtilsPtr());

	Init::InitTestPipeline(
		apiData,
		apiData.frameAllocator);


	GuiResourceManager::Init(
		apiData.guiResourceManager, 
		device,
		vma,
		globUtils.inFlightCount, 
		globUtils.guiRenderPass,
		transientAlloc,
		debugUtils);

	GizmoManager::InitInfo gizmoManagerInfo = {};
	gizmoManagerInfo.apiData = &apiData;
	gizmoManagerInfo.arrowMesh = { initInfo.gizmoArrowMesh.data(), initInfo.gizmoArrowMesh.size() };
	gizmoManagerInfo.arrowScaleMesh2d = { initInfo.gizmoArrowScaleMesh2d.data(), initInfo.gizmoArrowScaleMesh2d.size() };
	gizmoManagerInfo.circleLineMesh = { initInfo.gizmoCircleLineMesh.data(), initInfo.gizmoCircleLineMesh.size() };
	gizmoManagerInfo.debugUtils = globUtils.DebugUtilsPtr();
	gizmoManagerInfo.delQueue = &globUtils.delQueue;
	gizmoManagerInfo.device = &globUtils.device;
	gizmoManagerInfo.inFlightCount = globUtils.inFlightCount;
	gizmoManagerInfo.frameAlloc = &apiData.frameAllocator;
	gizmoManagerInfo.queues = &globUtils.queues;
	gizmoManagerInfo.vma = &globUtils.vma;
	GizmoManager::Initialize(apiData.gizmoManager, gizmoManagerInfo);

	apiData.renderingThread = std::thread(&RenderingThreadEntryPoint, &apiData);

	return apiDataPtr;
}

void Vk::Init::InitTestPipeline(APIData& apiData, Std::FrameAllocator& frameAlloc)
{
	vk::Result vkResult = {};

	Std::Array<vk::DescriptorSetLayout, 3> layouts{ 
		apiData.viewportManager.cameraDescrLayout, 
		apiData.objectDataManager.descrSetLayout,
		apiData.textureManager.descrSetLayout };

	vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.setLayoutCount = 3;
	pipelineLayoutInfo.pSetLayouts = layouts.Data();
	apiData.testPipelineLayout = apiData.globUtils.device.createPipelineLayout(pipelineLayoutInfo);
	if (apiData.globUtils.UsingDebugUtils())
	{
		apiData.globUtils.debugUtils.Helper_SetObjectName(
			apiData.globUtils.device.handle,
			apiData.testPipelineLayout,
			"Test Pipelinelayout");
	}

	App::FileInputStream vertFile { "data/vert.spv" };
	if (!vertFile.IsOpen())
		throw std::runtime_error("Could not open vertex shader file");
	vertFile.Seek(0, App::FileInputStream::SeekOrigin::End);
	u64 vertFileLength = vertFile.Tell().Value();
	vertFile.Seek(0, App::FileInputStream::SeekOrigin::Start);
	auto vertCode = Std::Vec<char, Std::FrameAllocator>(frameAlloc);
	vertCode.Resize((uSize)vertFileLength);
	vertFile.Read(vertCode.Data(), vertFileLength);
	
	vk::ShaderModuleCreateInfo vertModCreateInfo = {};
	vertModCreateInfo.codeSize = vertCode.Size();
	vertModCreateInfo.pCode = reinterpret_cast<u32 const*>(vertCode.Data());
	vk::ShaderModule vertModule = apiData.globUtils.device.createShaderModule(vertModCreateInfo);
	vk::PipelineShaderStageCreateInfo vertStageInfo = {};
	vertStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
	vertStageInfo.module = vertModule;
	vertStageInfo.pName = "main";

	App::FileInputStream fragFile{ "data/frag.spv" };
	if (!fragFile.IsOpen())
		throw std::runtime_error("Could not open fragment shader file");
	fragFile.Seek(0, App::FileInputStream::SeekOrigin::End);
	u64 fragFileLength = fragFile.Tell().Value();
	fragFile.Seek(0, App::FileInputStream::SeekOrigin::Start);
	auto fragCode = Std::Vec<char, Std::FrameAllocator>(frameAlloc);
	fragCode.Resize((uSize)fragFileLength);
	fragFile.Read(fragCode.Data(), fragFileLength);

	vk::ShaderModuleCreateInfo fragModInfo{};
	fragModInfo.codeSize = fragCode.Size();
	fragModInfo.pCode = reinterpret_cast<const u32*>(fragCode.Data());
	vk::ShaderModule fragModule = apiData.globUtils.device.createShaderModule(fragModInfo);
	vk::PipelineShaderStageCreateInfo fragStageInfo{};
	fragStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
	fragStageInfo.module = fragModule;
	fragStageInfo.pName = "main";

	Std::Array<vk::PipelineShaderStageCreateInfo, 2> shaderStages = { vertStageInfo, fragStageInfo };

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
	rasterizer.cullMode = vk::CullModeFlagBits::eNone;

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
	colorBlendAttachment.colorWriteMask |= vk::ColorComponentFlagBits::eR;
	colorBlendAttachment.colorWriteMask |= vk::ColorComponentFlagBits::eG;
	colorBlendAttachment.colorWriteMask |= vk::ColorComponentFlagBits::eB;
	colorBlendAttachment.colorWriteMask |= vk::ColorComponentFlagBits::eA;
	colorBlendAttachment.blendEnable = true;
	colorBlendAttachment.colorBlendOp = vk::BlendOp::eAdd;
	colorBlendAttachment.alphaBlendOp = vk::BlendOp::eAdd;
	colorBlendAttachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
	colorBlendAttachment.dstAlphaBlendFactor = vk::BlendFactor::eOne;
	colorBlendAttachment.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
	colorBlendAttachment.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;

	vk::PipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;

	vk::DynamicState temp = vk::DynamicState::eViewport;
	vk::PipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.dynamicStateCount = 1;
	dynamicState.pDynamicStates = &temp;

	vk::GraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.pDepthStencilState = &depthStencilInfo;
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

	vkResult = apiData.globUtils.device.createGraphicsPipelines(
		vk::PipelineCache(),
		{ 1, &pipelineInfo },
		nullptr,
		&apiData.testPipeline);
	if (vkResult != vk::Result::eSuccess)
		throw std::runtime_error("Unable to make graphics pipeline.");
	if (apiData.globUtils.UsingDebugUtils())
	{
		apiData.globUtils.debugUtils.Helper_SetObjectName(
			apiData.globUtils.device.handle,
			apiData.testPipeline,
			"Test Pipeline");
	}

	apiData.globUtils.device.destroy(vertModule);
	apiData.globUtils.device.destroy(fragModule);
}
