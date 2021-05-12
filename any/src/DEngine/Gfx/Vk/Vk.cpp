#include "Vk.hpp"
#include <DEngine/Gfx/detail/Assert.hpp>
#include "Init.hpp"

#include "GizmoManager.hpp"

#include <DEngine/FixedWidthTypes.hpp>
#include <DEngine/Std/Containers/Span.hpp>
#include <DEngine/Std/Containers/StackVec.hpp>
#include <DEngine/Std/Utility.hpp>
// For file IO
#include <DEngine/Application.hpp>

#include <string>

//vk::DispatchLoaderDynamic vk::defaultDispatchLoaderDynamic;

namespace DEngine::Gfx::Vk
{
	bool InitializeBackend(Context& gfxData, InitInfo const& initInfo, void*& apiDataBuffer);

	namespace Init
	{
		void GetPrettyFunctionString(APIData& apiData);
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
	[[noreturn]] void GetPrettyFunctionString(APIData* inApiData)
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

bool Vk::InitializeBackend(Context& gfxData, InitInfo const& initInfo, void*& apiDataBuffer)
{
	apiDataBuffer = new APIData;
	APIData& apiData = *static_cast<APIData*>(apiDataBuffer);
	GlobUtils& globUtils = apiData.globUtils;

	apiData.renderingThread = std::thread(&GetPrettyFunctionString, &apiData);


	vk::Result vkResult{};
	bool boolResult = false;

	apiData.logger = initInfo.optional_logger;
	apiData.globUtils.logger = initInfo.optional_logger;
	apiData.test_textureAssetInterface = initInfo.texAssetInterface;

	globUtils.editorMode = true;
	globUtils.inFlightCount = Constants::preferredInFlightCount;

	// Make the VkInstance
	PFN_vkGetInstanceProcAddr instanceProcAddr = Vk::loadInstanceProcAddressPFN();
	BaseDispatch baseDispatch{};
	BaseDispatch::BuildInPlace(baseDispatch, instanceProcAddr);
	Init::CreateVkInstance_Return createVkInstanceResult = Init::CreateVkInstance(
		initInfo.requiredVkInstanceExtensions, 
		true, 
		baseDispatch, 
		apiData.logger);
	InstanceDispatch::BuildInPlace(
		globUtils.instance, 
		createVkInstanceResult.instanceHandle, 
		instanceProcAddr);

	// Enable DebugUtils functionality.
	// If we enabled debug utils, we build the debug utils dispatch table and the debug utils messenger.
	if (createVkInstanceResult.debugUtilsEnabled)
	{
		DebugUtilsDispatch::BuildInPlace(
			globUtils.debugUtils,
			globUtils.instance.handle, 
			instanceProcAddr);
		globUtils.debugMessenger = Init::CreateLayerMessenger(
			globUtils.instance.handle, 
			globUtils.DebugUtilsPtr(), 
			initInfo.optional_logger);
	}

	// TODO: I don't think I like this code
	// Create the VkSurface using the callback
	vk::SurfaceKHR surface{};
	vk::Result surfaceCreateResult = (vk::Result)initInfo.initialWindowConnection->CreateVkSurface(
		(uSize)static_cast<VkInstance>(globUtils.instance.handle), 
		nullptr, 
		*reinterpret_cast<u64*>(&surface));
	if (surfaceCreateResult != vk::Result::eSuccess)
		throw std::runtime_error("Unable to create VkSurfaceKHR object during initialization.");

	// Pick our phys device and load the info for it
	apiData.globUtils.physDevice = Init::LoadPhysDevice(globUtils.instance, surface);

	SurfaceInfo::BuildInPlace(
		globUtils.surfaceInfo,
		surface,
		globUtils.instance,
		apiData.globUtils.physDevice.handle);

	PFN_vkGetDeviceProcAddr deviceProcAddr = (PFN_vkGetDeviceProcAddr)instanceProcAddr((VkInstance)globUtils.instance.handle, "vkGetDeviceProcAddr");
	// Create the device and the dispatch table for it.
	vk::Device deviceHandle = Init::CreateDevice(globUtils.instance, globUtils.physDevice);
	DeviceDispatch::BuildInPlace(
		globUtils.device,
		deviceHandle,
		deviceProcAddr);

	QueueData::Init(
		globUtils.queues,
		globUtils.device,
		globUtils.physDevice.queueIndices,
		globUtils.DebugUtilsPtr());

	globUtils.device.m_queueDataPtr = &globUtils.queues;

	boolResult = DeletionQueue::Init(
		globUtils.delQueue,
		globUtils.inFlightCount);
	if (!boolResult)
		throw std::runtime_error("DEngine - Vulkan: Failed to initialize DeletionQueue");

	// Init VMA
	apiData.vma_trackingData.deviceHandle = globUtils.device.handle;
	apiData.vma_trackingData.debugUtils = globUtils.DebugUtilsPtr();
	vk::ResultValue<VmaAllocator> vmaResult = Init::InitializeVMA(
		globUtils.instance,
		globUtils.physDevice.handle,
		globUtils.device,
		&apiData.vma_trackingData);
	if (vmaResult.result != vk::Result::eSuccess)
		throw std::runtime_error("DEngine - Vulkan: Failed to initialize VMA.");
	globUtils.vma = vmaResult.value;

	// Initialize main cmd buffer
	{
		apiData.mainCmdPools.Resize(globUtils.inFlightCount);
		for (uSize i = 0; i < globUtils.inFlightCount; i += 1)
		{
			vk::CommandPoolCreateInfo cmdPoolInfo = {};
			//cmdPoolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
			cmdPoolInfo.queueFamilyIndex = globUtils.queues.graphics.FamilyIndex();
			auto cmdPool = globUtils.device.createCommandPool(cmdPoolInfo);
			if (cmdPool.result != vk::Result::eSuccess)
				throw std::runtime_error("Unable to make command pool");
			apiData.mainCmdPools[i] = cmdPool.value;
			if (globUtils.UsingDebugUtils())
			{
				std::string name = "Main CmdPool #";
				name += std::to_string(i);
				globUtils.debugUtils.Helper_SetObjectName(
					globUtils.device.handle,
					apiData.mainCmdPools[i],
					name.c_str());
			}

			vk::CommandBufferAllocateInfo cmdBufferAllocInfo = {};
			cmdBufferAllocInfo.commandBufferCount = 1;
			cmdBufferAllocInfo.commandPool = apiData.mainCmdPools[i];
			cmdBufferAllocInfo.level = vk::CommandBufferLevel::ePrimary;
			apiData.mainCmdBuffers.Resize(globUtils.inFlightCount);
			vkResult = globUtils.device.allocateCommandBuffers(cmdBufferAllocInfo, &apiData.mainCmdBuffers[i]);
			if (vkResult != vk::Result::eSuccess)
				throw std::runtime_error("DEngine - Vulkan: Failed to initialize main commandbuffers.");
			// We don't give the command buffers debug names here, because we need to rename them everytime we re-record anyways.
		}
	}

	NativeWindowManager::Initialize(
		apiData.nativeWindowManager,
		globUtils.device,
		globUtils.queues,
		globUtils.DebugUtilsPtr());

	boolResult = ViewportManager::Init(
		apiData.viewportManager,
		globUtils.device,
		globUtils.physDevice.properties.limits.minUniformBufferOffsetAlignment,
		globUtils.DebugUtilsPtr());
	if (!boolResult)
		throw std::runtime_error("DEngine - Vulkan: Failed to initialize ViewportManager.");

	TextureManager::Init(
		apiData.textureManager,
		globUtils.device,
		globUtils.queues,
		globUtils.DebugUtilsPtr());

	boolResult = ObjectDataManager::Init(
		apiData.objectDataManager,
		globUtils.physDevice.properties.limits.minUniformBufferOffsetAlignment,
		globUtils.inFlightCount,
		globUtils.vma,
		globUtils.device,
		globUtils.DebugUtilsPtr());
	if (!boolResult)
		throw std::runtime_error("DEngine - Vulkan: Failed to initialize ObjectDataManager.");


	// Create our main fences
	apiData.mainFences = Init::CreateMainFences(
		globUtils.device,
		globUtils.inFlightCount,
		globUtils.DebugUtilsPtr());

	
	apiData.globUtils.guiRenderPass = Init::CreateGuiRenderPass(
			globUtils.device,
			globUtils.surfaceInfo.surfaceFormatToUse.format,
			globUtils.DebugUtilsPtr());

	NativeWindowManager::BuildInitialNativeWindow(
		apiData.nativeWindowManager,
		globUtils.instance,
		globUtils.device,
		globUtils.physDevice.handle,
		globUtils.queues,
		globUtils.delQueue,
		globUtils.surfaceInfo,
		globUtils.vma,
		globUtils.inFlightCount,
		*initInfo.initialWindowConnection,
		surface,
		globUtils.guiRenderPass,
		globUtils.DebugUtilsPtr());


	// Create the main render stuff
	globUtils.gfxRenderPass = Init::BuildMainGfxRenderPass(
		globUtils.device,
		true, 
		globUtils.DebugUtilsPtr());

	Init::GetPrettyFunctionString(apiData);


	GuiResourceManager::Init(
		apiData.guiResourceManager, 
		globUtils.device,
		globUtils.vma, 
		globUtils.inFlightCount, 
		globUtils.guiRenderPass,
		globUtils.DebugUtilsPtr());

	GizmoManager::Initialize(
		apiData.gizmoManager,
		globUtils.inFlightCount,
		globUtils.device,
		globUtils.queues,
		globUtils.vma,
		globUtils.delQueue,
		globUtils.DebugUtilsPtr(),
		apiData,
		{ initInfo.gizmoArrowMesh.data(), initInfo.gizmoArrowMesh.size() },
		{ initInfo.gizmoCircleLineMesh.data(), initInfo.gizmoCircleLineMesh.size() });

	return true;
}

void Vk::Init::GetPrettyFunctionString(APIData& apiData)
{
	vk::Result vkResult{};

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

	App::FileInputStream vertFile{ "data/vert.spv" };
	if (!vertFile.IsOpen())
		throw std::runtime_error("Could not open vertex shader file");
	vertFile.Seek(0, App::FileInputStream::SeekOrigin::End);
	u64 vertFileLength = vertFile.Tell().Value();
	vertFile.Seek(0, App::FileInputStream::SeekOrigin::Start);
	std::vector<char> vertCode((uSize)vertFileLength);
	vertFile.Read(vertCode.data(), vertFileLength);
	
	vk::ShaderModuleCreateInfo vertModCreateInfo{};
	vertModCreateInfo.codeSize = vertCode.size();
	vertModCreateInfo.pCode = reinterpret_cast<const u32*>(vertCode.data());
	vk::ShaderModule vertModule = apiData.globUtils.device.createShaderModule(vertModCreateInfo);
	vk::PipelineShaderStageCreateInfo vertStageInfo{};
	vertStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
	vertStageInfo.module = vertModule;
	vertStageInfo.pName = "main";

	App::FileInputStream fragFile{ "data/frag.spv" };
	if (!fragFile.IsOpen())
		throw std::runtime_error("Could not open fragment shader file");
	fragFile.Seek(0, App::FileInputStream::SeekOrigin::End);
	u64 fragFileLength = fragFile.Tell().Value();
	fragFile.Seek(0, App::FileInputStream::SeekOrigin::Start);
	std::vector<char> fragCode((uSize)fragFileLength);
	fragFile.Read(fragCode.data(), fragFileLength);

	vk::ShaderModuleCreateInfo fragModInfo{};
	fragModInfo.codeSize = fragCode.size();
	fragModInfo.pCode = reinterpret_cast<const u32*>(fragCode.data());
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
