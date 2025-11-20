#include "GizmoManager.hpp"

#include "DeletionQueue.hpp"
#include "DynamicDispatch.hpp"
#include "QueueData.hpp"
#include "Vk.hpp"
#include "RaiiHandles.hpp"
#include "ShaderHelpers.hpp"

#include <DEngine/Math/LinearTransform3D.hpp>

namespace DEngine::Gfx::Vk::impl
{
	[[nodiscard]] static BoxVmaBuffer GizmoManager_Helper_Test(
		DevDispatch const& device,
		VmaAllocator vma,
		QueueData const& queues,
		DeletionQueue& delQueue,
		Std::Span<char const> bytes)
	{
		DENGINE_IMPL_GFX_ASSERT(!bytes.Empty());

		vk::Result result = vk::Result::eSuccess;

		vk::BufferCreateInfo vtxBufferInfo = {};
		vtxBufferInfo.sharingMode = vk::SharingMode::eExclusive;
		vtxBufferInfo.size = bytes.Size();
		vtxBufferInfo.usage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst;
		VmaAllocationCreateInfo vtxVmaAllocInfo = {};
		vtxVmaAllocInfo.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
		auto vtxBuffer = device.CreateBox(vma, vtxBufferInfo, vtxVmaAllocInfo);

		vk::BufferCreateInfo vtxBufferInfo_Staging = {};
		vtxBufferInfo_Staging.sharingMode = vk::SharingMode::eExclusive;
		vtxBufferInfo_Staging.size = bytes.Size();
		vtxBufferInfo_Staging.usage = vk::BufferUsageFlagBits::eTransferSrc;
		VmaAllocationCreateInfo vtxVmaAllocInfo_Staging = {};
		vtxVmaAllocInfo_Staging.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO;
		vtxVmaAllocInfo_Staging.flags =
			VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

		auto vtxBufferStaging = device.CreateBox(
			vma,
			vtxBufferInfo_Staging,
			vtxVmaAllocInfo_Staging);
		
		// Copy vertices over to staging buffer
		result = (vk::Result)vmaCopyMemoryToAllocation(
			vma,
			bytes.Data(),
			vtxBufferStaging.buffer.Alloc(),
			0,
			bytes.Size());
		if (result != vk::Result::eSuccess) {
			throw std::runtime_error("");
		}

		// Copy vertex data over
		auto cmdPool = device.CreateBox(vk::CommandPoolCreateInfo{});
		vk::CommandBufferAllocateInfo cmdBufferAllocInfo = {};
		cmdBufferAllocInfo.commandPool = cmdPool.Handle();
		cmdBufferAllocInfo.level = vk::CommandBufferLevel::ePrimary;
		cmdBufferAllocInfo.commandBufferCount = 1;
		vk::CommandBuffer cmdBuffer = {};
		result = device.allocateCommandBuffers(cmdBufferAllocInfo, &cmdBuffer);
		if (result != vk::Result::eSuccess) {
			throw std::runtime_error("Unable to allocate Vulkan command buffers");
		}

		vk::CommandBufferBeginInfo cmdBeginInfo = {};
		cmdBeginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
		device.beginCommandBuffer(cmdBuffer, cmdBeginInfo);

		vk::BufferCopy copyRegion = {};
		copyRegion.size = vtxBufferInfo_Staging.size;

		device.cmdCopyBuffer(
			cmdBuffer,
			vtxBufferStaging.buffer.Handle(),
			vtxBuffer.buffer.Handle(),
			{ copyRegion });

		vk::BufferMemoryBarrier buffBarrier = {};
		buffBarrier.buffer = vtxBuffer.buffer.Handle();
		buffBarrier.size = vtxBufferInfo.size;
		buffBarrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
		buffBarrier.dstAccessMask = vk::AccessFlagBits::eVertexAttributeRead;
		device.cmdPipelineBarrier(
			cmdBuffer,
			vk::PipelineStageFlagBits::eTransfer,
			vk::PipelineStageFlagBits::eVertexInput,
			vk::DependencyFlags(),
			nullptr,
			buffBarrier,
			{});

		device.endCommandBuffer(cmdBuffer);

		vk::SubmitInfo submit = {};
		submit.commandBufferCount = 1;
		submit.pCommandBuffers = &cmdBuffer;
		vk::Fence fence = device.createFence({});
		queues.graphics.submit(submit, fence);

		delQueue.Destroy(Std::Move(cmdPool));
		delQueue.Destroy(Std::Move(vtxBufferStaging.buffer));

		return Std::Move(vtxBuffer.buffer);
	}

	static void GizmoManager_InitializeArrowMesh(
		GizmoManager& manager,
		DeviceDispatch const& device,
		QueueData const& queues,
		VmaAllocator vma,
		DeletionQueue& delQueue,
		DebugUtilsDispatch const* debugUtils,
		Std::Span<Math::Vec3 const> arrowMesh)
	{
		auto buffer = GizmoManager_Helper_Test(
			device,
			vma,
			queues,
			delQueue,
			arrowMesh.ToConstByteSpan());
		if (debugUtils != nullptr) {
			debugUtils->Helper_SetObjectName(
				device.handle,
				buffer.Handle(),
				"GizmoManager - Translate Arrow VertexBuffer");
		}

		manager.arrowVtxCount = (u32)arrowMesh.Size();
		manager.arrowVtxBuffer = Std::Move(buffer);
	}

	static void GizmoManager_InitializeRotateCircleMesh(
		GizmoManager& manager,
		DeviceDispatch const& device,
		QueueData const& queues,
		VmaAllocator vma,
		DeletionQueue& delQueue,
		DebugUtilsDispatch const* debugUtils,
		Std::Span<Math::Vec3 const> circleLineMesh)
	{
		auto buffer = GizmoManager_Helper_Test(
			device,
			vma,
			queues,
			delQueue,
			circleLineMesh.ToConstByteSpan());
		if (debugUtils != nullptr) {
			debugUtils->Helper_SetObjectName(
				device.handle,
				buffer.Handle(),
				"GizmoManager - Rotate Circle VertexBuffer");
		}

		manager.circleVtxBuffer = Std::Move(buffer);
		manager.circleVtxCount = (u32)circleLineMesh.Size();
	}

	static void GizmoManager_InitializeScaleArrow2dMesh(
		GizmoManager& manager,
		DeviceDispatch const& device,
		QueueData const& queues,
		VmaAllocator vma,
		DeletionQueue& delQueue,
		DebugUtilsDispatch const* debugUtils,
		Std::Span<Math::Vec3 const> scaleArrow2d)
	{
		auto buffer = GizmoManager_Helper_Test(
			device,
			vma,
			queues,
			delQueue,
			scaleArrow2d.ToConstByteSpan());
		if (debugUtils != nullptr) {
			debugUtils->Helper_SetObjectName(
				device.handle,
				buffer.Handle(),
				"GizmoManager - Translate Scale Arrow 2D VtxBuffer");
		}

		manager.scaleArrow2d_VtxCount = (u32)scaleArrow2d.Size();
		manager.scaleArrow2d_VtxBuffer = Std::Move(buffer);
	}

	static void GizmoManager_InitializePipelineLayout(
		GizmoManager& manager,
		DeviceDispatch const& device,
		vk::DescriptorSetLayout viewportCameraDescrLayout,
		DebugUtilsDispatch const* debugUtils)
	{
		Std::Array<vk::DescriptorSetLayout, 1> layouts {
			viewportCameraDescrLayout };

		vk::PushConstantRange range = {};
		range.offset = 0;
		range.size = sizeof(GizmoManager::PushConstant);
		range.stageFlags =
			vk::ShaderStageFlagBits::eVertex
			| vk::ShaderStageFlagBits::eFragment;
		Std::Array<vk::PushConstantRange, 1> pushConstantRanges = { range };

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo = {};
		pipelineLayoutInfo.setLayoutCount = (u32)layouts.Size();
		pipelineLayoutInfo.pSetLayouts = layouts.Data();
		pipelineLayoutInfo.pushConstantRangeCount = (u32)pushConstantRanges.Size();
		pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.Data();
		auto pipelineLayout = device.CreateBox(pipelineLayoutInfo);
		if (debugUtils != nullptr) {
			debugUtils->Helper_SetObjectName(
				device.Handle(),
				pipelineLayout.Handle(),
				"GizmoManager - Arrow PipelineLayout");
		}

		manager.pipelineLayout = Std::Move(pipelineLayout);
	}

	static void GizmoManager_InitializeArrowShader(
		GizmoManager& manager,
		DeviceDispatch const& device,
		Std::AllocRef const& transientAlloc,
		DebugUtilsDispatch const* debugUtils,
		vk::DescriptorSetLayout viewportCameraDescrLayout,
		vk::PipelineLayout pipelineLayout,
		vk::RenderPass gfxRenderPass)
	{
		DENGINE_IMPL_GFX_ASSERT(Util::CheckNotNull(viewportCameraDescrLayout));
		DENGINE_IMPL_GFX_ASSERT(Util::CheckNotNull(pipelineLayout));
		DENGINE_IMPL_GFX_ASSERT(Util::CheckNotNull(gfxRenderPass));

		auto vertModule = ShaderHelpers::LoadShaderModuleFromFile(
			device,
			Std::CStrToSpan("assets/Gizmo/Arrow.vert.spv"),
			transientAlloc);
		vk::PipelineShaderStageCreateInfo vertStageInfo = {};
		vertStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
		vertStageInfo.module = vertModule.Handle();
		vertStageInfo.pName = "main";

		auto fragModule = ShaderHelpers::LoadShaderModuleFromFile(
			device,
			Std::CStrToSpan("assets/Gizmo/Arrow.frag.spv"),
			transientAlloc);
		vk::PipelineShaderStageCreateInfo fragStageInfo{};
		fragStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
		fragStageInfo.module = fragModule.Handle();
		fragStageInfo.pName = "main";

		Std::Array<vk::PipelineShaderStageCreateInfo, 2> shaderStages = { vertStageInfo, fragStageInfo };

		vk::VertexInputAttributeDescription position = {};
		position.binding = 0;
		position.format = vk::Format::eR32G32B32Sfloat;
		position.location = 0;
		position.offset = 0;
		vk::VertexInputBindingDescription binding = {};
		binding.binding = 0;
		binding.inputRate = vk::VertexInputRate::eVertex;
		binding.stride = 12;
		vk::PipelineVertexInputStateCreateInfo vertexInputInfo = {};
		vertexInputInfo.vertexAttributeDescriptionCount = 1;
		vertexInputInfo.pVertexAttributeDescriptions = &position;
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.pVertexBindingDescriptions = &binding;

		vk::PipelineInputAssemblyStateCreateInfo inputAssembly = {};
		inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;

		vk::Viewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (f32)0.f;
		viewport.height = (f32)0.f;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vk::Rect2D scissor = {};
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
		vk::PipelineDynamicStateCreateInfo dynamicState = {};
		dynamicState.dynamicStateCount = 1;
		dynamicState.pDynamicStates = &temp;

		vk::GraphicsPipelineCreateInfo pipelineInfo = {};
		pipelineInfo.layout = pipelineLayout;
		pipelineInfo.renderPass = gfxRenderPass;
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

		auto pipeline = device.CreateBox(pipelineInfo);
		if (debugUtils != nullptr) {
			debugUtils->Helper_SetObjectName(
				device.Handle(),
				pipeline.Handle(),
				"GizmoManager - Arrow Pipeline");
		}

		manager.arrowPipeline = Std::Move(pipeline);
	}

	static void GizmoManager_InitializeQuadShader(
		GizmoManager& manager,
		DeviceDispatch const& device,
		Std::AllocRef const& transientAlloc,
		vk::RenderPass gfxRenderPass,
		vk::PipelineLayout pipelineLayout,
		DebugUtilsDispatch const* debugUtils)
	{
		DENGINE_IMPL_GFX_ASSERT(Util::CheckNotNull(pipelineLayout));
		DENGINE_IMPL_GFX_ASSERT(Util::CheckNotNull(gfxRenderPass));

		auto vertModule = ShaderHelpers::LoadShaderModuleFromFile(
			device,
			Std::CStrToSpan("assets/Gizmo/Quad.vert.spv"),
			transientAlloc);
		vk::PipelineShaderStageCreateInfo vertStageInfo{};
		vertStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
		vertStageInfo.module = vertModule.Handle();
		vertStageInfo.pName = "main";

		auto fragModule = ShaderHelpers::LoadShaderModuleFromFile(
			device,
			Std::CStrToSpan("assets/Gizmo/Quad.frag.spv"),
			transientAlloc);
		vk::PipelineShaderStageCreateInfo fragStageInfo{};
		fragStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
		fragStageInfo.module = fragModule.Handle();
		fragStageInfo.pName = "main";

		Std::Array<vk::PipelineShaderStageCreateInfo, 2> shaderStages = { vertStageInfo, fragStageInfo };

		vk::PipelineVertexInputStateCreateInfo vertexInputInfo = {};

		vk::PipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.topology = vk::PrimitiveTopology::eTriangleStrip;

		vk::Viewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (f32)0.f;
		viewport.height = (f32)0.f;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vk::Rect2D scissor = {};
		scissor.offset = vk::Offset2D{ 0, 0 };
		scissor.extent = vk::Extent2D{ 8192, 8192 };
		vk::PipelineViewportStateCreateInfo viewportState = {};
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
		pipelineInfo.layout = pipelineLayout;
		pipelineInfo.renderPass = gfxRenderPass;
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

		auto pipeline = device.CreateBox(pipelineInfo);
		if (debugUtils != nullptr) {
			debugUtils->Helper_SetObjectName(
				device.Handle(),
				pipeline.Handle(),
				"GizmoManager - Quad Pipeline");
		}

		manager.quadPipeline = Std::Move(pipeline);
	}

	static void GizmoManager_InitializeLineShader(
		GizmoManager& manager,
		DeviceDispatch const& device,
		Std::AllocRef const& transientAlloc,
		vk::PipelineLayout pipelineLayout,
		vk::RenderPass gfxRenderPass,
		DebugUtilsDispatch const* debugUtils)
	{
		DENGINE_IMPL_GFX_ASSERT(Util::CheckNotNull(pipelineLayout));
		DENGINE_IMPL_GFX_ASSERT(Util::CheckNotNull(gfxRenderPass));

		auto vertModule = ShaderHelpers::LoadShaderModuleFromFile(
			device,
			Std::CStrToSpan("assets/Gizmo/Line.vert.spv"),
			transientAlloc);
		vk::PipelineShaderStageCreateInfo vertStageInfo = {};
		vertStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
		vertStageInfo.module = vertModule.Handle();
		vertStageInfo.pName = "main";

		auto fragModule = ShaderHelpers::LoadShaderModuleFromFile(
			device,
			Std::CStrToSpan("assets/Gizmo/Line.frag.spv"),
			transientAlloc);
		vk::PipelineShaderStageCreateInfo fragStageInfo = {};
		fragStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
		fragStageInfo.module = fragModule.Handle();
		fragStageInfo.pName = "main";

		Std::Array<vk::PipelineShaderStageCreateInfo, 2> shaderStages = { vertStageInfo, fragStageInfo };

		vk::VertexInputAttributeDescription position = {};
		position.binding = 0;
		position.format = vk::Format::eR32G32B32Sfloat;
		position.location = 0;
		position.offset = 0;
		vk::VertexInputBindingDescription binding = {};
		binding.binding = 0;
		binding.inputRate = vk::VertexInputRate::eVertex;
		binding.stride = 12;
		vk::PipelineVertexInputStateCreateInfo vertexInputInfo = {};
		vertexInputInfo.vertexAttributeDescriptionCount = 1;
		vertexInputInfo.pVertexAttributeDescriptions = &position;
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.pVertexBindingDescriptions = &binding;

		vk::PipelineInputAssemblyStateCreateInfo inputAssembly = {};
		inputAssembly.topology = vk::PrimitiveTopology::eLineStrip;

		vk::Viewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (f32)0.f;
		viewport.height = (f32)0.f;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vk::Rect2D scissor = {};
		scissor.offset = vk::Offset2D{ 0, 0 };
		scissor.extent = vk::Extent2D{ 8192, 8192 };
		vk::PipelineViewportStateCreateInfo viewportState = {};
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

		vk::PipelineMultisampleStateCreateInfo multisampling = {};

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

		vk::GraphicsPipelineCreateInfo pipelineInfo = {};
		pipelineInfo.layout = pipelineLayout;
		pipelineInfo.renderPass = gfxRenderPass;
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

		auto pipeline = device.CreateBox(pipelineInfo);
		manager.linePipeline = Std::Move(pipeline);
	}

	static void GizmoManager_InitializeLineVtxBuffer(
		GizmoManager& manager,
		u8 inFlightCount,
		DeviceDispatch const& device,
		VmaAllocator const& vma,
		DebugUtilsDispatch const* debugUtils)
	{
		vk::BufferCreateInfo bufferInfo = {};
		bufferInfo.sharingMode = vk::SharingMode::eExclusive;
		bufferInfo.size = GizmoManager::lineVtxElementSize * GizmoManager::lineVtxMinCapacity * inFlightCount;
		bufferInfo.usage = vk::BufferUsageFlagBits::eVertexBuffer;
		VmaAllocationCreateInfo allocCreateInfo = {};
		allocCreateInfo.flags =
			VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
		allocCreateInfo.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO;
		auto buffer = device.CreateBox(vma, bufferInfo, allocCreateInfo);
		if (debugUtils != nullptr) {
			debugUtils->Helper_SetObjectName(
				device.Handle(),
				buffer.buffer.Handle(),
				"GizmoManager - LineVtxBuffer");
		}

		manager.lineVtxBuffer = Std::Move(buffer.buffer);
		manager.lineVtxVmaAllocInfo = buffer.allocInfo;
		manager.lineVtxBufferCapacity = GizmoManager::lineVtxMinCapacity;
	}

	static void GizmoManager_RecordTranslateGizmoDrawCalls(
		GlobUtils const& globUtils,
		GizmoManager const& gizmoManager,
		ViewportMgr_ViewportData const& viewportData,
		ViewportUpdate::Gizmo const& gizmo,
		vk::CommandBuffer cmdBuffer,
		u8 inFlightIndex)
	{
		auto const& device = globUtils.device;
		auto pipelineLayout = gizmoManager.pipelineLayout.Handle();

		device.cmdBindPipeline(
			cmdBuffer,
			vk::PipelineBindPoint::eGraphics,
			gizmoManager.arrowPipeline.Handle());

		device.cmdBindVertexBuffers(
			cmdBuffer, 0, { gizmoManager.arrowVtxBuffer.Handle() }, { 0 });
		Std::Array<vk::DescriptorSet, 1> descrSets = {
			viewportData.camDataDescrSets[inFlightIndex] };
		device.cmdBindDescriptorSets(
			cmdBuffer,
			vk::PipelineBindPoint::eGraphics,
			pipelineLayout,
			0,
			{ (u32)descrSets.Size(), descrSets.Data() },
			nullptr);

		// Draw X arrow
		{
			Math::Mat4 gizmoMatrix = Math::LinAlg3D::Scale_Homo(gizmo.scale, gizmo.scale, gizmo.scale);
			gizmoMatrix = Math::LinAlg3D::Rotate_Homo(Math::ElementaryAxis::Z, gizmo.rotation) * gizmoMatrix;
			Math::LinAlg3D::SetTranslation(gizmoMatrix, gizmo.position);

			device.cmdPushConstants(
				cmdBuffer,
				pipelineLayout,
				vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
				0,
				sizeof(gizmoMatrix),
				&gizmoMatrix);
			Math::Vec4 color = { 1.f, 0.f, 0.f, GizmoManager::gizmoTransparency };
			device.cmdPushConstants(
				cmdBuffer,
				pipelineLayout,
				vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
				sizeof(gizmoMatrix),
				sizeof(color),
				&color);

			device.cmdDraw(
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

			device.cmdPushConstants(
				cmdBuffer,
				pipelineLayout,
				vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
				0,
				sizeof(gizmoMatrix),
				&gizmoMatrix);
			Math::Vec4 color = { 0.f, 1.f, 0.f, GizmoManager::gizmoTransparency };
			device.cmdPushConstants(
				cmdBuffer,
				pipelineLayout,
				vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
				sizeof(gizmoMatrix),
				sizeof(color),
				&color);
			device.cmdDraw(
				cmdBuffer,
				gizmoManager.arrowVtxCount,
				1,
				0,
				0);
		}

		// Draw the floating quad thing
		{
			device.cmdBindPipeline(
				cmdBuffer,
				vk::PipelineBindPoint::eGraphics,
				gizmoManager.quadPipeline.Handle());

			Math::Mat4 gizmoMatrix = Math::LinAlg3D::Scale_Homo(gizmo.quadScale, gizmo.quadScale, gizmo.quadScale);
			Math::Vec3 preTranslation = Math::Vec3{ 1.f, 1.f, 0.f } * gizmo.quadOffset;
			Math::LinAlg3D::SetTranslation(gizmoMatrix, preTranslation);
			gizmoMatrix = Math::LinAlg3D::Rotate_Homo(Math::ElementaryAxis::Z, gizmo.rotation) * gizmoMatrix;
			Math::Vec3 translation = gizmo.position;
			gizmoMatrix = Math::LinAlg3D::Translate(translation) * gizmoMatrix;
			device.cmdPushConstants(
				cmdBuffer,
				pipelineLayout,
				vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
				0,
				sizeof(gizmoMatrix),
				&gizmoMatrix);
			Math::Vec4 color = { 1.f, 1.f, 0.f, GizmoManager::gizmoTransparency };
			device.cmdPushConstants(
				cmdBuffer,
				pipelineLayout,
				vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
				sizeof(gizmoMatrix),
				sizeof(color),
				&color);
			device.cmdDraw(
				cmdBuffer,
				4,
				1,
				0,
				0);
		}
	}

	static void GizmoManager_RotateGizmo_RecordDrawCalls(
		GlobUtils const& globUtils,
		GizmoManager const& gizmoManager,
		ViewportMgr_ViewportData const& viewportData,
		ViewportUpdate::Gizmo const& gizmo,
		vk::CommandBuffer cmdBuffer,
		u8 inFlightIndex)
	{
		auto const& device = globUtils.device;
		auto const pipelineLayout = gizmoManager.pipelineLayout.Handle();

		device.cmdBindPipeline(
			cmdBuffer,
			vk::PipelineBindPoint::eGraphics,
			gizmoManager.arrowPipeline.Handle());

		device.cmdBindVertexBuffers(
			cmdBuffer,
			0,
			{ gizmoManager.circleVtxBuffer.Handle() },
			{ 0 });
		Std::Array<vk::DescriptorSet, 1> descrSets = {
			viewportData.camDataDescrSets[inFlightIndex] };
		device.cmdBindDescriptorSets(
			cmdBuffer,
			vk::PipelineBindPoint::eGraphics,
			pipelineLayout,
			0,
			{ (u32)descrSets.Size(), descrSets.Data() },
			nullptr);

		Math::Mat4 gizmoMatrix = Math::LinAlg3D::Scale_Homo(gizmo.scale, gizmo.scale, gizmo.scale);
		Math::LinAlg3D::SetTranslation(gizmoMatrix, gizmo.position);
		device.cmdPushConstants(
			cmdBuffer,
			pipelineLayout,
			vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
			0,
			sizeof(gizmoMatrix),
			&gizmoMatrix);
		Math::Vec4 color = { 0.f, 0.f, 1.f, GizmoManager::gizmoTransparency };
		device.cmdPushConstants(
			cmdBuffer,
			pipelineLayout,
			vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
			sizeof(gizmoMatrix),
			sizeof(color),
			&color);

		device.cmdDraw(
			cmdBuffer,
			gizmoManager.circleVtxCount,
			1,
			0,
			0);
	}

	static void GizmoManager_ScaleGizmo_RecordDrawCalls(
		GlobUtils const& globUtils,
		GizmoManager const& manager,
		ViewportMgr_ViewportData const& viewportData,
		ViewportUpdate::Gizmo const& gizmo,
		vk::CommandBuffer cmdBuffer,
		u8 inFlightIndex)
	{
		auto const& device = globUtils.device;
		auto const pipelineLayout = manager.pipelineLayout.Handle();

		device.cmdBindPipeline(
			cmdBuffer,
			vk::PipelineBindPoint::eGraphics,
			manager.arrowPipeline.Handle());

		device.cmdBindVertexBuffers(
			cmdBuffer,
			0,
			{ manager.scaleArrow2d_VtxBuffer.Handle() },
			{ 0 });

		Std::Array<vk::DescriptorSet, 1> descrSets = { viewportData.camDataDescrSets[inFlightIndex] };
		device.cmdBindDescriptorSets(
			cmdBuffer,
			vk::PipelineBindPoint::eGraphics,
			pipelineLayout,
			0,
			{ (u32)descrSets.Size(), descrSets.Data() },
			nullptr);

		// Draw X arrow
		{
			Math::Mat4 gizmoMatrix = Math::LinAlg3D::Scale_Homo(gizmo.scale, gizmo.scale, gizmo.scale);
			gizmoMatrix = Math::LinAlg3D::Rotate_Homo(Math::ElementaryAxis::Z, gizmo.rotation) * gizmoMatrix;
			Math::LinAlg3D::SetTranslation(gizmoMatrix, gizmo.position);

			device.cmdPushConstants(
				cmdBuffer,
				pipelineLayout,
				vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
				0,
				sizeof(gizmoMatrix),
				&gizmoMatrix);

			Math::Vec4 color = { 1.f, 0.f, 0.f, GizmoManager::gizmoTransparency };
			device.cmdPushConstants(
				cmdBuffer,
				pipelineLayout,
				vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
				sizeof(gizmoMatrix),
				sizeof(color),
				&color);

			device.cmdDraw(
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

			device.cmdPushConstants(
				cmdBuffer,
				pipelineLayout,
				vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
				0,
				sizeof(gizmoMatrix),
				&gizmoMatrix);

			Math::Vec4 color = { 0.f, 1.f, 0.f, GizmoManager::gizmoTransparency };
			device.cmdPushConstants(
				cmdBuffer,
				pipelineLayout,
				vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
				sizeof(gizmoMatrix),
				sizeof(color),
				&color);

			device.cmdDraw(
				cmdBuffer,
				manager.scaleArrow2d_VtxCount,
				1,
				0,
				0);
		}

		// Draw the floating quad thing
		{
			device.cmdBindPipeline(
				cmdBuffer,
				vk::PipelineBindPoint::eGraphics,
				manager.quadPipeline.Handle());

			Math::Mat4 gizmoMatrix = Math::LinAlg3D::Scale_Homo(gizmo.quadScale, gizmo.quadScale, gizmo.quadScale);
			Math::Vec3 preTranslation = Math::Vec3{ 1.f, 1.f, 0.f } *gizmo.quadOffset;
			Math::LinAlg3D::SetTranslation(gizmoMatrix, preTranslation);
			gizmoMatrix = Math::LinAlg3D::Rotate_Homo(Math::ElementaryAxis::Z, gizmo.rotation) * gizmoMatrix;
			Math::Vec3 translation = gizmo.position;
			gizmoMatrix = Math::LinAlg3D::Translate(translation) * gizmoMatrix;
			globUtils.device.cmdPushConstants(
				cmdBuffer,
				pipelineLayout,
				vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
				0,
				sizeof(gizmoMatrix),
				&gizmoMatrix);
			Math::Vec4 color = { 1.f, 1.f, 0.f, GizmoManager::gizmoTransparency };
			globUtils.device.cmdPushConstants(
				cmdBuffer,
				pipelineLayout,
				vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
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
	auto const& device = *initInfo.device;
	auto& delQueue = *initInfo.delQueue;
	auto const& vma = *initInfo.vma;
	auto const& transientAlloc = Std::AllocRef{ *initInfo.frameAlloc };
	auto const* debugUtils = initInfo.debugUtils;

	auto const& viewportDescrSetLayout = initInfo.apiData->viewportManager.cameraDescrLayout;
	auto const& gfxRenderPass = initInfo.apiData->m_globUtils.gfxRenderPass;

	impl::GizmoManager_InitializeArrowMesh(
		manager,
		device,
		*initInfo.queues,
		vma,
		delQueue,
		debugUtils,
		initInfo.arrowMesh);

	impl::GizmoManager_InitializeRotateCircleMesh(
		manager,
		device,
		*initInfo.queues,
		vma,
		delQueue,
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

	impl::GizmoManager_InitializePipelineLayout(
		manager,
		device,
		initInfo.apiData->viewportManager.cameraDescrLayout,
		debugUtils);
	auto pipelineLayout = manager.pipelineLayout.Handle();

	impl::GizmoManager_InitializeArrowShader(
		manager,
		device,
		transientAlloc,
		debugUtils,
		viewportDescrSetLayout,
		pipelineLayout,
		gfxRenderPass);

	impl::GizmoManager_InitializeQuadShader(
		manager,
		device,
		transientAlloc,
		gfxRenderPass,
		pipelineLayout,
		debugUtils);

	impl::GizmoManager_InitializeLineShader(
		manager,
		device,
		transientAlloc,
		pipelineLayout,
		gfxRenderPass,
		debugUtils);

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
	DENGINE_IMPL_GFX_ASSERT(vertices.Size() < manager.lineVtxBufferCapacity);

	uSize const ptrOffset =
		manager.lineVtxBufferCapacity * GizmoManager::lineVtxElementSize * inFlightIndex;

	DENGINE_IMPL_GFX_ASSERT(
		ptrOffset + manager.lineVtxElementSize * vertices.Size() < manager.lineVtxVmaAllocInfo.size);

	auto result = (vk::Result)vmaCopyMemoryToAllocation(
		globUtils.vma,
		vertices.Data(),
		manager.lineVtxBuffer.Alloc(),
		ptrOffset,
		vertices.Size() * sizeof(vertices[0]));
	if (result != vk::Result::eSuccess) {
		throw std::runtime_error("Unable to copy memory to VkBuffer");
	}
}

void Vk::GizmoManager::DebugLines_RecordDrawCalls(
	GizmoManager const& manager,
	GlobUtils const& globUtils,
	ViewportMgr_ViewportData const& viewportData,
	Std::Span<LineDrawCmd const> lineDrawCmds,
	vk::CommandBuffer cmdBuffer,
	u8 inFlightIndex) noexcept
{
	auto const& device = globUtils.device;
	auto const pipelineLayout = manager.pipelineLayout.Handle();

	device.cmdBindPipeline(
		cmdBuffer,
		vk::PipelineBindPoint::eGraphics,
		manager.linePipeline.Handle());

	Std::Array<vk::DescriptorSet, 1> descrSets = {
		viewportData.camDataDescrSets[inFlightIndex] };
	device.cmdBindDescriptorSets(
		cmdBuffer,
		vk::PipelineBindPoint::eGraphics,
		pipelineLayout,
		0,
		{ (u32)descrSets.Size(), descrSets.Data() },
		nullptr);

	Math::Mat4 gizmoMatrix = Math::Mat4::Identity();
	device.cmdPushConstants(
		cmdBuffer,
		pipelineLayout,
		vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
		0,
		sizeof(gizmoMatrix),
		&gizmoMatrix);

	uSize vertexOffset = 0;
	for (auto const& drawCmd : lineDrawCmds) {
		// Push the color to the push-constant
		device.cmdPushConstants(
			cmdBuffer,
			pipelineLayout,
			vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
			sizeof(gizmoMatrix),
			sizeof(drawCmd.color),
			&drawCmd.color);

		// Bind the vertex-array
		vk::Buffer vertexBuffer = manager.lineVtxBuffer.Handle();
		uSize vertexBufferOffset = 0;
		// Offset into the right in-flight part of the buffer
		vertexBufferOffset += manager.lineVtxBufferCapacity * manager.lineVtxElementSize * inFlightIndex;
		// Index into the correct vertex
		vertexBufferOffset += manager.lineVtxElementSize * vertexOffset;
		device.cmdBindVertexBuffers(
			cmdBuffer,
			0,
			vertexBuffer,
			vertexBufferOffset);

		device.cmdDraw(
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
	ViewportMgr_ViewportData const& viewportData,
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
			impl::GizmoManager_RotateGizmo_RecordDrawCalls(
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
			DENGINE_IMPL_GFX_UNREACHABLE();
			break;
	};
}