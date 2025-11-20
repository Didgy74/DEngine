#include "GuiResourceManager.hpp"

#include <DEngine/Gfx/Gfx.hpp>

#include "DeletionQueue.hpp"
#include "DynamicDispatch.hpp"
#include "GlobUtils.hpp"
#include "ObjectDataManager.hpp"
#include "RaiiHandles.hpp"
#include "ShaderHelpers.hpp"
#include "StagingBufferAlloc.hpp"
#include "ViewportManager.hpp"

#include <DEngine/Gfx/impl/Assert.hpp>

#include <DEngine/Std/Containers/Defer.hpp>
#include <DEngine/Std/Containers/AllocRef.hpp>

// For file IO
#include <DEngine/Platform/Platform.hpp>

#include <format>

import DEngine.Math.Common;

using namespace DEngine;
using namespace DEngine::Gfx;
using namespace DEngine::Gfx::Vk;

namespace DEngine::Gfx::Vk::GuiResourceManagerImpl
{
	[[nodiscard]] static auto BuildDynamicStateArrayForColor() {
		return Std::Array<vk::DynamicState, 3>{
			vk::DynamicState::eViewport,
			vk::DynamicState::eScissor,
			vk::DynamicState::eStencilReference };
	}

	[[nodiscard]] static Std::Array<vk::VertexInputAttributeDescription, 2> BuildShaderVertexInputAttrDescr() {
		vk::VertexInputAttributeDescription position = {};
		position.binding = 0;
		position.format = vk::Format::eR32G32Sfloat;
		position.location = 0;
		position.offset = offsetof(GuiVertex, position);

		vk::VertexInputAttributeDescription uv = {};
		uv.binding = 0;
		uv.format = vk::Format::eR32G32Sfloat;
		uv.location = 1;
		uv.offset = offsetof(GuiVertex, uv);

		return { position, uv };
	}

	[[nodiscard]] static Std::Array<vk::VertexInputBindingDescription, 1> BuildShaderVertexInputBindingDescr() {
		vk::VertexInputBindingDescription binding{};
		binding.binding = 0;
		binding.inputRate = vk::VertexInputRate::eVertex;
		binding.stride = sizeof(GuiVertex);

		return { binding };
	}

	[[nodiscard]] static vk::PipelineColorBlendAttachmentState
	BuildPipelineColorBlendAttachmentState_ForBlendedColor() {
		vk::PipelineColorBlendAttachmentState colorBlendAttachment = {};
		colorBlendAttachment.colorWriteMask =
			vk::ColorComponentFlagBits::eR
			| vk::ColorComponentFlagBits::eG
			| vk::ColorComponentFlagBits::eB
			| vk::ColorComponentFlagBits::eA;
		colorBlendAttachment.blendEnable = true;
		colorBlendAttachment.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
		colorBlendAttachment.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
		colorBlendAttachment.colorBlendOp = vk::BlendOp::eAdd;
		colorBlendAttachment.srcAlphaBlendFactor = vk::BlendFactor::eSrcAlpha;
		colorBlendAttachment.dstAlphaBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
		colorBlendAttachment.alphaBlendOp = vk::BlendOp::eAdd;
		return colorBlendAttachment;
	}

	// Builds the correct vk::PipelineDepthStencilStateCreateInfo for the GUI shaders
	// that draw color to the screen, that also need to respect the stencil mask.
	[[nodiscard]] static vk::PipelineDepthStencilStateCreateInfo BuildPipelineDepthStencilStateInfoForColor() {
		vk::StencilOpState stencilOpState = {};
		stencilOpState.failOp = vk::StencilOp::eKeep; // We don't want to write anything to stencil
		stencilOpState.passOp = vk::StencilOp::eKeep; // We don't want to write anything to stencil
		stencilOpState.depthFailOp = vk::StencilOp::eKeep;
		stencilOpState.compareOp = vk::CompareOp::eEqual;
		stencilOpState.compareMask = 0xFF;
		stencilOpState.writeMask = 0x00; // We don't want to write anything
		stencilOpState.reference = 0; // Dynamically controlled. Ignored.

		vk::PipelineDepthStencilStateCreateInfo depthStencilInfo = {};
		depthStencilInfo.depthTestEnable = 0;
		depthStencilInfo.depthCompareOp = vk::CompareOp::eLess;
		depthStencilInfo.stencilTestEnable = 1;
		depthStencilInfo.front = stencilOpState;
		depthStencilInfo.back = stencilOpState;
		depthStencilInfo.depthWriteEnable = 0;
		depthStencilInfo.minDepthBounds = 0.f;
		depthStencilInfo.maxDepthBounds = 1.f;

		return depthStencilInfo;
	}

	static void CreateRectangleShader(
		GuiResourceManager& manager,
		DeviceDispatch const& device,
		Std::Span<vk::DescriptorSetLayout const> descrSetLayouts,
		vk::RenderPass guiRenderPass,
		Std::AllocRef const& transientAlloc,
		DebugUtilsDispatch const* debugUtils)
	{
		DENGINE_IMPL_GFX_ASSERT(Util::CheckNotNull(descrSetLayouts));
		DENGINE_IMPL_GFX_ASSERT(Util::CheckNotNull(guiRenderPass));

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo = {};

		vk::PushConstantRange vertPushConstantRange = {};
		vertPushConstantRange.stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment;
		vertPushConstantRange.size = GuiResourceManager::RectanglePushConstant::sizeInBytes;
		Std::Array<vk::PushConstantRange, 1> pushConstantRanges { vertPushConstantRange};
		pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.Data();
		pipelineLayoutInfo.pushConstantRangeCount = (u32)pushConstantRanges.Size();
		pipelineLayoutInfo.setLayoutCount = descrSetLayouts.Size();
		pipelineLayoutInfo.pSetLayouts = descrSetLayouts.Data();

		auto pipelineLayout = device.CreateBox(pipelineLayoutInfo);
		if (debugUtils) {
			debugUtils->Helper_SetObjectName(
				device.handle,
				pipelineLayout.Handle(),
				"GuiResourceManager - Rectangle PipelineLayout");
		}

		auto vertModule = ShaderHelpers::LoadShaderModuleFromFile(
			device,
			Std::CStrToSpan("assets/Gui/Rectangle.vert.spv"),
			transientAlloc);
		vk::PipelineShaderStageCreateInfo vertStageInfo = {};
		vertStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
		vertStageInfo.module = vertModule.Handle();
		vertStageInfo.pName = "main";

		auto fragModule = ShaderHelpers::LoadShaderModuleFromFile(
			device,
			Std::CStrToSpan("assets/Gui/Rectangle.frag.spv"),
			transientAlloc);
		vk::PipelineShaderStageCreateInfo fragStageInfo = {};
		fragStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
		fragStageInfo.module = fragModule.Handle();
		fragStageInfo.pName = "main";

		Std::Array<vk::PipelineShaderStageCreateInfo, 2> shaderStages = {
			vertStageInfo,
			fragStageInfo };

		auto dynamicStates = BuildDynamicStateArrayForColor();
		vk::PipelineDynamicStateCreateInfo dynamicState = {};
		dynamicState.dynamicStateCount = dynamicStates.Size();
		dynamicState.pDynamicStates = dynamicStates.Data();
		vk::PipelineInputAssemblyStateCreateInfo inputAssemblyState = {};
		inputAssemblyState.topology = vk::PrimitiveTopology::eTriangleList;
		vk::PipelineMultisampleStateCreateInfo multiSampleState = {};
		multiSampleState.rasterizationSamples = vk::SampleCountFlagBits::e1;
		vk::PipelineRasterizationStateCreateInfo rasterizationState = {};
		rasterizationState.lineWidth = 1.f;
		rasterizationState.polygonMode = vk::PolygonMode::eFill;
		rasterizationState.frontFace = vk::FrontFace::eClockwise;
		rasterizationState.cullMode = vk::CullModeFlagBits::eNone;
		vk::PipelineVertexInputStateCreateInfo vertexInputState = {};
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
		auto colorBlendAttachment = BuildPipelineColorBlendAttachmentState_ForBlendedColor();

		vk::PipelineColorBlendStateCreateInfo colorBlendState = {};
		colorBlendState.attachmentCount = 1;
		colorBlendState.pAttachments = &colorBlendAttachment;

		auto depthStencilInfo = BuildPipelineDepthStencilStateInfoForColor();

		vk::GraphicsPipelineCreateInfo pipelineInfo = {};
		pipelineInfo.layout = pipelineLayout.Handle();
		pipelineInfo.pColorBlendState = &colorBlendState;
		pipelineInfo.pDepthStencilState = &depthStencilInfo;
		pipelineInfo.pDynamicState = &dynamicState;
		pipelineInfo.pInputAssemblyState = &inputAssemblyState;
		pipelineInfo.pMultisampleState = &multiSampleState;
		pipelineInfo.pRasterizationState = &rasterizationState;
		pipelineInfo.pVertexInputState = &vertexInputState;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.renderPass = guiRenderPass;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages.Data();

		auto pipeline = device.CreateBox(pipelineInfo);
		if (debugUtils != nullptr) {
			debugUtils->Helper_SetObjectName(
				device.handle,
				pipeline.Handle(),
				"GuiResourceManager - Rectangle Pipeline");
		}

		manager.m_rectanglePipelineLayout = Std::Move(pipelineLayout);
		manager.m_rectanglePipeline = Std::Move(pipeline);
	}

	static void CreateRectangleShadowShader(
		GuiResourceManager& manager,
		DeviceDispatch const& device,
		Std::Span<vk::DescriptorSetLayout const> descrSetLayouts,
		vk::RenderPass guiRenderPass,
		Std::AllocRef const& transientAlloc,
		DebugUtilsDispatch const* debugUtils)
	{
		DENGINE_IMPL_GFX_ASSERT(Util::CheckNotNull(descrSetLayouts));
		DENGINE_IMPL_GFX_ASSERT(guiRenderPass != vk::RenderPass{});

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo = {};

		vk::PushConstantRange vertPushConstantRange = {};
		vertPushConstantRange.stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment;
		vertPushConstantRange.size = sizeof(GuiResourceManager::RectangleShadowPushConstant);
		Std::Array<vk::PushConstantRange, 1> pushConstantRanges { vertPushConstantRange};
		pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.Data();
		pipelineLayoutInfo.pushConstantRangeCount = (u32)pushConstantRanges.Size();
		pipelineLayoutInfo.setLayoutCount = descrSetLayouts.Size();
		pipelineLayoutInfo.pSetLayouts = descrSetLayouts.Data();

		auto pipelineLayout = device.CreateBox(pipelineLayoutInfo);
		if (debugUtils) {
			debugUtils->Helper_SetObjectName(
				device.handle,
				pipelineLayout.Handle(),
				"GuiResourceManager - Rectangle Shadow PipelineLayout");
		}

		auto vertModule = ShaderHelpers::LoadShaderModuleFromFile(
			device,
			Std::CStrToSpan("assets/Gui/RectangleShadow.vert.spv"),
			transientAlloc);
		vk::PipelineShaderStageCreateInfo vertStageInfo = {};
		vertStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
		vertStageInfo.module = vertModule.Handle();
		vertStageInfo.pName = "main";

		auto fragModule = ShaderHelpers::LoadShaderModuleFromFile(
			device,
			Std::CStrToSpan("assets/Gui/RectangleShadow.frag.spv"),
			transientAlloc);
		vk::PipelineShaderStageCreateInfo fragStageInfo = {};
		fragStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
		fragStageInfo.module = fragModule.Handle();
		fragStageInfo.pName = "main";

		Std::Array<vk::PipelineShaderStageCreateInfo, 2> shaderStages = {
			vertStageInfo,
			fragStageInfo};

		auto dynamicStates = BuildDynamicStateArrayForColor();
		vk::PipelineDynamicStateCreateInfo dynamicState = {};
		dynamicState.dynamicStateCount = dynamicStates.Size();
		dynamicState.pDynamicStates = dynamicStates.Data();
		vk::PipelineInputAssemblyStateCreateInfo inputAssemblyState = {};
		inputAssemblyState.topology = vk::PrimitiveTopology::eTriangleList;
		vk::PipelineMultisampleStateCreateInfo multiSampleState = {};
		multiSampleState.rasterizationSamples = vk::SampleCountFlagBits::e1;
		vk::PipelineRasterizationStateCreateInfo rasterizationState = {};
		rasterizationState.lineWidth = 1.f;
		rasterizationState.polygonMode = vk::PolygonMode::eFill;
		rasterizationState.frontFace = vk::FrontFace::eClockwise;
		rasterizationState.cullMode = vk::CullModeFlagBits::eNone;
		vk::PipelineVertexInputStateCreateInfo vertexInputState = {};
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

		auto colorBlendAttachment = BuildPipelineColorBlendAttachmentState_ForBlendedColor();
		vk::PipelineColorBlendStateCreateInfo colorBlendState = {};
		colorBlendState.attachmentCount = 1;
		colorBlendState.pAttachments = &colorBlendAttachment;

		auto depthStencilInfo = BuildPipelineDepthStencilStateInfoForColor();

		vk::GraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.layout = pipelineLayout.Handle();
		pipelineInfo.pColorBlendState = &colorBlendState;
		pipelineInfo.pDepthStencilState = &depthStencilInfo;
		pipelineInfo.pDynamicState = &dynamicState;
		pipelineInfo.pInputAssemblyState = &inputAssemblyState;
		pipelineInfo.pMultisampleState = &multiSampleState;
		pipelineInfo.pRasterizationState = &rasterizationState;
		pipelineInfo.pVertexInputState = &vertexInputState;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.renderPass = guiRenderPass;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages.Data();

		auto pipeline = device.CreateBox(pipelineInfo);
		if (debugUtils != nullptr) {
			debugUtils->Helper_SetObjectName(
				device.handle,
				pipeline.Handle(),
				"GuiResourceManager - Rectangle Shadow Pipeline");
		}

		manager.m_rectangleShadowPipelineLayout = Std::Move(pipelineLayout);
		manager.m_rectangleShadowPipeline = Std::Move(pipeline);
	}

	static void CreateGradientShader(
		GuiResourceManager& manager,
		DeviceDispatch const& device,
		Std::Span<vk::DescriptorSetLayout const> descrSetLayouts,
		vk::RenderPass guiRenderPass,
		Std::AllocRef const& transientAlloc,
		DebugUtilsDispatch const* debugUtils)
	{
		DENGINE_IMPL_GFX_ASSERT(Util::CheckNotNull(descrSetLayouts));
		DENGINE_IMPL_GFX_ASSERT(Util::CheckNotNull(guiRenderPass));

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo = {};

		vk::PushConstantRange vertPushConstantRange = {};
		vertPushConstantRange.stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment;
		vertPushConstantRange.size = sizeof(GuiResourceManager::GradientPushConstant);
		Std::Array<vk::PushConstantRange, 1> pushConstantRanges { vertPushConstantRange};
		pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.Data();
		pipelineLayoutInfo.pushConstantRangeCount = (u32)pushConstantRanges.Size();
		pipelineLayoutInfo.setLayoutCount = descrSetLayouts.Size();
		pipelineLayoutInfo.pSetLayouts = descrSetLayouts.Data();

		auto pipelineLayout = device.CreateBox(pipelineLayoutInfo);
		if (debugUtils) {
			debugUtils->Helper_SetObjectName(
				device.handle,
				pipelineLayout.Handle(),
				"GuiResourceManager - Gradient PipelineLayout");
		}

		auto vertModule = ShaderHelpers::LoadShaderModuleFromFile(
			device,
			Std::CStrToSpan("assets/Gui/Gradient.vert.spv"),
			transientAlloc);
		vk::PipelineShaderStageCreateInfo vertStageInfo = {};
		vertStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
		vertStageInfo.module = vertModule.Handle();
		vertStageInfo.pName = "main";

		auto fragModule = ShaderHelpers::LoadShaderModuleFromFile(
			device,
			Std::CStrToSpan("assets/Gui/Gradient.frag.spv"),
			transientAlloc);
		vk::PipelineShaderStageCreateInfo fragStageInfo = {};
		fragStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
		fragStageInfo.module = fragModule.Handle();
		fragStageInfo.pName = "main";

		Std::Array<vk::PipelineShaderStageCreateInfo, 2> shaderStages = {
			vertStageInfo,
			fragStageInfo};

		auto dynamicStates = BuildDynamicStateArrayForColor();
		vk::PipelineDynamicStateCreateInfo dynamicState = {};
		dynamicState.dynamicStateCount = dynamicStates.Size();
		dynamicState.pDynamicStates = dynamicStates.Data();
		vk::PipelineInputAssemblyStateCreateInfo inputAssemblyState = {};
		inputAssemblyState.topology = vk::PrimitiveTopology::eTriangleList;
		vk::PipelineMultisampleStateCreateInfo multiSampleState = {};
		multiSampleState.rasterizationSamples = vk::SampleCountFlagBits::e1;
		vk::PipelineRasterizationStateCreateInfo rasterizationState = {};
		rasterizationState.lineWidth = 1.f;
		rasterizationState.polygonMode = vk::PolygonMode::eFill;
		rasterizationState.frontFace = vk::FrontFace::eClockwise;
		rasterizationState.cullMode = vk::CullModeFlagBits::eNone;
		vk::PipelineVertexInputStateCreateInfo vertexInputState = {};
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

		auto colorBlendAttachment = BuildPipelineColorBlendAttachmentState_ForBlendedColor();
		vk::PipelineColorBlendStateCreateInfo colorBlendState = {};
		colorBlendState.attachmentCount = 1;
		colorBlendState.pAttachments = &colorBlendAttachment;

		auto depthStencilInfo = BuildPipelineDepthStencilStateInfoForColor();

		vk::GraphicsPipelineCreateInfo pipelineInfo = {};
		pipelineInfo.layout = pipelineLayout.Handle();
		pipelineInfo.pColorBlendState = &colorBlendState;
		pipelineInfo.pDepthStencilState = &depthStencilInfo;
		pipelineInfo.pDynamicState = &dynamicState;
		pipelineInfo.pInputAssemblyState = &inputAssemblyState;
		pipelineInfo.pMultisampleState = &multiSampleState;
		pipelineInfo.pRasterizationState = &rasterizationState;
		pipelineInfo.pVertexInputState = &vertexInputState;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.renderPass = guiRenderPass;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages.Data();

		auto pipeline = device.CreateBox(pipelineInfo);
		if (debugUtils != nullptr) {
			debugUtils->Helper_SetObjectName(
				device.handle,
				pipeline.Handle(),
				"GuiResourceManager - Gradient Pipeline");
		}

		manager.m_gradientPipelineLayout = Std::Move(pipelineLayout);
		manager.m_gradientPipeline = Std::Move(pipeline);
	}

	// This creates a shader that writes a rounded rectangle to the
    // stencil mask
    static auto CreateStencilRectangleShader(
        GuiResourceManager& manager,
        DeviceDispatch const& device,
        vk::RenderPass guiRenderPass,
        Std::Span<vk::DescriptorSetLayout const> descrSetLayouts,
        Std::AllocRef const& transientAlloc,
        DebugUtilsDispatch const* debugUtils)
    {
		DENGINE_IMPL_GFX_ASSERT(Util::CheckNotNull(guiRenderPass));
		DENGINE_IMPL_GFX_ASSERT(Util::CheckNotNull(descrSetLayouts));

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo = {};

		vk::PushConstantRange vertPushConstantRange = {};
		vertPushConstantRange.stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment;
		vertPushConstantRange.size = GuiResourceManager::StencilRectanglePushConstant::sizeInBytes;
		Std::Array<vk::PushConstantRange, 1> pushConstantRanges { vertPushConstantRange };
		pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.Data();
		pipelineLayoutInfo.pushConstantRangeCount = (u32)pushConstantRanges.Size();
		pipelineLayoutInfo.setLayoutCount = descrSetLayouts.Size();
		pipelineLayoutInfo.pSetLayouts = descrSetLayouts.Data();

		auto pipelineLayout = device.CreateBox(pipelineLayoutInfo);
		if (debugUtils) {
			debugUtils->Helper_SetObjectName(
				device.handle,
				pipelineLayout.Handle(),
				"GuiResourceManager - Stencil Rectangle PipelineLayout");
		}

		auto vertModule = ShaderHelpers::LoadShaderModuleFromFile(
			device,
			Std::CStrToSpan("assets/Gui/RectangleStencil.vert.spv"),
			transientAlloc);
		vk::PipelineShaderStageCreateInfo vertStageInfo = {};
		vertStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
		vertStageInfo.module = vertModule.Handle();
		vertStageInfo.pName = "main";

		auto fragModule = ShaderHelpers::LoadShaderModuleFromFile(
			device,
			Std::CStrToSpan("assets/Gui/RectangleStencil.frag.spv"),
			transientAlloc);
		vk::PipelineShaderStageCreateInfo fragStageInfo = {};
		fragStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
		fragStageInfo.module = fragModule.Handle();
		fragStageInfo.pName = "main";

		Std::Array<vk::PipelineShaderStageCreateInfo, 2> shaderStages = {
			vertStageInfo,
			fragStageInfo };

		auto dynamicStates = BuildDynamicStateArrayForColor();
        vk::PipelineDynamicStateCreateInfo dynamicState = {};
        dynamicState.dynamicStateCount = dynamicStates.Size();
        dynamicState.pDynamicStates = dynamicStates.Data();
        vk::PipelineInputAssemblyStateCreateInfo inputAssemblyState;
        inputAssemblyState.topology = vk::PrimitiveTopology::eTriangleList;
        vk::PipelineMultisampleStateCreateInfo multiSampleState = {};
        multiSampleState.rasterizationSamples = vk::SampleCountFlagBits::e1;
        vk::PipelineRasterizationStateCreateInfo rasterizationState = {};
        rasterizationState.lineWidth = 1.f;
        rasterizationState.polygonMode = vk::PolygonMode::eFill;
        vk::PipelineVertexInputStateCreateInfo vertexInputState = {};
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
		vk::PipelineColorBlendAttachmentState colorBlendAttachment{};
		vk::PipelineColorBlendStateCreateInfo colorBlendState{};
		colorBlendState.attachmentCount = 1;
		colorBlendState.pAttachments = &colorBlendAttachment;

        vk::StencilOpState stencilOpStateIncrement = {};
        stencilOpStateIncrement.failOp = vk::StencilOp::eIncrementAndClamp;
        stencilOpStateIncrement.passOp = vk::StencilOp::eIncrementAndClamp;
        stencilOpStateIncrement.depthFailOp = vk::StencilOp::eIncrementAndClamp;
        stencilOpStateIncrement.compareOp = vk::CompareOp::eAlways;
        stencilOpStateIncrement.compareMask = 0xFF;
        stencilOpStateIncrement.writeMask = 0xFF;
        stencilOpStateIncrement.reference = 0;

        vk::PipelineDepthStencilStateCreateInfo depthStencilInfo = {};
        depthStencilInfo.depthTestEnable = 0;
        depthStencilInfo.depthCompareOp = vk::CompareOp::eLess;
        depthStencilInfo.stencilTestEnable = true;
        depthStencilInfo.front = stencilOpStateIncrement;
        depthStencilInfo.back = stencilOpStateIncrement;
        depthStencilInfo.depthWriteEnable = 0;
        depthStencilInfo.minDepthBounds = 0.f;
        depthStencilInfo.maxDepthBounds = 1.f;

        vk::GraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.layout = pipelineLayout.Handle();
        pipelineInfo.pColorBlendState = &colorBlendState;
        pipelineInfo.pDepthStencilState = &depthStencilInfo;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.pInputAssemblyState = &inputAssemblyState;
        pipelineInfo.pMultisampleState = &multiSampleState;
        pipelineInfo.pRasterizationState = &rasterizationState;
        pipelineInfo.pVertexInputState = &vertexInputState;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.renderPass = guiRenderPass;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages.Data();

        auto pipelineIncrement = device.CreateBox(pipelineInfo);
        if (debugUtils) {
            debugUtils->Helper_SetObjectName(
                device.handle,
                pipelineIncrement.Handle(),
                "GuiResourceManager - StencilRect Pipeline Increment");
        }

		vk::StencilOpState stencilOpStateDecrement = stencilOpStateIncrement;
		stencilOpStateDecrement.failOp = vk::StencilOp::eDecrementAndClamp;
		stencilOpStateDecrement.passOp = vk::StencilOp::eDecrementAndClamp;
		depthStencilInfo.front = stencilOpStateDecrement;
		depthStencilInfo.back = stencilOpStateDecrement;

		auto pipelineDecrement = device.CreateBox(pipelineInfo);
		if (debugUtils) {
			debugUtils->Helper_SetObjectName(
				device.handle,
				pipelineDecrement.Handle(),
				"GuiResourceManager - StencilRect Pipeline Decrement");
		}

		manager.m_stencilRectanglePipelineLayout = Std::Move(pipelineLayout);
		manager.m_stencilRectanglePipelineIncrement = Std::Move(pipelineIncrement);
		manager.m_stencilRectanglePipelineDecrement = Std::Move(pipelineDecrement);
    }

	static void CreateFilledMeshShader(
		GuiResourceManager& manager,
		DeviceDispatch const& device,
		vk::DescriptorSetLayout descrSetLayout,
		vk::RenderPass guiRenderPass,
		Std::AllocRef const& transientAlloc,
		DebugUtilsDispatch const* debugUtils)
	{
		DENGINE_IMPL_GFX_ASSERT(descrSetLayout != vk::DescriptorSetLayout{});
		DENGINE_IMPL_GFX_ASSERT(guiRenderPass != vk::RenderPass{});

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo = {};
		vk::PushConstantRange vertPushConstantRange = {};
		vertPushConstantRange.stageFlags = vk::ShaderStageFlagBits::eVertex;
		vertPushConstantRange.size = 32;
		vertPushConstantRange.offset = 0;
		vk::PushConstantRange fragPushConstantRange = {};
		fragPushConstantRange.stageFlags = vk::ShaderStageFlagBits::eFragment;
		// TODO: This looks wrong.
		fragPushConstantRange.size = sizeof(GuiResourceManager::FilledMeshPushConstant::color);
		fragPushConstantRange.offset = vertPushConstantRange.size;

		Std::Array<vk::PushConstantRange, 2> pushConstantRanges{ vertPushConstantRange, fragPushConstantRange };
		pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.Data();
		pipelineLayoutInfo.pushConstantRangeCount = (u32)pushConstantRanges.Size();

		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &descrSetLayout;

		auto pipelineLayout = device.CreateBox(pipelineLayoutInfo);
		if (debugUtils) {
			debugUtils->Helper_SetObjectName(
				device.handle,
				pipelineLayout.Handle(),
				"GuiResourceManager - FilledMesh PipelineLayout");
		}

		auto dynamicStates = BuildDynamicStateArrayForColor();
		vk::PipelineDynamicStateCreateInfo dynamicState = {};
		dynamicState.dynamicStateCount = dynamicStates.Size();
		dynamicState.pDynamicStates = dynamicStates.Data();
		vk::PipelineInputAssemblyStateCreateInfo inputAssemblyState{};
		inputAssemblyState.topology = vk::PrimitiveTopology::eTriangleList;
		vk::PipelineMultisampleStateCreateInfo multiSampleState{};
		multiSampleState.rasterizationSamples = vk::SampleCountFlagBits::e1;
		vk::PipelineRasterizationStateCreateInfo rasterizationState{};
		rasterizationState.lineWidth = 1.f;
		rasterizationState.polygonMode = vk::PolygonMode::eFill;
		vk::PipelineVertexInputStateCreateInfo vertexInputState{};
		auto vertexAttribDescrs = GuiResourceManagerImpl::BuildShaderVertexInputAttrDescr();
		vertexInputState.vertexAttributeDescriptionCount = (u32)vertexAttribDescrs.Size();
		vertexInputState.pVertexAttributeDescriptions = vertexAttribDescrs.Data();
		auto vertexBindingDescrs = GuiResourceManagerImpl::BuildShaderVertexInputBindingDescr();
		vertexInputState.vertexBindingDescriptionCount = (u32)1;
		vertexInputState.pVertexBindingDescriptions = vertexBindingDescrs.Data();
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
		auto colorBlendAttachment = BuildPipelineColorBlendAttachmentState_ForBlendedColor();
		vk::PipelineColorBlendStateCreateInfo colorBlendState = {};
		colorBlendState.attachmentCount = 1;
		colorBlendState.pAttachments = &colorBlendAttachment;

		auto vertModule = ShaderHelpers::LoadShaderModuleFromFile(
			device,
			Std::CStrToSpan("data/Gui/FilledMesh.vert.spv"),
			transientAlloc);
		vk::PipelineShaderStageCreateInfo vertStageInfo{};
		vertStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
		vertStageInfo.module = vertModule.Handle();
		vertStageInfo.pName = "main";

		auto fragModuleOpt = ShaderHelpers::LoadShaderModuleFromFile(
			device,
			Std::CStrToSpan("data/Gui/FilledMesh.frag.spv"),
			transientAlloc);
		vk::PipelineShaderStageCreateInfo fragStageInfo = {};
		fragStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
		fragStageInfo.module = fragModuleOpt.Handle();
		fragStageInfo.pName = "main";

		Std::Array<vk::PipelineShaderStageCreateInfo, 2> shaderStages = { vertStageInfo, fragStageInfo };

        auto depthStencilInfo = BuildPipelineDepthStencilStateInfoForColor();

		vk::GraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.layout = pipelineLayout.Handle();
		pipelineInfo.pColorBlendState = &colorBlendState;
		pipelineInfo.pDepthStencilState = &depthStencilInfo;
		pipelineInfo.pDynamicState = &dynamicState;
		pipelineInfo.pInputAssemblyState = &inputAssemblyState;
		pipelineInfo.pMultisampleState = &multiSampleState;
		pipelineInfo.pRasterizationState = &rasterizationState;
		pipelineInfo.pVertexInputState = &vertexInputState;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.renderPass = guiRenderPass;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages.Data();

		auto pipeline = device.CreateBox(pipelineInfo);
		if (debugUtils) {
			debugUtils->Helper_SetObjectName(
				device.handle,
				pipeline.Handle(),
				"GuiResourceManager - FilledMesh Pipeline");
		}
		manager.m_filledMeshPipelineLayout = Std::Move(pipelineLayout);
		manager.m_filledMeshPipeline = Std::Move(pipeline);
	}

	static void CreateViewportShader(
		GuiResourceManager& manager,
		DeviceDispatch const& device,
		Std::Span<vk::DescriptorSetLayout const> descrSetLayoutsIn,
		vk::DescriptorSetLayout viewportImgDescrLayout,
		vk::RenderPass guiRenderPass,
		Std::AllocRef const& transientAlloc,
		DebugUtilsDispatch const* debugUtils)
	{
		DENGINE_IMPL_GFX_ASSERT(Util::CheckNotNull(descrSetLayoutsIn));
		DENGINE_IMPL_GFX_ASSERT(viewportImgDescrLayout != vk::DescriptorSetLayout{});
		DENGINE_IMPL_GFX_ASSERT(guiRenderPass != vk::RenderPass{});

		Std::StackVec<vk::DescriptorSetLayout, 5> descrLayouts = { descrSetLayoutsIn };
		descrLayouts.PushBack(viewportImgDescrLayout);

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo = {};
		pipelineLayoutInfo.setLayoutCount = descrLayouts.Size();
		pipelineLayoutInfo.pSetLayouts = descrLayouts.Data();

		vk::PushConstantRange pushConstantRange = {};
		pushConstantRange.stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment;
		pushConstantRange.size = sizeof(GuiResourceManager::ViewportPushConstant);
		pushConstantRange.offset = 0;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
		pipelineLayoutInfo.pushConstantRangeCount = 1;

		auto pipelineLayout = device.CreateBox(pipelineLayoutInfo);
		if (debugUtils) {
			debugUtils->Helper_SetObjectName(
				device.handle,
				pipelineLayout.Handle(),
				"GuiResourceManager - Viewport PipelineLayout");
		}

		auto vertModule = ShaderHelpers::LoadShaderModuleFromFile(
			device,
			Std::CStrToSpan("assets/Gui/Viewport.vert.spv"),
			transientAlloc);
		vk::PipelineShaderStageCreateInfo vertStageInfo{};
		vertStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
		vertStageInfo.module = vertModule.Handle();
		vertStageInfo.pName = "main";

		auto fragModule = ShaderHelpers::LoadShaderModuleFromFile(
			device,
			Std::CStrToSpan("assets/Gui/Viewport.frag.spv"),
			transientAlloc);
		vk::PipelineShaderStageCreateInfo fragStageInfo = {};
		fragStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
		fragStageInfo.module = fragModule.Handle();
		fragStageInfo.pName = "main";

		Std::Array<vk::PipelineShaderStageCreateInfo, 2> shaderStages = { vertStageInfo, fragStageInfo };

		auto dynamicStates = BuildDynamicStateArrayForColor();
		vk::PipelineDynamicStateCreateInfo dynamicState = {};
		dynamicState.dynamicStateCount = dynamicStates.Size();
		dynamicState.pDynamicStates = dynamicStates.Data();
		vk::PipelineInputAssemblyStateCreateInfo inputAssemblyState = {};
		inputAssemblyState.topology = vk::PrimitiveTopology::eTriangleList;
		vk::PipelineMultisampleStateCreateInfo multiSampleState = {};
		multiSampleState.rasterizationSamples = vk::SampleCountFlagBits::e1;
		vk::PipelineRasterizationStateCreateInfo rasterizationState = {};
		rasterizationState.lineWidth = 1.f;
		rasterizationState.polygonMode = vk::PolygonMode::eFill;
		vk::PipelineVertexInputStateCreateInfo vertexInputState = {};
		vk::Viewport viewport = {};
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
		vk::PipelineColorBlendAttachmentState colorBlendAttachment = {};
		colorBlendAttachment.colorWriteMask |= vk::ColorComponentFlagBits::eR;
		colorBlendAttachment.colorWriteMask |= vk::ColorComponentFlagBits::eG;
		colorBlendAttachment.colorWriteMask |= vk::ColorComponentFlagBits::eB;
		colorBlendAttachment.colorWriteMask |= vk::ColorComponentFlagBits::eA;
		vk::PipelineColorBlendStateCreateInfo colorBlendState{};
		colorBlendState.attachmentCount = 1;
		colorBlendState.pAttachments = &colorBlendAttachment;

		auto depthStencilInfo = BuildPipelineDepthStencilStateInfoForColor();

		vk::GraphicsPipelineCreateInfo pipelineInfo = {};
		pipelineInfo.layout = pipelineLayout.Handle();
		pipelineInfo.pColorBlendState = &colorBlendState;
		pipelineInfo.pDepthStencilState = &depthStencilInfo;
		pipelineInfo.pDynamicState = &dynamicState;
		pipelineInfo.pInputAssemblyState = &inputAssemblyState;
		pipelineInfo.pMultisampleState = &multiSampleState;
		pipelineInfo.pRasterizationState = &rasterizationState;
		pipelineInfo.pVertexInputState = &vertexInputState;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.renderPass = guiRenderPass;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages.Data();

		auto pipeline = device.CreateBox(pipelineInfo);
		if (debugUtils) {
			debugUtils->Helper_SetObjectName(
				device.handle,
				pipeline.Handle(),
				"GuiResourceManager - Viewport Pipeline");
		}

		manager.m_viewportPipelineLayout = Std::Move(pipelineLayout);
		manager.m_viewportPipeline = Std::Move(pipeline);
	}

	static void CreateTextShader(
		GuiResourceManager& manager,
		DeviceDispatch const& device,
		vk::RenderPass guiRenderPass,
		vk::PipelineLayout pipelineLayout,
		Std::AllocRef const& transientAlloc,
		DebugUtilsDispatch const* debugUtils)
	{
		DENGINE_IMPL_GFX_ASSERT(guiRenderPass != vk::RenderPass{});
		DENGINE_IMPL_GFX_ASSERT(pipelineLayout != vk::PipelineLayout{});

		auto vertModule = ShaderHelpers::LoadShaderModuleFromFile(
			device,
			Std::CStrToSpan("assets/Gui/Text.vert.spv"),
			transientAlloc);
		vk::PipelineShaderStageCreateInfo vertStageInfo = {};
		vertStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
		vertStageInfo.module = vertModule.Handle();
		vertStageInfo.pName = "main";

		auto fragModule = ShaderHelpers::LoadShaderModuleFromFile(
			device,
			Std::CStrToSpan("assets/Gui/Text.frag.spv"),
			transientAlloc);
		vk::PipelineShaderStageCreateInfo fragStageInfo = {};
		fragStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
		fragStageInfo.module = fragModule.Handle();
		fragStageInfo.pName = "main";

		Std::Array<vk::PipelineShaderStageCreateInfo, 2> shaderStages = { vertStageInfo, fragStageInfo };

		auto dynamicStates = BuildDynamicStateArrayForColor();
		vk::PipelineDynamicStateCreateInfo dynamicState = {};
		dynamicState.dynamicStateCount = dynamicStates.Size();
		dynamicState.pDynamicStates = dynamicStates.Data();
		vk::PipelineInputAssemblyStateCreateInfo inputAssemblyState{};
		inputAssemblyState.topology = vk::PrimitiveTopology::eTriangleList;
		vk::PipelineMultisampleStateCreateInfo multiSampleState{};
		multiSampleState.rasterizationSamples = vk::SampleCountFlagBits::e1;
		vk::PipelineRasterizationStateCreateInfo rasterizationState{};
		rasterizationState.lineWidth = 1.f;
		rasterizationState.polygonMode = vk::PolygonMode::eFill;
		vk::PipelineVertexInputStateCreateInfo vertexInputState{};
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
		auto colorBlendAttachment = BuildPipelineColorBlendAttachmentState_ForBlendedColor();
		vk::PipelineColorBlendStateCreateInfo colorBlendState = {};
		colorBlendState.attachmentCount = 1;
		colorBlendState.pAttachments = &colorBlendAttachment;

		auto depthStencilInfo = BuildPipelineDepthStencilStateInfoForColor();

		vk::GraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.layout = pipelineLayout;
		pipelineInfo.pColorBlendState = &colorBlendState;
		pipelineInfo.pDepthStencilState = &depthStencilInfo;
		pipelineInfo.pDynamicState = &dynamicState;
		pipelineInfo.pInputAssemblyState = &inputAssemblyState;
		pipelineInfo.pMultisampleState = &multiSampleState;
		pipelineInfo.pRasterizationState = &rasterizationState;
		pipelineInfo.pVertexInputState = &vertexInputState;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.renderPass = guiRenderPass;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages.Data();

		auto pipeline = device.CreateBox(pipelineInfo);
		if (debugUtils) {
			debugUtils->Helper_SetObjectName(
				device.handle,
				pipeline.Handle(),
				"GuiResourceManager - Text Pipeline");
		}

		manager.m_font_pipeline = Std::Move(pipeline);
	}

	static auto AllocateDescriptorSets(
		DeviceDispatch const& device,
		vk::DescriptorPool pool,
		vk::DescriptorSetLayout layout,
		int count,
		Std::AllocRef transientAlloc)
	{
		auto setLayouts = Std::NewVec<vk::DescriptorSetLayout>(transientAlloc);
		setLayouts.Resize(count, layout);
		vk::DescriptorSetAllocateInfo descrAllocInfo = {};
		descrAllocInfo.descriptorPool = pool;
		descrAllocInfo.descriptorSetCount = count;
		descrAllocInfo.pSetLayouts = setLayouts.Data();
		auto descrSets = Std::NewVec<vk::DescriptorSet>(transientAlloc);
		descrSets.Resize(setLayouts.Size());
		auto vkResult = device.Alloc(descrAllocInfo, descrSets.Data());
		return vk::ResultValue{ vkResult, Std::Move(descrSets) };
	}

	static void SetupDummyCameraObjectUniforms(
		GuiResourceManager& manager,
		DeviceDispatch const& device,
		VmaAllocator vma,
		vk::DescriptorSetLayout cameraDataUniformDescrLayout,
		vk::DescriptorSetLayout objectDataUniformDescrLayout,
		Std::AllocRef transientAlloc,
		DebugUtilsDispatch const* debugUtils)
	{
		vk::Result vkResult = {};

		vk::BufferCreateInfo bufferInfo = {};
		bufferInfo.sharingMode = vk::SharingMode::eExclusive;
		bufferInfo.size = ViewportManager::cameraDataUniformStructSize + sizeof(ObjectDataManager::ObjectDataUniform);
		bufferInfo.usage = vk::BufferUsageFlagBits::eUniformBuffer;
		VmaAllocationCreateInfo vmaAllocCreateInfo = {};
		vmaAllocCreateInfo.flags =
			VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
		vmaAllocCreateInfo.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

		manager.m_dummyCameraObjectUniforms.buffer = device.CreateBox(vma, bufferInfo, vmaAllocCreateInfo).buffer;
		if (debugUtils != nullptr) {
			debugUtils->Helper_SetObjectName(
				device.handle,
				manager.m_dummyCameraObjectUniforms.buffer,
				"GuiResourceManager - DummyCameraObjectUniforms Buffer");
		}
		Math::Mat4 identityMat = Math::Mat4::Identity();

		void* mappedMem = nullptr;
		vmaMapMemory(vma, manager.m_dummyCameraObjectUniforms.buffer.Alloc(), &mappedMem);
		struct CombinedUniformData {
			Math::Mat4 cameraTransform;
			ObjectDataManager::ObjectDataUniform objectDataUniform;
		};
		CombinedUniformData combinedUniformData = {
			identityMat,
			ObjectDataManager::ObjectDataUniform{
				.transform = identityMat }, };
		// Hack: We are using the same shaders for regular GUI and in-scene GUI.
		// But the in-scene shaders will transform into coordinate space where Y
		// goes upwards, so we have to adjust.
		combinedUniformData.cameraTransform.At(0, 0) = 2.f;
		combinedUniformData.cameraTransform.At(1, 1) = 2.f;

		std::memcpy(mappedMem, &combinedUniformData, sizeof(combinedUniformData));
		vmaFlushAllocation(
			vma,
			manager.m_dummyCameraObjectUniforms.buffer.Alloc(),
			0,
			bufferInfo.size);
		vmaUnmapMemory(vma, manager.m_dummyCameraObjectUniforms.buffer.Alloc());

		Std::Array<vk::DescriptorPoolSize, 2> descrPoolSizes = {};
		auto& cameraDataDescrPoolSize = descrPoolSizes[0];
		cameraDataDescrPoolSize.descriptorCount = 1;
		cameraDataDescrPoolSize.type = ViewportManager::cameraDataUniformDescrType;
		auto& objectDataDescrPoolSize = descrPoolSizes[1];
		objectDataDescrPoolSize.descriptorCount = 1;
		objectDataDescrPoolSize.type = ObjectDataManager::objectDataUniformDescrType;
		vk::DescriptorPoolCreateInfo descrPoolInfo = {};
		descrPoolInfo.maxSets = 2;
		descrPoolInfo.poolSizeCount = descrPoolSizes.Size();
		descrPoolInfo.pPoolSizes = descrPoolSizes.Data();
		manager.m_dummyCameraObjectUniforms.descrPool = device.CreateBox(descrPoolInfo);
		if (debugUtils != nullptr) {
			debugUtils->Helper_SetObjectName(
				device.handle,
				manager.m_dummyCameraObjectUniforms.descrPool,
				"GuiResourceManager - DummyCameraObjectUniforms DescrPool");
		}

		Std::Array<vk::DescriptorSetLayout, 2> descrSetLayouts = {
			cameraDataUniformDescrLayout,
			objectDataUniformDescrLayout, };
		Std::Array<vk::DescriptorSet, 2> descrSetsOut = {};
		vk::DescriptorSetAllocateInfo descrAllocInfo = {};
		descrAllocInfo.descriptorPool = manager.m_dummyCameraObjectUniforms.descrPool.Handle();;
		descrAllocInfo.descriptorSetCount = 2;
		descrAllocInfo.pSetLayouts = descrSetLayouts.Data();
		vkResult = device.Alloc(descrAllocInfo, descrSetsOut.Data());
		if (vkResult != vk::Result::eSuccess) {
			throw std::runtime_error("Failed to allocate descriptor sets for dummy camera+object uniforms.");
		}

		manager.m_dummyCameraObjectUniforms.cameraDescrSet = descrSetsOut[0];
		if (debugUtils != nullptr) {
			debugUtils->Helper_SetObjectName(
				device.handle,
				manager.m_dummyCameraObjectUniforms.cameraDescrSet,
				"GuiResourceManager - DummyCameraObjectUniforms CameraDescrSet");
		}
		manager.m_dummyCameraObjectUniforms.objectDescrSet = descrSetsOut[1];
		if (debugUtils != nullptr) {
			debugUtils->Helper_SetObjectName(
				device.handle,
				manager.m_dummyCameraObjectUniforms.objectDescrSet,
				"GuiResourceManager - DummyCameraObjectUniforms ObjectDescrSet");
		}

		// Write to the DescriptorSets
		vk::DescriptorBufferInfo cameraDescrBufferInfo = {};
		cameraDescrBufferInfo.buffer = manager.m_dummyCameraObjectUniforms.buffer.Handle();
		cameraDescrBufferInfo.offset = 0;
		cameraDescrBufferInfo.range = sizeof(Math::Mat4);
		vk::WriteDescriptorSet cameraWriteDescrSet = {};
		cameraWriteDescrSet.descriptorType = ViewportManager::cameraDataUniformDescrType;
		cameraWriteDescrSet.dstBinding = 0;
		cameraWriteDescrSet.dstSet = manager.m_dummyCameraObjectUniforms.cameraDescrSet;
		cameraWriteDescrSet.descriptorCount = 1;
		cameraWriteDescrSet.pBufferInfo = &cameraDescrBufferInfo;

		vk::DescriptorBufferInfo objectDescrBufferInfo = {};
		objectDescrBufferInfo.buffer = manager.m_dummyCameraObjectUniforms.buffer.Handle();
		objectDescrBufferInfo.offset = sizeof(Math::Mat4);
		objectDescrBufferInfo.range = sizeof(Math::Mat4);
		vk::WriteDescriptorSet objectWriteDescrSet = {};
		objectWriteDescrSet.descriptorType = ObjectDataManager::objectDataUniformDescrType;
		objectWriteDescrSet.dstBinding = 0;
		objectWriteDescrSet.dstSet = manager.m_dummyCameraObjectUniforms.objectDescrSet;
		objectWriteDescrSet.descriptorCount = 1;
		objectWriteDescrSet.pBufferInfo = &objectDescrBufferInfo;
		device.UpdateDescriptorSets({ cameraWriteDescrSet, objectWriteDescrSet }, {});
	}

	static void SetupGuiWindowUniforms(
		GuiResourceManager& manager,
		DeviceDispatch const& device,
		VmaAllocator vma,
		int inFlightCount,
		Std::AllocRef transientAlloc,
		DebugUtilsDispatch const* debugUtils)
	{
		vk::Result vkResult = {};

		auto startCapacity = GuiResourceManager::WindowShaderUniforms::minimumCapacity;

		auto uniformElementSize = manager.WindowUniformElementAlignment();

		vk::BufferCreateInfo bufferInfo = {};
		bufferInfo.sharingMode = vk::SharingMode::eExclusive;
		bufferInfo.size = uniformElementSize * startCapacity * inFlightCount;
		bufferInfo.usage = vk::BufferUsageFlagBits::eUniformBuffer;
		VmaAllocationCreateInfo vmaAllocCreateInfo = {};
		// We're gonna modify the data every frame, so we persistently map the memory.
		vmaAllocCreateInfo.flags =
			VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
			| VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_MAPPED_BIT;
		vmaAllocCreateInfo.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO;
		auto buffer = device.CreateBox(vma, bufferInfo, vmaAllocCreateInfo);
		if (debugUtils != nullptr) {
			debugUtils->Helper_SetObjectName(
				device.handle,
				buffer.buffer,
				"GuiResourceManager - PerWindowUniform - Buffer");
		}

		// TODO: This can be a function.
		vk::DescriptorPoolSize descrPoolSize = {};
		descrPoolSize.descriptorCount = startCapacity * inFlightCount;
		descrPoolSize.type = GuiResourceManager::windowDataUniformDescrType;
		vk::DescriptorPoolCreateInfo descrPoolInfo = {};
		descrPoolInfo.maxSets = startCapacity * inFlightCount;
		descrPoolInfo.poolSizeCount = 1;
		descrPoolInfo.pPoolSizes = &descrPoolSize;
		auto descrPool = device.CreateBox(descrPoolInfo);
		if (debugUtils != nullptr) {
			debugUtils->Helper_SetObjectName(
				device.handle,
				descrPool,
				"GuiResourceManager - PerWindowUniform - DescriptorPool");
		}


		// Allocate all the descriptors and write to them.
		auto descrAllocResult = AllocateDescriptorSets(
			device,
			descrPool.Handle(),
			manager.GetGuiWindowUniformDescrLayout(),
			startCapacity * inFlightCount,
			transientAlloc);
		vkResult = descrAllocResult.result;
		if (vkResult != vk::Result::eSuccess)
			throw std::runtime_error("");
		auto& descrSets = descrAllocResult.value;

		// Create the names for the descriptors.
		if (debugUtils != nullptr) {
			for (int inFlightIndex = 0; inFlightIndex < inFlightCount; inFlightIndex++) {
				for (int i = 0; i < startCapacity; i++) {
					int linearIndex = inFlightIndex * startCapacity + i;
					auto name = std::format("GuiResourceManager - PerWindowUniform DescrSet #{}", i);
					debugUtils->Helper_SetObjectName(
						device.handle,
						descrSets[linearIndex],
						name.c_str());
				}
			}
		}

		// Update the descriptors.
		{
			auto descrWrites = Std::NewVec<vk::WriteDescriptorSet>(transientAlloc);
			descrWrites.Resize(descrSets.Size());
			auto descrBufferInfos = Std::NewVec<vk::DescriptorBufferInfo>(transientAlloc);
			descrBufferInfos.Resize(descrWrites.Size());
			for (int i = 0; i < descrWrites.Size(); i++) {
				auto& descrWrite = descrWrites[i];

				auto& descrBuffer = descrBufferInfos[i];
				descrBuffer.buffer = buffer.buffer.Handle();
				descrBuffer.offset = uniformElementSize * i;
				descrBuffer.range = uniformElementSize;

				descrWrite.dstSet = descrSets[i];
				descrWrite.dstBinding = 0;
				descrWrite.dstArrayElement = 0;
				descrWrite.descriptorCount = 1;
				descrWrite.descriptorType = GuiResourceManager::windowDataUniformDescrType;
				descrWrite.pBufferInfo = &descrBuffer;
			}
			device.UpdateDescriptorSets(descrWrites.ToSpan(), {});
		}

		manager.windowUniforms.buffer = Std::Move(buffer.buffer);
		manager.windowUniforms.vmaAllocResultInfo = buffer.allocInfo;
		manager.windowUniforms.m_descrPool = Std::Move(descrPool);
		// Copy all the descr sets
		for (auto const& item : descrSets)
			manager.windowUniforms.windowUniformDescrSets.push_back(item);
	}

	static void SetupTextResources(
		GuiResourceManager& guiResMgr,
		DeviceDispatch const& device,
		Std::Span<vk::DescriptorSetLayout const> descrSetLayoutsIn,
		DebugUtilsDispatch const* debugUtils)
	{
		DENGINE_IMPL_GFX_ASSERT(Util::CheckNotNull(descrSetLayoutsIn));

		vk::DescriptorPoolSize sampledImgDescrPoolSize = {};
		sampledImgDescrPoolSize.descriptorCount = 8196;
		sampledImgDescrPoolSize.type = vk::DescriptorType::eCombinedImageSampler;
		vk::DescriptorPoolCreateInfo descrPoolInfo{};
		descrPoolInfo.maxSets = 8196;
		descrPoolInfo.poolSizeCount = 1;
		descrPoolInfo.pPoolSizes = &sampledImgDescrPoolSize;
		auto descrPool = device.CreateBox(descrPoolInfo);
		if (debugUtils) {
			debugUtils->Helper_SetObjectName(
				device.handle,
				descrPool.Handle(),
				"GuiResourceManager - Text DescrPool");
		}

		vk::DescriptorSetLayoutBinding imgDescrBinding = {};
		imgDescrBinding.binding = 0;
		imgDescrBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;
		imgDescrBinding.descriptorCount = 1;
		imgDescrBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		vk::DescriptorSetLayoutCreateInfo descrSetLayoutInfo = {};
		descrSetLayoutInfo.bindingCount = 1;
		descrSetLayoutInfo.pBindings = &imgDescrBinding;
		auto descrSetLayout = device.CreateBox(descrSetLayoutInfo);
		if (debugUtils) {
			debugUtils->Helper_SetObjectName(
				device.handle,
				descrSetLayout.Handle(),
				"GuiResourceManager - Text DescrSetLayout");
		}

		vk::SamplerCreateInfo samplerInfo = {};
		samplerInfo.addressModeU = vk::SamplerAddressMode::eClampToEdge;
		samplerInfo.addressModeV = vk::SamplerAddressMode::eClampToEdge;
		samplerInfo.addressModeW = vk::SamplerAddressMode::eClampToEdge;
		samplerInfo.magFilter = vk::Filter::eNearest;
		samplerInfo.maxLod = VK_LOD_CLAMP_NONE;
		samplerInfo.minFilter = vk::Filter::eNearest;
		samplerInfo.mipmapMode = vk::SamplerMipmapMode::eNearest;
		auto sampler = device.CreateBox(samplerInfo);
		if (debugUtils) {
			debugUtils->Helper_SetObjectName(
				device.handle,
				sampler.Handle(),
				"GuiResourceManager - Text Sampler");
		}

		vk::PushConstantRange pushConstantRange = {};
		pushConstantRange.stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment;
		pushConstantRange.size = sizeof(GuiResourceManager::FontPushConstant);

		Std::StackVec<vk::DescriptorSetLayout, 5> descrSetLayouts = { descrSetLayoutsIn };
		descrSetLayouts.PushBack(descrSetLayout.Handle());

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo = {};
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.setLayoutCount = descrSetLayouts.Size();
		pipelineLayoutInfo.pSetLayouts = descrSetLayouts.Data();
		auto pipelineLayout = device.CreateBox(pipelineLayoutInfo);
		if (debugUtils) {
			debugUtils->Helper_SetObjectName(
				device.handle,
				pipelineLayout.Handle(),
				"GuiResourceManager - Text PipelineLayout");
		}

		guiResMgr.m_font_descrPool = Std::Move(descrPool);
		guiResMgr.m_font_descrSetLayout = Std::Move(descrSetLayout);
		guiResMgr.m_font_sampler = Std::Move(sampler);
		guiResMgr.m_font_pipelineLayout = Std::Move(pipelineLayout);
	}
}

vk::DescriptorSetLayout Vk::GuiResourceManager::BuildGuiWindowDescrLayout(
	GuiResourceManager& manager,
	DeviceDispatch const& device,
	DebugUtilsDispatch const* debugUtils)
{
	vk::DescriptorSetLayoutBinding binding = {};
	binding.binding = 0;
	binding.descriptorCount = 1;
	binding.descriptorType = GuiResourceManager::windowDataUniformDescrType;
	binding.stageFlags = vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eVertex;
	vk::DescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.bindingCount = 1;
	layoutInfo.pBindings = &binding;
	manager.windowUniforms.m_descrLayout = device.CreateBox(layoutInfo);
	if (debugUtils != nullptr) {
		debugUtils->Helper_SetObjectName(
			device.handle,
			manager.windowUniforms.m_descrLayout,
			"GuiResourceManager - PerWindowUniform - DescriptorSetLayout");
	}

	return manager.windowUniforms.m_descrLayout.Handle();
}

vk::DescriptorSetLayout Vk::GuiResourceManager::GetGuiWindowUniformDescrLayout() const {
	DENGINE_IMPL_GFX_ASSERT(!windowUniforms.m_descrLayout.IsNull());
	return windowUniforms.m_descrLayout.Handle();
}

void Vk::GuiResourceManager::Init(
	GuiResourceManager& manager,
	Init_Params const& params)
{
	DENGINE_IMPL_GFX_ASSERT(Util::CheckNotNull(params.cameraDataUniformDescrLayout));
	DENGINE_IMPL_GFX_ASSERT(Util::CheckNotNull(params.objectDataUniformDescrLayout));
	DENGINE_IMPL_GFX_ASSERT(Util::CheckNotNull(params.guiRenderPass));
	DENGINE_IMPL_GFX_ASSERT(params.inFlightCount != 0);

	auto const& device = params.device;
	auto const& vma = params.vma;
	auto inFlightCount = params.inFlightCount;
	auto guiRenderPass = params.guiRenderPass;
	auto cameraDataUniformDescrLayout = params.cameraDataUniformDescrLayout;
	auto objectDataUniformDescrLayout = params.objectDataUniformDescrLayout;
	auto& transientAlloc = params.transientAlloc;
	auto const* debugUtils = params.debugUtils;

	manager.m_inFlightCount = inFlightCount;
	manager.m_minUniformBufferAlignment = device.physDeviceLimits.minUniformBufferOffsetAlignment;

	vk::Result vkResult = {};

	GuiResourceManagerImpl::SetupDummyCameraObjectUniforms(
		manager,
		device,
		vma,
		cameraDataUniformDescrLayout,
		objectDataUniformDescrLayout,
		transientAlloc,
		debugUtils);

	GuiResourceManagerImpl::SetupGuiWindowUniforms(
		manager,
		device,
		vma,
		inFlightCount,
		transientAlloc,
		debugUtils);

	auto windowDescrSetLayout = manager.GetGuiWindowUniformDescrLayout();

	Std::Array<vk::DescriptorSetLayout, 3> combinedDescrSetLayouts {
		cameraDataUniformDescrLayout,
		objectDataUniformDescrLayout,
		windowDescrSetLayout };

	GuiResourceManagerImpl::CreateRectangleShader(
		manager,
		device,
		combinedDescrSetLayouts.ToSpan(),
		guiRenderPass,
		transientAlloc,
		debugUtils);

	GuiResourceManagerImpl::SetupTextResources(
		manager,
		device,
		combinedDescrSetLayouts.ToSpan(),
		debugUtils);
	auto fontPipelineLayout = manager.m_font_pipelineLayout.Handle();

	GuiResourceManagerImpl::CreateRectangleShadowShader(
		manager,
		device,
		combinedDescrSetLayouts.ToSpan(),
		guiRenderPass,
		transientAlloc,
		debugUtils);

	GuiResourceManagerImpl::CreateStencilRectangleShader(
		manager,
		device,
		guiRenderPass,
		combinedDescrSetLayouts.ToSpan(),
		transientAlloc,
		debugUtils);

	GuiResourceManagerImpl::CreateGradientShader(
		manager,
		device,
		combinedDescrSetLayouts.ToSpan(),
		guiRenderPass,
		transientAlloc,
		debugUtils);

	GuiResourceManagerImpl::CreateTextShader(
		manager,
		device,
		guiRenderPass,
		fontPipelineLayout,
		transientAlloc,
		debugUtils);

	GuiResourceManagerImpl::CreateViewportShader(
		manager,
		device,
		combinedDescrSetLayouts.ToSpan(),
		params.viewportImgDescrSetLayout,
		guiRenderPass,
		transientAlloc,
		debugUtils);

	vk::BufferCreateInfo vtxBufferInfo = {};
	vtxBufferInfo.sharingMode = vk::SharingMode::eExclusive;
	vtxBufferInfo.size = sizeof(GuiVertex) * minVtxCapacity * inFlightCount;
	vtxBufferInfo.usage = vk::BufferUsageFlagBits::eVertexBuffer;
	VmaAllocationCreateInfo vtxVmaAllocInfo{};
	vtxVmaAllocInfo.flags = VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	vtxVmaAllocInfo.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO;
	VmaAllocationInfo vtxVmaAllocResultInfo;
	vkResult = (vk::Result)vmaCreateBuffer(
		vma,
		(VkBufferCreateInfo const*)&vtxBufferInfo,
		&vtxVmaAllocInfo,
		(VkBuffer*)&manager.vtxBuffer,
		&manager.vtxVmaAlloc,
		&vtxVmaAllocResultInfo);
	if (vkResult != vk::Result::eSuccess)
		throw std::runtime_error("DEngine - Vulkan: VMA was unable to allocate memory for GUI vertices.");
	if (debugUtils) {
		debugUtils->Helper_SetObjectName(
			device.handle,
			manager.vtxBuffer,
			"GuiResourceManager - VertexBuffer");
	}
	manager.vtxMappedMem = { (u8*)vtxVmaAllocResultInfo.pMappedData, (uSize)vtxVmaAllocResultInfo.size };
	manager.vtxInFlightCapacity = manager.vtxMappedMem.Size() / inFlightCount;

	vk::BufferCreateInfo indexBufferInfo = {};
	indexBufferInfo.sharingMode = vk::SharingMode::eExclusive;
	indexBufferInfo.size = sizeof(u32) * minIndexCapacity * inFlightCount;
	indexBufferInfo.usage = vk::BufferUsageFlagBits::eIndexBuffer;
	VmaAllocationCreateInfo indexVmaAllocInfo = {};
	indexVmaAllocInfo.flags = VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	indexVmaAllocInfo.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO;
	VmaAllocationInfo indexVmaAllocResultInfo;
	vkResult = (vk::Result)vmaCreateBuffer(
		vma,
		(VkBufferCreateInfo*)&indexBufferInfo,
		&indexVmaAllocInfo,
		(VkBuffer*)&manager.indexBuffer,
		&manager.indexVmaAlloc,
		&indexVmaAllocResultInfo);
	if (vkResult != vk::Result::eSuccess)
		throw std::runtime_error("DEngine - Vulkan: VMA was unable to allocate memory for GUI indices.");
	if (debugUtils) {
		debugUtils->Helper_SetObjectName(
			device.handle,
			manager.indexBuffer,
			"GuiResourceManager - IndexBuffer");
	}
	manager.indexMappedMem = { (u8*)indexVmaAllocResultInfo.pMappedData, (uSize)indexVmaAllocResultInfo.size };
	manager.indexInFlightCapacity = manager.indexMappedMem.Size() / inFlightCount;
}

namespace DEngine::Gfx::Vk {
	namespace Helper {
		[[nodiscard]] BoxVmaImg AllocSampledFontGlyphImage(
			DeviceDispatch const& device,
			VmaAllocator vma,
			u32 width,
			u32 height)
		{
			// Allocate the destination image
			vk::ImageCreateInfo imgInfo = {};
			imgInfo.arrayLayers = 1;
			imgInfo.extent = vk::Extent3D{ width, height, 1 };
			imgInfo.format = vk::Format::eR8Unorm;
			imgInfo.imageType = vk::ImageType::e2D;
			imgInfo.initialLayout = vk::ImageLayout::eUndefined;
			imgInfo.mipLevels = 1;
			imgInfo.samples = vk::SampleCountFlagBits::e1;
			imgInfo.sharingMode = vk::SharingMode::eExclusive;
			imgInfo.tiling = vk::ImageTiling::eOptimal;
			imgInfo.usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
			VmaAllocationCreateInfo vmaAllocInfo {};
			vmaAllocInfo.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

			return device.CreateBox(vma, imgInfo, vmaAllocInfo).img;
		}

		[[nodiscard]] vk::ImageMemoryBarrier CreateSampledImgBarrier_PreCopy(vk::Image handle) {
			vk::ImageMemoryBarrier preCopyBarrier {};
			preCopyBarrier.image = handle;
			preCopyBarrier.oldLayout = vk::ImageLayout::eUndefined;
			preCopyBarrier.newLayout = vk::ImageLayout::eTransferDstOptimal;
			preCopyBarrier.srcAccessMask = {};
			preCopyBarrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
			preCopyBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
			preCopyBarrier.subresourceRange.layerCount = 1;
			preCopyBarrier.subresourceRange.levelCount = 1;
			return preCopyBarrier;
		}

		[[nodiscard]] vk::ImageMemoryBarrier CreateSampledImgBarrier_PostCopy(vk::Image handle) {
			vk::ImageMemoryBarrier postCopyBarrier {};
			postCopyBarrier.image = handle;
			postCopyBarrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
			postCopyBarrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
			postCopyBarrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
			postCopyBarrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
			postCopyBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
			postCopyBarrier.subresourceRange.layerCount = 1;
			postCopyBarrier.subresourceRange.levelCount = 1;
			return postCopyBarrier;
		}
	}

	[[nodiscard]] vk::BufferImageCopy FontGlyphs_CreateBufferImageCopy(
		u32 imgWidth,
		u32 imgHeight,
		u64 bufferOffset)
	{
		vk::BufferImageCopy buffImgCopy {};
		buffImgCopy.bufferOffset = bufferOffset;
		buffImgCopy.bufferImageHeight = imgHeight;
		buffImgCopy.bufferRowLength = 0;
		buffImgCopy.imageExtent = vk::Extent3D{ imgWidth, imgHeight, 1 };
		buffImgCopy.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
		buffImgCopy.imageSubresource.layerCount = 1;
		return buffImgCopy;
	}

	void FontGlyphs_RecordTransfers_PreCopyBarriers(
		DeviceDispatch const& device,
		vk::CommandBuffer cmdBuffer,
		Std::Span<BoxVmaImg const> images,
		TransientAllocRef transientAlloc)
	{
		int jobCount = (int)images.Size();

		auto preCopyBarriers = Std::NewVec<vk::ImageMemoryBarrier>(transientAlloc);
		preCopyBarriers.Reserve(jobCount);
		for (auto const& img : images)
			preCopyBarriers.PushBack(Helper::CreateSampledImgBarrier_PreCopy(img.Handle()));
		device.cmdPipelineBarrier(
			cmdBuffer,
			vk::PipelineStageFlagBits::eTopOfPipe,
			vk::PipelineStageFlagBits::eTransfer,
			{},
			{},{},
			{ (u32)preCopyBarriers.Size(), preCopyBarriers.Data() });
	}


	void FontGlyphs_RecordTransfers_PostCopyBarriers(
		DeviceDispatch const& device,
		vk::CommandBuffer cmdBuffer,
		Std::Span<BoxVmaImg const> images,
		TransientAllocRef transientAlloc)
	{
		int jobCount = (int)images.Size();

		auto preCopyBarriers = Std::NewVec<vk::ImageMemoryBarrier>(transientAlloc);
		preCopyBarriers.Reserve(jobCount);
		for (auto const& img : images)
			preCopyBarriers.PushBack(Helper::CreateSampledImgBarrier_PostCopy(img.Handle()));
		device.cmdPipelineBarrier(
			cmdBuffer,
			vk::PipelineStageFlagBits::eTransfer,
			vk::PipelineStageFlagBits::eFragmentShader,
			vk::DependencyFlagBits(),
			{},
			{},
			{ (u32)preCopyBarriers.Size(), preCopyBarriers.Data() });
	}

	auto FontGlyphs_RecordTransfers(
		 DeviceDispatch const& device,
		 vk::CommandBuffer cmdBuffer,
		 vk::Buffer stagingBuffer,
		 int stagingBufferOffset,
		 Std::Span<GuiResourceManager::NewGlyphJob const> glyphJobs,
		 Std::Span<BoxVmaImg const> images,
		 TransientAllocRef transientAlloc)
	{
		DENGINE_IMPL_GFX_ASSERT(glyphJobs.Size() == images.Size());

		int jobCount = (int)glyphJobs.Size();

		FontGlyphs_RecordTransfers_PreCopyBarriers(
			device,
			cmdBuffer,
			images,
			transientAlloc);
		// Then we make the actual transfers
		for (int i = 0; i < jobCount; i++) {
			auto const& job = glyphJobs[i];
			auto buffImgCopy = FontGlyphs_CreateBufferImageCopy(
				job.imgWidth,
				job.imgHeight,
				stagingBufferOffset + job.dataOffset);
			device.cmdCopyBufferToImage(
				cmdBuffer,
				stagingBuffer,
				images[i].Handle(),
				vk::ImageLayout::eTransferDstOptimal,
				buffImgCopy);
		}

		FontGlyphs_RecordTransfers_PostCopyBarriers(
			device,
			cmdBuffer,
			images,
			transientAlloc);
	}

	[[nodiscard]] std::string CreateFontGlyphOjectName(
		FontFaceId fontFaceId,
		u32 utfValue)
	{
		std::string name = "GuiResourceManager, ";
		name += "FontFace " + std::to_string((int)fontFaceId) + ", ";
		name += "Glyph " + std::to_string(utfValue);
		return name;
	}

	[[nodiscard]] auto FontGlyphs_CreateVkImages(
		DeviceDispatch const& device,
		VmaAllocator vma,
		Std::Span<GuiResourceManager::NewGlyphJob const> glyphJobs,
		TransientAllocRef transientAlloc,
		DebugUtilsDispatch const* debugUtils)
	{
		auto jobCount = glyphJobs.Size();

		// Create an array of all our images we want to insert,
		// And alloc space for each of these images.
		auto imgVec = Std::NewVec_Reserve<BoxVmaImg>(transientAlloc, (int)jobCount);
		for (int i = 0; i < jobCount; i++) {
			auto const& job = glyphJobs[i];

			imgVec.PushBack(Helper::AllocSampledFontGlyphImage(device, vma, job.imgWidth, job.imgHeight));
			auto& img = imgVec[i];

			if (debugUtils) {
				auto name = CreateFontGlyphOjectName(job.fontFaceId, job.utfValue);
				name += " - VkImage";
				debugUtils->Helper_SetObjectName(device.handle, img.Handle(), name.c_str());
			}
		}

		return imgVec;
	}

	[[nodiscard]] vk::ImageView CreateFontGlyphImgView(
		DeviceDispatch const& device,
		vk::Image imgHandle)
	{
		vk::ImageViewCreateInfo imgViewInfo {};
		imgViewInfo.format = vk::Format::eR8Unorm;
		imgViewInfo.image = imgHandle;
		imgViewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		imgViewInfo.subresourceRange.layerCount = 1;
		imgViewInfo.subresourceRange.levelCount = 1;
		imgViewInfo.viewType = vk::ImageViewType::e2D;
		return device.createImageView(imgViewInfo);
	}

	[[nodiscard]] auto FontGlyphs_CreateImgViews(
		DeviceDispatch const& device,
		Std::Span<GuiResourceManager::NewGlyphJob const> glyphJobs,
		Std::Span<BoxVmaImg const> images,
		TransientAllocRef transientAlloc,
		DebugUtilsDispatch const* debugUtils)
	{
		auto jobCount = glyphJobs.Size();

		auto imgViewVec = Std::NewVec<vk::ImageView>(transientAlloc);
		imgViewVec.Resize(jobCount);
		for (int i = 0; i < jobCount; i++) {
			auto& imgView = imgViewVec[i];
			imgView = CreateFontGlyphImgView(device, images[i].Handle());

			if (debugUtils) {
				auto const& job = glyphJobs[i];
				auto name = CreateFontGlyphOjectName(job.fontFaceId, job.utfValue);
				name += " - VkImageView";
				debugUtils->Helper_SetObjectName(device.handle, imgView, name.c_str());
			}
		}

		return imgViewVec;
	}

	[[nodiscard]] auto FontGlyphs_AllocDescrSets(
		DeviceDispatch const& device,
		vk::DescriptorPool descrPool,
		vk::DescriptorSetLayout setLayout,
		uSize count,
		TransientAllocRef transientAlloc,
		Std::Span<GuiResourceManager::NewGlyphJob const> glyphJobs,
		DebugUtilsDispatch const* debugUtils)
	{
		DENGINE_IMPL_GFX_ASSERT(descrPool != vk::DescriptorPool{});
		DENGINE_IMPL_GFX_ASSERT(setLayout != vk::DescriptorSetLayout{});

		// Create the descriptor sets.
		auto setLayouts = Std::NewVec<vk::DescriptorSetLayout>(transientAlloc);
		setLayouts.Resize(count);
		for (auto& item : setLayouts)
			item = setLayout;

		auto descrSets = Std::NewVec<vk::DescriptorSet>(transientAlloc);
		descrSets.Resize(count);

		vk::DescriptorSetAllocateInfo descrSetAllocInfo = {};
		descrSetAllocInfo.descriptorPool = descrPool;
		descrSetAllocInfo.descriptorSetCount = count;
		descrSetAllocInfo.pSetLayouts = setLayouts.Data();
		auto result = device.Alloc(descrSetAllocInfo, descrSets.Data());
		if (result != vk::Result::eSuccess)
			throw std::runtime_error("Unable to allocate descriptor set memory.");

		if (debugUtils) {
			for (int i = 0; i < count; i++) {
				auto const& job = glyphJobs[i];
				auto name = CreateFontGlyphOjectName(job.fontFaceId, job.utfValue);
				name += " - DescrSet";
				debugUtils->Helper_SetObjectName(device.handle, descrSets[i], name.c_str());
			}
		}

		return descrSets;
	}

	void FontGlyphs_WriteDescriptorSets(
		DeviceDispatch const& device,
		vk::Sampler fontSampler,
		Std::Span<vk::DescriptorSet const> descrSets,
		Std::Span<vk::ImageView const> imgViews,
		TransientAllocRef transientAlloc)
	{
		DENGINE_IMPL_GFX_ASSERT(fontSampler != vk::Sampler{});

		int count = (int)descrSets.Size();

		auto descrImgInfos = Std::NewVec<vk::DescriptorImageInfo>(transientAlloc);
		descrImgInfos.Resize(count);
		for (int i = 0; i < count; i++) {
			auto& descrImgInfo = descrImgInfos[i];
			descrImgInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
			descrImgInfo.imageView = imgViews[i];
			descrImgInfo.sampler = fontSampler;
		}

		auto descrWrites = Std::NewVec<vk::WriteDescriptorSet>(transientAlloc);
		descrWrites.Resize(count);
		for (int i = 0; i < count; i++) {
			auto& descrWrite = descrWrites[i];
			descrWrite.descriptorCount = 1;
			descrWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
			descrWrite.dstBinding = 0;
			descrWrite.dstSet = descrSets[i];
			descrWrite.pImageInfo = &descrImgInfos[i];
		}

		device.UpdateDescriptorSets(descrWrites.ToSpan(), {});
	}


	struct Fonts_FlushJobs_Params {
		DeviceDispatch const& device;
		StagingBufferAlloc& stagingBufferAlloc;
		VmaAllocator vma;
		vk::CommandBuffer cmdBuffer;
		TransientAllocRef transientAlloc;
		DebugUtilsDispatch const* debugUtils;
	};
	void FlushJobs(
		GuiResourceManager& guiResMgr,
		Fonts_FlushJobs_Params const& params)
	{
		auto& device = params.device;
		auto& stagingBufferAlloc = params.stagingBufferAlloc;
		auto& vma = params.vma;
		auto& transientAlloc = params.transientAlloc;
		auto& cmdBuffer = params.cmdBuffer;
		auto* debugUtils = params.debugUtils;

		std::lock_guard queueLock { guiResMgr.jobQueueLock };
		// Flush the new font face creation jobs
		{
			auto& fontFaceJobs = guiResMgr.newFontFaceJobs;
			for (auto const& item : fontFaceJobs) {
				GuiResourceManager::FontFaceNode newNode {};
				newNode.id = item.id;
				guiResMgr.fontFaceNodes.push_back(Std::Move(newNode));
			}
			fontFaceJobs.clear();
		}

		auto& glyphJobs = guiResMgr.newGlyphJobs;
		auto& glyphBitmapData = guiResMgr.queuedGlyphBitmapData;

		// Defer the cleanup to the end of this function.
		Std::Defer clearQueueRoutine { [&] {
			guiResMgr.queuedGlyphBitmapData.clear();
			guiResMgr.newGlyphJobs.clear();
		} };

		int jobCount = (int)glyphJobs.size();
		if (jobCount == 0)
			return;
		Std::ConstByteSpan allBitmapData = {
			glyphBitmapData.data(),
			glyphBitmapData.size() };

		// Allocate the staging buffer and copy our glyph-data over. This contains all the bitmap
		// data. We will then transfer the sections of individual bitmaps into the destination images
		// in GPU memory.
		const auto stagingBuffer = stagingBufferAlloc.SubAlloc(
			device,
			allBitmapData);

		// Create an array of all our images we want to insert,
		// And alloc space for each of these images.
		auto imgVec = FontGlyphs_CreateVkImages(
			device,
			vma,
			{ glyphJobs.data(), glyphJobs.size() },
			transientAlloc,
			debugUtils);

		auto imgViewVec = FontGlyphs_CreateImgViews(
			device,
			{ glyphJobs.data(), glyphJobs.size() },
			imgVec.ToSpan(),
			transientAlloc,
			debugUtils);

		FontGlyphs_RecordTransfers(
			device,
			cmdBuffer,
			stagingBuffer.buffer,
			stagingBuffer.bufferOffset,
			{ guiResMgr.newGlyphJobs.data(), guiResMgr.newGlyphJobs.size() },
			imgVec.ToSpan(),
			transientAlloc);

		// Create the descriptor sets.
		auto descrSets = FontGlyphs_AllocDescrSets(
			device,
			guiResMgr.m_font_descrPool.Handle(),
			guiResMgr.m_font_descrSetLayout.Handle(),
			jobCount,
			transientAlloc,
			{ guiResMgr.newGlyphJobs.data(), guiResMgr.newGlyphJobs.size() },
			debugUtils);

		FontGlyphs_WriteDescriptorSets(
			device,
			guiResMgr.m_font_sampler.Handle(),
			descrSets.ToSpan(),
			imgViewVec.ToSpan(),
			transientAlloc);

		// Now create descriptor sets and insert all the stuff into our containers.
		for (int i = 0; i < jobCount; i++) {
			auto const& job = guiResMgr.newGlyphJobs[i];

			auto fontFaceIt = Std::FindIf(
				guiResMgr.fontFaceNodes.begin(),
				guiResMgr.fontFaceNodes.end(),
				[&job](auto const& item) { return item.id == job.fontFaceId; });
			DENGINE_IMPL_GFX_ASSERT(fontFaceIt != guiResMgr.fontFaceNodes.end());
			auto& fontFace = fontFaceIt->face;

			GuiResourceManager::GlyphData glyphData {};
			auto releasedImg = imgVec[i].Release();
			glyphData.img = releasedImg.handle;
			glyphData.imgAlloc = releasedImg.alloc;
			glyphData.imgView = imgViewVec[i];
			glyphData.descrSet = descrSets[i];

			// Then insert it into the font-face
			if (job.utfValue < fontFace.lowUtfGlyphDatas.Size()) {
				fontFace.lowUtfGlyphDatas[job.utfValue] = glyphData;
			} else {
				DENGINE_IMPL_UNREACHABLE();
			}
		}
	}
}

void Vk::GuiResourceManager::PreDraw(
	GuiResourceManager& manager,
	GlobUtils const& globUtils,
	Std::Span<GuiVertex const> guiVertices,
	Std::Span<u32 const> guiIndices,
	vk::CommandBuffer cmdBuffer,
	DeletionQueue& delQueue,
	TransientAllocRef transientAlloc,
	u8 inFlightIndex)
{
	DENGINE_IMPL_GFX_ASSERT(inFlightIndex < globUtils.inFlightCount);
	DENGINE_IMPL_GFX_ASSERT(manager.vtxMappedMem.Size() % globUtils.inFlightCount == 0);

	// We upload our new GUI vertices
	uSize srcVtxDataSize = guiVertices.Size() * sizeof(decltype(guiVertices)::ValueType);
	DENGINE_IMPL_GFX_ASSERT(srcVtxDataSize <= manager.vtxInFlightCapacity);
	std::memcpy(
		manager.vtxMappedMem.Data() + manager.vtxInFlightCapacity * inFlightIndex,
		guiVertices.Data(),
		srcVtxDataSize);

	// We upload the indeces for our new GUI vertices.
	DENGINE_IMPL_GFX_ASSERT(manager.indexMappedMem.Size() % globUtils.inFlightCount == 0);
	uSize srcIndexDataSize = guiIndices.Size() * sizeof(decltype(guiIndices)::ValueType);
	DENGINE_IMPL_GFX_ASSERT(srcIndexDataSize <= manager.indexInFlightCapacity);
	std::memcpy(
		manager.indexMappedMem.Data() + manager.indexInFlightCapacity * inFlightIndex,
		guiIndices.Data(),
		srcIndexDataSize);

	// TODO: This needs a barrier.
	
}

void Vk::GuiResourceManager::NewFontTextures(
	GuiResourceManager& manager,
	Std::Span<FontBitmapUploadJob const> const& jobs)
{
	std::lock_guard queueLock { manager.jobQueueLock };

	for (auto const& job : jobs) {
		// Append the data
		auto const oldLength = (int)manager.queuedGlyphBitmapData.size();
		auto const newLength = oldLength + job.data.Size();
		manager.queuedGlyphBitmapData.resize((uSize)newLength);
		memcpy(
			manager.queuedGlyphBitmapData.data() + oldLength,
			job.data.Data(),
			job.data.Size());

		NewGlyphJob newJob {};
		newJob.fontFaceId = job.fontFaceId;
		newJob.utfValue = job.utfValue;
		newJob.imgWidth = (int)job.width;
		newJob.imgHeight = (int)job.height;
		newJob.dataOffset = oldLength;
		newJob.dataLength = (int)job.data.Size();
		manager.newGlyphJobs.push_back(newJob);
	}
}

GuiResourceManager::GlyphData GuiResourceManager::GetGlyphData(
	GuiResourceManager const& mgr,
	FontFaceId fontFace,
	u32 utfValue)
{
	// First find the font face with this node.
	auto const nodeIt = Std::FindIf(
		mgr.fontFaceNodes.begin(),
		mgr.fontFaceNodes.end(),
		[fontFace](auto const& item) { return item.id == fontFace; });
	DENGINE_IMPL_GFX_ASSERT(nodeIt != mgr.fontFaceNodes.end());
	auto const& face = nodeIt->face;

	if (utfValue < face.lowUtfGlyphDatas.Size()) {
		return face.lowUtfGlyphDatas[utfValue];
	} else {
		auto it = face.glyphDatas.find(utfValue);
		DENGINE_IMPL_GFX_ASSERT(it != face.glyphDatas.end());
		return it->second;
	}
}

void GuiResourceManager::NewFontFace(
	GuiResourceManager& manager,
	FontFaceId id)
{
	std::lock_guard queueLock { manager.jobQueueLock };

	NewFontFaceJob newJob {};
	newJob.id = id;
	manager.newFontFaceJobs.push_back(newJob);
}

void GuiResourceManager::PerformGuiDrawCmd_Text(
	GuiResourceManager const& manager,
	DeviceDispatch const& device,
	vk::CommandBuffer cmdBuffer,
	Std::Span<vk::DescriptorSet const> descrSetsIn,
	Std::Span<u32 const> descrDynamicOffsets,
	GuiDrawCmd::Text const& drawCmd,
	Std::Span<u32 const> utfValuesAll,
	Std::Span<GlyphRect const> glyphRectsAll,
	Math::Vec2Int posPx)
{
	DENGINE_IMPL_GFX_ASSERT(cmdBuffer != vk::CommandBuffer{});
	DENGINE_IMPL_GFX_ASSERT(Util::CheckNotNull(descrSetsIn));

	auto utfValues = utfValuesAll.Subspan(drawCmd.startIndex, drawCmd.count);
	auto glyphRects = glyphRectsAll.Subspan(drawCmd.startIndex, drawCmd.count);

	auto fontFaceIt = Std::FindIf(
		manager.fontFaceNodes.begin(),
		manager.fontFaceNodes.end(),
		[&](auto const& item) { return item.id == drawCmd.fontFaceId; });
	DENGINE_IMPL_ASSERT(fontFaceIt != manager.fontFaceNodes.end());
	auto const& fontFace = fontFaceIt->face;

	device.cmdBindPipeline(
		cmdBuffer,
		vk::PipelineBindPoint::eGraphics,
		manager.m_font_pipeline.Handle());

	for (int i = 0; i < (int)drawCmd.count; i++) {
		auto const& glyphRect = glyphRects[i];
		if (glyphRect.extentPx == Math::Vec2Int::Zero())
			continue;
		GuiResourceManager::FontPushConstant pushConstant {};
		pushConstant.color = drawCmd.color;
		pushConstant.rectOffsetPx = posPx + glyphRect.posPx;
		pushConstant.rectExtentPx = glyphRect.extentPx;
		device.cmdPushConstants(
			cmdBuffer,
			manager.m_font_pipelineLayout.Handle(),
			vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
			0,
			32,
			&pushConstant);

		// Get glyph-data
		auto utfValue = utfValues[i];

		GuiResourceManager::GlyphData const* glyphData = nullptr;
		if (utfValue < fontFace.lowUtfGlyphDatas.Size()) {
			glyphData = &fontFace.lowUtfGlyphDatas[utfValue];
		} else {
			DENGINE_IMPL_GFX_UNREACHABLE();
		}

		DENGINE_IMPL_GFX_ASSERT(Util::CheckNotNull(glyphData->img));

		Std::StackVec<vk::DescriptorSet, 5> descrSets = { descrSetsIn };
		descrSets.PushBack(glyphData->descrSet);
		device.cmdBindDescriptorSets(
			cmdBuffer,
			vk::PipelineBindPoint::eGraphics,
			manager.m_font_pipelineLayout.Handle(),
			0,
			descrSets.ToSpan(),
			descrDynamicOffsets);
		device.cmdDraw(
			cmdBuffer,
			6,
			1,
			0,
			0);
	}
}

void GuiResourceManager::RenderRectangle(
	GuiResourceManager const& manager,
	DeviceDispatch const& device,
	Std::Span<vk::DescriptorSet const> descrSets,
	Std::Span<u32 const> descrDynamicOffsets,
	vk::CommandBuffer cmdBuffer,
	GuiDrawCmd::Rectangle const& drawCmd,
	Math::Vec2Int posPx,
	Math::Vec2Int extentPx)
{
	DENGINE_IMPL_GFX_ASSERT(Util::CheckNotNull(cmdBuffer));
	DENGINE_IMPL_GFX_ASSERT(Util::CheckNotNull(descrSets));
	DENGINE_IMPL_GFX_ASSERT(descrSets.Size() == 3);
	DENGINE_IMPL_GFX_ASSERT(descrDynamicOffsets.Size() == 1);

	device.cmdBindPipeline(
		cmdBuffer,
		vk::PipelineBindPoint::eGraphics,
		manager.m_rectanglePipeline.Handle());

	device.cmdBindDescriptorSets(
		cmdBuffer,
		vk::PipelineBindPoint::eGraphics,
		manager.m_rectanglePipelineLayout.Handle(),
		0,
		descrSets,
		descrDynamicOffsets);

	GuiResourceManager::RectanglePushConstant pushConst = {};
	pushConst.color = drawCmd.color;
	pushConst.rectExtentPx = extentPx;
	pushConst.rectOffsetPx = posPx;
	pushConst.radiusPx = drawCmd.radiusPx;

	device.cmdPushConstants(
		cmdBuffer,
		manager.m_rectanglePipelineLayout.Handle(),
		vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
		0,
		sizeof(pushConst),
		&pushConst);

	device.cmdDraw(
		cmdBuffer,
		6,
		1,
		0,
		0);
}

void GuiResourceManager::RenderRectangleShadow(
	GuiResourceManager const& manager,
	DeviceDispatch const& device,
	Std::Span<vk::DescriptorSet const> descrSets,
	Std::Span<u32 const> descrDynamicOffsets,
	vk::CommandBuffer cmdBuffer,
	GuiDrawCmd::RectangleShadow const& drawCmd,
	Math::Vec2Int posPx,
	Math::Vec2Int extentPx)
{
	device.cmdBindPipeline(
		cmdBuffer,
		vk::PipelineBindPoint::eGraphics,
		manager.m_rectangleShadowPipeline.Handle());

	device.cmdBindDescriptorSets(
		cmdBuffer,
		vk::PipelineBindPoint::eGraphics,
		manager.m_rectangleShadowPipelineLayout.Handle(),
		0,
		descrSets,
		descrDynamicOffsets);

	auto radiusPx = drawCmd.radiusPx;
	auto falloffPx = drawCmd.falloffPx;
	auto alpha = drawCmd.alpha;

	GuiResourceManager::RectangleShadowPushConstant pushConst = {};
	// Our input assumes that the position and extent refers to the source rectangle that will
	// cast the shadow. But the shader requires it to represent the entire rectangle that will be
	// used in the draw call. We need to expand the incoming data accordingly.
	// TODO: The shader should be able to accept these sizes in floats.
	pushConst.rectOffsetPx = posPx - Math::Vec2Int::SingleValue((i32)Math::Round(falloffPx));
	pushConst.rectExtentPx = extentPx + Math::Vec2Int::SingleValue((i32)Math::Round(falloffPx)) * 2;
	pushConst.radiusPx = radiusPx;
	pushConst.falloffPx = falloffPx;
	pushConst.alpha = alpha;

	device.cmdPushConstants(
		cmdBuffer,
		manager.m_rectangleShadowPipelineLayout.Handle(),
		vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
		0,
		sizeof(pushConst),
		&pushConst);

	device.cmdDraw(
		cmdBuffer,
		6,
		1,
		0,
		0);
}

void GuiResourceManager::RenderGradient(
	GuiResourceManager const& manager,
	DeviceDispatch const& device,
	Std::Span<vk::DescriptorSet const> descrSets,
	Std::Span<u32 const> descrDynamicOffsets,
	vk::CommandBuffer cmdBuffer,
	GuiDrawCmd::Gradient const& drawCmd,
	Math::Vec2Int posPx,
	Math::Vec2Int extentPx)
{
	device.cmdBindPipeline(
		cmdBuffer,
		vk::PipelineBindPoint::eGraphics,
		manager.m_gradientPipeline.Handle());

	device.cmdBindDescriptorSets(
		cmdBuffer,
		vk::PipelineBindPoint::eGraphics,
		manager.m_gradientPipelineLayout.Handle(),
		0,
		descrSets,
		descrDynamicOffsets);

	GuiResourceManager::GradientPushConstant pushConst = {};
	pushConst.rectOffsetPx = posPx;
	pushConst.rectExtentPx = extentPx;
	pushConst.colorA = drawCmd.colorA;
	pushConst.colorB = drawCmd.colorB;
	pushConst.dirAngle = drawCmd.dirAngle;

	device.cmdPushConstants(
		cmdBuffer,
		manager.m_gradientPipelineLayout.Handle(),
		vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
		0,
		sizeof(pushConst),
		&pushConst);

	device.cmdDraw(
		cmdBuffer,
		6,
		1,
		0,
		0);
}

void GuiResourceManager::RenderRectangleStencil(
	GuiResourceManager const& manager,
	DeviceDispatch const& device,
	Std::Span<vk::DescriptorSet const> descrSets,
	Std::Span<u32 const> descrDynamicOffsets,
	vk::CommandBuffer cmdBuffer,
	GuiDrawCmd::RectangleStencil const& drawCmd,
	u8& stencilRef,
	Math::Vec2Int posPx,
	Math::Vec2Int extentPx)
{
	bool const increment = drawCmd.increment;
	auto maskOp = drawCmd.op;

	// Update stencil ref
	if (maskOp == GuiDrawCmd::MaskOp::Outside) {
		if (increment) {
			DENGINE_IMPL_GFX_ASSERT(stencilRef < 255);
			stencilRef += 1;
		} else {
			DENGINE_IMPL_GFX_ASSERT(stencilRef > 0);
			stencilRef -= 1;
		}

		device.cmdSetStencilReference(
			cmdBuffer,
			vk::StencilFaceFlagBits::eFrontAndBack,
			stencilRef);
	}

	// If we are doing the inside render mask, we want to flip our stencil decrement/increment
	auto incrementTemp = increment;
	if (maskOp == GuiDrawCmd::MaskOp::Inside) {
		incrementTemp = !incrementTemp;
	}
	if (incrementTemp) {
		device.cmdBindPipeline(
			cmdBuffer,
			vk::PipelineBindPoint::eGraphics,
			manager.m_stencilRectanglePipelineIncrement.Handle());
	} else {
		device.cmdBindPipeline(
			cmdBuffer,
			vk::PipelineBindPoint::eGraphics,
			manager.m_stencilRectanglePipelineDecrement.Handle());
	}

	device.cmdBindDescriptorSets(
		cmdBuffer,
		vk::PipelineBindPoint::eGraphics,
		manager.m_stencilRectanglePipelineLayout.Handle(),
		0,
		descrSets,
		descrDynamicOffsets);

	GuiResourceManager::StencilRectanglePushConstant pushConst = {};
	pushConst.rectOffsetPx = posPx;
	pushConst.rectExtentPx = extentPx;
	pushConst.radiusPx = drawCmd.radiusPx;

	device.cmdPushConstants(
		cmdBuffer,
		manager.m_rectanglePipelineLayout.Handle(),
		vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
		0,
		sizeof(pushConst),
		&pushConst);

	device.cmdDraw(
		cmdBuffer,
		6,
		1,
		0,
		0);
}

void GuiResourceManager::PerformGuiDrawCmd_Scissor(
	GuiResourceManager const& manager,
	DeviceDispatch const& device,
	vk::CommandBuffer cmdBuffer,
	PerformGuiDrawCmd_Scissor_Params const& params)
{
	// Resolution of the window in its upright position. If the rotation is 90/270 degrees,
	// the incoming rect is based on the flipped resolution.
	auto const resolutionX = params.resolutionX;
	auto const resolutionY = params.resolutionY;
	auto const rectPosPx= params.rectPosPx;
	auto const rectExtentPx = params.rectExtentPx;
	auto const rotation = params.rotation;

	vk::Rect2D scissor = {};

	auto const swapExtent = (rotation == Gfx::WindowRotation::e90 || rotation == Gfx::WindowRotation::e270);
	auto const width = static_cast<u32>(swapExtent ? rectExtentPx.y : rectExtentPx.x);
	auto const height = static_cast<u32>(swapExtent ? rectExtentPx.x : rectExtentPx.y);
	switch (rotation) {
		case Gfx::WindowRotation::e0:
			scissor.offset = vk::Offset2D{rectPosPx.x,rectPosPx.y };
			break;
		case Gfx::WindowRotation::e90:
			scissor.offset = vk::Offset2D{rectPosPx.y,resolutionY - rectPosPx.x - rectExtentPx.x };
			break;
		case Gfx::WindowRotation::e180:
			scissor.offset = vk::Offset2D{resolutionX - rectPosPx.x - rectExtentPx.x, resolutionY - rectPosPx.y - rectExtentPx.y };
			break;
		case Gfx::WindowRotation::e270:
			scissor.offset = vk::Offset2D{resolutionX - rectExtentPx.y - rectPosPx.y,rectPosPx.x };
			break;
		default: DENGINE_IMPL_GFX_UNREACHABLE();
	}

	scissor.extent = vk::Extent2D{ width, height };


	// TODO: Architecture issues cause this to be problematic when doing smooth resizing,
	// specifically when making the window smaller.
	// The GUI will grab window-size from the main thread, and then hand off this
	// task to the rendering thread, which will grab the window-size from Vulkan, which
	// during smooth resizing will then be smaller.
	//DENGINE_IMPL_GFX_ASSERT(scissor.offset.x >= 0 && scissor.offset.x + scissor.extent.width  <= resolutionX);
	//DENGINE_IMPL_GFX_ASSERT(scissor.offset.y >= 0 && scissor.offset.y + scissor.extent.height <= resolutionY);

	device.cmdSetScissor(cmdBuffer, 0, scissor);
}

void GuiResourceManager::UpdateWindowUniforms(
	GuiResourceManager& guiResMgr,
	UpdateWindowUniforms_Params const& params)
{
	auto const& device = params.device;
	auto& stagingBufferAlloc = params.stagingBufferAlloc;
	auto& transientAlloc = params.transientAlloc;
	auto& vma = params.vma;
	const auto* debugUtils = params.debugUtils;
	auto const& windowRange = params.windowRangeRef;
	auto cmdBuffer = params.stagingCmdBuffer;
	auto windowCount = windowRange.Size();
	auto inFlightIndex = params.inFlightIndex;

	auto& windowUniforms = guiResMgr.windowUniforms;

	// Then we flush any jobs for the glyphs
	Fonts_FlushJobs_Params fontsFlushJobsParams = {
		.device = device,
		.stagingBufferAlloc = stagingBufferAlloc,
		.vma = vma,
		.cmdBuffer = cmdBuffer,
		.transientAlloc = transientAlloc,
		.debugUtils = debugUtils,
	};
	FlushJobs(guiResMgr, fontsFlushJobsParams);

	// Make sure we can fit all our new window-uniforms in our current buffer.
	// TODO: We gotta re-allocate if we need more.

	DENGINE_IMPL_GFX_ASSERT(windowCount <= guiResMgr.WindowUniformCapacity());
	auto dstBufferSpan = guiResMgr.WindowUniformsInFlightBufferSpan(inFlightIndex);

	// Clear all the window-ids from previous frame.
	windowUniforms.windowIds.clear();
	auto windowUniformDataAlignment = guiResMgr.WindowUniformElementAlignment();

	// Go through each element in the buffer and update it accordingly.
	// Also assign the window-id.
	for (int i = 0; i < windowCount; i++) {
		auto temp = windowRange.Invoke(i);

		windowUniforms.windowIds.push_back(temp.windowId);

		auto byteOffset = windowUniformDataAlignment * i;

		// Write the stuff to this buffer
		WindowShaderUniforms::PerWindowUniform uniform = {};
		uniform.orientation = (int)temp.orientation;
		uniform.resolution = { temp.resolutionWidth, temp.resolutionHeight };

		std::memcpy(
			dstBufferSpan.Data() + byteOffset,
			&uniform,
			sizeof(uniform));
	}

	if (windowCount > 0) {
		windowUniforms.buffer.Flush(0, windowUniformDataAlignment * windowCount);
	}
}

vk::DescriptorSet GuiResourceManager::GetPerWindowDescrSet(
	GuiResourceManager const& manager,
	NativeWindowID windowIdIn,
	int inFlightIndex)
{
	auto const& uniforms = manager.windowUniforms;

	vk::DescriptorSet out = {};
	for (int i = 0; i < (int)uniforms.windowIds.size(); i++) {
		auto testWindowId = uniforms.windowIds[i];
		if (testWindowId == windowIdIn) {
			auto offset = (uniforms.windowUniformDescrSets.size() / manager.m_inFlightCount) * inFlightIndex;
			out = uniforms.windowUniformDescrSets[i + offset];
			break;
		}
	}

	DENGINE_IMPL_GFX_ASSERT(Util::CheckNotNull(out));
	return out;
}

void GuiResourceManager::PerformGuiDrawCmd_Viewport(
	GuiResourceManager const& manager,
	DeviceDispatch const& device,
	vk::CommandBuffer cmdBuffer,
	Std::Span<vk::DescriptorSet const> descrSetsIn,
	Std::Span<u32 const> descrDynamicOffsets,
	vk::DescriptorSet viewportDescr,
	Gfx::WindowRotation rotation,
	Math::Vec2Int rectOffsetPx,
	Math::Vec2Int rectExtentPx)
{
	device.cmdBindPipeline(
		cmdBuffer,
		vk::PipelineBindPoint::eGraphics,
		manager.m_viewportPipeline.Handle());

	Std::StackVec<vk::DescriptorSet, 5> descrSets = { descrSetsIn };
	descrSets.PushBack(viewportDescr);

	device.cmdBindDescriptorSets(
		cmdBuffer,
		vk::PipelineBindPoint::eGraphics,
		manager.m_viewportPipelineLayout.Handle(),
		0,
		descrSets.ToSpan(),
		descrDynamicOffsets);


	GuiResourceManager::ViewportPushConstant pushConstant = {};
	pushConstant.rectExtentPx = rectExtentPx;
	pushConstant.rectOffsetPx = rectOffsetPx;
	device.cmdPushConstants(
		cmdBuffer,
		manager.m_viewportPipelineLayout.Handle(),
		vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
		0,
		sizeof(pushConstant),
		&pushConstant);

	device.cmdDraw(
		cmdBuffer,
		6,
		1,
		0,
		0);
}
