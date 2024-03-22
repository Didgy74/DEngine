#include "ObjectDataManager.hpp"
#include "GlobUtils.hpp"

#include "DeletionQueue.hpp"

#include <DEngine/Math/Common.hpp>

#include <DEngine/Gfx/impl/Assert.hpp>

#include <string>

using namespace DEngine;
using namespace DEngine::Gfx;
using namespace DEngine::Gfx::Vk;

namespace DEngine::Gfx::Vk {
	void ObjectDataManager_AllocateMemoryAndDescriptors(
		ObjectDataManager& manager,
		GlobUtils const& globUtils,
		int newCapacity,
		DebugUtilsDispatch const* debugUtils)
	{
		auto& device = globUtils.device;
		auto& vma = globUtils.vma;
		auto inFlightCount = globUtils.inFlightCount;
		auto elementSize = manager.elementSize;

		// Allocate the buffer
		vk::BufferCreateInfo buffInfo = {};
		buffInfo.sharingMode = vk::SharingMode::eExclusive;
		buffInfo.size = elementSize * newCapacity * inFlightCount;
		buffInfo.usage = vk::BufferUsageFlagBits::eUniformBuffer;
		VmaAllocationCreateInfo vmaAllocInfo = {};
		vmaAllocInfo.flags = VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_MAPPED_BIT;
		vmaAllocInfo.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_CPU_TO_GPU;
		VmaAllocationInfo vmaAllocResultInfo = {};
		vk::Buffer newBuffer = {};
		VmaAllocation vmaAlloc = {};
		auto vkResult = (vk::Result)vmaCreateBuffer(
			vma,
			(VkBufferCreateInfo const*)&buffInfo,
			&vmaAllocInfo,
			(VkBuffer*)&newBuffer,
			&vmaAlloc,
			&vmaAllocResultInfo);
		if (vkResult != vk::Result::eSuccess)
			throw std::runtime_error("DEngine - Vulkan: VMA failed to allocate memory for object data.");
		if (debugUtils) {
			debugUtils->Helper_SetObjectName(
				device.handle,
				newBuffer,
				"ObjectData - Buffer");
		}

		// Allocate the descriptor-set stuff
		vk::DescriptorPoolSize descrPoolSize = {};
		descrPoolSize.descriptorCount = 1;
		descrPoolSize.type = vk::DescriptorType::eUniformBufferDynamic;
		vk::DescriptorPoolCreateInfo descrPoolInfo = {};
		descrPoolInfo.maxSets = 1;
		descrPoolInfo.poolSizeCount = 1;
		descrPoolInfo.pPoolSizes = &descrPoolSize;
		auto descrPool = device.Create(descrPoolInfo);
		if (debugUtils) {
			debugUtils->Helper_SetObjectName(
				device.handle,
				descrPool,
				"ObjectData - DescrPool");
		}

		vk::DescriptorSetAllocateInfo descrSetAllocInfo{};
		descrSetAllocInfo.descriptorPool = descrPool;
		descrSetAllocInfo.descriptorSetCount = 1;
		descrSetAllocInfo.pSetLayouts = &manager.descrSetLayout;
		vk::DescriptorSet descrSet = {};
		vkResult = device.Alloc(descrSetAllocInfo, &descrSet);
		if (vkResult != vk::Result::eSuccess)
			throw std::runtime_error("DEngine - Vulkan: Unable to allocate descriptor set for object-data.");
		if (debugUtils) {
			debugUtils->Helper_SetObjectName(
				device.handle,
				descrSet,
				"ObjectData - DescrSet");
		}

		// Write to our descriptor set
		vk::DescriptorBufferInfo descrBuffInfo = {};
		descrBuffInfo.buffer = newBuffer;
		descrBuffInfo.offset = 0;
		descrBuffInfo.range = (int)elementSize;
		vk::WriteDescriptorSet write = {};
		write.descriptorCount = 1;
		write.descriptorType = vk::DescriptorType::eUniformBufferDynamic;
		write.dstBinding = 0;
		write.dstSet = descrSet;
		write.pBufferInfo = &descrBuffInfo;
		device.updateDescriptorSets(write, nullptr);

		manager.capacity = newCapacity;
		manager.buffer = newBuffer;
		manager.vmaAlloc = vmaAlloc;
		manager.mappedMem = vmaAllocResultInfo.pMappedData;
		manager.descrPool = descrPool;
		manager.descrSet = descrSet;
	}
}


void ObjectDataManager::Update(
	ObjectDataManager& manager,
	GlobUtils const& globUtils,
	Std::Span<Math::Mat4 const> transforms,
	vk::CommandBuffer cmdBuffer,
	DeletionQueue& delQueue,
	u8 inFlightIndex,
	DebugUtilsDispatch const* debugUtils)
{

	if (transforms.Empty())
		return;

	auto& device = globUtils.device;

	if (transforms.Size() > manager.capacity) {
		if (manager.descrPool != vk::DescriptorPool{}) {
			delQueue.Destroy(manager.descrPool);
		}
		if (manager.buffer != vk::Buffer{}) {
			delQueue.Destroy(manager.vmaAlloc, manager.buffer);
		}


		// Allocate new stuff
		auto newSize = (int)Math::Max((u64)transforms.Size(), (u64)manager.capacity);
		newSize = (int)Math::Max((u64)newSize, (u64)ObjectDataManager::minCapacity);
		newSize *= 2;

		ObjectDataManager_AllocateMemoryAndDescriptors(
			manager,
			globUtils,
			newSize,
			debugUtils);
	}


	auto resourceSetSize = manager.capacity * manager.elementSize;
	auto offset = resourceSetSize * inFlightIndex;
	auto dstResourceSet = (char*)manager.mappedMem + offset;
	for (uSize i = 0; i < transforms.Size(); i += 1) {
		auto const& item = transforms[i];
		char* dst = dstResourceSet + manager.elementSize * i;
		std::memcpy(dst, item.Data(), sizeof(item));
	}

	vk::BufferMemoryBarrier barrier = {};
	barrier.buffer = manager.buffer;
	barrier.offset = offset;
	barrier.size = resourceSetSize;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.srcAccessMask = vk::AccessFlagBits::eHostWrite;
	barrier.dstAccessMask = vk::AccessFlagBits::eUniformRead;
	device.cmdPipelineBarrier(
		cmdBuffer,
		vk::PipelineStageFlagBits::eHost,
		vk::PipelineStageFlagBits::eVertexShader,
		vk::DependencyFlagBits::eByRegion,
		{}, { barrier }, {});
}

bool DEngine::Gfx::Vk::ObjectDataManager::Init(
	ObjectDataManager& manager,
	uSize minUniformBufferOffsetAlignment,
	DeviceDispatch const& device,
	DebugUtilsDispatch const* debugUtils)
{
	vk::Result vkResult{};

	manager.elementSize = Math::Max(
		ObjectDataManager::minElementSize,
		minUniformBufferOffsetAlignment);
	manager.capacity = 0;

	// Create descriptor set layout
	vk::DescriptorSetLayoutBinding objectDataBinding{};
	objectDataBinding.binding = 0;
	objectDataBinding.descriptorCount = 1;
	objectDataBinding.descriptorType = vk::DescriptorType::eUniformBufferDynamic;
	objectDataBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;
	vk::DescriptorSetLayoutCreateInfo descrSetLayoutInfo{};
	descrSetLayoutInfo.bindingCount = 1;
	descrSetLayoutInfo.pBindings = &objectDataBinding;
	auto descrLayout = device.Create(descrSetLayoutInfo);
	if (debugUtils) {
		debugUtils->Helper_SetObjectName(
			device.handle,
			descrLayout,
			"ObjectData - DescrSetLayout");
	}
	manager.descrSetLayout = descrLayout;

	return true;
}