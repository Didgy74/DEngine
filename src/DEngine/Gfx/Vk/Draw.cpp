#include "Vk.hpp"
#include "DEngine/Gfx/Assert.hpp"

#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_vulkan.h"

#include "Init.hpp"

#include <iostream>

namespace DEngine::Gfx::Vk
{
	void RecordGUICmdBuffer(
		DeviceDispatch const& device,
		vk::CommandBuffer cmdBuffer,
		GUIRenderTarget const& renderTarget,
		vk::RenderPass renderPass,
		std::uint_least8_t resourceSetIndex,
		DebugUtilsDispatch const* debugUtils)
	{
		// We need to name the command buffer every time we reset it
		if (debugUtils)
		{
			vk::DebugUtilsObjectNameInfoEXT nameInfo{};
			nameInfo.objectHandle = (u64)(VkCommandBuffer)cmdBuffer;
			nameInfo.objectType = cmdBuffer.objectType;
			std::string name = std::string("GUI CmdBuffer #") + std::to_string(resourceSetIndex);
			nameInfo.pObjectName = name.data();
			debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
		}

		{
			vk::CommandBufferBeginInfo beginInfo{};
			beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
			device.beginCommandBuffer(cmdBuffer, beginInfo);

			vk::RenderPassBeginInfo rpBegin{};
			rpBegin.framebuffer = renderTarget.framebuffer;
			rpBegin.renderPass = renderPass;
			rpBegin.renderArea.extent = renderTarget.extent;
			rpBegin.clearValueCount = 1;
			vk::ClearColorValue clearVal{};
			clearVal.setFloat32({ 0.5f, 0.f, 0.f, 1.f });
			vk::ClearValue test = clearVal;
			rpBegin.pClearValues = &test;

			device.cmdBeginRenderPass(cmdBuffer, rpBegin, vk::SubpassContents::eInline);
			{
				ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), VkCommandBuffer(cmdBuffer));
			}
			device.cmdEndRenderPass(cmdBuffer);

			device.endCommandBuffer(cmdBuffer);
		}
	}

	void PresentShit(APIData& apiData)
	{
		vk::Result vkResult{};

		vk::ResultValue<std::uint32_t> imageIndexOpt = apiData.globUtils.device.acquireNextImageKHR(
			apiData.swapchain.handle,
			std::numeric_limits<u64>::max(),
			apiData.swapchain.imageAvailableSemaphore,
			vk::Fence());
		// Submit the copy-to-swapchainData operation and present image.
		if (imageIndexOpt.result == vk::Result::eSuccess)
		{
			u32 const nextSwapchainImgIndex = imageIndexOpt.value;
			// We don't need semaphore (I think) between this cmd buffer and the rendering one, it's handled by barriers
			vk::SubmitInfo copyImageSubmitInfo{};
			copyImageSubmitInfo.commandBufferCount = 1;
			copyImageSubmitInfo.pCommandBuffers = &apiData.swapchain.cmdBuffers[nextSwapchainImgIndex];
			vk::PipelineStageFlags const imgCopyDoneStage = vk::PipelineStageFlagBits::eTransfer;
			copyImageSubmitInfo.waitSemaphoreCount = 1;
			copyImageSubmitInfo.pWaitSemaphores = &apiData.swapchain.imageAvailableSemaphore;
			copyImageSubmitInfo.pWaitDstStageMask = &imgCopyDoneStage;
			apiData.globUtils.queues.graphics.submit({ copyImageSubmitInfo }, vk::Fence());

			vk::PresentInfoKHR presentInfo{};
			presentInfo.swapchainCount = 1;
			presentInfo.pSwapchains = &apiData.swapchain.handle;
			presentInfo.pImageIndices = &nextSwapchainImgIndex;
			vkResult = apiData.globUtils.queues.graphics.presentKHR(presentInfo);
			if (vkResult != vk::Result::eSuccess && vkResult != vk::Result::eSuboptimalKHR)
			{
				apiData.logger->log("DEngine, Vulkan: Encountered bad present result.");
			}
		}
		else if (imageIndexOpt.result == vk::Result::eSuboptimalKHR)
		{
			apiData.logger->log("DEngine, Vulkan: Encountered suboptimal image-acquire result.");
		}
		else
		{
			apiData.logger->log("DEngine, Vulkan: Encountered bad image-acquire result.");
		}
	}

	// Assumes the resource set is available
	void RecordGraphicsCmdBuffer(
		GlobUtils const& globUtils, 
		ViewportVkData const& viewportInfo,
		ViewportUpdateData const& viewportUpdate,
		u8 resourceSetIndex,
		APIData& apiData)
	{
		std::memcpy(
			(char*)viewportInfo.mappedMem + viewportInfo.camElementSize * resourceSetIndex,
			&viewportUpdate.transform,
			viewportInfo.camElementSize);

		vk::CommandBuffer cmdBuffer = viewportInfo.cmdBuffers[resourceSetIndex];

		// We need to rename the command buffer every time we record it
		if (globUtils.UsingDebugUtils())
		{
			vk::DebugUtilsObjectNameInfoEXT nameInfo{};
			nameInfo.objectHandle = (u64)(VkCommandBuffer)cmdBuffer;
			nameInfo.objectType = cmdBuffer.objectType;
			std::string name = std::string("Graphics viewport #") + std::to_string(viewportInfo.id) + 
				std::string(" - CmdBuffer #") + std::to_string(resourceSetIndex);
			nameInfo.pObjectName = name.data();
			globUtils.debugUtils.setDebugUtilsObjectNameEXT(globUtils.device.handle, nameInfo);
		}

		vk::CommandBufferBeginInfo beginInfo{};
		beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
		globUtils.device.beginCommandBuffer(cmdBuffer, beginInfo);

		vk::RenderPassBeginInfo rpBegin{};
		rpBegin.framebuffer = viewportInfo.renderTarget.framebuffer;
		rpBegin.renderPass = globUtils.gfxRenderPass;
		rpBegin.renderArea.extent = viewportInfo.renderTarget.extent;
		rpBegin.clearValueCount = 1;
		vk::ClearColorValue clearVal{};
		clearVal.setFloat32({ 0.f, 0.f, 0.5f, 1.f });
		vk::ClearValue test = clearVal;
		rpBegin.pClearValues = &test;
		globUtils.device.cmdBeginRenderPass(cmdBuffer, rpBegin, vk::SubpassContents::eInline);

		vk::Viewport viewport{};
		viewport.width = static_cast<float>(viewportInfo.renderTarget.extent.width);
		viewport.height = static_cast<float>(viewportInfo.renderTarget.extent.height);
		globUtils.device.cmdSetViewport(cmdBuffer, 0, { viewport });

		vk::DescriptorSet descrSet = viewportInfo.camDataDescrSets[resourceSetIndex];
		globUtils.device.cmdBindDescriptorSets(
			cmdBuffer,
			vk::PipelineBindPoint::eGraphics,
			apiData.testPipelineLayout,
			0,
			descrSet,
			{});
		globUtils.device.cmdBindPipeline(cmdBuffer, vk::PipelineBindPoint::eGraphics, apiData.testPipeline);
		globUtils.device.cmdDraw(cmdBuffer, 4, 1, 0, 0);

		globUtils.device.cmdEndRenderPass(cmdBuffer);

		// We need a barrier for the render-target if we are going to sample from it in the gui pass
		if (globUtils.useEditorPipeline)
		{
			vk::ImageMemoryBarrier imgBarrier{};
			imgBarrier.image = viewportInfo.renderTarget.img;
			imgBarrier.oldLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
			imgBarrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
			imgBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
			imgBarrier.subresourceRange.baseArrayLayer = 0;
			imgBarrier.subresourceRange.baseMipLevel = 0;
			imgBarrier.subresourceRange.layerCount = 1;
			imgBarrier.subresourceRange.levelCount = 1;
			imgBarrier.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
			imgBarrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

			globUtils.device.cmdPipelineBarrier(
				cmdBuffer,
				vk::PipelineStageFlagBits::eColorAttachmentOutput,
				vk::PipelineStageFlagBits::eFragmentShader,
				vk::DependencyFlagBits(),
				{},
				{},
				{ imgBarrier });
		}

		globUtils.device.endCommandBuffer(cmdBuffer);
	}

	// Assumes the viewportManager.viewportDatas is already locked.
	void HandleViewportCreationJobs(
		ViewportManager& viewportManager,
		GlobUtils const& globUtils)
	{
		vk::Result vkResult{};

		for (ViewportManager::CreateJob const& createJob : viewportManager.createQueue)
		{
			ViewportVkData viewport{};
			viewport.imguiTextureID = createJob.imguiTexID;
			viewport.id = createJob.id;
			viewport.camElementSize = viewportManager.camElementSize;

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
				item = viewportManager.cameraDescrLayout;
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
				globUtils.debugUtils.setDebugUtilsObjectNameEXT(globUtils.device.handle, nameInfo);
				for (uSize i = 0; i < viewport.camDataDescrSets.Size(); i += 1)
				{
					vk::DebugUtilsObjectNameInfoEXT nameInfo{};
					nameInfo.objectHandle = (u64)(VkDescriptorSet)viewport.camDataDescrSets[i];
					nameInfo.objectType = viewport.camDataDescrSets[i].objectType;
					std::string name = std::string("Graphics viewport #") + std::to_string(createJob.id) +
						" - Camera-data DescrSet #" + std::to_string(i);
					globUtils.debugUtils.setDebugUtilsObjectNameEXT(globUtils.device.handle, nameInfo);
				}
			}


			// Allocate the data
			vk::BufferCreateInfo bufferCreateInfo{};
			bufferCreateInfo.sharingMode = vk::SharingMode::eExclusive;
			bufferCreateInfo.size = globUtils.resourceSetCount * viewportManager.camElementSize;
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
				bufferInfo.offset = viewportManager.camElementSize * i;
				bufferInfo.range = viewportManager.camElementSize;
			}
			globUtils.device.updateDescriptorSets({ (u32)writes.Size(), writes.Data() }, {});

			viewportManager.viewportData.push_back({ createJob.id, viewport });
		}
		viewportManager.createQueue.clear();
	}

	// Assumes the viewportManager.viewportDatas is already locked.s
	void HandleViewportRenderTargetInitialization(
		ViewportVkData& viewport,
		GlobUtils const& globUtils,
		ViewportUpdateData updateData)
	{
		vk::Result vkResult{};

		// We need to create this virtual viewport
		viewport.renderTarget = Init::InitializeGfxViewportRenderTarget(
			globUtils, 
			updateData.id, 
			{ updateData.width, updateData.height });

		// We previously wrote a null image to this ImGui texture handle
		// So we overwrite it with a valid value now.

		globUtils.queues.graphics.waitIdle();
		ImGui_ImplVulkan_OverwriteTexture(
			viewport.imguiTextureID,
			(VkImageView)viewport.renderTarget.imgView,
			(VkImageLayout)vk::ImageLayout::eShaderReadOnlyOptimal);
	}

	// Assumes the viewportManager.viewportDatas is already locked.
	void HandleViewportResize(
		ViewportVkData& viewport,
		GlobUtils const& globUtils,
		ViewportUpdateData updateData)
	{
		// This image needs to be resized. We just make a new one and push a job to delete the old one.
		// VMA should handle memory allocations to be effective and reused.
		globUtils.deletionQueue.Destroy(viewport.renderTarget.framebuffer);
		globUtils.deletionQueue.Destroy(viewport.renderTarget.imgView);
		globUtils.deletionQueue.Destroy(viewport.renderTarget.vmaAllocation, viewport.renderTarget.img);

		viewport.renderTarget = Init::InitializeGfxViewportRenderTarget(
			globUtils, 
			updateData.id, 
			{ updateData.width, updateData.height });

		globUtils.queues.graphics.waitIdle();
		ImGui_ImplVulkan_OverwriteTexture(
			viewport.imguiTextureID,
			(VkImageView)viewport.renderTarget.imgView,
			(VkImageLayout)vk::ImageLayout::eShaderReadOnlyOptimal);
	}

	// Assumes that the mutex for viewport-datas is locked
	void HandleVirtualViewportEvents(
		GlobUtils const& globUtils,
		Std::Span<ViewportUpdateData const> viewportUpdates,
		ViewportManager& viewportManager)
	{
		vk::Result vkResult{};

		// Execute pending deletions
		for (uSize id : viewportManager.deleteQueue)
		{
			// Find the viewport with the ID
			uSize viewportIndex = static_cast<uSize>(-1);
			for (uSize i = 0; i < viewportManager.viewportData.size(); i += 1)
			{
				if (viewportManager.viewportData[i].a == id)
				{
					viewportIndex = i;
					break;
				}
			}
			DENGINE_GFX_ASSERT(viewportIndex != static_cast<uSize>(-1));
			ViewportVkData& viewportData = viewportManager.viewportData[viewportIndex].b;

			// Delete all the resources
			globUtils.deletionQueue.Destroy(viewportData.cameraDescrPool);
			globUtils.deletionQueue.Destroy(viewportData.camVmaAllocation, viewportData.camDataBuffer);
			globUtils.deletionQueue.Destroy(viewportData.cmdPool);
			globUtils.deletionQueue.Destroy(viewportData.renderTarget.framebuffer);
			globUtils.deletionQueue.DestroyImGuiTexture(viewportData.imguiTextureID);
			globUtils.deletionQueue.Destroy(viewportData.renderTarget.imgView);
			globUtils.deletionQueue.Destroy(viewportData.renderTarget.vmaAllocation, viewportData.renderTarget.img);

			// Remove the structure from the vector
			viewportManager.viewportData.erase(viewportManager.viewportData.begin() + viewportIndex);
		}
		viewportManager.deleteQueue.clear();


		HandleViewportCreationJobs(viewportManager, globUtils);

		// First we handle any viewportManager that need to be initialized, not resized.
		for (Gfx::ViewportUpdateData const& updateData : viewportUpdates)
		{
			// Find the viewport with the ID
			uSize viewportIndex = static_cast<uSize>(-1);
			for (uSize i = 0; i < viewportManager.viewportData.size(); i += 1)
			{
				if (viewportManager.viewportData[i].a == updateData.id)
				{
					viewportIndex = i;
					break;
				}
			}
			DENGINE_GFX_ASSERT(viewportIndex != static_cast<uSize>(-1));
			ViewportVkData& viewportPtr = viewportManager.viewportData[viewportIndex].b;
			if (viewportPtr.renderTarget.img == vk::Image())
			{
				// This image is not initalized.
				HandleViewportRenderTargetInitialization(viewportPtr, globUtils, updateData);
			}
			else if (updateData.width != viewportPtr.renderTarget.extent.width ||
				updateData.height != viewportPtr.renderTarget.extent.height)
			{
				// This image needs to be resized.
				HandleViewportResize(viewportPtr, globUtils, updateData);
			}
		}
	}

	void HandleSwapchainResizeEvent(
		GlobUtils const& globUtils,
		GUIData& guiData,
		SurfaceInfo& surface,
		SwapchainData& swapchain)
	{
		globUtils.device.WaitIdle();

		Init::RecreateSwapchain(globUtils, surface, swapchain);

		globUtils.device.Destroy(guiData.renderTarget.framebuffer);
		globUtils.device.Destroy(guiData.renderTarget.imgView);
		vmaDestroyImage(globUtils.vma, (VkImage)guiData.renderTarget.img, guiData.renderTarget.vmaAllocation);
		guiData.renderTarget = Init::CreateGUIRenderTarget(
			globUtils.device,
			globUtils.vma,
			globUtils.deletionQueue,
			globUtils.queues,
			swapchain.extents,
			swapchain.surfaceFormat.format,
			globUtils.guiRenderPass,
			globUtils.DebugUtilsPtr());

		Init::RecordSwapchainCmdBuffers(globUtils.device, swapchain, guiData.renderTarget.img);
	}

	void HandleSurfaceRecreationEvent(
		GlobUtils const& globUtils,
		SurfaceInfo& surfaceInfo,
		IWsi& wsiInterface,
		SwapchainData& swapchainData,
		GUIData& guiData)
	{
		globUtils.device.WaitIdle();		

		globUtils.device.Destroy(swapchainData.handle);
		swapchainData.handle = vk::SwapchainKHR();
		globUtils.instance.Destroy(surfaceInfo.handle);
		surfaceInfo.handle = vk::SurfaceKHR();
		

		// TODO: I don't think I like this code
		// Create the VkSurface using the callback
		vk::SurfaceKHR surface{};
		vk::Result surfaceCreateResult = (vk::Result)wsiInterface.createVkSurface(
			(u64)(VkInstance)globUtils.instance.handle, 
			nullptr, 
			reinterpret_cast<u64*>(&surface));
		if (surfaceCreateResult != vk::Result::eSuccess)
			throw std::runtime_error("Unable to create VkSurfaceKHR object during initialization.");

		
		Init::RebuildSurfaceInfo(
			globUtils.instance,
			globUtils.physDevice,
			surface,
			surfaceInfo);
		
		swapchainData.handle = vk::SwapchainKHR();

		HandleSwapchainResizeEvent(
			globUtils,
			guiData,
			surfaceInfo,
			swapchainData);
	}
}

void DEngine::Gfx::Vk::APIData::Draw(Data& gfxData, Draw_Params const& drawParams)
{
	vk::Result vkResult{};
	APIData& apiData = *this;
	GlobUtils const& globUtils = apiData.globUtils;
	DevDispatch const& device = globUtils.device;

	// Handle surface recreation
	if (drawParams.rebuildSurface)
		HandleSurfaceRecreationEvent(
			globUtils,
			apiData.surface,
			*apiData.wsiInterface,
			apiData.swapchain,
			apiData.guiData);
	// Handle swapchain resize
	else if (drawParams.resizeEvent)
		HandleSwapchainResizeEvent(
			globUtils, 
			apiData.guiData,
			apiData.surface, 
			apiData.swapchain);

	apiData.viewportManager.mutexLock.lock();
	HandleVirtualViewportEvents(
		globUtils, 
		{ drawParams.viewportUpdates.data(), drawParams.viewportUpdates.size() },
		apiData.viewportManager);

	u8 currentInFlightIndex = apiData.currentInFlightFrame;

	vkResult = apiData.globUtils.device.waitForFences(
		{ apiData.mainFences[currentInFlightIndex] }, 
		true, 
		static_cast<u64>(-1));
	if (vkResult != vk::Result::eSuccess)
		throw std::runtime_error("Vulkan: Failed to wait for cmd buffer fence.");
	device.resetFences({ apiData.mainFences[currentInFlightIndex] });


	std::vector<vk::CommandBuffer> cmdBuffersToSubmit{};
	for (auto const& viewportUpdate : drawParams.viewportUpdates)
	{
		uSize viewportIndex = static_cast<uSize>(-1);
		for (uSize i = 0; i < apiData.viewportManager.viewportData.size(); i += 1)
		{
			if (viewportUpdate.id == apiData.viewportManager.viewportData[i].a)
			{
				viewportIndex = i;
				break;
			}
		}
		ViewportVkData const& viewport = apiData.viewportManager.viewportData[viewportIndex].b;
		vk::CommandBuffer cmdBuffer = viewport.cmdBuffers[currentInFlightIndex];
		cmdBuffersToSubmit.push_back(cmdBuffer);
		RecordGraphicsCmdBuffer(
			apiData.globUtils,
			viewport,
			viewportUpdate,
			currentInFlightIndex,
			apiData);
	}

	// TODO: This viewportmanager-synchronization must be fixed.
	apiData.viewportManager.mutexLock.unlock();

	if (drawParams.presentMainWindow)
	{
		// First we re-record the GUI cmd buffer and submit it.
		RecordGUICmdBuffer(
			apiData.globUtils.device,
			apiData.guiData.cmdBuffers[currentInFlightIndex],
			apiData.guiData.renderTarget,
			globUtils.guiRenderPass,
			currentInFlightIndex,
			apiData.globUtils.DebugUtilsPtr());
		cmdBuffersToSubmit.push_back(apiData.guiData.cmdBuffers[currentInFlightIndex]);
	}

	// Submit the command cmdBuffers
	vk::SubmitInfo submitInfo{};
	submitInfo.commandBufferCount = (u32)cmdBuffersToSubmit.size();
	submitInfo.pCommandBuffers = cmdBuffersToSubmit.data();
	apiData.globUtils.queues.graphics.submit({ submitInfo }, apiData.mainFences[currentInFlightIndex]);


	if (drawParams.presentMainWindow)
	{
		// We query the next available swapchain image, so we know which image we need to copy to
		PresentShit(apiData);
	}

	DeletionQueue::ExecuteCurrentTick(apiData.globUtils.deletionQueue);
	apiData.currentInFlightFrame = (apiData.currentInFlightFrame + 1) % apiData.globUtils.resourceSetCount;
	apiData.globUtils.SetCurrentResourceSetIndex(apiData.currentInFlightFrame);
}

