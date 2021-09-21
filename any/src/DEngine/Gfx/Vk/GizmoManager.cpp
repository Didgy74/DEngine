#include "GizmoManager.hpp"

#include "DeletionQueue.hpp"
#include "DynamicDispatch.hpp"
#include "QueueData.hpp"
#include "Vk.hpp"

#include <DEngine/Math/LinearTransform3D.hpp>
#include <DEngine/Application.hpp> // VERY WIP. NEEDS TO BE SWITCHED OUT FOR SOMETHING AGNOSTIC

namespace DEngine::Gfx::Vk::impl
{
	class BoxVkBuffer
	{
	public:
		constexpr BoxVkBuffer() noexcept = default;
		BoxVkBuffer(BoxVkBuffer const&) = delete;
		BoxVkBuffer& operator=(BoxVkBuffer const&) = delete;

		vk::Buffer handle = {};
		VmaAllocator vma = {};
		VmaAllocation alloc = {};

		void Release() noexcept { handle = vk::Buffer{}; }

		~BoxVkBuffer() noexcept
		{
			if (handle != vk::Buffer{})
			{
				vmaDestroyBuffer(vma, (VkBuffer)handle, alloc);
			}
		}
	};

	class BoxVkCmdPool
	{
	public:
		constexpr BoxVkCmdPool() noexcept = default;
		constexpr BoxVkCmdPool(BoxVkCmdPool const&) = delete;
		BoxVkCmdPool& operator=(BoxVkCmdPool const&) = delete;

		vk::CommandPool handle = {};
		DeviceDispatch const* device = nullptr;

		void Release() noexcept { handle = vk::CommandPool{}; }

		~BoxVkCmdPool() noexcept
		{
			if (handle != vk::CommandPool{})
			{
				device->destroy(handle);
			}
		}
	};

	struct GizmoManager_Helper_TestReturn
	{
		vk::Result result;
		char const* error;
		vk::Buffer buffer;
		VmaAllocation alloc;
	};

	[[nodiscard]] static GizmoManager_Helper_TestReturn GizmoManager_Helper_Test(
		DevDispatch const& device,
		VmaAllocator vma,
		QueueData const& queues,
		DeletionQueue const& delQueue,
		Std::Span<char const> bytes) noexcept
	{
		GizmoManager_Helper_TestReturn returnVal = {};

		BoxVkBuffer dstBuffer = {};
		dstBuffer.vma = vma;
		vk::BufferCreateInfo vtxBufferInfo = {};
		vtxBufferInfo.sharingMode = vk::SharingMode::eExclusive;
		vtxBufferInfo.size = bytes.Size();
		vtxBufferInfo.usage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst;
		VmaAllocationCreateInfo vtxVmaAllocInfo = {};
		vtxVmaAllocInfo.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_GPU_ONLY;
		returnVal.result = (vk::Result)vmaCreateBuffer(
			vma,
			(VkBufferCreateInfo*)&vtxBufferInfo,
			&vtxVmaAllocInfo,
			(VkBuffer*)&dstBuffer.handle,
			&dstBuffer.alloc,
			nullptr);
		if (returnVal.result != vk::Result::eSuccess)
		{
			returnVal.error = "DEngine-Gfx-Vk: Error while creating dst buffer.";
			return returnVal;
		}

		vk::BufferCreateInfo vtxBufferInfo_Staging = {};
		vtxBufferInfo_Staging.sharingMode = vk::SharingMode::eExclusive;
		vtxBufferInfo_Staging.size = bytes.Size();
		vtxBufferInfo_Staging.usage = vk::BufferUsageFlagBits::eTransferSrc;
		VmaAllocationCreateInfo vtxVmaAllocInfo_Staging = {};
		vtxVmaAllocInfo_Staging.flags = VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_MAPPED_BIT;
		vtxVmaAllocInfo_Staging.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_CPU_ONLY;
		VmaAllocationInfo vtxVmaAllocResultInfo_Staging = {};

		BoxVkBuffer vtxBuffer_Staging = {};
		vtxBuffer_Staging.vma = vma;

		returnVal.result = (vk::Result)vmaCreateBuffer(
			vma,
			(VkBufferCreateInfo*)&vtxBufferInfo_Staging,
			&vtxVmaAllocInfo_Staging,
			(VkBuffer*)&vtxBuffer_Staging.handle,
			&vtxBuffer_Staging.alloc,
			&vtxVmaAllocResultInfo_Staging);

		if (returnVal.result != vk::Result::eSuccess)
		{
			returnVal.error = "DEngine-Gfx-Vk - GizmoManager_Helper_Test: Error while creating staging buffer.";
			return returnVal;
		}
		
		// Copy vertices over to staging buffer
		std::memcpy(vtxVmaAllocResultInfo_Staging.pMappedData, bytes.Data(), vtxBufferInfo_Staging.size);

		// Copy vertex data over
		vk::CommandPoolCreateInfo cmdPoolInfo = {};
		vk::ResultValue<vk::CommandPool> cmdPoolResult = device.createCommandPool(cmdPoolInfo);
		BoxVkCmdPool cmdPool = {};
		cmdPool.device = &device;
		cmdPool.handle = cmdPoolResult.value;
		if (cmdPoolResult.result != vk::Result::eSuccess)
		{
			returnVal.error = "DEngine-Gfx-Vk - GizmoManager_Helper_Test: Error while creating staging buffer.";
			return returnVal;
		}
		vk::CommandBufferAllocateInfo cmdBufferAllocInfo{};
		cmdBufferAllocInfo.commandPool = cmdPool.handle;
		cmdBufferAllocInfo.level = vk::CommandBufferLevel::ePrimary;
		cmdBufferAllocInfo.commandBufferCount = 1;
		vk::CommandBuffer cmdBuffer = {};
		returnVal.result = device.allocateCommandBuffers(cmdBufferAllocInfo, &cmdBuffer);
		if (returnVal.result != vk::Result::eSuccess)
		{
			returnVal.error = "DEngine-Gfx-Vk - GizmoManager_Helper_Test: Error while creating .";
			return returnVal;
		}

		vk::CommandBufferBeginInfo cmdBeginInfo = {};
		cmdBeginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
		device.beginCommandBuffer(cmdBuffer, cmdBeginInfo);

		vk::BufferCopy copyRegion{};
		copyRegion.size = vtxBufferInfo_Staging.size;
		device.cmdCopyBuffer(
			cmdBuffer,
			vtxBuffer_Staging.handle,
			dstBuffer.handle,
			{ copyRegion });

		vk::BufferMemoryBarrier buffBarrier = {};
		buffBarrier.buffer = dstBuffer.handle;
		buffBarrier.size = vtxBufferInfo.size;
		buffBarrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
		buffBarrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead;
		device.cmdPipelineBarrier(
			cmdBuffer,
			vk::PipelineStageFlagBits::eTransfer,
			vk::PipelineStageFlagBits::eColorAttachmentOutput,
			vk::DependencyFlags(),
			nullptr,
			buffBarrier,
			{});

		device.endCommandBuffer(cmdBuffer);

		vk::SubmitInfo submit{};
		submit.commandBufferCount = 1;
		submit.pCommandBuffers = &cmdBuffer;
		vk::Fence fence = device.createFence({});
		queues.graphics.submit(submit, fence);

		struct DestroyData
		{
			vk::Buffer vtxBuffer_Staging;
			VmaAllocation vtxVmaAlloc_Staging;
			vk::CommandPool cmdPool;
		};
		DestroyData destroyData = {};
		destroyData.vtxBuffer_Staging = vtxBuffer_Staging.handle;
		destroyData.vtxVmaAlloc_Staging = vtxBuffer_Staging.alloc;
		vtxBuffer_Staging.Release();
		destroyData.cmdPool = cmdPool.handle;
		cmdPool.Release();
		DeletionQueue::TestCallback<DestroyData> destroyCallback = [](
			GlobUtils const& globUtils,
			DestroyData destroyData)
		{
			vmaDestroyBuffer(globUtils.vma, (VkBuffer)destroyData.vtxBuffer_Staging, destroyData.vtxVmaAlloc_Staging);
			globUtils.device.destroy(destroyData.cmdPool);
		};
		delQueue.DestroyTest(fence, destroyCallback, destroyData);

		returnVal.buffer = dstBuffer.handle;
		returnVal.alloc = dstBuffer.alloc;
		dstBuffer.Release();
		return returnVal;
	}

	static void GizmoManager_InitializeArrowMesh(
		GizmoManager& manager,
		DeviceDispatch const& device,
		QueueData const& queues,
		VmaAllocator vma,
		DeletionQueue const& delQueue,
		DebugUtilsDispatch const* debugUtils,
		Std::Span<Math::Vec3 const> arrowMesh)
	{
		auto test = GizmoManager_Helper_Test(
			device,
			vma,
			queues,
			delQueue,
			{ (char const*)arrowMesh.Data(), arrowMesh.Size() * sizeof(arrowMesh[0]) });
		if (test.result != vk::Result::eSuccess)
			throw std::runtime_error(std::string("Unable to create gizmo translate arrow mesh") + test.error);

		manager.arrowVtxCount = (u32)arrowMesh.Size();
		manager.arrowVtxBuffer = test.buffer;
		manager.arrowVtxVmaAlloc = test.alloc;

		if (debugUtils)
		{
			debugUtils->Helper_SetObjectName(
				device.handle,
				manager.arrowVtxBuffer,
				"GizmoManager - Translate Arrow VertexBuffer");
		}
	}

	static void GizmoManager_InitializeRotateCircleMesh(
		GizmoManager& manager,
		DeviceDispatch const& device,
		QueueData const& queues,
		VmaAllocator vma,
		DeletionQueue const& delQueue,
		DebugUtilsDispatch const* debugUtils,
		Std::Span<Math::Vec3 const> circleLineMesh)
	{
		auto test = GizmoManager_Helper_Test(
			device,
			vma,
			queues,
			delQueue,
			{ (char const*)circleLineMesh.Data(), circleLineMesh.Size() * sizeof(circleLineMesh[0]) });
		if (test.result != vk::Result::eSuccess)
			throw std::runtime_error(std::string("Unable to create gizmo rotate circle mesh") + test.error);

		manager.circleVtxBuffer = test.buffer;
		manager.circleVtxAlloc = test.alloc;
		manager.circleVtxCount = (u32)circleLineMesh.Size();

		if (debugUtils)
		{
			debugUtils->Helper_SetObjectName(
				device.handle,
				manager.circleVtxBuffer,
				"GizmoManager - Rotate Circle VertexBuffer");
		}
	}

	static void GizmoManager_InitializeScaleArrow2dMesh(
		GizmoManager& manager,
		DeviceDispatch const& device,
		QueueData const& queues,
		VmaAllocator vma,
		DeletionQueue const& delQueue,
		DebugUtilsDispatch const* debugUtils,
		Std::Span<Math::Vec3 const> scaleArrow2d)
	{
		auto test = GizmoManager_Helper_Test(
			device,
			vma,
			queues,
			delQueue,
			{ (char const*)scaleArrow2d.Data(), scaleArrow2d.Size() * sizeof(scaleArrow2d[0]) });
		if (test.result != vk::Result::eSuccess)
			throw std::runtime_error(std::string("Unable to create gizmo translate arrow mesh") + test.error);

		manager.scaleArrow2d_VtxCount = (u32)scaleArrow2d.Size();
		manager.scaleArrow2d_VtxBuffer = test.buffer;
		manager.scaleArrow2d_VtxAlloc = test.alloc;

		if (debugUtils)
		{
			debugUtils->Helper_SetObjectName(
				device.handle,
				manager.scaleArrow2d_VtxBuffer,
				"GizmoManager - Translate Scale Arrow 2D VtxBuffer");
		}
	}

	static void GizmoManager_InitializeArrowShader(
		GizmoManager& manager,
		DeviceDispatch const& device,
		DebugUtilsDispatch const* debugUtils,
		APIData const& apiData)
	{
		vk::Result vkResult{};

		Std::Array<vk::DescriptorSetLayout, 1> layouts{
			apiData.viewportManager.cameraDescrLayout };

		vk::PushConstantRange vertPushConstantRange{};
		vertPushConstantRange.size = sizeof(Math::Mat4);
		vertPushConstantRange.stageFlags = vk::ShaderStageFlagBits::eVertex;
		vk::PushConstantRange fragPushConstantRange{};
		fragPushConstantRange.offset = vertPushConstantRange.size;
		fragPushConstantRange.size = sizeof(Math::Vec4);
		fragPushConstantRange.stageFlags = vk::ShaderStageFlagBits::eFragment;
		Std::Array<vk::PushConstantRange, 2> pushConstantRanges = {
			vertPushConstantRange,
			fragPushConstantRange };

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.setLayoutCount = (u32)layouts.Size();
		pipelineLayoutInfo.pSetLayouts = layouts.Data();
		pipelineLayoutInfo.pushConstantRangeCount = (u32)pushConstantRanges.Size();
		pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.Data();
		manager.pipelineLayout = apiData.globUtils.device.createPipelineLayout(pipelineLayoutInfo);

		App::FileInputStream vertFile{ "data/Gizmo/Arrow/vert.spv" };
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

		App::FileInputStream fragFile{ "data/Gizmo/Arrow/frag.spv" };
		if (!fragFile.IsOpen())
			throw std::runtime_error("Could not open fragment shader file");
		fragFile.Seek(0, App::FileInputStream::SeekOrigin::End);
		u64 fragFileLength = fragFile.Tell().Value();
		fragFile.Seek(0, App::FileInputStream::SeekOrigin::Start);
		std::vector<char> fragCode((uSize)fragFileLength);
		fragFile.Read(fragCode.data(), fragFileLength);

		vk::ShaderModuleCreateInfo fragModInfo{};
		fragModInfo.codeSize = fragCode.size();
		fragModInfo.pCode = reinterpret_cast<u32 const*>(fragCode.data());
		vk::ShaderModule fragModule = apiData.globUtils.device.createShaderModule(fragModInfo);
		vk::PipelineShaderStageCreateInfo fragStageInfo{};
		fragStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
		fragStageInfo.module = fragModule;
		fragStageInfo.pName = "main";

		Std::Array<vk::PipelineShaderStageCreateInfo, 2> shaderStages = { vertStageInfo, fragStageInfo };

		vk::VertexInputAttributeDescription position{};
		position.binding = 0;
		position.format = vk::Format::eR32G32B32Sfloat;
		position.location = 0;
		position.offset = 0;
		vk::VertexInputBindingDescription binding{};
		binding.binding = 0;
		binding.inputRate = vk::VertexInputRate::eVertex;
		binding.stride = 12;
		vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.vertexAttributeDescriptionCount = 1;
		vertexInputInfo.pVertexAttributeDescriptions = &position;
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.pVertexBindingDescriptions = &binding;

		vk::PipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;

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
		pipelineInfo.layout = manager.pipelineLayout;
		pipelineInfo.renderPass = apiData.globUtils.gfxRenderPass;
		pipelineInfo.pDepthStencilState = &depthStencilInfo;
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
			&manager.arrowPipeline);
		if (vkResult != vk::Result::eSuccess)
			throw std::runtime_error("Unable to make graphics pipeline.");

		apiData.globUtils.device.destroy(vertModule);
		apiData.globUtils.device.destroy(fragModule);
	}

	static void GizmoManager_InitializeQuadShader(
		GizmoManager& manager,
		DeviceDispatch const& device,
		DebugUtilsDispatch const* debugUtils,
		APIData const& apiData)
	{
		vk::Result vkResult{};

		App::FileInputStream vertFile{ "data/Gizmo/Quad/vert.spv" };
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

		App::FileInputStream fragFile{ "data/Gizmo/Quad/frag.spv" };
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
		pipelineInfo.layout = manager.pipelineLayout;
		pipelineInfo.renderPass = apiData.globUtils.gfxRenderPass;
		pipelineInfo.pDepthStencilState = &depthStencilInfo;
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
			&manager.quadPipeline);
		if (vkResult != vk::Result::eSuccess)
			throw std::runtime_error("Unable to make graphics pipeline.");

		apiData.globUtils.device.destroy(vertModule);
		apiData.globUtils.device.destroy(fragModule);
	}

	static void GizmoManager_InitializeLineShader(
		GizmoManager& manager,
		DeviceDispatch const& device,
		DebugUtilsDispatch const* debugUtils,
		APIData const& apiData)
	{
		vk::Result vkResult{};

		App::FileInputStream vertFile{ "data/Gizmo/Line/vert.spv" };
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

		App::FileInputStream fragFile{ "data/Gizmo/Line/frag.spv" };
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

		vk::VertexInputAttributeDescription position{};
		position.binding = 0;
		position.format = vk::Format::eR32G32B32Sfloat;
		position.location = 0;
		position.offset = 0;
		vk::VertexInputBindingDescription binding{};
		binding.binding = 0;
		binding.inputRate = vk::VertexInputRate::eVertex;
		binding.stride = 12;
		vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.vertexAttributeDescriptionCount = 1;
		vertexInputInfo.pVertexAttributeDescriptions = &position;
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.pVertexBindingDescriptions = &binding;

		vk::PipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.topology = vk::PrimitiveTopology::eLineStrip;

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

		vk::PipelineMultisampleStateCreateInfo multisampling{};

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
		pipelineInfo.layout = manager.pipelineLayout;
		pipelineInfo.renderPass = apiData.globUtils.gfxRenderPass;
		pipelineInfo.pDepthStencilState = &depthStencilInfo;
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
			&manager.linePipeline);
		if (vkResult != vk::Result::eSuccess)
			throw std::runtime_error("Unable to make graphics pipeline.");

		apiData.globUtils.device.destroy(vertModule);
		apiData.globUtils.device.destroy(fragModule);
	}

	static void GizmoManager_InitializeLineVtxBuffer(
		GizmoManager& manager,
		u8 inFlightCount,
		DeviceDispatch const& device,
		VmaAllocator const& vma,
		DebugUtilsDispatch const* debugUtils)
	{
		vk::Result vkResult{};

		vk::BufferCreateInfo vtxBufferInfo{};
		vtxBufferInfo.sharingMode = vk::SharingMode::eExclusive;
		vtxBufferInfo.size = manager.lineVtxElementSize * manager.lineVtxMinCapacity * inFlightCount;
		vtxBufferInfo.usage = vk::BufferUsageFlagBits::eVertexBuffer;
		VmaAllocationCreateInfo vtxVmaAllocInfo{};
		vtxVmaAllocInfo.flags = VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_MAPPED_BIT;
		vtxVmaAllocInfo.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_CPU_TO_GPU;
		VmaAllocationInfo vmaAllocInfo{};
		vkResult = (vk::Result)vmaCreateBuffer(
			vma,
			(VkBufferCreateInfo*)&vtxBufferInfo,
			&vtxVmaAllocInfo,
			(VkBuffer*)&manager.lineVtxBuffer,
			&manager.lineVtxVmaAlloc,
			&vmaAllocInfo);
		if (vkResult != vk::Result::eSuccess)
			throw std::runtime_error("DEngine - Vulkan: VMA was unable to allocate memory for gizmo line vertices.");
		if (debugUtils)
		{
			debugUtils->Helper_SetObjectName(
				device.handle,
				manager.lineVtxBuffer,
				"GizmoManager - LineVtxBuffer");
		}

		manager.lineVtxBufferCapacity = manager.lineVtxMinCapacity;
		manager.lineVtxBufferMappedMem = { (u8*)vmaAllocInfo.pMappedData, (uSize)vtxBufferInfo.size };
	}

	constexpr f32 gizmoTransparency = 0.8f;

	static void GizmoManager_RecordTranslateGizmoDrawCalls(
		GlobUtils const& globUtils,
		GizmoManager const& gizmoManager,
		ViewportData const& viewportData,
		ViewportUpdate::Gizmo const& gizmo,
		vk::CommandBuffer cmdBuffer,
		u8 inFlightIndex)
	{
		globUtils.device.cmdBindPipeline(cmdBuffer, vk::PipelineBindPoint::eGraphics, gizmoManager.arrowPipeline);

		globUtils.device.cmdBindVertexBuffers(cmdBuffer, 0, { gizmoManager.arrowVtxBuffer }, { 0 });
		Std::Array<vk::DescriptorSet, 1> descrSets = {
			viewportData.camDataDescrSets[inFlightIndex] };
		globUtils.device.cmdBindDescriptorSets(
			cmdBuffer,
			vk::PipelineBindPoint::eGraphics,
			gizmoManager.pipelineLayout,
			0,
			{ (u32)descrSets.Size(), descrSets.Data() },
			nullptr);

		// Draw X arrow
		{
			Math::Mat4 gizmoMatrix = Math::LinAlg3D::Scale_Homo(gizmo.scale, gizmo.scale, gizmo.scale);
			gizmoMatrix = Math::LinAlg3D::Rotate_Homo(Math::ElementaryAxis::Z, gizmo.rotation) * gizmoMatrix;
			Math::LinAlg3D::SetTranslation(gizmoMatrix, gizmo.position);

			globUtils.device.cmdPushConstants(
				cmdBuffer,
				gizmoManager.pipelineLayout,
				vk::ShaderStageFlagBits::eVertex,
				0,
				sizeof(gizmoMatrix),
				&gizmoMatrix);
			Math::Vec4 color = { 1.f, 0.f, 0.f, gizmoTransparency };
			globUtils.device.cmdPushConstants(
				cmdBuffer,
				gizmoManager.pipelineLayout,
				vk::ShaderStageFlagBits::eFragment,
				sizeof(gizmoMatrix),
				sizeof(color),
				&color);

			globUtils.device.cmdDraw(
				cmdBuffer,
				gizmoManager.arrowVtxCount,
				1,
				0,
				0);
		}
		// Draw Y arrow
		{
			Math::Mat4 gizmoMatrix = Math::LinAlg3D::Scale_Homo(gizmo.scale, gizmo.scale, gizmo.scale);
			gizmoMatrix = Math::LinAlg3D::Rotate_Homo(Math::ElementaryAxis::Z, Math::pi / 2 + gizmo.rotation) * gizmoMatrix;
			Math::LinAlg3D::SetTranslation(gizmoMatrix, gizmo.position);

			globUtils.device.cmdPushConstants(
				cmdBuffer,
				gizmoManager.pipelineLayout,
				vk::ShaderStageFlagBits::eVertex,
				0,
				sizeof(gizmoMatrix),
				&gizmoMatrix);
			Math::Vec4 color = { 0.f, 1.f, 0.f, gizmoTransparency };
			globUtils.device.cmdPushConstants(
				cmdBuffer,
				gizmoManager.pipelineLayout,
				vk::ShaderStageFlagBits::eFragment,
				sizeof(gizmoMatrix),
				sizeof(color),
				&color);
			globUtils.device.cmdDraw(
				cmdBuffer,
				gizmoManager.arrowVtxCount,
				1,
				0,
				0);
		}

		// Draw the floating quad thing
		{
			globUtils.device.cmdBindPipeline(cmdBuffer, vk::PipelineBindPoint::eGraphics, gizmoManager.quadPipeline);

			Math::Mat4 gizmoMatrix = Math::LinAlg3D::Scale_Homo(gizmo.quadScale, gizmo.quadScale, gizmo.quadScale);
			Math::Vec3 preTranslation = Math::Vec3{ 1.f, 1.f, 0.f } * gizmo.quadOffset;
			Math::LinAlg3D::SetTranslation(gizmoMatrix, preTranslation);
			gizmoMatrix = Math::LinAlg3D::Rotate_Homo(Math::ElementaryAxis::Z, gizmo.rotation) * gizmoMatrix;
			Math::Vec3 translation = gizmo.position;
			gizmoMatrix = Math::LinAlg3D::Translate(translation) * gizmoMatrix;
			globUtils.device.cmdPushConstants(
				cmdBuffer,
				gizmoManager.pipelineLayout,
				vk::ShaderStageFlagBits::eVertex,
				0,
				sizeof(gizmoMatrix),
				&gizmoMatrix);
			Math::Vec4 color = { 1.f, 1.f, 0.f, gizmoTransparency };
			globUtils.device.cmdPushConstants(
				cmdBuffer,
				gizmoManager.pipelineLayout,
				vk::ShaderStageFlagBits::eFragment,
				sizeof(gizmoMatrix),
				sizeof(color),
				&color);
			globUtils.device.cmdDraw(
				cmdBuffer,
				4,
				1,
				0,
				0);
		}
	}

	static void GizmoManager_RotateGizmo_RecordoDrawCalls(
		GlobUtils const& globUtils,
		GizmoManager const& gizmoManager,
		ViewportData const& viewportData,
		ViewportUpdate::Gizmo const& gizmo,
		vk::CommandBuffer cmdBuffer,
		u8 inFlightIndex)
	{
		globUtils.device.cmdBindPipeline(cmdBuffer, vk::PipelineBindPoint::eGraphics, gizmoManager.arrowPipeline);

		globUtils.device.cmdBindVertexBuffers(cmdBuffer, 0, { gizmoManager.circleVtxBuffer }, { 0 });
		Std::Array<vk::DescriptorSet, 1> descrSets = {
			viewportData.camDataDescrSets[inFlightIndex] };
		globUtils.device.cmdBindDescriptorSets(
			cmdBuffer,
			vk::PipelineBindPoint::eGraphics,
			gizmoManager.pipelineLayout,
			0,
			{ (u32)descrSets.Size(), descrSets.Data() },
			nullptr);

		Math::Mat4 gizmoMatrix = Math::LinAlg3D::Scale_Homo(gizmo.scale, gizmo.scale, gizmo.scale);
		Math::LinAlg3D::SetTranslation(gizmoMatrix, gizmo.position);
		globUtils.device.cmdPushConstants(
			cmdBuffer,
			gizmoManager.pipelineLayout,
			vk::ShaderStageFlagBits::eVertex,
			0,
			sizeof(gizmoMatrix),
			&gizmoMatrix);
		Math::Vec4 color = { 0.f, 0.f, 1.f, gizmoTransparency };
		globUtils.device.cmdPushConstants(
			cmdBuffer,
			gizmoManager.pipelineLayout,
			vk::ShaderStageFlagBits::eFragment,
			sizeof(gizmoMatrix),
			sizeof(color),
			&color);

		globUtils.device.cmdDraw(
			cmdBuffer,
			gizmoManager.circleVtxCount,
			1,
			0,
			0);
	}

	static void GizmoManager_ScaleGizmo_RecordDrawCalls(
		GlobUtils const& globUtils,
		GizmoManager const& manager,
		ViewportData const& viewportData,
		ViewportUpdate::Gizmo const& gizmo,
		vk::CommandBuffer cmdBuffer,
		u8 inFlightIndex)
	{
		globUtils.device.cmdBindPipeline(cmdBuffer, vk::PipelineBindPoint::eGraphics, manager.arrowPipeline);

		globUtils.device.cmdBindVertexBuffers(cmdBuffer, 0, { manager.scaleArrow2d_VtxBuffer }, { 0 });

		Std::Array<vk::DescriptorSet, 1> descrSets = { viewportData.camDataDescrSets[inFlightIndex] };
		globUtils.device.cmdBindDescriptorSets(
			cmdBuffer,
			vk::PipelineBindPoint::eGraphics,
			manager.pipelineLayout,
			0,
			{ (u32)descrSets.Size(), descrSets.Data() },
			nullptr);

		// Draw X arrow
		{
			Math::Mat4 gizmoMatrix = Math::LinAlg3D::Scale_Homo(gizmo.scale, gizmo.scale, gizmo.scale);
			gizmoMatrix = Math::LinAlg3D::Rotate_Homo(Math::ElementaryAxis::Z, gizmo.rotation) * gizmoMatrix;
			Math::LinAlg3D::SetTranslation(gizmoMatrix, gizmo.position);

			globUtils.device.cmdPushConstants(
				cmdBuffer,
				manager.pipelineLayout,
				vk::ShaderStageFlagBits::eVertex,
				0,
				sizeof(gizmoMatrix),
				&gizmoMatrix);

			Math::Vec4 color = { 1.f, 0.f, 0.f, gizmoTransparency };
			globUtils.device.cmdPushConstants(
				cmdBuffer,
				manager.pipelineLayout,
				vk::ShaderStageFlagBits::eFragment,
				sizeof(gizmoMatrix),
				sizeof(color),
				&color);

			globUtils.device.cmdDraw(
				cmdBuffer,
				manager.scaleArrow2d_VtxCount,
				1,
				0,
				0);
		}

		// Draw Y arrow
		{
			Math::Mat4 gizmoMatrix = Math::LinAlg3D::Scale_Homo(gizmo.scale, gizmo.scale, gizmo.scale);
			gizmoMatrix = Math::LinAlg3D::Rotate_Homo(Math::ElementaryAxis::Z, Math::pi / 2 + gizmo.rotation) * gizmoMatrix;
			Math::LinAlg3D::SetTranslation(gizmoMatrix, gizmo.position);

			globUtils.device.cmdPushConstants(
				cmdBuffer,
				manager.pipelineLayout,
				vk::ShaderStageFlagBits::eVertex,
				0,
				sizeof(gizmoMatrix),
				&gizmoMatrix);

			Math::Vec4 color = { 0.f, 1.f, 0.f, gizmoTransparency };
			globUtils.device.cmdPushConstants(
				cmdBuffer,
				manager.pipelineLayout,
				vk::ShaderStageFlagBits::eFragment,
				sizeof(gizmoMatrix),
				sizeof(color),
				&color);

			globUtils.device.cmdDraw(
				cmdBuffer,
				manager.scaleArrow2d_VtxCount,
				1,
				0,
				0);
		}

		// Draw the floating quad thing
		{
			globUtils.device.cmdBindPipeline(cmdBuffer, vk::PipelineBindPoint::eGraphics, manager.quadPipeline);

			Math::Mat4 gizmoMatrix = Math::LinAlg3D::Scale_Homo(gizmo.quadScale, gizmo.quadScale, gizmo.quadScale);
			Math::Vec3 preTranslation = Math::Vec3{ 1.f, 1.f, 0.f } *gizmo.quadOffset;
			Math::LinAlg3D::SetTranslation(gizmoMatrix, preTranslation);
			gizmoMatrix = Math::LinAlg3D::Rotate_Homo(Math::ElementaryAxis::Z, gizmo.rotation) * gizmoMatrix;
			Math::Vec3 translation = gizmo.position;
			gizmoMatrix = Math::LinAlg3D::Translate(translation) * gizmoMatrix;
			globUtils.device.cmdPushConstants(
				cmdBuffer,
				manager.pipelineLayout,
				vk::ShaderStageFlagBits::eVertex,
				0,
				sizeof(gizmoMatrix),
				&gizmoMatrix);
			Math::Vec4 color = { 1.f, 1.f, 0.f, gizmoTransparency };
			globUtils.device.cmdPushConstants(
				cmdBuffer,
				manager.pipelineLayout,
				vk::ShaderStageFlagBits::eFragment,
				sizeof(gizmoMatrix),
				sizeof(color),
				&color);
			globUtils.device.cmdDraw(
				cmdBuffer,
				4,
				1,
				0,
				0);
		}
	}
}

using namespace DEngine;
using namespace DEngine::Gfx;
using namespace DEngine::Gfx::Vk;

void Vk::GizmoManager::Initialize(GizmoManager& manager, InitInfo const& initInfo)
{
	impl::GizmoManager_InitializeArrowMesh(
		manager,
		*initInfo.device,
		*initInfo.queues,
		*initInfo.vma,
		*initInfo.delQueue,
		initInfo.debugUtils,
		initInfo.arrowMesh);

	impl::GizmoManager_InitializeRotateCircleMesh(
		manager,
		*initInfo.device,
		*initInfo.queues,
		*initInfo.vma,
		*initInfo.delQueue,
		initInfo.debugUtils,
		initInfo.circleLineMesh);

	impl::GizmoManager_InitializeScaleArrow2dMesh(
		manager,
		*initInfo.device,
		*initInfo.queues,
		*initInfo.vma,
		*initInfo.delQueue,
		initInfo.debugUtils,
		initInfo.arrowScaleMesh2d);

	impl::GizmoManager_InitializeArrowShader(
		manager,
		*initInfo.device,
		initInfo.debugUtils,
		*initInfo.apiData);

	impl::GizmoManager_InitializeQuadShader(
		manager,
		*initInfo.device,
		initInfo.debugUtils,
		*initInfo.apiData);

	impl::GizmoManager_InitializeLineShader(
		manager,
		*initInfo.device,
		initInfo.debugUtils,
		*initInfo.apiData);

	impl::GizmoManager_InitializeLineVtxBuffer(
		manager,
		initInfo.inFlightCount,
		*initInfo.device,
		*initInfo.vma,
		initInfo.debugUtils);
}

void Vk::GizmoManager::UpdateLineVtxBuffer(
	GizmoManager& manager,
	GlobUtils const& globUtils,
	u8 inFlightIndex,
	Std::Span<Math::Vec3 const> vertices)
{
	uSize const ptrOffset = manager.lineVtxBufferCapacity * manager.lineVtxElementSize * inFlightIndex;
	DENGINE_DETAIL_GFX_ASSERT(ptrOffset + manager.lineVtxElementSize * vertices.Size() < manager.lineVtxBufferMappedMem.Size());

	u8* ptr = manager.lineVtxBufferMappedMem.Data() + ptrOffset;

	std::memcpy(ptr, vertices.Data(), manager.lineVtxElementSize * vertices.Size());
}

void Vk::GizmoManager::DebugLines_RecordDrawCalls(
	GizmoManager const& manager,
	GlobUtils const& globUtils,
	ViewportData const& viewportData,
	Std::Span<LineDrawCmd const> lineDrawCmds,
	vk::CommandBuffer cmdBuffer,
	u8 inFlightIndex) noexcept
{
	globUtils.device.cmdBindPipeline(
		cmdBuffer,
		vk::PipelineBindPoint::eGraphics,
		manager.linePipeline);

	Std::Array<vk::DescriptorSet, 1> descrSets = {
		viewportData.camDataDescrSets[inFlightIndex] };
	globUtils.device.cmdBindDescriptorSets(
		cmdBuffer,
		vk::PipelineBindPoint::eGraphics,
		manager.pipelineLayout,
		0,
		{ (u32)descrSets.Size(), descrSets.Data() },
		nullptr);

	Math::Mat4 gizmoMatrix = Math::Mat4::Identity();
	globUtils.device.cmdPushConstants(
		cmdBuffer,
		manager.pipelineLayout,
		vk::ShaderStageFlagBits::eVertex,
		0,
		sizeof(gizmoMatrix),
		&gizmoMatrix);

	uSize vertexOffset = 0;
	for (auto const& drawCmd : lineDrawCmds)
	{
		// Push the color to the push-constant
		globUtils.device.cmdPushConstants(
			cmdBuffer,
			manager.pipelineLayout,
			vk::ShaderStageFlagBits::eFragment,
			sizeof(gizmoMatrix),
			sizeof(drawCmd.color),
			&drawCmd.color);

		// Bind the vertex-array
		vk::Buffer vertexBuffer = manager.lineVtxBuffer;
		uSize vertexBufferOffset = 0;
		// Offset into the right in-flight part of the buffer
		vertexBufferOffset += manager.lineVtxBufferCapacity * manager.lineVtxElementSize * inFlightIndex;
		// Index into the correct vertex
		vertexBufferOffset += manager.lineVtxElementSize * vertexOffset;
		globUtils.device.cmdBindVertexBuffers(
			cmdBuffer,
			0,
			vertexBuffer,
			vertexBufferOffset);

		globUtils.device.cmdDraw(
			cmdBuffer,
			drawCmd.vertCount,
			1,
			0,
			0);

		vertexOffset += drawCmd.vertCount;
	}
}

void Vk::GizmoManager::Gizmo_RecordDrawCalls(
	GizmoManager const& gizmoManager,
	GlobUtils const& globUtils, 
	ViewportData const& viewportData,
	ViewportUpdate::Gizmo const& gizmo,
	vk::CommandBuffer cmdBuffer, 
	u8 inFlightIndex) noexcept
{
	switch (gizmo.type)
	{
		case ViewportUpdate::GizmoType::Translate:
			impl::GizmoManager_RecordTranslateGizmoDrawCalls(
				globUtils,
				gizmoManager,
				viewportData,
				gizmo,
				cmdBuffer,
				inFlightIndex);
			break;

		case ViewportUpdate::GizmoType::Rotate:
			impl::GizmoManager_RotateGizmo_RecordoDrawCalls(
				globUtils,
				gizmoManager,
				viewportData,
				gizmo,
				cmdBuffer,
				inFlightIndex);
			break;

		case ViewportUpdate::GizmoType::Scale:
			impl::GizmoManager_ScaleGizmo_RecordDrawCalls(
				globUtils,
				gizmoManager,
				viewportData,
				gizmo,
				cmdBuffer,
				inFlightIndex);
			break;

		default:
			DENGINE_IMPL_UNREACHABLE();
			break;
	};
}