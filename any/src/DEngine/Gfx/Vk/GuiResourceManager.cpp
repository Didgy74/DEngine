#include "GuiResourceManager.hpp"

#include <DEngine/Gfx/Gfx.hpp>
#include "DynamicDispatch.hpp"
#include "GlobUtils.hpp"
#include "RaiiHandles.hpp"
#include "DeletionQueue.hpp"
#include "StagingBufferAlloc.hpp"
#include <DEngine/Gfx/impl/Assert.hpp>

#include <DEngine/Std/Containers/Array.hpp>
#include <DEngine/Std/Containers/Defer.hpp>
#include <DEngine/Std/Containers/Vec.hpp>
#include <DEngine/Std/Containers/AllocRef.hpp>
#include <DEngine/Std/Utility.hpp>

// For file IO
#include <DEngine/Application.hpp>

using namespace DEngine;
using namespace DEngine::Gfx;
using namespace DEngine::Gfx::Vk;

namespace DEngine::Gfx::Vk::GuiResourceManagerImpl
{
	static Std::Array<vk::VertexInputAttributeDescription, 2> BuildShaderVertexInputAttrDescr()
	{
		vk::VertexInputAttributeDescription position{};
		position.binding = 0;
		position.format = vk::Format::eR32G32Sfloat;
		position.location = 0;
		position.offset = offsetof(GuiVertex, position);

		vk::VertexInputAttributeDescription uv{};
		uv.binding = 0;
		uv.format = vk::Format::eR32G32Sfloat;
		uv.location = 1;
		uv.offset = offsetof(GuiVertex, uv);

		return { position, uv };
	}

	static Std::Array<vk::VertexInputBindingDescription, 1> BuildShaderVertexInputBindingDescr()
	{
		vk::VertexInputBindingDescription binding{};
		binding.binding = 0;
		binding.inputRate = vk::VertexInputRate::eVertex;
		binding.stride = sizeof(GuiVertex);

		return { binding };
	}

	static void CreateRectangleShader(
		GuiResourceManager& manager,
		DeviceDispatch const& device,
		vk::DescriptorSetLayout descrSetLayout,
		vk::RenderPass guiRenderPass,
		Std::AllocRef const& transientAlloc,
		DebugUtilsDispatch const* debugUtils)
	{
		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};

		vk::PushConstantRange vertPushConstantRange{};
		vertPushConstantRange.stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment;
		vertPushConstantRange.size = GuiResourceManager::RectanglePushConstant::sizeInBytes;
		Std::Array<vk::PushConstantRange, 1> pushConstantRanges { vertPushConstantRange};
		pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.Data();
		pipelineLayoutInfo.pushConstantRangeCount = (u32)pushConstantRanges.Size();
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &descrSetLayout;

		auto pipelineLayout = device.Create(pipelineLayoutInfo);
		if (debugUtils) {
			debugUtils->Helper_SetObjectName(
				device.handle,
				pipelineLayout,
				"GuiResourceManager - Rectangle PipelineLayout");
		}

		vk::DynamicState dynamicStates[2] = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };
		vk::PipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.dynamicStateCount = 2;
		dynamicState.pDynamicStates = dynamicStates;
		vk::PipelineInputAssemblyStateCreateInfo inputAssemblyState{};
		inputAssemblyState.topology = vk::PrimitiveTopology::eTriangleList;
		vk::PipelineMultisampleStateCreateInfo multiSampleState{};
		multiSampleState.rasterizationSamples = vk::SampleCountFlagBits::e1;
		vk::PipelineRasterizationStateCreateInfo rasterizationState{};
		rasterizationState.lineWidth = 1.f;
		rasterizationState.polygonMode = vk::PolygonMode::eFill;
		rasterizationState.frontFace = vk::FrontFace::eClockwise;
		rasterizationState.cullMode = vk::CullModeFlagBits::eNone;
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
		vk::PipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.colorWriteMask =
			vk::ColorComponentFlagBits::eR |
			vk::ColorComponentFlagBits::eG |
			vk::ColorComponentFlagBits::eB |
			vk::ColorComponentFlagBits::eA;
		colorBlendAttachment.blendEnable = true;
		colorBlendAttachment.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
		colorBlendAttachment.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
		colorBlendAttachment.colorBlendOp = vk::BlendOp::eAdd;
		colorBlendAttachment.srcAlphaBlendFactor = vk::BlendFactor::eSrcAlpha;
		colorBlendAttachment.dstAlphaBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
		colorBlendAttachment.alphaBlendOp = vk::BlendOp::eAdd;


		vk::PipelineColorBlendStateCreateInfo colorBlendState{};
		colorBlendState.attachmentCount = 1;
		colorBlendState.pAttachments = &colorBlendAttachment;
		colorBlendState.blendConstants[0] = 0.0f;
		colorBlendState.blendConstants[1] = 0.0f;
		colorBlendState.blendConstants[2] = 0.0f;
		colorBlendState.blendConstants[3] = 0.0f;


		App::FileInputStream vertFile{ "data/gui/Rectangle/vert.spv" };
		if (!vertFile.IsOpen())
			throw std::runtime_error("Could not open vertex shader file");
		vertFile.Seek(0, App::FileInputStream::SeekOrigin::End);
		u64 vertFileLength = vertFile.Tell().Value();
		vertFile.Seek(0, App::FileInputStream::SeekOrigin::Start);
		auto vertCode = Std::NewVec<char>(transientAlloc);
		vertCode.Resize((uSize)vertFileLength);
		vertFile.Read(vertCode.Data(), vertFileLength);
		vertFile.Close();
		vk::ShaderModuleCreateInfo vertModCreateInfo{};
		vertModCreateInfo.codeSize = vertCode.Size();
		vertModCreateInfo.pCode = reinterpret_cast<const u32*>(vertCode.Data());
		vk::ShaderModule vertModule = device.createShaderModule(vertModCreateInfo);
		vk::PipelineShaderStageCreateInfo vertStageInfo{};
		vertStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
		vertStageInfo.module = vertModule;
		vertStageInfo.pName = "main";


		App::FileInputStream fragFile{ "data/gui/Rectangle/frag.spv" };
		if (!fragFile.IsOpen())
			throw std::runtime_error("Could not open fragment shader file");
		fragFile.Seek(0, App::FileInputStream::SeekOrigin::End);
		u64 fragFileLength = fragFile.Tell().Value();
		fragFile.Seek(0, App::FileInputStream::SeekOrigin::Start);
		auto fragCode = Std::NewVec<char>(transientAlloc);
		fragCode.Resize((uSize)fragFileLength);
		fragFile.Read(fragCode.Data(), fragFileLength);
		fragFile.Close();
		vk::ShaderModuleCreateInfo fragModInfo{};
		fragModInfo.codeSize = fragCode.Size();
		fragModInfo.pCode = reinterpret_cast<u32 const*>(fragCode.Data());
		vk::ShaderModule fragModule = device.createShaderModule(fragModInfo);
		vk::PipelineShaderStageCreateInfo fragStageInfo{};
		fragStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
		fragStageInfo.module = fragModule;
		fragStageInfo.pName = "main";

		Std::Array<vk::PipelineShaderStageCreateInfo, 2> shaderStages = { vertStageInfo, fragStageInfo };

		vk::PipelineDepthStencilStateCreateInfo depthStencilInfo{};
		depthStencilInfo.depthTestEnable = 0;
		depthStencilInfo.depthCompareOp = vk::CompareOp::eLess;
		depthStencilInfo.stencilTestEnable = 0;
		depthStencilInfo.depthWriteEnable = 0;
		depthStencilInfo.minDepthBounds = 0.f;
		depthStencilInfo.maxDepthBounds = 1.f;

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

		vk::Pipeline pipeline = {};
		vk::Result vkResult = device.Create(pipelineInfo, &pipeline);
		if (vkResult != vk::Result::eSuccess)
			throw std::runtime_error("DEngine - Vulkan: Unable to create GUI shader.");
		if (debugUtils != nullptr) {
			debugUtils->Helper_SetObjectName(
				device.handle,
				pipeline,
				"GuiResourceManager - Rectangle Pipeline");
		}

		device.Destroy(vertModule);
		device.Destroy(fragModule);

		manager.rectanglePipelineLayout = pipelineLayout;
		manager.rectanglePipeline = pipeline;
	}

	static void CreateFilledMeshShader(
		GuiResourceManager& manager,
		DeviceDispatch const& device,
		vk::DescriptorSetLayout descrSetLayout,
		vk::RenderPass guiRenderPass,
		Std::AllocRef const& transientAlloc,
		DebugUtilsDispatch const* debugUtils)
	{
		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
		vk::PushConstantRange vertPushConstantRange{};
		vertPushConstantRange.stageFlags = vk::ShaderStageFlagBits::eVertex;
		vertPushConstantRange.size = 32;
		vertPushConstantRange.offset = 0;
		vk::PushConstantRange fragPushConstantRange{};
		fragPushConstantRange.stageFlags = vk::ShaderStageFlagBits::eFragment;
		fragPushConstantRange.size = sizeof(GuiResourceManager::FilledMeshPushConstant::color);
		fragPushConstantRange.offset = vertPushConstantRange.size;

		Std::Array<vk::PushConstantRange, 2> pushConstantRanges{ vertPushConstantRange, fragPushConstantRange };
		pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.Data();
		pipelineLayoutInfo.pushConstantRangeCount = (u32)pushConstantRanges.Size();

		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &descrSetLayout;

		manager.filledMeshPipelineLayout = device.Create(pipelineLayoutInfo);
		if (debugUtils) {
			debugUtils->Helper_SetObjectName(
				device.handle,
				manager.filledMeshPipelineLayout,
				"GuiResourceManager - FilledMesh PipelineLayout");
		}

		vk::DynamicState dynamicStates[2] = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };
		vk::PipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.dynamicStateCount = 2;
		dynamicState.pDynamicStates = dynamicStates;
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
		vk::PipelineColorBlendStateCreateInfo colorBlendState{};
		colorBlendState.attachmentCount = 1;
		colorBlendState.pAttachments = &colorBlendAttachment;


		App::FileInputStream vertFile{ "data/gui/FilledMesh/vert.spv" };
		if (!vertFile.IsOpen())
			throw std::runtime_error("Could not open vertex shader file");
		vertFile.Seek(0, App::FileInputStream::SeekOrigin::End);
		u64 vertFileLength = vertFile.Tell().Value();
		vertFile.Seek(0, App::FileInputStream::SeekOrigin::Start);
		auto vertCode = Std::NewVec<char>(transientAlloc);
		vertCode.Resize((uSize)vertFileLength);
		vertFile.Read(vertCode.Data(), vertFileLength);
		vertFile.Close();
		vk::ShaderModuleCreateInfo vertModCreateInfo{};
		vertModCreateInfo.codeSize = vertCode.Size();
		vertModCreateInfo.pCode = reinterpret_cast<const u32*>(vertCode.Data());
		vk::ShaderModule vertModule = device.createShaderModule(vertModCreateInfo);
		vk::PipelineShaderStageCreateInfo vertStageInfo{};
		vertStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
		vertStageInfo.module = vertModule;
		vertStageInfo.pName = "main";


		App::FileInputStream fragFile{ "data/gui/FilledMesh/frag.spv" };
		if (!fragFile.IsOpen())
			throw std::runtime_error("Could not open fragment shader file");
		fragFile.Seek(0, App::FileInputStream::SeekOrigin::End);
		u64 fragFileLength = fragFile.Tell().Value();
		fragFile.Seek(0, App::FileInputStream::SeekOrigin::Start);
		auto fragCode = Std::NewVec<char>(transientAlloc);
		fragCode.Resize((uSize)fragFileLength);
		fragFile.Read(fragCode.Data(), fragFileLength);
		fragFile.Close();
		vk::ShaderModuleCreateInfo fragModInfo{};
		fragModInfo.codeSize = fragCode.Size();
		fragModInfo.pCode = reinterpret_cast<u32 const*>(fragCode.Data());
		vk::ShaderModule fragModule = device.createShaderModule(fragModInfo);
		vk::PipelineShaderStageCreateInfo fragStageInfo{};
		fragStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
		fragStageInfo.module = fragModule;
		fragStageInfo.pName = "main";

		Std::Array<vk::PipelineShaderStageCreateInfo, 2> shaderStages = { vertStageInfo, fragStageInfo };

        vk::PipelineDepthStencilStateCreateInfo depthStencilInfo{};
        depthStencilInfo.depthTestEnable = 0;
        depthStencilInfo.depthCompareOp = vk::CompareOp::eLess;
        depthStencilInfo.stencilTestEnable = 0;
        depthStencilInfo.depthWriteEnable = 0;
        depthStencilInfo.minDepthBounds = 0.f;
        depthStencilInfo.maxDepthBounds = 1.f;

		vk::GraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.layout = manager.filledMeshPipelineLayout;
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

		vk::Result vkResult = device.Create(pipelineInfo, &manager.filledMeshPipeline);
		if (vkResult != vk::Result::eSuccess)
			throw std::runtime_error("DEngine - Vulkan: Unable to create GUI shader.");

		if (debugUtils) {
			debugUtils->Helper_SetObjectName(
				device.handle,
				manager.filledMeshPipeline,
				"GuiResourceManager - FilledMesh Pipeline");
		}

		device.Destroy(vertModule);
		device.Destroy(fragModule);
	}

	static void CreateViewportShader(
		GuiResourceManager& manager,
		DeviceDispatch const& device,
		vk::DescriptorSetLayout perWindowDescrLayout,
		vk::DescriptorSetLayout viewportImgDescrLayout,
		vk::RenderPass guiRenderPass,
		Std::AllocRef const& transientAlloc,
		DebugUtilsDispatch const* debugUtils)
	{
		vk::DescriptorSetLayout descrLayouts[2] = { perWindowDescrLayout, viewportImgDescrLayout };

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo = {};
		pipelineLayoutInfo.setLayoutCount = 2;
		pipelineLayoutInfo.pSetLayouts = descrLayouts;

		vk::PushConstantRange vertPushConstantRange = {};
		vertPushConstantRange.stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment;
		vertPushConstantRange.size = sizeof(GuiResourceManager::ViewportPushConstant);
		vertPushConstantRange.offset = 0;
		pipelineLayoutInfo.pPushConstantRanges = &vertPushConstantRange;
		pipelineLayoutInfo.pushConstantRangeCount = 1;

		manager.viewportPipelineLayout = device.Create(pipelineLayoutInfo);
		if (debugUtils) {
			debugUtils->Helper_SetObjectName(
				device.handle,
				manager.viewportPipelineLayout,
				"GuiResourceManager - Viewport PipelineLayout");
		}

		vk::DynamicState dynamicStates[2] = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };
		vk::PipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.dynamicStateCount = 2;
		dynamicState.pDynamicStates = dynamicStates;
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
		vk::PipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.colorWriteMask |= vk::ColorComponentFlagBits::eR;
		colorBlendAttachment.colorWriteMask |= vk::ColorComponentFlagBits::eG;
		colorBlendAttachment.colorWriteMask |= vk::ColorComponentFlagBits::eB;
		colorBlendAttachment.colorWriteMask |= vk::ColorComponentFlagBits::eA;
		vk::PipelineColorBlendStateCreateInfo colorBlendState{};
		colorBlendState.attachmentCount = 1;
		colorBlendState.pAttachments = &colorBlendAttachment;

		App::FileInputStream vertFile{ "data/gui/Viewport/vert.spv" };
		if (!vertFile.IsOpen())
			throw std::runtime_error("Could not open vertex shader file");
		vertFile.Seek(0, App::FileInputStream::SeekOrigin::End);
		u64 vertFileLength = vertFile.Tell().Value();
		vertFile.Seek(0, App::FileInputStream::SeekOrigin::Start);
		auto vertCode = Std::NewVec<char>(transientAlloc);
		vertCode.Resize((uSize)vertFileLength);
		vertFile.Read(vertCode.Data(), vertFileLength);
		vertFile.Close();
		vk::ShaderModuleCreateInfo vertModCreateInfo{};
		vertModCreateInfo.codeSize = vertCode.Size();
		vertModCreateInfo.pCode = reinterpret_cast<const u32*>(vertCode.Data());
		vk::ShaderModule vertModule = device.createShaderModule(vertModCreateInfo);
		vk::PipelineShaderStageCreateInfo vertStageInfo{};
		vertStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
		vertStageInfo.module = vertModule;
		vertStageInfo.pName = "main";
		
		
		App::FileInputStream fragFile{ "data/gui/Viewport/frag.spv" };
		if (!fragFile.IsOpen())
			throw std::runtime_error("Could not open fragment shader file");
		fragFile.Seek(0, App::FileInputStream::SeekOrigin::End);
		u64 fragFileLength = fragFile.Tell().Value();
		fragFile.Seek(0, App::FileInputStream::SeekOrigin::Start);
		auto fragCode = Std::NewVec<char>(transientAlloc);
		fragCode.Resize((uSize)fragFileLength);
		fragFile.Read(fragCode.Data(), fragFileLength);
		fragFile.Close();
		vk::ShaderModuleCreateInfo fragModInfo{};
		fragModInfo.codeSize = fragCode.Size();
		fragModInfo.pCode = reinterpret_cast<u32 const*>(fragCode.Data());
		vk::ShaderModule fragModule = device.createShaderModule(fragModInfo);
		vk::PipelineShaderStageCreateInfo fragStageInfo{};
		fragStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
		fragStageInfo.module = fragModule;
		fragStageInfo.pName = "main";

		Std::Array<vk::PipelineShaderStageCreateInfo, 2> shaderStages = { vertStageInfo, fragStageInfo };

        vk::PipelineDepthStencilStateCreateInfo depthStencilInfo{};
        depthStencilInfo.depthTestEnable = 0;
        depthStencilInfo.depthCompareOp = vk::CompareOp::eLess;
        depthStencilInfo.stencilTestEnable = 0;
        depthStencilInfo.depthWriteEnable = 0;
        depthStencilInfo.minDepthBounds = 0.f;
        depthStencilInfo.maxDepthBounds = 1.f;

		vk::GraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.layout = manager.viewportPipelineLayout;
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

		vk::Result vkResult = device.Create(pipelineInfo, &manager.viewportPipeline);
		if (vkResult != vk::Result::eSuccess)
			throw std::runtime_error("DEngine - Vulkan: Unable to create GUI shader.");
		if (debugUtils)
		{
			debugUtils->Helper_SetObjectName(
				device.handle,
				manager.viewportPipeline,
				"GuiResourceManager - Viewport Pipeline");
		}

		device.Destroy(vertModule);
		device.Destroy(fragModule);
	}

	static void CreateTextShader(
		GuiResourceManager& manager,
		DeviceDispatch const& device,
		vk::RenderPass guiRenderPass,
		vk::PipelineLayout pipelineLayout,
		Std::AllocRef const& transientAlloc,
		DebugUtilsDispatch const* debugUtils)
	{
		vk::DynamicState dynamicStates[2] = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };
		vk::PipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.dynamicStateCount = 2;
		dynamicState.pDynamicStates = dynamicStates;
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
		vk::PipelineColorBlendStateCreateInfo colorBlendState{};
		colorBlendState.attachmentCount = 1;
		colorBlendState.pAttachments = &colorBlendAttachment;


		App::FileInputStream vertFile{ "data/gui/Text/vert.spv" };
		if (!vertFile.IsOpen())
			throw std::runtime_error("Could not open vertex shader file");
		vertFile.Seek(0, App::FileInputStream::SeekOrigin::End);
		u64 vertFileLength = vertFile.Tell().Value();
		vertFile.Seek(0, App::FileInputStream::SeekOrigin::Start);
		auto vertCode = Std::NewVec<char>(transientAlloc);
		vertCode.Resize((uSize)vertFileLength);
		vertFile.Read(vertCode.Data(), vertFileLength);
		vertFile.Close();
		vk::ShaderModuleCreateInfo vertModCreateInfo{};
		vertModCreateInfo.codeSize = vertCode.Size();
		vertModCreateInfo.pCode = reinterpret_cast<const u32*>(vertCode.Data());
		vk::ShaderModule vertModule = device.createShaderModule(vertModCreateInfo);
		vk::PipelineShaderStageCreateInfo vertStageInfo{};
		vertStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
		vertStageInfo.module = vertModule;
		vertStageInfo.pName = "main";


		App::FileInputStream fragFile{ "data/gui/Text/frag.spv" };
		if (!fragFile.IsOpen())
			throw std::runtime_error("Could not open fragment shader file");
		fragFile.Seek(0, App::FileInputStream::SeekOrigin::End);
		u64 fragFileLength = fragFile.Tell().Value();
		fragFile.Seek(0, App::FileInputStream::SeekOrigin::Start);
		auto fragCode = Std::NewVec<char>(transientAlloc);
		fragCode.Resize((uSize)fragFileLength);
		fragFile.Read(fragCode.Data(), fragFileLength);
		fragFile.Close();
		vk::ShaderModuleCreateInfo fragModInfo{};
		fragModInfo.codeSize = fragCode.Size();
		fragModInfo.pCode = reinterpret_cast<u32 const*>(fragCode.Data());
		vk::ShaderModule fragModule = device.createShaderModule(fragModInfo);
		vk::PipelineShaderStageCreateInfo fragStageInfo = {};
		fragStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
		fragStageInfo.module = fragModule;
		fragStageInfo.pName = "main";

		Std::Array<vk::PipelineShaderStageCreateInfo, 2> shaderStages = { vertStageInfo, fragStageInfo };

		vk::PipelineDepthStencilStateCreateInfo depthStencilInfo{};
		depthStencilInfo.depthTestEnable = 0;
		depthStencilInfo.depthCompareOp = vk::CompareOp::eLess;
		depthStencilInfo.stencilTestEnable = 0;
		depthStencilInfo.depthWriteEnable = 0;
		depthStencilInfo.minDepthBounds = 0.f;
		depthStencilInfo.maxDepthBounds = 1.f;

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

		vk::Pipeline pipeline = {};
		auto vkResult = device.Create(pipelineInfo, &pipeline);
		if (vkResult != vk::Result::eSuccess)
			throw std::runtime_error("DEngine - Vulkan: Unable to create GUI shader.");
		if (debugUtils) {
			debugUtils->Helper_SetObjectName(
				device.handle,
				pipeline,
				"GuiResourceManager - Text Pipeline");
		}

		manager.font_pipeline = pipeline;

		device.Destroy(vertModule);
		device.Destroy(fragModule);
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

	static void SetupGuiWindowUniforms(
		GuiResourceManager& manager,
		DeviceDispatch const& device,
		VmaAllocator vma,
		int inFlightCount,
		Std::AllocRef transientAlloc,
		DebugUtilsDispatch const* debugUtils)
	{
		vk::Result vkResult{};

		auto startCapacity = GuiResourceManager::WindowShaderUniforms::minimumCapacity;
		auto uniformElementSize = Math::CeilToMultiple(
			(unsigned int)GuiResourceManager::WindowShaderUniforms::PerWindowUniform::sizeInBytes,
			(unsigned int)device.PhysDeviceLimits().minUniformBufferOffsetAlignment);

		vk::BufferCreateInfo bufferInfo = {};
		bufferInfo.sharingMode = vk::SharingMode::eExclusive;
		bufferInfo.size = uniformElementSize * startCapacity * inFlightCount;
		bufferInfo.usage = vk::BufferUsageFlagBits::eUniformBuffer;
		VmaAllocationCreateInfo vmaAllocInfo{};
		vmaAllocInfo.flags = VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_MAPPED_BIT;
		vmaAllocInfo.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_CPU_TO_GPU;

		VmaAllocationInfo vmaAllocResultInfo = {};
		vk::Buffer buffer = {};
		VmaAllocation vmaAlloc = {};
		vkResult = (vk::Result)vmaCreateBuffer(
			vma,
			(VkBufferCreateInfo*)&bufferInfo,
			&vmaAllocInfo,
			(VkBuffer*)&buffer,
			&vmaAlloc,
			&vmaAllocResultInfo);
		if (vkResult != vk::Result::eSuccess)
			throw std::runtime_error("DEngine - Vulkan: VMA was unable to allocate memory for GUI global uniform.");
		if (debugUtils != nullptr) {
			debugUtils->Helper_SetObjectName(
				device.handle,
				buffer,
				"GuiResourceManager - PerWindowUniform - Buffer");
		}

		// Create descriptors and a descriptor set layout
		vk::DescriptorSetLayoutBinding binding = {};
		binding.binding = 0;
		binding.descriptorCount = 1;
		binding.descriptorType = vk::DescriptorType::eUniformBuffer;
		binding.stageFlags = vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eVertex;
		vk::DescriptorSetLayoutCreateInfo layoutInfo = {};
		layoutInfo.bindingCount = 1;
		layoutInfo.pBindings = &binding;
		auto descrSetLayout = device.Create(layoutInfo);
		if (debugUtils != nullptr) {
			debugUtils->Helper_SetObjectName(
				device.handle,
				descrSetLayout,
				"GuiResourceManager - PerWindowUniform - DescriptorSetLayout");
		}

		vk::DescriptorPoolSize descrPoolSize = {};
		descrPoolSize.descriptorCount = startCapacity * inFlightCount;
		descrPoolSize.type = vk::DescriptorType::eUniformBuffer;
		vk::DescriptorPoolCreateInfo descrPoolInfo{};
		descrPoolInfo.maxSets = startCapacity * inFlightCount;
		descrPoolInfo.poolSizeCount = 1;
		descrPoolInfo.pPoolSizes = &descrPoolSize;
		auto descrPool = device.Create(descrPoolInfo);
		if (debugUtils != nullptr) {
			debugUtils->Helper_SetObjectName(
				device.handle,
				descrPool,
				"GuiResourceManager - PerWindowUniform - DescriptorPool");
		}

		// Allocate all the descriptors and write to them.
		auto descrAllocResult = AllocateDescriptorSets(
			device,
			descrPool,
			descrSetLayout,
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
					// Create one per inflight-index
					std::string name = "GuiResourceManager - PerWindowUniform - DescrSet #";
					name += std::to_string(i);
					name += " - InFlight #" + std::to_string(inFlightIndex);
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
				descrBuffer.buffer = buffer;
				descrBuffer.offset = uniformElementSize * i;
				descrBuffer.range = uniformElementSize;

				descrWrite.dstSet = descrSets[i];
				descrWrite.dstBinding = 0;
				descrWrite.dstArrayElement = 0;
				descrWrite.descriptorCount = 1;
				descrWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
				descrWrite.pBufferInfo = &descrBuffer;
			}
			device.updateDescriptorSets({ (u32)descrWrites.Size(), descrWrites.Data()}, {});
		}


		manager.windowUniforms.currentCapacity = startCapacity;
		manager.windowUniforms.buffer = buffer;
		manager.windowUniforms.bufferElementSize = (int)uniformElementSize;
		manager.windowUniforms.vmaAlloc = vmaAlloc;
		manager.windowUniforms.vmaAllocResultInfo = vmaAllocResultInfo;
		manager.windowUniforms.mappedMem = { (char*)vmaAllocResultInfo.pMappedData, (uSize)bufferInfo.size };
		manager.windowUniforms.setLayout = descrSetLayout;
		manager.windowUniforms.descrPool = descrPool;
		// Copy all the descr sets
		for (auto const& item : descrSets)
			manager.windowUniforms.windowUniformDescrSets.push_back(item);
	}

	static void SetupTextResources(
		GuiResourceManager& guiResMgr,
		DeviceDispatch const& device,
		vk::DescriptorSetLayout windowDescrSetLayout,
		DebugUtilsDispatch const* debugUtils)
	{
		vk::DescriptorPoolSize sampledImgDescrPoolSize{};
		sampledImgDescrPoolSize.descriptorCount = 8196;
		sampledImgDescrPoolSize.type = vk::DescriptorType::eCombinedImageSampler;
		vk::DescriptorPoolCreateInfo descrPoolInfo{};
		descrPoolInfo.maxSets = 8196;
		descrPoolInfo.poolSizeCount = 1;
		descrPoolInfo.pPoolSizes = &sampledImgDescrPoolSize;
		auto descrPool = device.Create(descrPoolInfo);
		if (debugUtils) {
			debugUtils->Helper_SetObjectName(
				device.handle,
				descrPool,
				"GuiResourceManager - Text DescrPool");
		}

		vk::DescriptorSetLayoutBinding imgDescrBinding{};
		imgDescrBinding.binding = 0;
		imgDescrBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;
		imgDescrBinding.descriptorCount = 1;
		imgDescrBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		vk::DescriptorSetLayoutCreateInfo descrSetLayoutInfo{};
		descrSetLayoutInfo.bindingCount = 1;
		descrSetLayoutInfo.pBindings = &imgDescrBinding;
		auto descrSetLayout = device.Create(descrSetLayoutInfo);
		if (debugUtils) {
			debugUtils->Helper_SetObjectName(
				device.handle,
				descrSetLayout,
				"GuiResourceManager - Text DescrSetLayout");
		}

		vk::SamplerCreateInfo samplerInfo{};
		samplerInfo.addressModeU = vk::SamplerAddressMode::eClampToEdge;
		samplerInfo.addressModeV = vk::SamplerAddressMode::eClampToEdge;
		samplerInfo.addressModeW = vk::SamplerAddressMode::eClampToEdge;
		samplerInfo.magFilter = vk::Filter::eNearest;
		samplerInfo.maxLod = VK_LOD_CLAMP_NONE;
		samplerInfo.minFilter = vk::Filter::eNearest;
		samplerInfo.mipmapMode = vk::SamplerMipmapMode::eNearest;
		auto sampler = device.Create(samplerInfo);
		if (debugUtils) {
			debugUtils->Helper_SetObjectName(
				device.handle,
				sampler,
				"GuiResourceManager - Text Sampler");
		}

		vk::PushConstantRange pushConstantRange = {};
		pushConstantRange.stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment;
		pushConstantRange.size = sizeof(GuiResourceManager::FontPushConstant);

		vk::DescriptorSetLayout descrSetLayouts[] = { windowDescrSetLayout, descrSetLayout };

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo = {};
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.setLayoutCount = 2;
		pipelineLayoutInfo.pSetLayouts = descrSetLayouts;
		auto pipelineLayout = device.Create(pipelineLayoutInfo);
		if (debugUtils) {
			debugUtils->Helper_SetObjectName(
				device.handle,
				pipelineLayout,
				"GuiResourceManager - Text PipelineLayout");
		}


		guiResMgr.font_descrPool = descrPool;
		guiResMgr.font_descrSetLayout = descrSetLayout;
		guiResMgr.font_sampler = sampler;
		guiResMgr.font_pipelineLayout = pipelineLayout;
	}
}

void Vk::GuiResourceManager::Init(
	GuiResourceManager& manager,
	Init_Params const& params)
{
	auto& device = params.device;
	auto& vma = params.vma;
	auto inFlightCount = params.inFlightCount;
	auto guiRenderPass = params.guiRenderPass;
	auto& transientAlloc = params.transientAlloc;
	auto viewportImgDescrSetLayout = params.viewportImgDescrLayout;
	auto* debugUtils = params.debugUtils;

	vk::Result vkResult = {};

	GuiResourceManagerImpl::SetupGuiWindowUniforms(
		manager,
		device,
		vma,
		inFlightCount,
		transientAlloc,
		debugUtils);

	auto windowDescrSetLayout = manager.windowUniforms.setLayout;

	GuiResourceManagerImpl::SetupTextResources(
		manager,
		device,
		windowDescrSetLayout,
		debugUtils);
	auto fontPipelineLayout = manager.font_pipelineLayout;

	GuiResourceManagerImpl::CreateRectangleShader(
		manager,
		device,
		windowDescrSetLayout,
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
		windowDescrSetLayout,
		viewportImgDescrSetLayout,
		guiRenderPass,
		transientAlloc,
		debugUtils);

	vk::BufferCreateInfo vtxBufferInfo{};
	vtxBufferInfo.sharingMode = vk::SharingMode::eExclusive;
	vtxBufferInfo.size = sizeof(GuiVertex) * minVtxCapacity * inFlightCount;
	vtxBufferInfo.usage = vk::BufferUsageFlagBits::eVertexBuffer;
	VmaAllocationCreateInfo vtxVmaAllocInfo{};
	vtxVmaAllocInfo.flags = VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_MAPPED_BIT;
	vtxVmaAllocInfo.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_CPU_TO_GPU;
	VmaAllocationInfo vtxVmaAllocResultInfo;
	vkResult = (vk::Result)vmaCreateBuffer(
		vma,
		(VkBufferCreateInfo*)&vtxBufferInfo,
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


	vk::BufferCreateInfo indexBufferInfo{};
	indexBufferInfo.sharingMode = vk::SharingMode::eExclusive;
	indexBufferInfo.size = sizeof(u32) * minIndexCapacity * inFlightCount;
	indexBufferInfo.usage = vk::BufferUsageFlagBits::eIndexBuffer;
	VmaAllocationCreateInfo indexVmaAllocInfo{};
	indexVmaAllocInfo.flags = VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_MAPPED_BIT;
	indexVmaAllocInfo.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_CPU_TO_GPU;
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


	/*
	GuiResourceManagerImpl::CreateFilledMeshShader(
		manager,
		device,
		manager.windowUniforms.setLayout,
		guiRenderPass,
		transientAlloc,
		debugUtils);
	

	 */

	/*
	GuiResourceManagerImpl::CreateViewportShader(
		manager,
		device,
		guiRenderPass,
		transientAlloc,
		debugUtils);
	 */

}

namespace DEngine::Gfx::Vk {
	namespace Helper {
		struct AllocStagingBuffer_Return {
			vk::Buffer handle;
			VmaAllocation vmaAlloc;
			void* mappedPtr;
		};
		[[nodiscard]] AllocStagingBuffer_Return AllocStagingBuffer(
			VmaAllocator vma,
			u64 size)
		{
			vk::BufferCreateInfo bufferInfo {};
			bufferInfo.sharingMode = vk::SharingMode::eExclusive;
			bufferInfo.size = size;
			bufferInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;
			VmaAllocationCreateInfo vmaAllocInfo {};
			vmaAllocInfo.flags = VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_MAPPED_BIT;
			vmaAllocInfo.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_CPU_ONLY;
			VmaAllocationInfo vmaResultInfo {};
			vk::Buffer buffer {};
			VmaAllocation vmaAlloc {};
			auto result = (vk::Result)vmaCreateBuffer(
				vma,
				(VkBufferCreateInfo const*)&bufferInfo,
				&vmaAllocInfo,
				(VkBuffer*)&buffer,
				&vmaAlloc,
				&vmaResultInfo);
			if (result != vk::Result::eSuccess)
				throw std::runtime_error("Unable to allocate temp font buffer");

			AllocStagingBuffer_Return returnVal {};
			returnVal.handle = buffer;
			returnVal.vmaAlloc = vmaAlloc;
			returnVal.mappedPtr = vmaResultInfo.pMappedData;
			return returnVal;
		}

		[[nodiscard]] BoxVkImg AllocSampledImage(
			VmaAllocator vma,
			u32 width,
			u32 height)
		{
			// Allocate the destination image
			vk::ImageCreateInfo imgInfo {};
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
			vmaAllocInfo.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_GPU_ONLY;
			VmaAllocation vmaAlloc {};
			vk::Image img {};
			auto result = (vk::Result)vmaCreateImage(
				vma,
				(VkImageCreateInfo const*)&imgInfo,
				&vmaAllocInfo,
				(VkImage*)&img,
				&vmaAlloc,
				nullptr);
			if (result != vk::Result::eSuccess)
				throw std::runtime_error("Unable to allocate VkImage");

			return BoxVkImg::Adopt(vma, img, vmaAlloc);
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
		Std::Span<BoxVkImg const> images,
		TransientAllocRef transientAlloc)
	{
		int jobCount = (int)images.Size();

		auto preCopyBarriers = Std::NewVec<vk::ImageMemoryBarrier>(transientAlloc);
		preCopyBarriers.Reserve(jobCount);
		for (auto const& img : images)
			preCopyBarriers.PushBack(
				Helper::CreateSampledImgBarrier_PreCopy(img.handle));
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
		Std::Span<BoxVkImg const> images,
		TransientAllocRef transientAlloc)
	{
		int jobCount = (int)images.Size();

		auto preCopyBarriers = Std::NewVec<vk::ImageMemoryBarrier>(transientAlloc);
		preCopyBarriers.Reserve(jobCount);
		for (auto const& img : images)
			preCopyBarriers.PushBack(
				Helper::CreateSampledImgBarrier_PostCopy(img.handle));
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
		 Std::Span<BoxVkImg const> images,
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
				images[i].handle,
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
		auto imgVec = Std::NewVec<BoxVkImg>(transientAlloc);
		imgVec.Resize(jobCount);
		for (int i = 0; i < jobCount; i++) {
			auto const& job = glyphJobs[i];
			auto& img = imgVec[i];
			img = Helper::AllocSampledImage(vma, job.imgWidth, job.imgHeight);

			if (debugUtils) {
				auto name = CreateFontGlyphOjectName(job.fontFaceId, job.utfValue);
				name += " - VkImage";
				debugUtils->Helper_SetObjectName(device.handle, img.handle, name.c_str());
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
		Std::Span<BoxVkImg const> images,
		TransientAllocRef transientAlloc,
		DebugUtilsDispatch const* debugUtils)
	{
		auto jobCount = glyphJobs.Size();

		auto imgViewVec = Std::NewVec<vk::ImageView>(transientAlloc);
		imgViewVec.Resize(jobCount);
		for (int i = 0; i < jobCount; i++) {
			auto& imgView = imgViewVec[i];
			imgView = CreateFontGlyphImgView(device, images[i].handle);

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
		// Create the descriptor sets.
		auto setLayouts = Std::NewVec<vk::DescriptorSetLayout>(transientAlloc);
		setLayouts.Resize(count);
		for (auto& item : setLayouts)
			item = setLayout;

		auto descrSets = Std::NewVec<vk::DescriptorSet>(transientAlloc);
		descrSets.Resize(count);

		vk::DescriptorSetAllocateInfo descrSetAllocInfo {};
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

		device.updateDescriptorSets({ (u32)descrWrites.Size(), descrWrites.Data() }, {});
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
		Std::Defer clearQueueRoutine = [&]() {
			guiResMgr.queuedGlyphBitmapData.clear();
			guiResMgr.newGlyphJobs.clear();
		};

		int jobCount = (int)glyphJobs.size();
		if (jobCount == 0)
			return;
		Std::ConstByteSpan allBitmapData = {
			glyphBitmapData.data(),
			glyphBitmapData.size() };


		// Allocate the staging buffer
		// This contains all the bitmap data.
		// We will then transfer parts of into the destination images
		// in GPU memory.
		auto stagingBuffer = stagingBufferAlloc.Alloc(device, allBitmapData.Size(), 1);
		std::memcpy(stagingBuffer.mappedMem.Data(), allBitmapData.Data(), allBitmapData.Size());


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
			guiResMgr.font_descrPool,
			guiResMgr.font_descrSetLayout,
			jobCount,
			transientAlloc,
			{ guiResMgr.newGlyphJobs.data(), guiResMgr.newGlyphJobs.size() },
			debugUtils);

		FontGlyphs_WriteDescriptorSets(
			device,
			guiResMgr.font_sampler,
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
	vk::DescriptorSet perWindowDescrSet,
	GuiDrawCmd::Text const& drawCmd,
	Std::Span<u32 const> utfValuesAll,
	Std::Span<GlyphRect const> glyphRectsAll)
{
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
		manager.font_pipeline);

	for (int i = 0; i < (int)drawCmd.count; i++)
	{
		auto const& glyphRect = glyphRects[i];
		if (glyphRect.extent == Math::Vec2::Zero())
			continue;
		GuiResourceManager::FontPushConstant pushConstant {};
		pushConstant.color = drawCmd.color;
		pushConstant.rectOffset = drawCmd.posOffset + glyphRect.pos;
		pushConstant.rectExtent = glyphRect.extent;
		device.cmdPushConstants(
			cmdBuffer,
			manager.font_pipelineLayout,
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

		DENGINE_IMPL_GFX_ASSERT(glyphData->img != vk::Image{});

		device.cmdBindDescriptorSets(
			cmdBuffer,
			vk::PipelineBindPoint::eGraphics,
			manager.font_pipelineLayout,
			0,
			{ perWindowDescrSet, glyphData->descrSet },
			nullptr);
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
	vk::DescriptorSet descrSet,
	vk::CommandBuffer cmdBuffer,
	GuiDrawCmd::Rectangle const& drawCmd)
{
	device.cmdBindPipeline(
		cmdBuffer,
		vk::PipelineBindPoint::eGraphics,
		manager.rectanglePipeline);

	device.cmdBindDescriptorSets(
		cmdBuffer,
		vk::PipelineBindPoint::eGraphics,
		manager.rectanglePipelineLayout,
		0,
		descrSet,
		{});

	GuiResourceManager::RectanglePushConstant pushConst = {};
	pushConst.color = drawCmd.color;
	pushConst.rectExtent = drawCmd.extent;
	pushConst.rectOffset = drawCmd.pos;
	pushConst.radius = drawCmd.radius;

	device.cmdPushConstants(
		cmdBuffer,
		manager.rectanglePipelineLayout,
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
	auto resolutionX = params.resolutionX;
	auto resolutionY = params.resolutionY;
	auto rectPos = params.rectPos;
	auto rectExtent = params.rectExtent;
	auto windowRotation = params.rotation;
	auto isSideways = windowRotation == Gfx::WindowRotation::e90 || windowRotation == Gfx::WindowRotation::e270;

	auto flippedExtent = rectExtent;
	auto flippedPos = rectPos;
	if (isSideways) {
		Std::Swap(flippedExtent.x, flippedExtent.y);
		Std::Swap(flippedPos.x, flippedPos.y);
	}

	// If rotated, we need to position it differently
	Math::Vec2Int pos = {};
	Math::Vec2Int extent = {};
	if (windowRotation == Gfx::WindowRotation::e0) {
		extent = {
			(i32)Math::Round(rectExtent.x * (f32)resolutionX),
			(i32)Math::Round(rectExtent.y * (f32)resolutionY), };
		pos = {
			(i32)Math::Round(rectPos.x * (f32)resolutionX),
			(i32)Math::Round(rectPos.y * (f32)resolutionY), };
	} else if (windowRotation == Gfx::WindowRotation::e90) {
		extent = {
			(i32)Math::Round(rectExtent.x * (f32)resolutionY),
			(i32)Math::Round(rectExtent.y * (f32)resolutionX), };
		pos = {
			(i32)Math::Round(rectPos.x * (f32)resolutionY),
			(i32)Math::Round(rectPos.y * (f32)resolutionX), };
		pos = { pos.y, resolutionY - extent.x - pos.x };
	} else if (windowRotation == Gfx::WindowRotation::e180) {
		extent = {
			(i32)Math::Round(rectExtent.x * (f32)resolutionX),
			(i32)Math::Round(rectExtent.y * (f32)resolutionY), };
		pos = {
			(i32)Math::Round(rectPos.x * (f32)resolutionX),
			(i32)Math::Round(rectPos.y * (f32)resolutionY), };
		pos = { resolutionX - extent.x - pos.x, resolutionY - extent.y - pos.y };
	} else if (windowRotation == Gfx::WindowRotation::e270) {
		extent = {
			(i32)Math::Round(rectExtent.x * (f32)resolutionY),
			(i32)Math::Round(rectExtent.y * (f32)resolutionX), };
		pos = {
			(i32)Math::Round(rectPos.x * (f32)resolutionY),
			(i32)Math::Round(rectPos.y * (f32)resolutionX), };
		pos = { resolutionX - extent.y - pos.y, pos.x };
	}

	if (isSideways) {
		Std::Swap(extent.x, extent.y);
	}

	vk::Rect2D scissor = {};
	scissor.offset = vk::Offset2D{ pos.x, pos.y };
	scissor.extent = vk::Extent2D{ (u32)extent.x, (u32)extent.y };

	// Clamp the scissor we got.
    if (scissor.offset.x < 0) {
        scissor.extent.width = (u32)Math::Max(0, (i32)scissor.extent.width + scissor.offset.x);
        scissor.offset.x = 0;
    }
    if (scissor.offset.y < 0) {
        scissor.extent.height = (u32)Math::Max(0, (i32)scissor.extent.height + scissor.offset.y);
        scissor.offset.y = 0;
    }
	auto clampedWidth = Math::Min(resolutionX - scissor.offset.x, (i32)scissor.extent.width);
	clampedWidth = Math::Max(clampedWidth, 0);
	auto clampedHeight = Math::Min(resolutionY - scissor.offset.y, (i32)scissor.extent.height);
	clampedHeight = Math::Max(clampedHeight, 0);
	scissor.extent.width = (u32)clampedWidth;
	scissor.extent.height = (u32)clampedHeight;
	device.cmdSetScissor(cmdBuffer, 0, scissor);
}

void GuiResourceManager::UpdateWindowUniforms(
	GuiResourceManager& guiResMgr,
	UpdateWindowUniforms_Params const& params)
{
	auto const& device = params.device;
	auto& stagingBufferAlloc = params.stagingBufferAlloc;
	auto& delQueue = params.delQueue;
	auto& transientAlloc = params.transientAlloc;
	auto& vma = params.vma;
	auto debugUtils = params.debugUtils;
	auto const& windowRange = params.windowRange;
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
	DENGINE_IMPL_GFX_ASSERT(windowUniforms.currentCapacity >= windowCount);

	auto dstBufferSpan = windowUniforms.InFlightBufferSpan(inFlightIndex);

	// Clear all the window-ids from previous frame.l
	windowUniforms.windowIds.clear();

	// Go through each element in the buffer and update it accordingly.
	// Also assign the window-id.
	for (int i = 0; i < windowCount; i++) {
		auto temp = windowRange.Invoke(i);

		windowUniforms.windowIds.push_back(temp.windowId);

		auto byteOffset = windowUniforms.bufferElementSize * i;

		// Write the stuff to this buffer
		WindowShaderUniforms::PerWindowUniform uniform = {};
		uniform.orientation = (int)temp.orientation;
		uniform.resolution = { temp.resolutionWidth, temp.resolutionHeight };

		memcpy(
			dstBufferSpan.Data() + byteOffset,
			&uniform,
			sizeof(uniform));
	}
}

vk::DescriptorSet GuiResourceManager::GetPerWindowDescrSet(
	GuiResourceManager const& manager,
	NativeWindowID windowIdIn,
	int inFlightIndex)
{
	// Subspan into our inFlightIndex
	auto const& uniforms = manager.windowUniforms;
	auto capacity = uniforms.currentCapacity;
	auto inFlightSpan = Std::Span{
		&uniforms.windowUniformDescrSets[capacity * inFlightIndex],
		(uSize)capacity };

	vk::DescriptorSet out = {};
	for (int i = 0; i < (int)uniforms.windowIds.size(); i++) {
		auto testWindowId = uniforms.windowIds[i];
		if (testWindowId == windowIdIn) {
			out = uniforms.windowUniformDescrSets[i];
			break;
		}
	}
	DENGINE_IMPL_GFX_ASSERT(out != vk::DescriptorSet{});
	return out;
}

void GuiResourceManager::PerformGuiDrawCmd_Viewport(
	GuiResourceManager const& manager,
	DeviceDispatch const& device,
	vk::CommandBuffer cmdBuffer,
	vk::DescriptorSet perWindowDescr,
	vk::DescriptorSet viewportDescr,
	Gfx::WindowRotation rotation,
	Math::Vec2 rectOffset,
	Math::Vec2 rectExtent)
{
	device.cmdBindPipeline(cmdBuffer, vk::PipelineBindPoint::eGraphics, manager.viewportPipeline);

	device.cmdBindDescriptorSets(
		cmdBuffer,
		vk::PipelineBindPoint::eGraphics,
		manager.viewportPipelineLayout,
		0,
		{ perWindowDescr, viewportDescr },
		nullptr);


	GuiResourceManager::ViewportPushConstant pushConstant = {};
	pushConstant.rectExtent = rectExtent;
	pushConstant.rectOffset = rectOffset;
	device.cmdPushConstants(
		cmdBuffer,
		manager.viewportPipelineLayout,
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