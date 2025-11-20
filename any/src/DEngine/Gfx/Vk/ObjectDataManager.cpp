#include "ObjectDataManager.hpp"
#include "GlobUtils.hpp"

#include "DeletionQueue.hpp"

#include <DEngine/Gfx/impl/Assert.hpp>

#include <string>

import DEngine.Std.Vec;

using namespace DEngine;
using namespace DEngine::Gfx;
using namespace DEngine::Gfx::Vk;

namespace DEngine::Gfx::Vk {
	[[nodiscard]] static auto AllocObjectDataBuffer(
		ObjectDataManager const& manager,
		DeviceDispatch const& device,
		VmaAllocator vma,
		int capacity)
	{
		auto elementSize = manager.ObjectDataUniformElementAlignment();

		vk::BufferCreateInfo buffInfo = {};
		buffInfo.sharingMode = vk::SharingMode::eExclusive;
		buffInfo.size = elementSize * capacity * manager.inFlightCount;
		buffInfo.usage = vk::BufferUsageFlagBits::eUniformBuffer;
		VmaAllocationCreateInfo vmaAllocInfo = {};
		vmaAllocInfo.flags =
			VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
			| VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_MAPPED_BIT;
		vmaAllocInfo.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO;
		return device.CreateBox(vma, buffInfo, vmaAllocInfo);
	}

	[[nodiscard]] static auto CreateObjectDataDescrPool(DeviceDispatch const& device) {
		vk::DescriptorPoolSize descrPoolSize = {};
		descrPoolSize.descriptorCount = 1;
		descrPoolSize.type = ObjectDataManager::objectDataUniformDescrType;
		vk::DescriptorPoolCreateInfo descrPoolInfo = {};
		descrPoolInfo.maxSets = 1;
		descrPoolInfo.poolSizeCount = 1;
		descrPoolInfo.pPoolSizes = &descrPoolSize;
		return device.CreateBox(descrPoolInfo);
	}
	[[nodiscard]] static auto ObjectData_AllocateDescriptorSet(
		ObjectDataManager const& manager,
		DeviceDispatch const& device,
		vk::DescriptorPool descrPool,
		vk::Buffer buffer)
	{
		auto descrLayout = manager.objectDataDescrLayout.Handle();
		vk::DescriptorSetAllocateInfo descrSetAllocInfo = {};
		descrSetAllocInfo.descriptorPool = descrPool;
		descrSetAllocInfo.descriptorSetCount = 1;
		descrSetAllocInfo.pSetLayouts = &descrLayout;
		vk::DescriptorSet descrSet = {};
		auto vkResult = device.Alloc(descrSetAllocInfo, &descrSet);
		if (vkResult != vk::Result::eSuccess) {
			throw std::runtime_error("DEngine - Vulkan: Unable to allocate descriptor set for object-data.");
		}
		auto elementSize = manager.ObjectDataUniformElementAlignment();

		// Write to our descriptor set
		vk::DescriptorBufferInfo descrBuffInfo = {};
		descrBuffInfo.buffer = buffer;
		descrBuffInfo.offset = 0;
		descrBuffInfo.range = (int)elementSize;
		vk::WriteDescriptorSet write = {};
		write.descriptorCount = 1;
		write.descriptorType = ObjectDataManager::objectDataUniformDescrType;
		write.dstBinding = 0;
		write.dstSet = descrSet;
		write.pBufferInfo = &descrBuffInfo;
		device.UpdateDescriptorSets(write, {});
		return descrSet;
	}

	void ObjectData_AllocateMemoryAndDescriptors(
		ObjectDataManager& manager,
		DeviceDispatch const& device,
		VmaAllocator vma,
		int newCapacity,
		DebugUtilsDispatch const* debugUtils)
	{
		auto buffer = AllocObjectDataBuffer(manager, device, vma, newCapacity);
		manager.objectDataResources.buffer = Std::Move(buffer.buffer);
		manager.objectDataResources.allocInfo = buffer.allocInfo;
		if (debugUtils) {
			debugUtils->Helper_SetObjectName(
				device.handle,
				manager.objectDataResources.buffer,
				"ObjectDataManager - ObjectData Buffer");
		}

		manager.objectDataResources.descrPool = CreateObjectDataDescrPool(device);
		if (debugUtils) {
			debugUtils->Helper_SetObjectName(
				device.handle,
				manager.objectDataResources.descrPool,
				"ObjectDataManager - DescrPool");
		}

		manager.objectDataResources.descrSet = ObjectData_AllocateDescriptorSet(
			manager,
			device,
			manager.objectDataResources.descrPool.Handle(),
			manager.objectDataResources.buffer.Handle());
		if (debugUtils) {
			debugUtils->Helper_SetObjectName(
				device.handle,
				manager.objectDataResources.descrSet,
				"ObjectDataManager - DescrSet");
		}
	}
}

vk::DescriptorSetLayoutBinding ObjectDataManager::DescrLayoutBinding()
{
	vk::DescriptorSetLayoutBinding binding = {};
	binding.binding = 0;
	binding.descriptorCount = 1;
	binding.descriptorType = ObjectDataManager::objectDataUniformDescrType;
	binding.stageFlags = vk::ShaderStageFlagBits::eVertex;
	return binding;
}

namespace DEngine::Gfx::Vk {
	// Capacity is in number of guiPlanes that fit in our buffers.
	// The capacity is NOT in byte size.
	void ObjectDataManager_AllocateGuiPlaneResources(
		ObjectDataManager& manager,
		DeviceDispatch const& device,
		VmaAllocator vma,
		int newCapacity,
		Std::AllocRef const& transientAlloc,
		DebugUtilsDispatch const* debugUtils)
	{
		manager.guiPlaneResources.capacity = newCapacity;

		auto objectBuffer = AllocObjectDataBuffer(manager, device, vma, newCapacity);
		manager.guiPlaneResources.objectDataBuffer = Std::Move(objectBuffer.buffer);
		manager.guiPlaneResources.objectDataAllocInfo = objectBuffer.allocInfo;
		if (debugUtils) {
			debugUtils->Helper_SetObjectName(
				device.handle,
				manager.guiPlaneResources.objectDataBuffer,
				"ObjectDataManager - GuiPlane - ObjectData Buffer");
		}

		// Strictly speaking we could combine descr pool with the window-data stuff
		manager.guiPlaneResources.objectDataDescrPool = CreateObjectDataDescrPool(device);
		if (debugUtils) {
			debugUtils->Helper_SetObjectName(
				device.handle,
				manager.guiPlaneResources.objectDataDescrPool,
				"ObjectDataManager - GuiPlane - ObjectData DescrPool");
		}

		manager.guiPlaneResources.objectDataDescrSet = ObjectData_AllocateDescriptorSet(
			manager,
			device,
			manager.guiPlaneResources.objectDataDescrPool.Handle(),
			manager.guiPlaneResources.objectDataBuffer.Handle());
		if (debugUtils) {
			debugUtils->Helper_SetObjectName(
				device.handle,
				manager.guiPlaneResources.objectDataDescrSet,
				"ObjectDataManager - GuiPlane - ObjectData DescrSet");
		}

		vk::Result vkResult = {};
		auto windowDataUniformAlignment = manager.GuiPlaneWindowDataUniformAlignment();

		{
			vk::BufferCreateInfo windowBuffInfo = {};
			windowBuffInfo.sharingMode = vk::SharingMode::eExclusive;
			windowBuffInfo.size = windowDataUniformAlignment * newCapacity * manager.inFlightCount;
			windowBuffInfo.usage = vk::BufferUsageFlagBits::eUniformBuffer;
			VmaAllocationCreateInfo vmaAllocCreateInfo = {};
			vmaAllocCreateInfo.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO;
			vmaAllocCreateInfo.flags =
				VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
				| VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_MAPPED_BIT;
			auto windowBuffer = device.CreateBox(vma, windowBuffInfo, vmaAllocCreateInfo);
			manager.guiPlaneResources.windowBuffer = Std::Move(windowBuffer.buffer);
			manager.guiPlaneResources.windowAllocInfo = windowBuffer.allocInfo;
			if (debugUtils) {
				debugUtils->Helper_SetObjectName(
					device.handle,
					manager.guiPlaneResources.windowBuffer,
					"ObjectDataManager - GuiPlane - WindowData Buffer");
			}
		}

		// Allocate the descriptor pool
		// We need capacity * inFlightCount descriptor sets
		int totalDescriptorSets = newCapacity * manager.inFlightCount;
		vk::DescriptorPoolSize descrPoolSize = {};
		descrPoolSize.descriptorCount = totalDescriptorSets;
		descrPoolSize.type = GuiResourceManager::windowDataUniformDescrType;
		vk::DescriptorPoolCreateInfo descrPoolInfo = {};
		descrPoolInfo.maxSets = totalDescriptorSets;
		descrPoolInfo.poolSizeCount = 1;
		descrPoolInfo.pPoolSizes = &descrPoolSize;
		manager.guiPlaneResources.windowDataDescrPool = device.CreateBox(descrPoolInfo);
		if (debugUtils) {
			debugUtils->Helper_SetObjectName(
				device.handle,
				manager.guiPlaneResources.windowDataDescrPool,
				"ObjectDataManager- GuiPlane - WindowData DescrPool");
		}

		// Allocate all descriptor sets outputs
		manager.guiPlaneResources.windowDataDescrSets.resize(totalDescriptorSets);

		// Create list of descr layouts to pass in
		auto descrLayout = Std::NewVec_Fill(
			transientAlloc,
			totalDescriptorSets,
			manager.guiWindowDescrLayout);
		vk::DescriptorSetAllocateInfo descrSetAllocInfo = {};
		descrSetAllocInfo.descriptorPool = manager.guiPlaneResources.windowDataDescrPool.Handle();
		descrSetAllocInfo.descriptorSetCount = totalDescriptorSets;
		descrSetAllocInfo.pSetLayouts = descrLayout.Data();
		vkResult = device.Alloc(descrSetAllocInfo, manager.guiPlaneResources.windowDataDescrSets.data());
		if (vkResult != vk::Result::eSuccess) {
			throw std::runtime_error("DEngine - Vulkan: Unable to allocate descriptor sets for gui-planes.");
		}

		// Write to all descriptor sets
		auto descrWrites = Std::NewVec_Reserve<vk::WriteDescriptorSet>(
			transientAlloc,
			totalDescriptorSets);
		auto descrBufferInfos = Std::NewVec_Reserve<vk::DescriptorBufferInfo>(
			transientAlloc,
			totalDescriptorSets);

		int counter = 0;
		for (int inFlightIndex = 0; inFlightIndex < manager.inFlightCount; inFlightIndex++) {
			for (int planeIndex = 0; planeIndex < newCapacity; planeIndex++) {
				int descrSetIndex = inFlightIndex * newCapacity + planeIndex;

				vk::DescriptorBufferInfo objectBuffInfo = {};
				objectBuffInfo.buffer = manager.guiPlaneResources.windowBuffer.Handle();
				objectBuffInfo.offset = windowDataUniformAlignment * descrSetIndex;
				objectBuffInfo.range = windowDataUniformAlignment;
				descrBufferInfos.PushBack(objectBuffInfo);

				vk::WriteDescriptorSet objectWrite = {};
				objectWrite.descriptorCount = 1;
				objectWrite.descriptorType = GuiResourceManager::windowDataUniformDescrType;
				objectWrite.dstBinding = 0;
				objectWrite.dstSet = manager.guiPlaneResources.windowDataDescrSets[descrSetIndex];
				objectWrite.pBufferInfo = &descrBufferInfos[descrSetIndex];
				descrWrites.PushBack(objectWrite);

				counter++;
			}
		}

		device.UpdateDescriptorSets(descrWrites.ToSpan(), {});

		if (debugUtils) {
			for (int inFlight = 0; inFlight < manager.inFlightCount; inFlight++) {
				for (int i = 0; i < newCapacity; i++) {
					debugUtils->Helper_SetObjectName(
						device.handle,
						manager.guiPlaneResources.windowDataDescrSets[inFlight * newCapacity + i],
						std::format(
							"ObjectData - GuiPlaneWindowData - InFlight[{}] - DescrSet[{}]",
							inFlight,
							i).c_str());
				}
			}
		}
	}
}

void ObjectDataManager::Update(
	ObjectDataManager& manager,
	DeviceDispatch const& device,
	VmaAllocator vma,
	Std::Span<Math::Mat4 const> transforms,
	Std::Span<SceneGuiPlane const> guiPlanes,
	vk::CommandBuffer cmdBuffer,
	DeletionQueue& delQueue,
	u8 inFlightIndex,
	TransientAllocRef transientAlloc,
	DebugUtilsDispatch const* debugUtils)
{
	// We might have to reallocate memory if our incoming render list is too big for
	// the existing one.
	auto oldObjectDataCapacity = manager.objectDataResources.CapacityCount(manager);
	if (transforms.Size() > oldObjectDataCapacity) {
		if (oldObjectDataCapacity > 0) {
			manager.objectDataResources.QueueDeletion(delQueue);
		}

		// Allocate new stuff
		auto newSize = (int)Std::Max((u64)transforms.Size(), (u64)oldObjectDataCapacity);
		newSize = (int)Std::Max((u64)newSize, (u64)ObjectDataManager::minObjectDataCapacity);
		newSize *= 2;
		ObjectData_AllocateMemoryAndDescriptors(
			manager,
			device,
			vma,
			newSize,
			debugUtils);
		DENGINE_IMPL_GFX_ASSERT(!manager.objectDataResources.buffer.IsNull());
	}

	// Then populate the regular ObjectData uniforms.
	if (!transforms.Empty()) {
		auto& resources = manager.objectDataResources;
		auto elementSize = manager.ObjectDataUniformElementAlignment();
		auto resourceSetSize = resources.CapacityCount(manager) * elementSize;
		auto offset = resourceSetSize * inFlightIndex;
		auto dstResourceSet = resources
			.MappedMemory()
			.Subspan(offset, resourceSetSize);
		for (uSize i = 0; i < transforms.Size(); i += 1) {
			ObjectDataUniform item = {};
			item.transform = transforms[i];
			std::memcpy(
				dstResourceSet.Data() + manager.ObjectDataUniformElementAlignment() * i,
				&item,
				sizeof(item));
		}
		vmaFlushAllocation(
			vma,
			resources.buffer.Alloc(),
			offset,
			transforms.Size() * elementSize);

		vk::BufferMemoryBarrier barrier = {};
		barrier.buffer = resources.buffer.Handle();
		barrier.offset = offset;
		barrier.size = transforms.Size() * elementSize;
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

	// Next we have to do the same for GuiPlane resources. This includes the ObjectData for
	// for GuiPlanes, and also the WindowData uniforms.
	{
		auto oldCapacity = manager.GetGuiPlaneCapacity();
		if (guiPlanes.Size() > oldCapacity) {
			if (oldCapacity > 0) {
				manager.guiPlaneResources.QueueDeletion(delQueue);
			}

			// Allocate new stuff
			auto newSize = (int)Std::Max((u64)guiPlanes.Size(), (u64)oldCapacity);
			newSize = (int)Std::Max((u64)newSize, (u64)ObjectDataManager::minGuiPlanesCapacity);
			newSize *= 2;

			// Note, gotta make this function more generic
			ObjectDataManager_AllocateGuiPlaneResources(
				manager,
				device,
				vma,
				newSize,
				transientAlloc,
				debugUtils);
		}

		if (!guiPlanes.Empty()) {
			// Populate the ObjectData...
			auto elementSize = manager.ObjectDataUniformElementAlignment();
			auto capacity = manager.GetGuiPlaneCapacity();
			auto resourceSetSize = capacity * elementSize;
			auto offset = resourceSetSize * inFlightIndex;
			auto dstResourceSet = manager.guiPlaneResources
				.ObjectDataMappedMemory()
				.Subspan(offset, resourceSetSize);
			for (uSize i = 0; i < guiPlanes.Size(); i += 1) {
				ObjectDataUniform item = {};
				item.transform = guiPlanes[i].transform;
				std::memcpy(
					dstResourceSet.Data() + elementSize * i,
					&item,
					sizeof(item));
			}
			vmaFlushAllocation(
				vma,
				manager.guiPlaneResources.objectDataBuffer.Alloc(),
				offset,
				guiPlanes.Size() * elementSize);

			vk::BufferMemoryBarrier barrier = {};
			barrier.buffer = manager.guiPlaneResources.objectDataBuffer.Handle();
			barrier.offset = offset;
			barrier.size = guiPlanes.Size() * elementSize;
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
		if (!guiPlanes.Empty()) {
			// Then the GuiPlane WindowData uniforms...
			auto uniformBufferMem = manager.GetGuiPlaneWindowDataUniformMappedMemory();
			auto inFlightOffset = manager.GetGuiPlaneWindowDataUniformInFlightOffset(inFlightIndex);
			auto elementSize = GuiResourceManager::WindowShaderUniforms::UniformElementAlignment(
				manager.minUniformBufferOffsetAlignment);
			auto flushSize = guiPlanes.Size() * elementSize;

			for (uSize i = 0; i < guiPlanes.Size(); i += 1) {
				auto const& guiPlane = guiPlanes[i];
				GuiResourceManager::WindowShaderUniforms::PerWindowUniform item = {};
				item.resolution = { (i32)guiPlane.extentPx.width, (i32)guiPlane.extentPx.height };
				std::memcpy(
					uniformBufferMem.Data() + inFlightOffset + elementSize * i,
					&item,
					sizeof(item));
			}
			vmaFlushAllocation(
				vma,
				manager.guiPlaneResources.windowBuffer.Alloc(),
				inFlightOffset,
				flushSize);
		}
	}
}

bool DEngine::Gfx::Vk::ObjectDataManager::Init(
	ObjectDataManager& manager,
	DeviceDispatch const& device,
	uSize inFlightCount,
	vk::DescriptorSetLayout guiWindowDescrLayout,
	DebugUtilsDispatch const* debugUtils)
{
	manager.inFlightCount = inFlightCount;
	manager.guiWindowDescrLayout = guiWindowDescrLayout;
	manager.minUniformBufferOffsetAlignment = device.physDeviceLimits.minUniformBufferOffsetAlignment;

	// Create descriptor set layout
	auto objectDataBinding = DescrLayoutBinding();
	vk::DescriptorSetLayoutCreateInfo descrLayoutInfo = {};
	descrLayoutInfo.bindingCount = 1;
	descrLayoutInfo.pBindings = &objectDataBinding;
	manager.objectDataDescrLayout = device.CreateBox(descrLayoutInfo);
	if (debugUtils) {
		debugUtils->Helper_SetObjectName(
			device.handle,
			manager.objectDataDescrLayout,
			"ObjectData - DescrLayout");
	}

	return true;
}
