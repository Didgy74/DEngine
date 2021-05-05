#include "ObjectDataManager.hpp"
#include "GlobUtils.hpp"

#include <DEngine/Math/Common.hpp>

#include <DEngine/Gfx/detail/Assert.hpp>

#include <string>

void DEngine::Gfx::Vk::ObjectDataManager::HandleResizeEvent(
	ObjectDataManager& manager,
	GlobUtils const& globUtils,
	uSize dataCount)
{
	vk::Result vkResult{};

	if (dataCount <= manager.capacity)
		return;

	// Queue previous stuff for deletion
	globUtils.delQueue.Destroy(manager.descrPool);
	globUtils.delQueue.Destroy(manager.vmaAlloc, manager.buffer);

	manager.capacity *= 2;

	// Allocate the buffer
	vk::BufferCreateInfo buffInfo{};
	buffInfo.sharingMode = vk::SharingMode::eExclusive;
	buffInfo.size = manager.elementSize * manager.capacity * globUtils.inFlightCount;
	buffInfo.usage = vk::BufferUsageFlagBits::eUniformBuffer;
	VmaAllocationCreateInfo vmaAllocInfo{};
	vmaAllocInfo.flags = VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_MAPPED_BIT;
	vmaAllocInfo.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_CPU_TO_GPU;
	VmaAllocationInfo vmaAllocResultInfo{};
	vkResult = (vk::Result)vmaCreateBuffer(
		globUtils.vma,
		(VkBufferCreateInfo const*)&buffInfo,
		&vmaAllocInfo,
		(VkBuffer*)&manager.buffer,
		&manager.vmaAlloc,
		&vmaAllocResultInfo);
	if (vkResult != vk::Result::eSuccess)
		throw std::runtime_error("DEngine - Vulkan: VMA failed to allocate memory for object data.");
	manager.mappedMem = vmaAllocResultInfo.pMappedData;
	if (globUtils.UsingDebugUtils())
	{
		globUtils.debugUtils.Helper_SetObjectName(
			globUtils.device.handle,
			manager.buffer,
			"ObjectData - Buffer");
	}


	// Allocate the descriptor-set stuff
	vk::DescriptorPoolSize descrPoolSize{};
	descrPoolSize.descriptorCount = 1;
	descrPoolSize.type = vk::DescriptorType::eUniformBufferDynamic;

	vk::DescriptorPoolCreateInfo descrPoolInfo{};
	descrPoolInfo.maxSets = 1;
	descrPoolInfo.poolSizeCount = 1;
	descrPoolInfo.pPoolSizes = &descrPoolSize;

	manager.descrPool = globUtils.device.createDescriptorPool(descrPoolInfo);
	if (globUtils.UsingDebugUtils())
	{
		globUtils.debugUtils.Helper_SetObjectName(
			globUtils.device.handle,
			manager.descrPool,
			"ObjectData - DescrPool");
	}

	vk::DescriptorSetAllocateInfo descrSetAllocInfo{};
	descrSetAllocInfo.descriptorPool = manager.descrPool;
	descrSetAllocInfo.descriptorSetCount = 1;
	descrSetAllocInfo.pSetLayouts = &manager.descrSetLayout;
	vkResult = globUtils.device.allocateDescriptorSets(descrSetAllocInfo, &manager.descrSet);
	if (vkResult != vk::Result::eSuccess)
		throw std::runtime_error("DEngine - Vulkan: Unable to allocate descriptor set for object-data.");
	if (globUtils.UsingDebugUtils())
	{
		globUtils.debugUtils.Helper_SetObjectName(
			globUtils.device.handle,
			manager.descrSet,
			"ObjectData - DescrSet");
	}

	// Write to the descriptor set
	vk::DescriptorBufferInfo descrBuffInfo{};
	descrBuffInfo.buffer = manager.buffer;
	descrBuffInfo.offset = 0;
	descrBuffInfo.range = manager.minElementSize;

	vk::WriteDescriptorSet write{};
	write.descriptorCount = 1;
	write.descriptorType = vk::DescriptorType::eUniformBufferDynamic;
	write.dstBinding = 0;
	write.dstSet = manager.descrSet;
	write.pBufferInfo = &descrBuffInfo;
	globUtils.device.updateDescriptorSets(write, nullptr);

}

void DEngine::Gfx::Vk::ObjectDataManager::Update(
	ObjectDataManager& manager,
	Std::Span<Math::Mat4 const> transforms,
	u8 currentInFlightIndex)
{
	DENGINE_DETAIL_GFX_ASSERT(transforms.Size() <= manager.capacity);

	char* dstResourceSet = (char*)manager.mappedMem + manager.capacity * manager.elementSize * currentInFlightIndex;
	for (uSize i = 0; i < transforms.Size(); i += 1)
	{
		char* dst = dstResourceSet + manager.elementSize * i;
		std::memcpy(dst, transforms[i].Data(), 64);
	}
}

bool DEngine::Gfx::Vk::ObjectDataManager::Init(
	ObjectDataManager& manager,
	uSize minUniformBufferOffsetAlignment,
	u8 inFlightCount,
	VmaAllocator vma,
	DeviceDispatch const& device,
	DebugUtilsDispatch const* debugUtils)
{
	vk::Result vkResult{};

	manager.elementSize = Math::Max(
		ObjectDataManager::minElementSize,
		minUniformBufferOffsetAlignment);
	manager.capacity = ObjectDataManager::minCapacity;

	// Allocate the buffer
	vk::BufferCreateInfo buffInfo{};
	buffInfo.sharingMode = vk::SharingMode::eExclusive;
	buffInfo.size = manager.elementSize * manager.capacity * inFlightCount;
	buffInfo.usage = vk::BufferUsageFlagBits::eUniformBuffer;
	VmaAllocationCreateInfo vmaAllocInfo{};
	vmaAllocInfo.flags = VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_MAPPED_BIT;
	vmaAllocInfo.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_CPU_TO_GPU;
	VmaAllocationInfo vmaAllocResultInfo{};
	vkResult = (vk::Result)vmaCreateBuffer(
		vma,
		(VkBufferCreateInfo const*)&buffInfo,
		&vmaAllocInfo,
		(VkBuffer*)&manager.buffer,
		&manager.vmaAlloc,
		&vmaAllocResultInfo);
	if (vkResult != vk::Result::eSuccess)
		throw std::runtime_error("DEngine, Vulkan: VMA failed to allocate memory for object data.");
	manager.mappedMem = vmaAllocResultInfo.pMappedData;
	if (debugUtils)
	{
		debugUtils->Helper_SetObjectName(
			device.handle,
			manager.buffer,
			"ObjectData - Buffer");
	}

	// Create descriptor set layout
	vk::DescriptorSetLayoutBinding objectDataBinding{};
	objectDataBinding.binding = 0;
	objectDataBinding.descriptorCount = 1;
	objectDataBinding.descriptorType = vk::DescriptorType::eUniformBufferDynamic;
	objectDataBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;
	vk::DescriptorSetLayoutCreateInfo descrSetLayoutInfo{};
	descrSetLayoutInfo.bindingCount = 1;
	descrSetLayoutInfo.pBindings = &objectDataBinding;
	manager.descrSetLayout = device.createDescriptorSetLayout(descrSetLayoutInfo);
	if (debugUtils)
	{
		debugUtils->Helper_SetObjectName(
			device.handle,
			manager.descrSetLayout,
			"ObjectData - DescrSetLayout");
	}

	// Allocate the descriptor-set stuff
	vk::DescriptorPoolSize descrPoolSize{};
	descrPoolSize.descriptorCount = 1;
	descrPoolSize.type = vk::DescriptorType::eUniformBufferDynamic;

	vk::DescriptorPoolCreateInfo descrPoolInfo{};
	descrPoolInfo.maxSets = 1;
	descrPoolInfo.poolSizeCount = 1;
	descrPoolInfo.pPoolSizes = &descrPoolSize;

	manager.descrPool = device.createDescriptorPool(descrPoolInfo);
	if (debugUtils)
	{
		debugUtils->Helper_SetObjectName(
			device.handle,
			manager.descrPool,
			"ObjectData - DescrPool");
	}

	vk::DescriptorSetAllocateInfo descrSetAllocInfo{};
	descrSetAllocInfo.descriptorPool = manager.descrPool;
	descrSetAllocInfo.descriptorSetCount = 1;
	descrSetAllocInfo.pSetLayouts = &manager.descrSetLayout;
	vkResult = device.allocateDescriptorSets(descrSetAllocInfo, &manager.descrSet);
	if (vkResult != vk::Result::eSuccess)
		throw std::runtime_error("DEngine - Vulkan: Unable to allocate descriptor set for object-data.");
	if (debugUtils)
	{
		debugUtils->Helper_SetObjectName(
			device.handle,
			manager.descrSet,
			"ObjectData - DescrSet");
	}

	// Write to the descriptor set
	vk::DescriptorBufferInfo descrBuffInfo{};
	descrBuffInfo.buffer = manager.buffer;
	descrBuffInfo.offset = 0;
	descrBuffInfo.range = manager.minElementSize;

	vk::WriteDescriptorSet write{};
	write.descriptorCount = 1;
	write.descriptorType = vk::DescriptorType::eUniformBufferDynamic;
	write.dstBinding = 0;
	write.dstSet = manager.descrSet;
	write.pBufferInfo = &descrBuffInfo;
	device.updateDescriptorSets(write, nullptr);

	return true;
}