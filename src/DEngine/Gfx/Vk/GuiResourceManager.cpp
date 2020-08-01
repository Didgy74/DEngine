#include "GuiResourceManager.hpp"

#include "DEngine/Gfx/Gfx.hpp"
#include "DynamicDispatch.hpp"
#include "GlobUtils.hpp"

#include "DEngine/Containers/Array.hpp"

// For file IO
#include "DEngine/Application.hpp"

using namespace DEngine;
using namespace DEngine::Gfx;

namespace DEngine::Gfx::Vk::GuiResourceManagerImpl
{
	static constexpr Std::Array<vk::VertexInputAttributeDescription, 2> BuildShaderVertexInputAttrDescr()
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

	static constexpr Std::Array<vk::VertexInputBindingDescription, 1> BuildShaderVertexInputBindingDescr()
	{
		vk::VertexInputBindingDescription binding{};
		binding.binding = 0;
		binding.inputRate = vk::VertexInputRate::eVertex;
		binding.stride = sizeof(GuiVertex);

		return { binding };
	}

	static void CreateFilledMeshShader(
		GuiResourceManager& manager,
		DeviceDispatch const& device,
		vk::RenderPass guiRenderPass,
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

		manager.filledMeshPipelineLayout = device.createPipelineLayout(pipelineLayoutInfo);
		if (debugUtils)
		{
			vk::DebugUtilsObjectNameInfoEXT nameInfo{};
			nameInfo.objectHandle = (u64)(VkPipelineLayout)manager.filledMeshPipelineLayout;
			nameInfo.objectType = manager.filledMeshPipelineLayout.objectType;
			std::string name = "GuiResourceManager - FilledMesh PipelineLayout";
			nameInfo.pObjectName = name.c_str();
			debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
		}

		vk::DynamicState dynamicStates[1] = { vk::DynamicState::eViewport };
		vk::PipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.dynamicStateCount = 1;
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
		std::vector<char> vertCode((uSize)vertFileLength);
		vertFile.Read(vertCode.data(), vertFileLength);
		vk::ShaderModuleCreateInfo vertModCreateInfo{};
		vertFile.Close();
		vertModCreateInfo.codeSize = vertCode.size();
		vertModCreateInfo.pCode = reinterpret_cast<const u32*>(vertCode.data());
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
		std::vector<char> fragCode((uSize)fragFileLength);
		fragFile.Read(fragCode.data(), fragFileLength);
		fragFile.Close();
		vk::ShaderModuleCreateInfo fragModInfo{};
		fragModInfo.codeSize = fragCode.size();
		fragModInfo.pCode = reinterpret_cast<u32 const*>(fragCode.data());
		vk::ShaderModule fragModule = device.createShaderModule(fragModInfo);
		vk::PipelineShaderStageCreateInfo fragStageInfo{};
		fragStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
		fragStageInfo.module = fragModule;
		fragStageInfo.pName = "main";

		Std::Array<vk::PipelineShaderStageCreateInfo, 2> shaderStages = { vertStageInfo, fragStageInfo };

		vk::GraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.layout = manager.filledMeshPipelineLayout;
		pipelineInfo.pColorBlendState = &colorBlendState;
		pipelineInfo.pDynamicState = &dynamicState;
		pipelineInfo.pInputAssemblyState = &inputAssemblyState;
		pipelineInfo.pMultisampleState = &multiSampleState;
		pipelineInfo.pRasterizationState = &rasterizationState;
		pipelineInfo.pVertexInputState = &vertexInputState;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.renderPass = guiRenderPass;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages.Data();

		vk::Result vkResult = device.createGraphicsPipelines(
			vk::PipelineCache(),
			pipelineInfo,
			nullptr,
			&manager.filledMeshPipeline);
		if (vkResult != vk::Result::eSuccess)
			throw std::runtime_error("DEngine - Vulkan: Unable to create GUI shader.");

		if (debugUtils)
		{
			vk::DebugUtilsObjectNameInfoEXT nameInfo{};
			nameInfo.objectHandle = (u64)(VkPipeline)manager.filledMeshPipeline;
			nameInfo.objectType = manager.filledMeshPipeline.objectType;
			std::string name = "GuiResourceManager - FilledMesh Pipeline";
			nameInfo.pObjectName = name.c_str();
			debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
		}

		device.destroy(vertModule);
		device.destroy(fragModule);
	}

	static void CreateViewportShader(
		GuiResourceManager& manager,
		DeviceDispatch const& device,
		vk::RenderPass guiRenderPass,
		DebugUtilsDispatch const* debugUtils)
	{
		vk::DescriptorSetLayoutBinding imgDescrBinding{};
		imgDescrBinding.binding = 0;
		imgDescrBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;
		imgDescrBinding.descriptorCount = 1;
		imgDescrBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		vk::DescriptorSetLayoutCreateInfo descrSetLayoutInfo{};
		descrSetLayoutInfo.bindingCount = 1;
		descrSetLayoutInfo.pBindings = &imgDescrBinding;
		manager.viewportDescrSetLayout = device.createDescriptorSetLayout(descrSetLayoutInfo);
		if (debugUtils)
		{
			vk::DebugUtilsObjectNameInfoEXT nameInfo{};
			nameInfo.objectHandle = (u64)(VkDescriptorSetLayout)manager.viewportDescrSetLayout;
			nameInfo.objectType = manager.viewportDescrSetLayout.objectType;
			std::string name = "GuiResourceManager - Viewport DescrSetLayout";
			nameInfo.pObjectName = name.c_str();
			debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
		}

		vk::DescriptorPoolSize descrPoolSize{};
		descrPoolSize.descriptorCount = 10;
		descrPoolSize.type = vk::DescriptorType::eCombinedImageSampler;
		vk::DescriptorPoolCreateInfo descrPoolInfo{};
		descrPoolInfo.maxSets = 10;
		descrPoolInfo.poolSizeCount = 1;
		descrPoolInfo.pPoolSizes = &descrPoolSize;
		manager.viewportDescrPool = device.createDescriptorPool(descrPoolInfo);
		if (debugUtils)
		{
			vk::DebugUtilsObjectNameInfoEXT nameInfo{};
			nameInfo.objectHandle = (u64)(VkDescriptorPool)manager.viewportDescrPool;
			nameInfo.objectType = manager.viewportDescrPool.objectType;
			std::string name = "GuiResourceManager - Viewport DescrPool";
			nameInfo.pObjectName = name.c_str();
			debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
		}

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &manager.viewportDescrSetLayout;

		vk::PushConstantRange vertPushConstantRange{};
		vertPushConstantRange.stageFlags = vk::ShaderStageFlagBits::eVertex;
		vertPushConstantRange.size = sizeof(GuiResourceManager::ViewportPushConstant);
		vertPushConstantRange.offset = 0;

		pipelineLayoutInfo.pPushConstantRanges = &vertPushConstantRange;
		pipelineLayoutInfo.pushConstantRangeCount = 1;

		manager.viewportPipelineLayout = device.createPipelineLayout(pipelineLayoutInfo);
		if (debugUtils)
		{
			vk::DebugUtilsObjectNameInfoEXT nameInfo{};
			nameInfo.objectHandle = (u64)(VkPipelineLayout)manager.viewportPipelineLayout;
			nameInfo.objectType = manager.viewportPipelineLayout.objectType;
			std::string name = "GuiResourceManager - Viewport PipelineLayout";
			nameInfo.pObjectName = name.c_str();
			debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
		}

		vk::DynamicState dynamicStates[1] = { vk::DynamicState::eViewport };
		vk::PipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.dynamicStateCount = 1;
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
		std::vector<char> vertCode((uSize)vertFileLength);
		vertFile.Read(vertCode.data(), vertFileLength);
		vk::ShaderModuleCreateInfo vertModCreateInfo{};
		vertFile.Close();
		vertModCreateInfo.codeSize = vertCode.size();
		vertModCreateInfo.pCode = reinterpret_cast<const u32*>(vertCode.data());
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
		std::vector<char> fragCode((uSize)fragFileLength);
		fragFile.Read(fragCode.data(), fragFileLength);
		fragFile.Close();
		vk::ShaderModuleCreateInfo fragModInfo{};
		fragModInfo.codeSize = fragCode.size();
		fragModInfo.pCode = reinterpret_cast<u32 const*>(fragCode.data());
		vk::ShaderModule fragModule = device.createShaderModule(fragModInfo);
		vk::PipelineShaderStageCreateInfo fragStageInfo{};
		fragStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
		fragStageInfo.module = fragModule;
		fragStageInfo.pName = "main";

		Std::Array<vk::PipelineShaderStageCreateInfo, 2> shaderStages = { vertStageInfo, fragStageInfo };

		vk::GraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.layout = manager.viewportPipelineLayout;
		pipelineInfo.pColorBlendState = &colorBlendState;
		pipelineInfo.pDynamicState = &dynamicState;
		pipelineInfo.pInputAssemblyState = &inputAssemblyState;
		pipelineInfo.pMultisampleState = &multiSampleState;
		pipelineInfo.pRasterizationState = &rasterizationState;
		pipelineInfo.pVertexInputState = &vertexInputState;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.renderPass = guiRenderPass;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages.Data();

		vk::Result vkResult = device.createGraphicsPipelines(
			vk::PipelineCache(),
			pipelineInfo,
			nullptr,
			&manager.viewportPipeline);
		if (vkResult != vk::Result::eSuccess)
			throw std::runtime_error("DEngine - Vulkan: Unable to create GUI shader.");
		if (debugUtils)
		{
			vk::DebugUtilsObjectNameInfoEXT nameInfo{};
			nameInfo.objectHandle = (u64)(VkPipeline)manager.viewportPipeline;
			nameInfo.objectType = manager.viewportPipeline.objectType;
			std::string name = "GuiResourceManager - Viewport Pipeline";
			nameInfo.pObjectName = name.c_str();
			debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
		}

		device.destroy(vertModule);
		device.destroy(fragModule);
	}

	static void CreateTextShader(
		GuiResourceManager& manager,
		DeviceDispatch const& device,
		vk::RenderPass guiRenderPass,
		DebugUtilsDispatch const* debugUtils)
	{
		vk::DescriptorPoolSize sampledImgDescrPoolSize{};
		sampledImgDescrPoolSize.descriptorCount = 50;
		sampledImgDescrPoolSize.type = vk::DescriptorType::eCombinedImageSampler;
		vk::DescriptorPoolCreateInfo descrPoolInfo{};
		descrPoolInfo.maxSets = 50;
		descrPoolInfo.poolSizeCount = 1;
		descrPoolInfo.pPoolSizes = &sampledImgDescrPoolSize;
		manager.font_descrPool = device.createDescriptorPool(descrPoolInfo);
		if (debugUtils)
		{
			vk::DebugUtilsObjectNameInfoEXT nameInfo{};
			nameInfo.objectHandle = (u64)(VkDescriptorPool)manager.font_descrPool;
			nameInfo.objectType = manager.font_descrPool.objectType;
			std::string name = "GuiResourceManager - Text DescrPool";
			nameInfo.pObjectName = name.c_str();
			debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
		}

		vk::DescriptorSetLayoutBinding imgDescrBinding{};
		imgDescrBinding.binding = 0;
		imgDescrBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;
		imgDescrBinding.descriptorCount = 1;
		imgDescrBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		vk::DescriptorSetLayoutCreateInfo descrSetLayoutInfo{};
		descrSetLayoutInfo.bindingCount = 1;
		descrSetLayoutInfo.pBindings = &imgDescrBinding;
		manager.font_descrSetLayout = device.createDescriptorSetLayout(descrSetLayoutInfo);
		if (debugUtils)
		{
			vk::DebugUtilsObjectNameInfoEXT nameInfo{};
			nameInfo.objectHandle = (u64)(VkDescriptorSetLayout)manager.font_descrSetLayout;
			nameInfo.objectType = manager.font_descrSetLayout.objectType;
			std::string name = "GuiResourceManager - Text DescrSetLayout";
			nameInfo.pObjectName = name.c_str();
			debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
		}

		vk::SamplerCreateInfo samplerInfo{};
		samplerInfo.addressModeU = vk::SamplerAddressMode::eClampToEdge;
		samplerInfo.addressModeV = vk::SamplerAddressMode::eClampToEdge;
		samplerInfo.addressModeW = vk::SamplerAddressMode::eClampToEdge;
		samplerInfo.magFilter = vk::Filter::eNearest;
		samplerInfo.maxLod = 64.f;
		samplerInfo.minFilter = vk::Filter::eNearest;
		samplerInfo.mipmapMode = vk::SamplerMipmapMode::eNearest;
		manager.font_sampler = device.createSampler(samplerInfo);
		if (debugUtils)
		{
			vk::DebugUtilsObjectNameInfoEXT nameInfo{};
			nameInfo.objectHandle = (u64)(VkSampler)manager.font_sampler;
			nameInfo.objectType = manager.font_sampler.objectType;
			std::string name = "GuiResourceManager - Text Sampler";
			nameInfo.pObjectName = name.c_str();
			debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
		}

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
		vk::PushConstantRange vertPushConstantRange{};
		vertPushConstantRange.stageFlags = vk::ShaderStageFlagBits::eVertex;
		vertPushConstantRange.size = 32;
		vertPushConstantRange.offset = 0;
		vk::PushConstantRange fragPushConstantRange{};
		fragPushConstantRange.stageFlags = vk::ShaderStageFlagBits::eFragment;
		fragPushConstantRange.size = sizeof(GuiResourceManager::FontPushConstant::color);
		fragPushConstantRange.offset = vertPushConstantRange.size;

		Std::Array<vk::PushConstantRange, 2> pushConstantRanges{ vertPushConstantRange, fragPushConstantRange };

		pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.Data();
		pipelineLayoutInfo.pushConstantRangeCount = (u32)pushConstantRanges.Size();
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &manager.font_descrSetLayout;
		manager.font_pipelineLayout = device.createPipelineLayout(pipelineLayoutInfo);
		if (debugUtils)
		{
			vk::DebugUtilsObjectNameInfoEXT nameInfo{};
			nameInfo.objectHandle = (u64)(VkPipelineLayout)manager.font_pipelineLayout;
			nameInfo.objectType = manager.font_pipelineLayout.objectType;
			std::string name = "GuiResourceManager - Text PipelineLayout";
			nameInfo.pObjectName = name.c_str();
			debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
		}

		vk::DynamicState dynamicStates[1] = { vk::DynamicState::eViewport };
		vk::PipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.dynamicStateCount = 1;
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
		std::vector<char> vertCode((uSize)vertFileLength);
		vertFile.Read(vertCode.data(), vertFileLength);
		vk::ShaderModuleCreateInfo vertModCreateInfo{};
		vertFile.Close();
		vertModCreateInfo.codeSize = vertCode.size();
		vertModCreateInfo.pCode = reinterpret_cast<const u32*>(vertCode.data());
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
		std::vector<char> fragCode((uSize)fragFileLength);
		fragFile.Read(fragCode.data(), fragFileLength);
		fragFile.Close();
		vk::ShaderModuleCreateInfo fragModInfo{};
		fragModInfo.codeSize = fragCode.size();
		fragModInfo.pCode = reinterpret_cast<u32 const*>(fragCode.data());
		vk::ShaderModule fragModule = device.createShaderModule(fragModInfo);
		vk::PipelineShaderStageCreateInfo fragStageInfo{};
		fragStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
		fragStageInfo.module = fragModule;
		fragStageInfo.pName = "main";

		Std::Array<vk::PipelineShaderStageCreateInfo, 2> shaderStages = { vertStageInfo, fragStageInfo };

		vk::GraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.layout = manager.font_pipelineLayout;
		pipelineInfo.pColorBlendState = &colorBlendState;
		pipelineInfo.pDynamicState = &dynamicState;
		pipelineInfo.pInputAssemblyState = &inputAssemblyState;
		pipelineInfo.pMultisampleState = &multiSampleState;
		pipelineInfo.pRasterizationState = &rasterizationState;
		pipelineInfo.pVertexInputState = &vertexInputState;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.renderPass = guiRenderPass;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages.Data();

		vk::Result vkResult = device.createGraphicsPipelines(
			vk::PipelineCache(),
			pipelineInfo,
			nullptr,
			&manager.font_pipeline);
		if (vkResult != vk::Result::eSuccess)
			throw std::runtime_error("DEngine - Vulkan: Unable to create GUI shader.");
		if (debugUtils)
		{
			vk::DebugUtilsObjectNameInfoEXT nameInfo{};
			nameInfo.objectHandle = (u64)(VkPipeline)manager.font_pipeline;
			nameInfo.objectType = manager.font_pipeline.objectType;
			std::string name = "GuiResourceManager - Text Pipeline";
			nameInfo.pObjectName = name.c_str();
			debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
		}

		device.destroy(vertModule);
		device.destroy(fragModule);
	}
}

void Vk::GuiResourceManager::Init(
	GuiResourceManager& manager,
	DeviceDispatch const& device,
	VmaAllocator vma,
	u8 inFlightCount,
	vk::RenderPass guiRenderPass,
	DebugUtilsDispatch const* debugUtils)
{
	vk::Result vkResult{};

	vk::BufferCreateInfo vertexBufferInfo{};
	vertexBufferInfo.sharingMode = vk::SharingMode::eExclusive;
	vertexBufferInfo.size = sizeof(GuiVertex) * minVertexCapacity * inFlightCount;
	vertexBufferInfo.usage = vk::BufferUsageFlagBits::eVertexBuffer;
	VmaAllocationCreateInfo vertexVmaAllocInfo{};
	vertexVmaAllocInfo.flags = VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_MAPPED_BIT;
	vertexVmaAllocInfo.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_CPU_TO_GPU;
	VmaAllocationInfo vertexVmaAllocResultInfo;
	vkResult = (vk::Result)vmaCreateBuffer(
		vma,
		(VkBufferCreateInfo*)&vertexBufferInfo,
		&vertexVmaAllocInfo,
		(VkBuffer*)&manager.vertexBuffer,
		&manager.vertexVmaAlloc,
		&vertexVmaAllocResultInfo);
	if (vkResult != vk::Result::eSuccess)
		throw std::runtime_error("DEngine - Vulkan: VMA was unable to allocate memory for GUI vertices.");
	if (debugUtils)
	{
		vk::DebugUtilsObjectNameInfoEXT nameInfo{};
		nameInfo.objectHandle = (u64)(VkBuffer)manager.vertexBuffer;
		nameInfo.objectType = manager.vertexBuffer.objectType;
		std::string name = "GuiResourceManager - VertexBuffer";
		nameInfo.pObjectName = name.c_str();
		debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
	}
	manager.vertexCapacity = minVertexCapacity;
	manager.vertexBufferData = vertexVmaAllocResultInfo.pMappedData;


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
	if (debugUtils)
	{
		vk::DebugUtilsObjectNameInfoEXT nameInfo{};
		nameInfo.objectHandle = (u64)(VkBuffer)manager.indexBuffer;
		nameInfo.objectType = manager.indexBuffer.objectType;
		std::string name = "GuiResourceManager - IndexBuffer";
		nameInfo.pObjectName = name.c_str();
		debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
	}
	manager.indexCapacity = minIndexCapacity;
	manager.indexBufferData = indexVmaAllocResultInfo.pMappedData;


	GuiResourceManagerImpl::CreateFilledMeshShader(
		manager,
		device,
		guiRenderPass,
		debugUtils);
	
	GuiResourceManagerImpl::CreateTextShader(
		manager,
		device,
		guiRenderPass,
		debugUtils);

	GuiResourceManagerImpl::CreateViewportShader(
		manager,
		device,
		guiRenderPass,
		debugUtils);
}

void Vk::GuiResourceManager::Update(
	GuiResourceManager& manager,
	GlobUtils const& globUtils,
	Std::Span<GuiVertex const> guiVertices,
	Std::Span<u32 const> guiIndices,
	u8 inFlightIndex)
{
	std::memcpy(
		(char*)manager.vertexBufferData + manager.vertexCapacity * inFlightIndex * sizeof(decltype(guiVertices)::ValueType),
		guiVertices.Data(),
		guiVertices.Size() * sizeof(decltype(guiVertices)::ValueType));

	std::memcpy(
		(char*)manager.indexBufferData + manager.indexCapacity * inFlightIndex * sizeof(decltype(guiIndices)::ValueType),
		guiIndices.Data(),
		guiIndices.Size() * sizeof(decltype(guiIndices)::ValueType));

}

void Vk::GuiResourceManager::NewFontTexture(
	GuiResourceManager& manager,
	GlobUtils const& globUtils,
	u32 id,
	u32 width,
	u32 height,
	u32 pitch,
	Std::Span<std::byte const> data)
{
	auto& device = globUtils.device;
	vk::Result result{};

	GlyphData newGlyphData{};

	// Allocate the temporary buffer
	vk::BufferCreateInfo bufferInfo{};
	bufferInfo.sharingMode = vk::SharingMode::eExclusive;
	bufferInfo.size = data.Size();
	bufferInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;
	VmaAllocationCreateInfo bufferVmaInfo{};
	bufferVmaInfo.flags = VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_MAPPED_BIT;
	bufferVmaInfo.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_CPU_TO_GPU;
	VmaAllocationInfo tempBufferVmaResultInfo{};
	vk::Buffer tempBuffer{};
	VmaAllocation tempBufferVmaAlloc{};
	result = (vk::Result)vmaCreateBuffer(
		globUtils.vma,
		(VkBufferCreateInfo const*)&bufferInfo,
		&bufferVmaInfo,
		(VkBuffer*)&tempBuffer,
		&tempBufferVmaAlloc,
		&tempBufferVmaResultInfo);
	if (result != vk::Result::eSuccess)
		throw std::runtime_error("Unable to allocate temp font buffer");
	// Copy bitmap contents over to buffer
	std::memcpy(tempBufferVmaResultInfo.pMappedData, data.Data(), data.Size());

	// Allcoate the destination image
	vk::ImageCreateInfo imgInfo{};
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
	VmaAllocationCreateInfo imgVmaInfo{};
	imgVmaInfo.flags = VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_MAPPED_BIT;
	imgVmaInfo.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_GPU_ONLY;
	VmaAllocation tempImgVmaAlloc{};
	result = (vk::Result)vmaCreateImage(
		globUtils.vma,
		(VkImageCreateInfo const*)&imgInfo,
		&imgVmaInfo,
		(VkImage*)&newGlyphData.img,
		&tempImgVmaAlloc,
		nullptr);
	if (result != vk::Result::eSuccess)
		throw std::runtime_error("Unable to allocate font VkImage");
	if (globUtils.UsingDebugUtils())
	{
		vk::DebugUtilsObjectNameInfoEXT nameInfo{};
		nameInfo.objectHandle = (u64)(VkImage)newGlyphData.img;
		nameInfo.objectType = newGlyphData.img.objectType;
		std::string name = "GuiResourceManager - GlyphID #";
		name += std::to_string(id);
		name += " - Img";
		nameInfo.pObjectName = name.c_str();
		globUtils.debugUtils.setDebugUtilsObjectNameEXT(device.handle, nameInfo);
	}

	vk::ImageViewCreateInfo imgViewInfo{};
	imgViewInfo.format = imgInfo.format;
	imgViewInfo.image = newGlyphData.img;
	imgViewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	imgViewInfo.subresourceRange.layerCount = 1;
	imgViewInfo.subresourceRange.levelCount = 1;
	imgViewInfo.viewType = vk::ImageViewType::e2D;
	newGlyphData.imgView = globUtils.device.createImageView(imgViewInfo);
	if (globUtils.UsingDebugUtils())
	{
		vk::DebugUtilsObjectNameInfoEXT nameInfo{};
		nameInfo.objectHandle = (u64)(VkImageView)newGlyphData.imgView;
		nameInfo.objectType = newGlyphData.imgView.objectType;
		std::string name = "GuiResourceManager - GlyphID #";
		name += std::to_string(id);
		name += " - ImgView";
		nameInfo.pObjectName = name.c_str();
		globUtils.debugUtils.setDebugUtilsObjectNameEXT(device.handle, nameInfo);
	}

	vk::DescriptorSetAllocateInfo descrSetAllocInfo{};
	descrSetAllocInfo.descriptorPool = manager.font_descrPool;
	descrSetAllocInfo.descriptorSetCount = 1;
	descrSetAllocInfo.pSetLayouts = &manager.font_descrSetLayout;
	result = globUtils.device.allocateDescriptorSets(descrSetAllocInfo, &newGlyphData.descrSet);
	if (result != vk::Result::eSuccess)
		throw std::runtime_error("Unable to allocate descriptor set for glyph");
	if (globUtils.UsingDebugUtils())
	{
		vk::DebugUtilsObjectNameInfoEXT nameInfo{};
		nameInfo.objectHandle = (u64)(VkDescriptorSet)newGlyphData.descrSet;
		nameInfo.objectType = newGlyphData.descrSet.objectType;
		std::string name = "GuiResourceManager - GlyphID #";
		name += std::to_string(id);
		name += " - DescrSet";
		nameInfo.pObjectName = name.c_str();
		globUtils.debugUtils.setDebugUtilsObjectNameEXT(device.handle, nameInfo);
	}

	// Update descriptor set
	vk::DescriptorImageInfo descrImgInfo{};
	descrImgInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
	descrImgInfo.imageView = newGlyphData.imgView;
	descrImgInfo.sampler = manager.font_sampler;
	vk::WriteDescriptorSet descrSetWrite{};
	descrSetWrite.descriptorCount = 1;
	descrSetWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
	descrSetWrite.dstBinding = 0;
	descrSetWrite.dstSet = newGlyphData.descrSet;
	descrSetWrite.pImageInfo = &descrImgInfo;
	device.updateDescriptorSets(descrSetWrite, {});

	// Transfer the buffer to image, perform layout transition
	vk::CommandPoolCreateInfo cmdPoolInfo{};
	cmdPoolInfo.queueFamilyIndex = globUtils.queues.graphics.FamilyIndex();
	vk::CommandPool cmdPool = device.createCommandPool(cmdPoolInfo);

	vk::CommandBuffer cmdBuffer{};
	vk::CommandBufferAllocateInfo cmdBufferAllocInfo{};
	cmdBufferAllocInfo.commandBufferCount = 1;
	cmdBufferAllocInfo.commandPool = cmdPool;
	cmdBufferAllocInfo.level = vk::CommandBufferLevel::ePrimary;
	
	result = device.allocateCommandBuffers(cmdBufferAllocInfo, &cmdBuffer);
	if (result != vk::Result::eSuccess)
		throw std::runtime_error("Unable to allocate command buffer for copying font glyph");

	vk::CommandBufferBeginInfo cmdBufferBeginInfo{};
	cmdBufferBeginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
	device.beginCommandBuffer(cmdBuffer, cmdBufferBeginInfo);
	{
		vk::ImageMemoryBarrier preCopyBarrier{};
		preCopyBarrier.image = newGlyphData.img;
		preCopyBarrier.oldLayout = vk::ImageLayout::eUndefined;
		preCopyBarrier.newLayout = vk::ImageLayout::eTransferDstOptimal;
		preCopyBarrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
		preCopyBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		preCopyBarrier.subresourceRange.layerCount = 1;
		preCopyBarrier.subresourceRange.levelCount = 1;
		device.cmdPipelineBarrier(
			cmdBuffer,
			vk::PipelineStageFlagBits::eTopOfPipe,
			vk::PipelineStageFlagBits::eTransfer,
			vk::DependencyFlagBits(),
			{},
			{},
			preCopyBarrier);
		
		vk::BufferImageCopy buffImgCopy{};
		buffImgCopy.bufferImageHeight = height;
		buffImgCopy.bufferRowLength = pitch;
		buffImgCopy.imageExtent = vk::Extent3D{ width, height, 1 };
		buffImgCopy.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
		buffImgCopy.imageSubresource.layerCount = 1;
		
		device.cmdCopyBufferToImage(
			cmdBuffer,
			tempBuffer,
			newGlyphData.img,
			vk::ImageLayout::eTransferDstOptimal,
			buffImgCopy);

		vk::ImageMemoryBarrier postCopyBarrier{};
		postCopyBarrier.image = newGlyphData.img;
		postCopyBarrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
		postCopyBarrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		postCopyBarrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
		postCopyBarrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
		postCopyBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		postCopyBarrier.subresourceRange.layerCount = 1;
		postCopyBarrier.subresourceRange.levelCount = 1;
		device.cmdPipelineBarrier(
			cmdBuffer,
			vk::PipelineStageFlagBits::eTransfer,
			vk::PipelineStageFlagBits::eFragmentShader,
			vk::DependencyFlagBits(),
			{},
			{},
			postCopyBarrier);
	}
	device.endCommandBuffer(cmdBuffer);

	vk::Fence fence = device.createFence({});

	vk::SubmitInfo submit{};
	submit.commandBufferCount = 1;
	submit.pCommandBuffers = &cmdBuffer;
	globUtils.queues.graphics.submit(submit, fence);

	result = device.waitForFences(fence, true);
	if (result != vk::Result::eSuccess)
		throw std::runtime_error("Unable to wait on fence after uploading glyph.");

	vmaDestroyBuffer(globUtils.vma, (VkBuffer)tempBuffer, tempBufferVmaAlloc);
	device.destroy(fence);
	device.destroy(cmdPool);

	auto insertResult = manager.glyphDatas.insert({ id, newGlyphData });
	if (!insertResult.second)
		throw std::runtime_error("DEngine - Vulkan: Unable to insert new glyph.");
}