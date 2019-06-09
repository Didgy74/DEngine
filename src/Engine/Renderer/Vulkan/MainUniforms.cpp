#include "MainUniforms.hpp"

#include "../RendererData.hpp"

#include <vulkan/vulkan.h>
#include <vulkan/vulkan.hpp>

namespace DRenderer::Vulkan
{
	void ReallocateObjectDataBuffer(MainUniforms& data, size_t objectCount, DeletionQueues& deletionQueues);
}

void DRenderer::Vulkan::ReallocateObjectDataBuffer(MainUniforms& data, size_t objectCount, DeletionQueues& deletionQueues)
{
	data.device.unmapMemory(data.objectDataMemory);
	data.objectDataMappedMem = nullptr;

	deletionQueues.InsertObjectForDeletion(data.objectDataBuffer);
	deletionQueues.InsertObjectForDeletion(data.objectDataMemory);

	size_t newObjectCapacity = Math::CeilToNearestPowerOf2(objectCount);
	newObjectCapacity = Math::Max(newObjectCapacity, MainUniforms::Constants::minimumObjectDataCapacity);

	vk::BufferCreateInfo bufferInfo{};
	bufferInfo.sharingMode = vk::SharingMode::eExclusive;
	bufferInfo.usage = vk::BufferUsageFlagBits::eUniformBuffer;
	bufferInfo.size = newObjectCapacity * data.GetObjectDataSize() * data.resourceSetCount;

	vk::Buffer buffer = data.device.createBuffer(bufferInfo);

	vk::MemoryRequirements memReqs = data.device.getBufferMemoryRequirements(buffer);

	vk::MemoryAllocateInfo allocInfo{};
	allocInfo.allocationSize = memReqs.size;
	allocInfo.memoryTypeIndex = data.memoryTypeIndex;

	vk::DeviceMemory memory = data.device.allocateMemory(allocInfo);

	data.device.bindBufferMemory(buffer, memory, 0);

	void* modelMappedMemory = data.device.mapMemory(memory, 0, memReqs.size);

	data.objectDataBuffer = buffer;
	data.objectDataMemory = memory;
	data.objectDataMappedMem = static_cast<uint8_t*>(modelMappedMemory);
	data.objectDataResourceSetSize = newObjectCapacity * data.GetObjectDataSize();

	if constexpr (Core::debugLevel >= 2)
	{
		if (Core::GetData().debugData.useDebugging)
		{
			vk::DebugUtilsObjectNameInfoEXT objectNameInfo{};

			objectNameInfo.objectType = vk::ObjectType::eDeviceMemory;
			objectNameInfo.objectHandle = (uint64_t)(VkDeviceMemory)memory;
			objectNameInfo.pObjectName = "Object Data Memory";
			data.device.setDebugUtilsObjectNameEXT(objectNameInfo);

			objectNameInfo.objectType = vk::ObjectType::eBuffer;
			objectNameInfo.objectHandle = (uint64_t)(VkBuffer)buffer;
			objectNameInfo.pObjectName = "Object Data Buffer";
			data.device.setDebugUtilsObjectNameEXT(objectNameInfo);
		}
	}

	
}

void DRenderer::Vulkan::MainUniforms::Init_CreateDescriptorSetAndPipelineLayout(MainUniforms& data, vk::Device device)
{
	data.device = device;

	vk::DescriptorSetLayoutBinding cameraBindingInfo{};
	cameraBindingInfo.binding = 0;
	cameraBindingInfo.descriptorCount = 1;
	cameraBindingInfo.descriptorType = vk::DescriptorType::eUniformBuffer;
	cameraBindingInfo.stageFlags = vk::ShaderStageFlagBits::eVertex;

	vk::DescriptorSetLayoutBinding modelBindingInfo{};
	modelBindingInfo.binding = 1;
	modelBindingInfo.descriptorCount = 1;
	modelBindingInfo.descriptorType = vk::DescriptorType::eUniformBufferDynamic;
	modelBindingInfo.stageFlags = vk::ShaderStageFlagBits::eVertex;

	std::array bindingInfos{ cameraBindingInfo, modelBindingInfo };

	vk::DescriptorSetLayoutCreateInfo layoutCreateInfo{};
	layoutCreateInfo.bindingCount = uint32_t(bindingInfos.size());
	layoutCreateInfo.pBindings = bindingInfos.data();

	data.descrSetLayout = device.createDescriptorSetLayout(layoutCreateInfo);

	vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &data.descrSetLayout;
	data.pipelineLayout = data.device.createPipelineLayout(pipelineLayoutInfo);

	if constexpr (Core::debugLevel >= 2)
	{
		if (!data.descrSetLayout)
		{
			Core::LogDebugMessage("Init error - Couldn't generate main uniform DescriptorSetLayout.");
			std::abort();
		}

		if (!data.pipelineLayout)
		{
			Core::LogDebugMessage("Init error - Couldn't generate main uniform PipelineLayout.");
			std::abort();
		}

		// Give debug names to our layout
		if (Core::GetData().debugData.useDebugging)
		{
			vk::DebugUtilsObjectNameInfoEXT objectNameInfo{};
			objectNameInfo.objectType = vk::ObjectType::eDescriptorSetLayout;
			objectNameInfo.objectHandle = (uint64_t)(VkDescriptorSetLayout)data.descrSetLayout;
			objectNameInfo.pObjectName = "MainUniforms DescriptorSetLayout";
			device.setDebugUtilsObjectNameEXT(objectNameInfo);

			objectNameInfo.objectType = vk::ObjectType::ePipelineLayout;
			objectNameInfo.objectHandle = (uint64_t)(VkPipelineLayout)data.pipelineLayout;
			objectNameInfo.pObjectName = "MainUniforms PipelineLayout";
			device.setDebugUtilsObjectNameEXT(objectNameInfo);
		}
	}
}

void DRenderer::Vulkan::MainUniforms::Init_AllocateDescriptorSets(
	MainUniforms& data,
	uint32_t resourceSetCount)
{
	data.resourceSetCount = resourceSetCount;

	vk::DescriptorPoolSize cameraUBOPoolSize{};
	cameraUBOPoolSize.descriptorCount = resourceSetCount;
	cameraUBOPoolSize.type = vk::DescriptorType::eUniformBuffer;

	vk::DescriptorPoolSize modelUBOPoolSize{};
	modelUBOPoolSize.descriptorCount = resourceSetCount;
	modelUBOPoolSize.type = vk::DescriptorType::eUniformBufferDynamic;

	std::array poolSizes{ cameraUBOPoolSize, modelUBOPoolSize };

	vk::DescriptorPoolCreateInfo poolCreateInfo{};
	poolCreateInfo.maxSets = resourceSetCount;
	poolCreateInfo.poolSizeCount = uint32_t(poolSizes.size());
	poolCreateInfo.pPoolSizes = poolSizes.data();

	vk::DescriptorPool descriptorSetPool = data.device.createDescriptorPool(poolCreateInfo);

	std::array<vk::DescriptorSetLayout, Vulkan::Constants::maxResourceSets> descriptorSetLayouts{};
	descriptorSetLayouts.fill(data.descrSetLayout);

	vk::DescriptorSetAllocateInfo allocInfo{};
	allocInfo.descriptorPool = descriptorSetPool;
	allocInfo.descriptorSetCount = resourceSetCount;
	allocInfo.pSetLayouts = descriptorSetLayouts.data();

	assert(resourceSetCount <= Vulkan::Constants::maxResourceSets);

	vk::Result success = data.device.allocateDescriptorSets(&allocInfo, data.descrSets.data());
	assert(success == vk::Result::eSuccess);

	if constexpr (Core::debugLevel >= 2)
	{
		if (Core::GetData().debugData.useDebugging)
		{
			vk::DebugUtilsObjectNameInfoEXT objectNameInfo{};
			objectNameInfo.objectType = vk::ObjectType::eDescriptorPool;
			objectNameInfo.objectHandle = (uint64_t)(VkDescriptorPool)descriptorSetPool;
			objectNameInfo.pObjectName = "Primary Descriptor Pool";
			data.device.setDebugUtilsObjectNameEXT(objectNameInfo);

			for (size_t i = 0; i < data.descrSets.size(); i++)
			{
				objectNameInfo.objectType = vk::ObjectType::eDescriptorSet;
				objectNameInfo.objectHandle = (uint64_t)(VkDescriptorSet)data.descrSets[i];
				std::string name = "Primary Descriptor Set #" + std::to_string(i);
				objectNameInfo.pObjectName = name.data();
				data.device.setDebugUtilsObjectNameEXT(objectNameInfo);
			}
		}
	}

	// Setup the DescriptorUpdateTemplates for updating the descriptors dynamically
	vk::DescriptorUpdateTemplateCreateInfo descriptorUpdateInfo{};
	descriptorUpdateInfo.templateType = vk::DescriptorUpdateTemplateType::eDescriptorSet;
	descriptorUpdateInfo.descriptorSetLayout = data.descrSetLayout;
	descriptorUpdateInfo.pipelineLayout = data.pipelineLayout;
	descriptorUpdateInfo.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
	descriptorUpdateInfo.set = 0;

	vk::DescriptorUpdateTemplateEntry entry{};
	entry.descriptorType = vk::DescriptorType::eUniformBufferDynamic;
	entry.descriptorCount = 1;
	entry.dstArrayElement = 0;
	entry.dstBinding = MainUniforms::Constants::objectDataBindingIndex;
	entry.offset = 0;

	descriptorUpdateInfo.descriptorUpdateEntryCount = 1;
	descriptorUpdateInfo.pDescriptorUpdateEntries = &entry;

	for (size_t i = 0; i < data.resourceSetCount; i++)
	{
		entry.offset = i * sizeof(data.descrUpdateData[i]);
		data.descrUpdateTemplates[i] = data.device.createDescriptorUpdateTemplate(descriptorUpdateInfo);
	}
}

void DRenderer::Vulkan::MainUniforms::Init_BuildBuffers(
	MainUniforms& data,
	const MemoryTypes& memoryInfo,
	const vk::PhysicalDeviceMemoryProperties& memProperties,
	const vk::PhysicalDeviceLimits& limits)
{
	if (memoryInfo.deviceLocalAndHostVisible != MemoryTypes::invalidIndex)
		data.memoryTypeIndex = memoryInfo.deviceLocalAndHostVisible;
	else
		data.memoryTypeIndex = memoryInfo.hostVisible;
	data.nonCoherentAtomSize = limits.minUniformBufferOffsetAlignment;




	// SETUP CAMERA STUFF
	size_t cameraUBOByteLength = sizeof(std::array<float, 16>);
	cameraUBOByteLength = Math::CeilToNearestMultiple(cameraUBOByteLength, limits.minUniformBufferOffsetAlignment);
	// Since we are mapping the memory, it needs to be a multiple of atom size to allow flushing.
	size_t cameraResourceSetSize = Math::CeilToNearestMultiple(cameraUBOByteLength, limits.nonCoherentAtomSize);

	vk::BufferCreateInfo camBufferInfo{};
	camBufferInfo.usage = vk::BufferUsageFlagBits::eUniformBuffer;
	camBufferInfo.sharingMode = vk::SharingMode::eExclusive;
	camBufferInfo.size = cameraResourceSetSize * data.resourceSetCount;

	vk::Buffer camBuffer = data.device.createBuffer(camBufferInfo);

	vk::MemoryRequirements camMemReqs = data.device.getBufferMemoryRequirements(camBuffer);

	vk::MemoryAllocateInfo camAllocInfo{};
	camAllocInfo.allocationSize = camMemReqs.size;

	camAllocInfo.memoryTypeIndex = data.memoryTypeIndex;
	assert(camAllocInfo.memoryTypeIndex != std::numeric_limits<uint32_t>::max());

	vk::DeviceMemory camMem = data.device.allocateMemory(camAllocInfo);

	data.device.bindBufferMemory(camBuffer, camMem, 0);

	void* camMappedMemory = data.device.mapMemory(camMem, 0, camAllocInfo.allocationSize);

	data.cameraBuffersMem = camMem;
	data.cameraBuffer = camBuffer;
	data.cameraDataResourceSetSize = cameraResourceSetSize;
	data.cameraMemoryMap = static_cast<uint8_t*>(camMappedMemory);


	// SETUP PER OBJECT STUFF
	size_t singleUBOByteLength = sizeof(std::array<float, 16>);
	singleUBOByteLength = Math::CeilToNearestMultiple(singleUBOByteLength, limits.minUniformBufferOffsetAlignment);
	size_t objectDataResourceSetSize = singleUBOByteLength * Constants::minimumObjectDataCapacity;
	// Since we are mapping the memory, it needs to be a multiple of atom size to allow flushing.
	objectDataResourceSetSize = Math::CeilToNearestMultiple(objectDataResourceSetSize, limits.nonCoherentAtomSize);

	vk::BufferCreateInfo modelBufferInfo{};
	modelBufferInfo.sharingMode = vk::SharingMode::eExclusive;
	modelBufferInfo.usage = vk::BufferUsageFlagBits::eUniformBuffer;
	modelBufferInfo.size = objectDataResourceSetSize * data.resourceSetCount;

	vk::Buffer modelBuffer = data.device.createBuffer(modelBufferInfo);

	vk::MemoryRequirements modelMemReqs = data.device.getBufferMemoryRequirements(modelBuffer);

	vk::MemoryAllocateInfo modelAllocInfo{};
	modelAllocInfo.allocationSize = modelMemReqs.size;
	modelAllocInfo.memoryTypeIndex = data.memoryTypeIndex;

	vk::DeviceMemory modelMem = data.device.allocateMemory(modelAllocInfo);

	data.device.bindBufferMemory(modelBuffer, modelMem, 0);

	void* modelMappedMemory = data.device.mapMemory(modelMem, 0, modelBufferInfo.size);


	data.objectDataBuffer = modelBuffer;
	data.objectDataMemory = modelMem;
	data.objectDataMappedMem = static_cast<uint8_t*>(modelMappedMemory);
	data.objectDataSize = singleUBOByteLength;
	data.objectDataResourceSetSize = objectDataResourceSetSize;

	if constexpr (Core::debugLevel >= 2)
	{
		if (Core::GetData().debugData.useDebugging)
		{
			vk::DebugUtilsObjectNameInfoEXT objectNameInfo{};

			objectNameInfo.objectType = vk::ObjectType::eDeviceMemory;
			objectNameInfo.objectHandle = (uint64_t)(VkDeviceMemory)camMem;
			objectNameInfo.pObjectName = "Camera Data Memory";
			data.device.setDebugUtilsObjectNameEXT(objectNameInfo);

			objectNameInfo.objectType = vk::ObjectType::eBuffer;
			objectNameInfo.objectHandle = (uint64_t)(VkBuffer)camBuffer;
			objectNameInfo.pObjectName = "Camera Data Buffer";
			data.device.setDebugUtilsObjectNameEXT(objectNameInfo);

			objectNameInfo.objectType = vk::ObjectType::eDeviceMemory;
			objectNameInfo.objectHandle = (uint64_t)(VkDeviceMemory)modelMem;
			objectNameInfo.pObjectName = "Object Data Memory";
			data.device.setDebugUtilsObjectNameEXT(objectNameInfo);

			objectNameInfo.objectType = vk::ObjectType::eBuffer;
			objectNameInfo.objectHandle = (uint64_t)(VkBuffer)modelBuffer;
			objectNameInfo.pObjectName = "Object Data Buffer";
			data.device.setDebugUtilsObjectNameEXT(objectNameInfo);
		}
	}
}

void DRenderer::Vulkan::MainUniforms::Init_ConfigureDescriptors(MainUniforms& data)
{
	// Set up camera descriptor stuff
	std::array<vk::WriteDescriptorSet, Vulkan::Constants::maxResourceSets> camDescWrites{};
	std::array<vk::DescriptorBufferInfo, Vulkan::Constants::maxResourceSets> camBufferInfos{};

	for (size_t i = 0; i < data.resourceSetCount; i++)
	{
		vk::WriteDescriptorSet& camWrite = camDescWrites[i];
		camWrite.dstSet = data.descrSets[i];
		camWrite.dstBinding = MainUniforms::Constants::cameraDataBindingIndex;
		camWrite.descriptorType = vk::DescriptorType::eUniformBuffer;

		vk::DescriptorBufferInfo& camBufferInfo = camBufferInfos[i];
		camBufferInfo.buffer = data.cameraBuffer;
		camBufferInfo.offset = data.GetCameraResourceSetOffset(i);
		camBufferInfo.range = data.GetCameraResourceSetSize();

		camWrite.descriptorCount = 1;
		camWrite.pBufferInfo = &camBufferInfo;
	}

	// Set up object data descriptors
	std::array<vk::WriteDescriptorSet, Vulkan::Constants::maxResourceSets> modelDescWrites{};
	std::array<vk::DescriptorBufferInfo, Vulkan::Constants::maxResourceSets> modelBufferInfos{};

	for (size_t i = 0; i < data.resourceSetCount; i++)
	{
		vk::WriteDescriptorSet& modelWrite = modelDescWrites[i];
		modelWrite.dstSet = data.descrSets[i];
		modelWrite.dstBinding = MainUniforms::Constants::objectDataBindingIndex;
		modelWrite.descriptorType = vk::DescriptorType::eUniformBufferDynamic;
		modelWrite.descriptorCount = 1;

		vk::DescriptorBufferInfo& modelBufferInfo = modelBufferInfos[i];
		modelBufferInfo.buffer = data.objectDataBuffer;
		modelBufferInfo.range = data.GetObjectDataSize();
		modelBufferInfo.offset = data.GetObjectDataResourceSetOffset(i);

		modelWrite.pBufferInfo = &modelBufferInfo;
	}

	std::array<vk::WriteDescriptorSet, Vulkan::Constants::maxResourceSets * 2> totalDescWrites{};
	size_t loopCounter = 0;
	for (size_t i = 0; i < data.resourceSetCount; i++, loopCounter++)
		totalDescWrites[loopCounter] = camDescWrites[i];
	for (size_t i = 0; i < data.resourceSetCount; i++, loopCounter++)
		totalDescWrites[loopCounter] = modelDescWrites[i];

	data.device.updateDescriptorSets(vk::ArrayProxy<const vk::WriteDescriptorSet>(loopCounter, totalDescWrites.data()), {});
}

void DRenderer::Vulkan::MainUniforms::Update(MainUniforms& data, size_t objectCount, DeletionQueues& deletionQueues)
{
	if (objectCount > data.GetObjectDataCapacity())
		ReallocateObjectDataBuffer(data, objectCount, deletionQueues);

	
}

void DRenderer::Vulkan::MainUniforms::Terminate(MainUniforms& data)
{
	// Clean up main uniforms
	data.device.unmapMemory(data.cameraBuffersMem);
	data.device.unmapMemory(data.objectDataMemory);
	data.device.destroyBuffer(data.cameraBuffer);
	data.device.freeMemory(data.cameraBuffersMem);
	data.device.destroyBuffer(data.objectDataBuffer);
	data.device.freeMemory(data.objectDataMemory);

	// Clean up main descriptors
	data.device.destroyDescriptorSetLayout(data.descrSetLayout);
	data.device.destroyDescriptorPool(data.descrSetPool);
}

uint8_t* DRenderer::Vulkan::MainUniforms::GetObjectDataResourceSet(uint32_t resourceSet)
{
	assert(resourceSet < resourceSetCount);
	return objectDataMappedMem + GetObjectDataResourceSetOffset(resourceSet);
}

size_t DRenderer::Vulkan::MainUniforms::GetObjectDataResourceSetOffset(uint32_t resourceSet) const
{
	assert(resourceSet < resourceSetCount);
	return GetObjectDataResourceSetSize()* resourceSet;
}

size_t DRenderer::Vulkan::MainUniforms::GetObjectDataResourceSetSize() const
{
	return objectDataResourceSetSize;
}

size_t DRenderer::Vulkan::MainUniforms::GetObjectDataDynamicOffset(size_t modelDataIndex) const
{
	if constexpr (Core::debugLevel >= 2)
	{
		const auto& data = Core::GetData();
		if (data.debugData.useDebugging)
		{
			if (modelDataIndex >= GetObjectDataCapacity())
			{
				Core::LogDebugMessage("Error. Trying to access object model matrix outside object data buffer range.");
				std::abort();
			}
		}
	}
	return objectDataSize * modelDataIndex;
}

uint8_t* DRenderer::Vulkan::MainUniforms::GetCameraBufferResourceSet(uint32_t resourceSet)
{
	assert(resourceSet < resourceSetCount);
	return cameraMemoryMap + GetCameraResourceSetOffset(resourceSet);
}

size_t DRenderer::Vulkan::MainUniforms::GetCameraResourceSetOffset(uint32_t resourceSet) const
{
	assert(resourceSet < resourceSetCount);
	return GetCameraResourceSetSize()* resourceSet;
}

size_t DRenderer::Vulkan::MainUniforms::GetCameraResourceSetSize() const
{
	return cameraDataResourceSetSize;
}

size_t DRenderer::Vulkan::MainUniforms::GetObjectDataSize() const
{
	return objectDataSize;
}

size_t DRenderer::Vulkan::MainUniforms::GetObjectDataCapacity() const
{
	return GetObjectDataResourceSetSize() / GetObjectDataSize();
}