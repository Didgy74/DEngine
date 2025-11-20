#include "Vk.hpp"
#include "Init.hpp"
#include "GizmoManager.hpp"
#include "ShaderHelpers.hpp"

#include <DEngine/Gfx/impl/Assert.hpp>

// For file IO
#include <DEngine/Platform/Platform.hpp>

#include <iostream>
#include <stdexcept>
#include <string>

#include <DEngine/Std/Containers/Defer.hpp>
#include <DEngine/Std/Containers/AllocRef.hpp>

//vk::DispatchLoaderDynamic vk::defaultDispatchLoaderDynamic;

import DEngine.Std.Box;

namespace DEngine::Gfx::Vk
{
	APIDataBase* InitializeBackend(Context& gfxData, InitInfo const& initInfo);

	namespace Init {
		void InitTestPipeline(APIData& apiData, Std::AllocRef const& transientAlloc);
	}
}

using namespace DEngine;
using namespace DEngine::Gfx;

Vk::APIData::APIData()
{
}

Vk::APIData::~APIData()
{
	auto& apiData = *this;
	auto& globUtils = apiData.m_globUtils;

	if constexpr (Gfx::enableDedicatedThread)
	{
		// Push the shutdown command to the render thread
		std::unique_lock lock{ apiData.threadLock };

		auto& threadData = apiData.thread;
		thread.drawParamsCondVarProducer.wait(
			lock,
			[&threadData]() { return !threadData.nextJobReady; });
		threadData.nextJobReady = true;
		threadData.shutdownThread = true;
	}
	thread.drawParamsCondVarWorker.notify_one();
	apiData.thread.renderingThread.join();

	globUtils.device.waitIdle();

	//
	// Delete stuff here...
	//

	NativeWinMgr::Destroy(
		apiData.nativeWindowManager,
		globUtils.instance,
		globUtils.device);

	DelQueue::FlushAllJobs(apiData.delQueue, globUtils);

	/*
	if (globUtils.UsingDebugUtils())
	{
		globUtils.debugUtils.Destroy(
			globUtils.instance.handle,
			globUtils.debugMessenger);
	}
	*/

	globUtils.device.Destroy();

	globUtils.instance.Destroy();
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

void Vk::APIData::NewFontFace(FontFaceId fontFaceId)
{
	auto& apiData = *this;

	GuiResourceManager::NewFontFace(
		apiData.guiResourceManager,
		fontFaceId);
}

void Vk::APIData::NewFontTextures(Std::Span<FontBitmapUploadJob const> const& jobs)
{
	APIData& apiData = *this;

	GuiResourceManager::NewFontTextures(
		apiData.guiResourceManager,
		jobs);
}

Vk::GlobUtils::GlobUtils()
{
}

namespace DEngine::Gfx::Vk
{
	void RenderingThreadEntryPoint(APIData* inApiData)
	{
		constexpr char renderingThreadNameString[] = "RenderThread";
		Std::NameThisThread({ renderingThreadNameString, sizeof(renderingThreadNameString) - 1 });

		APIData& apiData = *inApiData;

		while (true) {
			std::unique_lock lock{ apiData.threadLock };

			auto& threadData = apiData.thread;
			threadData.drawParamsCondVarWorker.wait(
				lock,
				[&threadData](){ return threadData.nextJobReady; });

			DENGINE_IMPL_GFX_ASSERT(threadData.nextJobFn != nullptr);
			threadData.nextJobFn(apiData);

			threadData.nextJobReady = false;

			if (threadData.shutdownThread)
				break;

			lock.unlock();
			threadData.drawParamsCondVarProducer.notify_one();
		}
	}
}

APIDataBase* Vk::InitializeBackend(Context& gfxData, InitInfo const& initInfo)
{
	auto apiDataPtr = Std::BoxAdopt(new APIData);

	auto& apiData = *apiDataPtr;
	auto& globUtils = apiData.m_globUtils;

	apiData.frameAllocator = Std::BumpAllocator::PreAllocate(1024 * 1024).Value();
	auto& transientAlloc = apiData.frameAllocator;
	Std::Defer allocCleanup { [&transientAlloc]() { transientAlloc.Reset(); } };

	vk::Result vkResult = {};
	bool boolResult = false;

	apiData.m_globUtils.logger = initInfo.optional_logger;
	apiData.test_textureAssetInterface = initInfo.texAssetInterface;
	globUtils.wsiInterface = initInfo.wsiConnection;

	globUtils.editorMode = true;
	globUtils.inFlightCount = Constants::preferredInFlightCount;
	auto inFlightCount = globUtils.inFlightCount;
	apiData.inFlightCount = inFlightCount;

		// Make the VkInstance
	PFN_vkGetInstanceProcAddr instanceProcAddr = Vk::loadInstanceProcAddressPFN();
	BaseDispatch::BuildInPlace(globUtils.baseDispatch, instanceProcAddr);

	auto& baseDispatch = globUtils.baseDispatch;
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
	if (createVkInstanceResult.debugUtilsEnabled) {
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
			(void const*)baseDispatch.raw.vkGetInstanceProcAddr,
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

	auto deviceProcAddr = (PFN_vkGetDeviceProcAddr)instanceProcAddr(
		(VkInstance)instance.handle,
		"vkGetDeviceProcAddr");
	// Create the device and the dispatch table for it.
	auto deviceHandle = Init::CreateDevice(
		instance,
		physDevice,
		transientAlloc);
	DeviceDispatch::BuildInPlace(
		globUtils.device,
		deviceHandle,
		physDevice.properties.limits,
		deviceProcAddr);
	auto& device = globUtils.device;

	QueueData::Init(
		globUtils.queues,
		device,
		physDevice.queueIndices,
		debugUtils);
	globUtils.device.m_queueDataPtr = &globUtils.queues;
	auto& queues = globUtils.queues;

	boolResult = DeletionQueue::Init(
		apiData.delQueue,
		inFlightCount);
	if (!boolResult)
		throw std::runtime_error("DEngine - Vulkan: Failed to initialize DeletionQueue");
	auto& delQueue = apiData.delQueue;

	// Init VMA
	apiData.vma_trackingData.deviceHandle = device.handle;
	apiData.vma_trackingData.debugUtils = debugUtils;
	globUtils.vma = Init::InitializeVMA(
		instance,
		physDevice.handle,
		device,
		&apiData.vma_trackingData);
	auto& vma = globUtils.vma;

	// Create our main fences
	apiData.mainFences = Init::CreateMainFences(
		device,
		inFlightCount,
		debugUtils);

	// Initialize main command buffers.
	{
		auto result = Init::CreateMainCmdBuffers(
			device,
			(int)queues.graphics.FamilyIndex(),
			inFlightCount,
			debugUtils);
		apiData.mainCmdPools = Std::Move(result.cmdPools);
		apiData.mainCmdBuffers = result.cmdBuffers;
	}

	// Initialize our staging buffer allocators
	{
		apiData.mainStagingBufferAllocs.Resize(inFlightCount);
		for (int i = 0; i < inFlightCount; i++) {
			StagingBufferAlloc::BuildInPlace(
				apiData.mainStagingBufferAllocs[i],
				device,
				vma);
		}
	}

	NativeWinMgr::Initialize({
		.manager = apiData.nativeWindowManager,
		.initialWindow = initInfo.initialWindow,
		.sizeHint = initInfo.windowSizeHint.Map([](auto in) -> vk::Extent2D { return { in.width, in.height }; }),
		.surface = surface,
		.device = device,
		.queues = queues,
		.optional_debugUtils = debugUtils });

	boolResult = ViewportManager::Init(
		apiData.viewportManager,
		device,
		physDevice.properties.limits.minUniformBufferOffsetAlignment,
		debugUtils);
	auto const& viewportManager = apiData.viewportManager;
	if (!boolResult)
		throw std::runtime_error("DEngine - Vulkan: Failed to initialize ViewportManager.");

	TextureManager::Init(
		apiData.textureManager,
		device,
		queues,
		debugUtils);

	auto guiWindowUniformDescrLayout = GuiResourceManager::BuildGuiWindowDescrLayout(
		apiData.guiResourceManager,
		device,
		debugUtils);

	boolResult = ObjectDataManager::Init(
		apiData.objectDataManager,
		device,
		inFlightCount,
		guiWindowUniformDescrLayout,
		debugUtils);
	if (!boolResult)
		throw std::runtime_error("DEngine - Vulkan: Failed to initialize ObjectDataManager.");
	auto& objectDataManager = apiData.objectDataManager;

	auto guiRenderPass = Init::CreateGuiRenderPass(
		device,
		globUtils.surfaceInfo.surfaceFormatToUse.format,
		debugUtils);
	globUtils.guiRenderPass = guiRenderPass;

	// Create the main render stuff
	globUtils.gfxRenderPass = Init::BuildMainGfxRenderPass(
		device,
		globUtils.surfaceInfo.surfaceFormatToUse.format,
		true,
		globUtils.DebugUtilsPtr());

	Init::InitTestPipeline(
		apiData,
		transientAlloc);

	auto guiDescrSetLayouts = Std::Array<vk::DescriptorSetLayout, 2>{
		viewportManager.cameraDescrLayout,
		objectDataManager.GetObjectDataUniformDescrLayout(), };
	GuiResourceManager::Init(apiData.guiResourceManager, {
		.device = device,
		.vma = vma,
		.guiRenderPass = guiRenderPass,
		.cameraDataUniformDescrLayout = viewportManager.cameraDescrLayout,
		.objectDataUniformDescrLayout = objectDataManager.GetObjectDataUniformDescrLayout(),
		.viewportImgDescrSetLayout = viewportManager.imgDescrSetLayout,
		.descrSetLayouts = guiDescrSetLayouts.ToSpan(),
		.inFlightCount = inFlightCount,
		.transientAlloc = transientAlloc,
		.debugUtils = debugUtils, });

	GizmoManager::InitInfo gizmoManagerInfo = {};
	gizmoManagerInfo.apiData = &apiData;
	gizmoManagerInfo.arrowMesh = { initInfo.gizmoArrowMesh.data(), initInfo.gizmoArrowMesh.size() };
	gizmoManagerInfo.arrowScaleMesh2d = { initInfo.gizmoArrowScaleMesh2d.data(), initInfo.gizmoArrowScaleMesh2d.size() };
	gizmoManagerInfo.circleLineMesh = { initInfo.gizmoCircleLineMesh.data(), initInfo.gizmoCircleLineMesh.size() };
	gizmoManagerInfo.debugUtils = debugUtils;
	gizmoManagerInfo.delQueue = &delQueue;
	gizmoManagerInfo.device = &device;
	gizmoManagerInfo.inFlightCount = inFlightCount;
	gizmoManagerInfo.frameAlloc = &transientAlloc;
	gizmoManagerInfo.queues = &queues;
	gizmoManagerInfo.vma = &vma;
	GizmoManager::Initialize(apiData.gizmoManager, gizmoManagerInfo);


	if constexpr (Gfx::enableDedicatedThread) {
		apiData.thread.renderingThread = std::thread(&RenderingThreadEntryPoint, &apiData);
	}

	auto returnVal = apiDataPtr.Release();
	return returnVal;
}

void Vk::Init::InitTestPipeline(
	APIData& apiData,
	Std::AllocRef const& transientAlloc)
{
	auto const& globUtils = apiData.m_globUtils;
	auto const& device = globUtils.device;
	auto const* debugUtils = globUtils.DebugUtilsPtr();

	vk::Result vkResult = {};

	Std::Array<vk::DescriptorSetLayout, 3> layouts{
		apiData.viewportManager.cameraDescrLayout,
		apiData.objectDataManager.GetObjectDataUniformDescrLayout(),
		apiData.textureManager.descrSetLayout.Handle() };

	vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.setLayoutCount = 3;
	pipelineLayoutInfo.pSetLayouts = layouts.Data();
	apiData.testPipelineLayout = device.Create(pipelineLayoutInfo);
	if (debugUtils) {
		debugUtils->Helper_SetObjectName(
			device.handle,
			apiData.testPipelineLayout,
			"Test Pipelinelayout");
	}

	auto vertModule = ShaderHelpers::LoadShaderModuleFromFile(
		device,
		Std::CStrToSpan("assets/GameShaders/TexturedQuad.vert.spv"),
		transientAlloc);
	vk::PipelineShaderStageCreateInfo vertStageInfo = {};
	vertStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
	vertStageInfo.module = vertModule.Handle();
	vertStageInfo.pName = "main";

	auto fragModule = ShaderHelpers::LoadShaderModuleFromFile(
		device,
		Std::CStrToSpan("assets/GameShaders/TexturedQuad.frag.spv"),
		transientAlloc);
	vk::PipelineShaderStageCreateInfo fragStageInfo = {};
	fragStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
	fragStageInfo.module = fragModule.Handle();
	fragStageInfo.pName = "main";

	Std::Array<vk::PipelineShaderStageCreateInfo, 2> shaderStages = { vertStageInfo, fragStageInfo };

	vk::PipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vk::PipelineInputAssemblyStateCreateInfo inputAssembly = {};
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

	vk::PipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.lineWidth = 1.f;
	rasterizer.polygonMode = vk::PolygonMode::eFill;
	rasterizer.frontFace = vk::FrontFace::eCounterClockwise;
	rasterizer.rasterizerDiscardEnable = 0;
	rasterizer.cullMode = vk::CullModeFlagBits::eNone;

	vk::PipelineDepthStencilStateCreateInfo depthStencilInfo = {};
	depthStencilInfo.depthTestEnable = 0;
	depthStencilInfo.depthCompareOp = vk::CompareOp::eLess;
	depthStencilInfo.stencilTestEnable = 0;
	depthStencilInfo.depthWriteEnable = 0;
	depthStencilInfo.minDepthBounds = 0.f;
	depthStencilInfo.maxDepthBounds = 1.f;

	vk::PipelineMultisampleStateCreateInfo multisampling = {};
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
	vk::PipelineDynamicStateCreateInfo dynamicState = {};
	dynamicState.dynamicStateCount = 1;
	dynamicState.pDynamicStates = &temp;

	vk::GraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.pDepthStencilState = &depthStencilInfo;
	pipelineInfo.layout = apiData.testPipelineLayout;
	pipelineInfo.renderPass = globUtils.gfxRenderPass;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.stageCount = (u32)shaderStages.Size();
	pipelineInfo.pStages = shaderStages.Data();

	vkResult = device.Create(pipelineInfo, &apiData.testPipeline);
	if (vkResult != vk::Result::eSuccess)
		throw std::runtime_error("Unable to make graphics pipeline.");
	if (debugUtils) {
		debugUtils->Helper_SetObjectName(
			device.handle,
			apiData.testPipeline,
			"Test Pipeline");
	}
}
