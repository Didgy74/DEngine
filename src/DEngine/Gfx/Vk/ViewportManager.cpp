#include "ViewportManager.hpp"
#include "DeletionQueue.hpp"

#include "DynamicDispatch.hpp"
#include "GlobUtils.hpp"
#include "Init.hpp"

#include "../Assert.hpp"

namespace DEngine::Gfx::Vk
{
	// Assumes the viewportManager.viewportDatas is already locked.
	static void HandleViewportRenderTargetInitialization(
		GfxRenderTarget& renderTarget,
		void* imguiTexID,
		GlobUtils const& globUtils,
		ViewportUpdateData updateData)
	{
		//vk::Result vkResult{};

		// We need to create this virtual viewport
		renderTarget = Init::InitializeGfxViewportRenderTarget(
			globUtils,
			updateData.id,
			{ updateData.width, updateData.height });

		// We previously wrote a null image to this ImGui texture handle
		// So we overwrite it with a valid value now.
		/*
		ImGui_ImplVulkan_OverwriteTexture(
			imguiTexID,
			(VkImageView)renderTarget.imgView,
			(VkImageLayout)vk::ImageLayout::eShaderReadOnlyOptimal);
			*/
	}

	// Assumes the viewportManager.viewportDatas is already locked.
	static void HandleViewportResize(
		GfxRenderTarget& renderTarget,
		void* imguiTexID,
		GlobUtils const& globUtils,
		ViewportUpdateData updateData)
	{
		// This image needs to be resized. We just make a new one and push a job to delete the old one.
		// VMA should handle memory allocations to be effective and reused.
		globUtils.deletionQueue.Destroy(renderTarget.framebuffer);
		globUtils.deletionQueue.Destroy(renderTarget.imgView);
		globUtils.deletionQueue.Destroy(renderTarget.vmaAllocation, renderTarget.img);

		renderTarget = Init::InitializeGfxViewportRenderTarget(
			globUtils,
			updateData.id,
			{ updateData.width, updateData.height });

		globUtils.queues.graphics.waitIdle();
		/*
		ImGui_ImplVulkan_OverwriteTexture(
			imguiTexID,
			(VkImageView)renderTarget.imgView,
			(VkImageLayout)vk::ImageLayout::eShaderReadOnlyOptimal);
		*/
	}

	static ViewportData InitializeViewport(
		GlobUtils const& globUtils,
		ViewportManager::CreateJob createJob,
		uSize camElementSize,
		vk::DescriptorSetLayout cameraDescrLayout)
	{
		vk::Result vkResult{};

		ViewportData viewport{};
		viewport.imguiTextureID = createJob.imguiTexID;
		viewport.id = createJob.id;
		viewport.camElementSize = camElementSize;

		// Make the command buffer stuff
		vk::CommandPoolCreateInfo cmdPoolInfo{};
		cmdPoolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
		cmdPoolInfo.queueFamilyIndex = globUtils.queues.graphics.FamilyIndex();
		viewport.cmdPool = globUtils.device.createCommandPool(cmdPoolInfo);
		vk::CommandBufferAllocateInfo cmdBufferAllocInfo{};
		cmdBufferAllocInfo.commandBufferCount = globUtils.resourceSetCount;
		cmdBufferAllocInfo.commandPool = viewport.cmdPool;
		cmdBufferAllocInfo.level = vk::CommandBufferLevel::ePrimary;
		viewport.cmdBuffers.Resize(cmdBufferAllocInfo.commandBufferCount);
		vkResult = globUtils.device.allocateCommandBuffers(cmdBufferAllocInfo, viewport.cmdBuffers.Data());
		if (vkResult != vk::Result::eSuccess)
			throw std::runtime_error("DEngine, Vulkan: Unable to allocate command buffers for viewport.");
		if (globUtils.UsingDebugUtils())
		{
			vk::DebugUtilsObjectNameInfoEXT nameInfo{};
			nameInfo.objectHandle = (u64)(VkCommandPool)viewport.cmdPool;
			nameInfo.objectType = viewport.cmdPool.objectType;
			std::string name = std::string("Graphics viewport #") + std::to_string(createJob.id) + " - Cmd Pool";
			nameInfo.pObjectName = name.data();
			globUtils.debugUtils.setDebugUtilsObjectNameEXT(globUtils.device.handle, nameInfo);
			for (uSize i = 0; i < viewport.cmdBuffers.Size(); i += 1)
			{
				nameInfo.objectHandle = (u64)(VkCommandBuffer)viewport.cmdBuffers[i];
				nameInfo.objectType = viewport.cmdBuffers[i].objectType;
				name = std::string("Graphics viewport #") + std::to_string(createJob.id) +
					" - CmdBuffer #" + std::to_string(i);
				nameInfo.pObjectName = name.data();
				globUtils.debugUtils.setDebugUtilsObjectNameEXT(globUtils.device.handle, nameInfo);
			}
		}

		// Make the descriptorpool and sets
		vk::DescriptorPoolSize descrPoolSize{};
		descrPoolSize.type = vk::DescriptorType::eUniformBuffer;
		descrPoolSize.descriptorCount = globUtils.resourceSetCount;

		vk::DescriptorPoolCreateInfo descrPoolInfo{};
		descrPoolInfo.maxSets = globUtils.resourceSetCount;
		descrPoolInfo.poolSizeCount = 1;
		descrPoolInfo.pPoolSizes = &descrPoolSize;
		viewport.cameraDescrPool = globUtils.device.createDescriptorPool(descrPoolInfo);

		vk::DescriptorSetAllocateInfo descrSetAllocInfo{};
		descrSetAllocInfo.descriptorPool = viewport.cameraDescrPool;
		descrSetAllocInfo.descriptorSetCount = globUtils.resourceSetCount;
		Std::StaticVector<vk::DescriptorSetLayout, Constants::maxResourceSets> descrLayouts;
		descrLayouts.Resize(globUtils.resourceSetCount);
		for (auto& item : descrLayouts)
			item = cameraDescrLayout;
		descrSetAllocInfo.pSetLayouts = descrLayouts.Data();
		viewport.camDataDescrSets.Resize(globUtils.resourceSetCount);
		vkResult = globUtils.device.allocateDescriptorSets(descrSetAllocInfo, viewport.camDataDescrSets.Data());
		if (vkResult != vk::Result::eSuccess)
			throw std::runtime_error("DEngine - Vulkan: Unable to allocate descriptor sets for viewport camera data.");
		if (globUtils.UsingDebugUtils())
		{
			vk::DebugUtilsObjectNameInfoEXT nameInfo{};
			nameInfo.objectHandle = (u64)(VkDescriptorPool)viewport.cameraDescrPool;
			nameInfo.objectType = viewport.cameraDescrPool.objectType;
			std::string name = std::string("Graphics viewport #") + std::to_string(createJob.id) +
				" - Camera-data DescrPool";
			nameInfo.pObjectName = name.c_str();
			globUtils.debugUtils.setDebugUtilsObjectNameEXT(globUtils.device.handle, nameInfo);
			for (uSize i = 0; i < viewport.camDataDescrSets.Size(); i += 1)
			{
				vk::DebugUtilsObjectNameInfoEXT nameInfo{};
				nameInfo.objectHandle = (u64)(VkDescriptorSet)viewport.camDataDescrSets[i];
				nameInfo.objectType = viewport.camDataDescrSets[i].objectType;
				std::string name = std::string("Graphics viewport #") + std::to_string(createJob.id) +
					" - Camera-data DescrSet #" + std::to_string(i);
				nameInfo.pObjectName = name.c_str();
				globUtils.debugUtils.setDebugUtilsObjectNameEXT(globUtils.device.handle, nameInfo);
			}
		}


		// Allocate the data
		vk::BufferCreateInfo bufferCreateInfo{};
		bufferCreateInfo.sharingMode = vk::SharingMode::eExclusive;
		bufferCreateInfo.size = globUtils.resourceSetCount * camElementSize;
		bufferCreateInfo.usage = vk::BufferUsageFlagBits::eUniformBuffer;

		VmaAllocationCreateInfo memAllocInfo{};
		memAllocInfo.flags = VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_MAPPED_BIT;
		memAllocInfo.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_CPU_TO_GPU;

		VmaAllocationInfo resultMemInfo{};
		vkResult = (vk::Result)vmaCreateBuffer(
			globUtils.vma,
			(VkBufferCreateInfo*)&bufferCreateInfo,
			&memAllocInfo,
			(VkBuffer*)&viewport.camDataBuffer,
			&viewport.camVmaAllocation,
			&resultMemInfo);
		if (vkResult != vk::Result::eSuccess)
			throw std::runtime_error("DEngine - Vulkan: VMA unable to allocate cam-data memory for viewport.");
		viewport.mappedMem = resultMemInfo.pMappedData;
		if (globUtils.UsingDebugUtils())
		{
			vk::DebugUtilsObjectNameInfoEXT nameInfo{};
			nameInfo.objectHandle = (u64)(VkBuffer)viewport.camDataBuffer;
			nameInfo.objectType = viewport.camDataBuffer.objectType;
			std::string name = std::string("Graphics viewport #") + std::to_string(createJob.id) + " - CamData Buffer";
			nameInfo.pObjectName = name.data();
			globUtils.debugUtils.setDebugUtilsObjectNameEXT(globUtils.device.handle, nameInfo);
		}
		

		// Update descriptor sets
		Std::StaticVector<vk::WriteDescriptorSet, Constants::maxResourceSets> writes{};
		writes.Resize(globUtils.resourceSetCount);
		Std::StaticVector<vk::DescriptorBufferInfo, Constants::maxResourceSets> bufferInfos{};
		bufferInfos.Resize(globUtils.resourceSetCount);
		for (uSize i = 0; i < globUtils.resourceSetCount; i += 1)
		{
			vk::WriteDescriptorSet& writeData = writes[i];
			vk::DescriptorBufferInfo& bufferInfo = bufferInfos[i];

			writeData.descriptorCount = 1;
			writeData.descriptorType = vk::DescriptorType::eUniformBuffer;
			writeData.dstBinding = 0;
			writeData.dstSet = viewport.camDataDescrSets[i];
			writeData.pBufferInfo = &bufferInfo;

			bufferInfo.buffer = viewport.camDataBuffer;
			bufferInfo.offset = camElementSize * i;
			bufferInfo.range = camElementSize;
		}
		globUtils.device.updateDescriptorSets({ (u32)writes.Size(), writes.Data() }, nullptr);

		return viewport;
	}

	// Assumes the viewportManager.viewportDatas is already locked.
	void HandleViewportCreationJobs(
		ViewportManager& viewportManager,
		GlobUtils const& globUtils)
	{
		//vk::Result vkResult{};

		for (ViewportManager::CreateJob const& createJob : viewportManager.createQueue)
		{
			ViewportData newViewport = InitializeViewport(
				globUtils,
				createJob,
				viewportManager.camElementSize,
				viewportManager.cameraDescrLayout);

			viewportManager.viewportData.push_back({ createJob.id, newViewport });
		}
		viewportManager.createQueue.clear();
	}
}


bool DEngine::Gfx::Vk::ViewportManager::Init(
	ViewportManager& manager,
	DeviceDispatch const& device,
	uSize minUniformBufferOffsetAlignment,
	DebugUtilsDispatch const* debugUtils)
{
	vk::DeviceSize elementSize = 64;
	if (minUniformBufferOffsetAlignment > elementSize)
		elementSize = minUniformBufferOffsetAlignment;
	manager.camElementSize = (uSize)elementSize;

	vk::DescriptorSetLayoutBinding binding{};
	binding.binding = 0;
	binding.descriptorCount = 1;
	binding.descriptorType = vk::DescriptorType::eUniformBuffer;
	binding.stageFlags = vk::ShaderStageFlagBits::eVertex;
	vk::DescriptorSetLayoutCreateInfo descrLayoutInfo{};
	descrLayoutInfo.bindingCount = 1;
	descrLayoutInfo.pBindings = &binding;
	manager.cameraDescrLayout = device.createDescriptorSetLayout(descrLayoutInfo);
	if (debugUtils)
	{
		vk::DebugUtilsObjectNameInfoEXT nameInfo{};
		nameInfo.objectHandle = (u64)(VkDescriptorSetLayout)manager.cameraDescrLayout;
		nameInfo.objectType = manager.cameraDescrLayout.objectType;
		std::string name = "ViewportManager - Cam DescrLayout";
		nameInfo.pObjectName = name.c_str();
		debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
	}

	return true;
}

void DEngine::Gfx::Vk::ViewportManager::NewViewport(uSize& viewportID, void*& imguiTexID)
{
	ViewportManager::CreateJob createJob{};
	// We just create an empty descriptor set in ImGui for now
	// We overwrite this descriptor-set once we get to Draw() where we
	// get the actual dimensions of the viewport.
	//createJob.imguiTexID = ImGui_ImplVulkan_AddTexture(VkImageView(), VkImageLayout());
	imguiTexID = createJob.imguiTexID;

	std::lock_guard _{ this->mutexLock };
	createJob.id = this->viewportIDTracker;
	viewportID = createJob.id;

	this->createQueue.push_back(createJob);
	this->viewportIDTracker += 1;
}

void DEngine::Gfx::Vk::ViewportManager::DeleteViewport(uSize id)
{
	std::lock_guard _{ this->mutexLock };
	this->deleteQueue.push_back(id);
}

void DEngine::Gfx::Vk::ViewportManager::HandleEvents(
	ViewportManager& vpManager,
	GlobUtils const& globUtils,
	Std::Span<ViewportUpdateData const> viewportUpdates)
{
	//vk::Result vkResult{};

	// Execute pending deletions
	for (uSize id : vpManager.deleteQueue)
	{
		// Find the viewport with the ID
		uSize viewportIndex = static_cast<uSize>(-1);
		for (uSize i = 0; i < vpManager.viewportData.size(); i += 1)
		{
			if (vpManager.viewportData[i].a == id)
			{
				viewportIndex = i;
				break;
			}
		}
		DENGINE_DETAIL_GFX_ASSERT(viewportIndex != static_cast<uSize>(-1));
		ViewportData& viewportData = vpManager.viewportData[viewportIndex].b;

		// Delete all the resources
		globUtils.deletionQueue.Destroy(viewportData.cameraDescrPool);
		globUtils.deletionQueue.Destroy(viewportData.camVmaAllocation, viewportData.camDataBuffer);
		globUtils.deletionQueue.Destroy(viewportData.cmdPool);
		globUtils.deletionQueue.Destroy(viewportData.renderTarget.framebuffer);
		globUtils.deletionQueue.DestroyImGuiTexture(viewportData.imguiTextureID);
		globUtils.deletionQueue.Destroy(viewportData.renderTarget.imgView);
		globUtils.deletionQueue.Destroy(viewportData.renderTarget.vmaAllocation, viewportData.renderTarget.img);

		// Remove the structure from the vector
		std::swap(vpManager.viewportData[viewportIndex], vpManager.viewportData.back());
		vpManager.viewportData.pop_back();
	}
	vpManager.deleteQueue.clear();

	HandleViewportCreationJobs(vpManager, globUtils);

	// First we handle any viewportManager that need to be initialized, not resized.
	for (Gfx::ViewportUpdateData const& updateData : viewportUpdates)
	{
		// Find the viewport with the ID
		uSize viewportIndex = static_cast<uSize>(-1);
		for (uSize i = 0; i < vpManager.viewportData.size(); i += 1)
		{
			if (vpManager.viewportData[i].a == updateData.id)
			{
				viewportIndex = i;
				break;
			}
		}
		DENGINE_DETAIL_GFX_ASSERT(viewportIndex != static_cast<uSize>(-1));
		ViewportData& viewportPtr = vpManager.viewportData[viewportIndex].b;
		if (viewportPtr.renderTarget.img == vk::Image())
		{
			// This image is not initalized.
			HandleViewportRenderTargetInitialization(
				viewportPtr.renderTarget,
				viewportPtr.imguiTextureID,
				globUtils, 
				updateData);
		}
		else if (updateData.width != viewportPtr.renderTarget.extent.width ||
			updateData.height != viewportPtr.renderTarget.extent.height)
		{
			// This image needs to be resized.
			HandleViewportResize(
				viewportPtr.renderTarget,
				viewportPtr.imguiTextureID, 
				globUtils, 
				updateData);
		}
	}
}