#include "AssetSystem.hpp"

#include "DRenderer/MeshDocument.hpp"

#include "VulkanData.hpp"

namespace DRenderer::Vulkan::AssetSystem
{
	struct VBOTransferJob;

	VBO VBOFromMeshDoc(vk::Device device, const MemoryTypes& memInfo, const MeshDoc& in);
	VBO VBOFromMeshDoc_ForTransfer(
		vk::Device device,
		const MeshDoc& in, 
		std::byte* buffer);

	void LoadVBO_HostVisibleOnly(
		AssetData& assetSystem,
		const Core::PrepareRenderingEarlyParams& in,
		const std::vector<std::optional<DRenderer::MeshDoc>>& meshDocList);

	void LoadVBO_WithTransfer(
		AssetData& assetSystem,
		const MemoryTypes& memTypes,
		const QueueInfo& queueInfo,
		DeletionQueues& deletionQueues,
		const std::vector<size_t>& meshIDList,
		const std::vector<std::optional<MeshDoc>>& meshDocList);
}

struct DRenderer::Vulkan::AssetSystem::VBOTransferJob
{
	size_t srcBufferOffset = 0;
	vk::Buffer dst = nullptr;
	size_t size = 0;
};

size_t DRenderer::Vulkan::VertexBufferObject::GetByteOffset(VertexBufferObject::Attribute attribute) const
{
	size_t offset = 0;
	for (size_t i = 0; i < static_cast<size_t>(attribute); i++)
		offset += static_cast<size_t>(attributeSizes[i]);
	return offset;
}

size_t& DRenderer::Vulkan::VertexBufferObject::GetAttrSize(Attribute attr)
{
	return attributeSizes[static_cast<size_t>(attr)];
}

size_t DRenderer::Vulkan::VertexBufferObject::GetAttrSize(Attribute attr) const
{
	return attributeSizes[static_cast<size_t>(attr)];
}

size_t DRenderer::Vulkan::VertexBufferObject::GetTotalBufferSize() const
{
	size_t sum = 0;
	for (size_t i = 0; i < attributeSizes.size(); i++)
		sum += attributeSizes[i];
	return sum;
}

void DRenderer::Vulkan::AssetSystem::Init(
	vk::Device device,
	const QueueInfo& queueInfo,
	uint32_t resourceSetCount,
	AssetData& assetSystem)
{
	assetSystem.device = device;
	assetSystem.setCount = resourceSetCount;

	// Set up semaphores
	assetSystem.vboLoadingFinished.resize(resourceSetCount);
	assetSystem.transferDoneFence.resize(resourceSetCount);
	for (size_t i = 0; i < resourceSetCount; i++)
	{
		assetSystem.vboLoadingFinished[i] = device.createSemaphore({});

		vk::FenceCreateInfo fenceInfo{};
		fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;

		assetSystem.transferDoneFence[i] = device.createFence(fenceInfo);

		if constexpr (Core::debugLevel >= 2)
		{
			if (Core::GetData().debugData.useDebugging)
			{
				vk::DebugUtilsObjectNameInfoEXT objectNameInfo{};
				objectNameInfo.objectType = vk::ObjectType::eSemaphore;
				objectNameInfo.objectHandle = (uint64_t)(VkSemaphore)assetSystem.vboLoadingFinished[i];
				std::string objectName = "Semaphore - Asset Load Finished #" + std::to_string(i);
				objectNameInfo.pObjectName = objectName.data();
				device.setDebugUtilsObjectNameEXT(objectNameInfo);

				objectNameInfo.objectType = vk::ObjectType::eFence;
				objectNameInfo.objectHandle = (uint64_t)(VkFence)assetSystem.transferDoneFence[i];
				std::string objectName2 = "Fence - Asset Load Finished #" + std::to_string(i);
				objectNameInfo.pObjectName = objectName2.data();
				device.setDebugUtilsObjectNameEXT(objectNameInfo);
			}
		}
	}

	// Set up command buffers
	vk::CommandPoolCreateInfo poolInfo{};
	poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
	poolInfo.queueFamilyIndex = queueInfo.transfer.familyIndex;

	assetSystem.cmdPool = device.createCommandPool(poolInfo);
	if constexpr (Core::debugLevel >= 2)
	{
		if (Core::GetData().debugData.useDebugging)
		{
			vk::DebugUtilsObjectNameInfoEXT objectNameInfo{};
			objectNameInfo.objectType = vk::ObjectType::eCommandPool;
			objectNameInfo.objectHandle = (uint64_t)(VkCommandPool)assetSystem.cmdPool;
			std::string objectName = "Transfer Command Pool";
			objectNameInfo.pObjectName = objectName.data();
			device.setDebugUtilsObjectNameEXT(objectNameInfo);
		}
	}

	vk::CommandBufferAllocateInfo cmdAllocInfo{};
	cmdAllocInfo.commandPool = assetSystem.cmdPool;
	cmdAllocInfo.commandBufferCount = resourceSetCount;

	assetSystem.cmdBuffers = device.allocateCommandBuffers(cmdAllocInfo);
	for (size_t i = 0; i < resourceSetCount; i++)
	{
		if constexpr (Core::debugLevel >= 2)
		{
			if (Core::GetData().debugData.useDebugging)
			{
				vk::DebugUtilsObjectNameInfoEXT objectNameInfo{};
				objectNameInfo.objectType = vk::ObjectType::eCommandBuffer;
				objectNameInfo.objectHandle = (uint64_t)(VkCommandBuffer)assetSystem.cmdBuffers[i];
				std::string objectName = "Transfer Command Buffer #" + std::to_string(i);
				objectNameInfo.pObjectName = objectName.data();
				device.setDebugUtilsObjectNameEXT(objectNameInfo);
			}
		}
	}
}

void DRenderer::Vulkan::AssetSystem::Terminate(AssetData& assetData)
{
	for (auto& [key, vbo] : assetData.vboDatabase)
	{
		assetData.device.freeMemory(vbo.deviceMemory);
		assetData.device.destroyBuffer(vbo.buffer);
	}
	assetData.vboDatabase.clear();

	assetData.device.destroyCommandPool(assetData.cmdPool);
	
	for (auto item : assetData.vboLoadingFinished)
		assetData.device.destroySemaphore(item);
	for (auto item : assetData.transferDoneFence)
		assetData.device.destroyFence(item);

	assetData.waitSemaphoreQueue.clear();
	assetData.waitStageFlagsQueue.clear();
}

void DRenderer::Vulkan::AssetSystem::LoadAssets(
	AssetData& assetSystem, 
	const MemoryTypes& memTypes,
	const QueueInfo& queueInfo,
	DeletionQueues& deletionQueues,
	const Core::PrepareRenderingEarlyParams& in)
{
	APIData& data = GetAPIData();

	auto meshDocumentList = Core::GetData().assetLoadData.meshLoader(*in.meshLoadQueue);

	if (!meshDocumentList.empty())
	{
		bool deviceLocalIsHostVisible = data.physDevice.memInfo.DeviceLocalIsHostVisible();
		if (deviceLocalIsHostVisible)
		{
			LoadVBO_HostVisibleOnly(assetSystem, in, meshDocumentList);
		}
		else
		{
			LoadVBO_WithTransfer(assetSystem, memTypes, queueInfo, deletionQueues, *in.meshLoadQueue, meshDocumentList);
		}

		Core::GetData().assetLoadData.assetLoadEnd();
	}
}

std::pair<const std::vector<vk::Semaphore>&, const std::vector<vk::PipelineStageFlags>&> DRenderer::Vulkan::AssetSystem::GetWaitSemaphores(const AssetData& assetSystem)
{
	return { assetSystem.waitSemaphoreQueue, assetSystem.waitStageFlagsQueue };
}

void DRenderer::Vulkan::AssetSystem::SubmittedFrame(AssetData& assetSystem)
{
	assetSystem.currentSet = (assetSystem.currentSet + 1) % assetSystem.setCount;

	assetSystem.waitSemaphoreQueue.clear();
	assetSystem.waitStageFlagsQueue.clear();
}

DRenderer::Vulkan::VBO DRenderer::Vulkan::AssetSystem::VBOFromMeshDoc(vk::Device device, const MemoryTypes& memInfo, const MeshDoc& meshDoc)
{
	vk::BufferCreateInfo bufferInfo{};
	if (memInfo.DeviceLocalIsHostVisible())
		bufferInfo.usage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eIndexBuffer;
	else
		bufferInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;
	bufferInfo.sharingMode = vk::SharingMode::eExclusive;
	bufferInfo.size = meshDoc.GetTotalSizeRequired();

	vk::Buffer buffer = device.createBuffer(bufferInfo);

	vk::MemoryRequirements memReqs = device.getBufferMemoryRequirements(buffer);

	vk::MemoryAllocateInfo allocInfo{};
	allocInfo.allocationSize = memReqs.size;
	allocInfo.memoryTypeIndex = memInfo.hostVisible;

	vk::DeviceMemory mem = device.allocateMemory(allocInfo);

	device.bindBufferMemory(buffer, mem, 0);

	VBO vbo;
	vbo.buffer = buffer;
	vbo.deviceMemory = mem;

	uint8_t* ptr = static_cast<uint8_t*>(device.mapMemory(mem, 0, bufferInfo.size));

	using MeshDocAttr = MeshDoc::Attribute;
	using VBOAttr = VBO::Attribute;

	// Copy position data
	std::memcpy(ptr, meshDoc.GetDataPtr(MeshDocAttr::Position), meshDoc.GetBufferSize(MeshDocAttr::Position));
	vbo.GetAttrSize(VBOAttr::Position) = meshDoc.GetBufferSize(MeshDocAttr::Position);
	ptr += vbo.GetAttrSize(VBOAttr::Position);

	// Copy UV data
	std::memcpy(ptr, meshDoc.GetDataPtr(MeshDocAttr::TexCoord), meshDoc.GetBufferSize(MeshDocAttr::TexCoord));
	vbo.GetAttrSize(VBOAttr::TexCoord) = meshDoc.GetBufferSize(MeshDocAttr::TexCoord);
	ptr += vbo.GetAttrSize(VBOAttr::TexCoord);

	// Copy normal data
	std::memcpy(ptr, meshDoc.GetDataPtr(MeshDocAttr::Normal), meshDoc.GetBufferSize(MeshDocAttr::Normal));
	vbo.GetAttrSize(VBOAttr::Normal) = meshDoc.GetBufferSize(MeshDocAttr::Normal);
	ptr += vbo.GetAttrSize(VBOAttr::Normal);

	// Copy index data
	std::memcpy(ptr, meshDoc.GetDataPtr(MeshDocAttr::Index), meshDoc.GetBufferSize(MeshDocAttr::Index));
	vbo.GetAttrSize(VBOAttr::Index) = meshDoc.GetBufferSize(MeshDocAttr::Index);
	vbo.indexType = meshDoc.GetIndexType() == MeshDoc::IndexType::UInt32 ? vk::IndexType::eUint32 : vk::IndexType::eUint16;
	vbo.indexCount = meshDoc.GetIndexCount();


	device.unmapMemory(mem);

	return vbo;
}

void DRenderer::Vulkan::AssetSystem::LoadVBO_HostVisibleOnly(
	AssetData& assetSystem,
	const Core::PrepareRenderingEarlyParams& in,
	const std::vector<std::optional<DRenderer::MeshDoc>>& meshDocList)
{
	APIData& data = GetAPIData();

	for (size_t i = 0; i < meshDocList.size(); i++)
	{
		const auto& meshDocOpt = meshDocList[i];
		assert(meshDocOpt.has_value());
		const auto& meshDoc = meshDocOpt.value();

		VBO vbo = VBOFromMeshDoc(assetSystem.device, data.physDevice.memInfo, meshDoc);

		assetSystem.vboDatabase.insert({ (*in.meshLoadQueue)[i], vbo });
	}
}

void DRenderer::Vulkan::AssetSystem::LoadVBO_WithTransfer(
	AssetData& assetSystem,
	const MemoryTypes& memTypes,
	const QueueInfo& queueInfo,
	DeletionQueues& deletionQueues,
	const std::vector<size_t>& meshIDList,
	const std::vector<std::optional<MeshDoc>>& meshDocList)
{
	// Create staging buffer to put all temporary VBOs in
	size_t totalLoadVBOSize = 0;
	for (const auto& item : meshDocList)
	{
		assert(item.has_value());
		totalLoadVBOSize += item.value().GetTotalSizeRequired();
	}

	vk::BufferCreateInfo stagingBufferInfo{};
	stagingBufferInfo.sharingMode = vk::SharingMode::eExclusive;
	stagingBufferInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;
	stagingBufferInfo.size = totalLoadVBOSize;
	vk::Buffer stagingBuffer = assetSystem.device.createBuffer(stagingBufferInfo);
	vk::MemoryRequirements stagingMemReqs = assetSystem.device.getBufferMemoryRequirements(stagingBuffer);
	vk::MemoryAllocateInfo stagingAllocInfo{};
	stagingAllocInfo.allocationSize = stagingMemReqs.size;
	stagingAllocInfo.memoryTypeIndex = memTypes.hostVisible;
	vk::DeviceMemory stagingMem = assetSystem.device.allocateMemory(stagingAllocInfo);
	assetSystem.device.bindBufferMemory(stagingBuffer, stagingMem, 0);
	std::byte* const stagingMemPtr = static_cast<std::byte*>(assetSystem.device.mapMemory(stagingMem, 0, stagingBufferInfo.size));

	deletionQueues.InsertObjectForDeletion(stagingBuffer);
	deletionQueues.InsertObjectForDeletion(stagingMem);

	std::byte* stagingTargetPtr = stagingMemPtr;
	std::vector<VBOTransferJob> transferJobs(meshDocList.size());
	for (size_t i = 0; i < meshDocList.size(); i++)
	{
		const auto& meshDocOpt = meshDocList[i];
		assert(meshDocOpt.has_value());
		const auto& meshDoc = meshDocOpt.value();

		VBO stagingVBO = VBOFromMeshDoc_ForTransfer(assetSystem.device, meshDoc, stagingTargetPtr);
		
		vk::BufferCreateInfo deviceLocalBufferInfo{};
		deviceLocalBufferInfo.usage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst;
		deviceLocalBufferInfo.sharingMode = vk::SharingMode::eExclusive;
		deviceLocalBufferInfo.size = stagingVBO.GetTotalBufferSize();

		vk::Buffer deviceLocalBuffer = assetSystem.device.createBuffer(deviceLocalBufferInfo);

		VBOTransferJob& transferJob = transferJobs[i];
		transferJob.dst = deviceLocalBuffer;
		transferJob.size = deviceLocalBufferInfo.size;
		transferJob.srcBufferOffset = size_t(stagingTargetPtr - stagingMemPtr);

		vk::MemoryRequirements memReqs = assetSystem.device.getBufferMemoryRequirements(deviceLocalBuffer);

		vk::MemoryAllocateInfo allocInfo{};
		allocInfo.allocationSize = memReqs.size;
		allocInfo.memoryTypeIndex = memTypes.deviceLocal;

		vk::DeviceMemory mem = assetSystem.device.allocateMemory(allocInfo);

		assetSystem.device.bindBufferMemory(deviceLocalBuffer, mem, 0);

		auto meshDocID = meshIDList[i];
		VBO vbo = stagingVBO;
		vbo.buffer = deviceLocalBuffer;
		vbo.deviceMemory = mem;
		assetSystem.vboDatabase.insert({meshDocID, vbo});

		stagingTargetPtr += meshDoc.GetTotalSizeRequired();
	}

	assetSystem.device.unmapMemory(stagingMem);


	// Build cmd buffer
	vk::CommandBuffer& cmdBuffer = assetSystem.cmdBuffers[assetSystem.currentSet];

	vk::CommandBufferBeginInfo cmdBeginInfo{};
	cmdBeginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

	assetSystem.device.waitForFences(assetSystem.transferDoneFence[assetSystem.currentSet], 1, std::numeric_limits<uint64_t>::max());
	assetSystem.device.resetFences(assetSystem.transferDoneFence[assetSystem.currentSet]);

	cmdBuffer.begin(cmdBeginInfo);

	for (size_t i = 0; i < transferJobs.size(); i++)
	{
		const VBOTransferJob& transferJob = transferJobs[i];

		vk::BufferCopy bufferCopy{};
		bufferCopy.size = transferJob.size;
		bufferCopy.srcOffset = transferJob.srcBufferOffset;

		cmdBuffer.copyBuffer(stagingBuffer, transferJob.dst, bufferCopy);
	}

	cmdBuffer.end();


	// Submit transfer cmd buffer
	vk::SubmitInfo submitInfo{};
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuffer;

	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &assetSystem.vboLoadingFinished[assetSystem.currentSet];

	queueInfo.transfer.handle.submit(submitInfo, assetSystem.transferDoneFence[assetSystem.currentSet]);

	assetSystem.waitSemaphoreQueue.push_back(assetSystem.vboLoadingFinished[assetSystem.currentSet]);
	assetSystem.waitStageFlagsQueue.push_back(vk::PipelineStageFlagBits::eVertexInput);
}

DRenderer::Vulkan::VBO DRenderer::Vulkan::AssetSystem::VBOFromMeshDoc_ForTransfer(
	vk::Device device,
	const MeshDoc& meshDoc,
	std::byte* ptr)
{
	VBO vbo{};

	using MeshDocAttr = MeshDoc::Attribute;
	using VBOAttr = VBO::Attribute;

	// Copy position data
	std::memcpy(ptr, meshDoc.GetDataPtr(MeshDocAttr::Position), meshDoc.GetBufferSize(MeshDocAttr::Position));
	vbo.GetAttrSize(VBOAttr::Position) = meshDoc.GetBufferSize(MeshDocAttr::Position);
	ptr += vbo.GetAttrSize(VBOAttr::Position);

	// Copy UV data
	std::memcpy(ptr, meshDoc.GetDataPtr(MeshDocAttr::TexCoord), meshDoc.GetBufferSize(MeshDocAttr::TexCoord));
	vbo.GetAttrSize(VBOAttr::TexCoord) = meshDoc.GetBufferSize(MeshDocAttr::TexCoord);
	ptr += vbo.GetAttrSize(VBOAttr::TexCoord);

	// Copy normal data
	std::memcpy(ptr, meshDoc.GetDataPtr(MeshDocAttr::Normal), meshDoc.GetBufferSize(MeshDocAttr::Normal));
	vbo.GetAttrSize(VBOAttr::Normal) = meshDoc.GetBufferSize(MeshDocAttr::Normal);
	ptr += vbo.GetAttrSize(VBOAttr::Normal);

	// Copy index data
	std::memcpy(ptr, meshDoc.GetDataPtr(MeshDocAttr::Index), meshDoc.GetBufferSize(MeshDocAttr::Index));
	vbo.GetAttrSize(VBOAttr::Index) = meshDoc.GetBufferSize(MeshDocAttr::Index);
	vbo.indexType = meshDoc.GetIndexType() == MeshDoc::IndexType::UInt32 ? vk::IndexType::eUint32 : vk::IndexType::eUint16;
	vbo.indexCount = meshDoc.GetIndexCount();

	return vbo;
}