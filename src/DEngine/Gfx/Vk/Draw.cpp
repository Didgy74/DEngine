#include "Vk.hpp"
#include "../VkInterface.hpp"
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
				std::cout << "DEngine, Vulkan: Encountered bad present result." << std::endl;
			}
		}
		else if (imageIndexOpt.result == vk::Result::eSuboptimalKHR)
		{
			std::cout << "DEngine, Vulkan: Encountered suboptimal image-acquire result." << std::endl;
		}
		else
		{
			std::cout << "DEngine, Vulkan: Encountered bad image-acquire result." << std::endl;
		}
	}

	struct RecordGraphicsCmdBuffer_Params
	{
		APIData const* apiData = nullptr;
		DevDispatch const* device = nullptr;
		ViewportVkData const* gfxViewportData = nullptr;
		u8 viewportID = 255;
		u8 resourceSetIndex = 255;
		u8 resourceSetCount = 255;
		vk::RenderPass renderPass{};
		bool useEditorPipeline = false;
		vk::Pipeline testPipeline{};
		DebugUtilsDispatch const* debugUtils = nullptr;
	};
	void RecordGraphicsCmdBuffer(
		GlobUtils const& globUtils, 
		vk::CommandBuffer cmdBuffer, 
		uSize viewportID,
		u8 resourceSetIndex,
		ViewportVkData const& viewportInfo,
		APIData& apiData,
		float const* matrixPtr)
	{
		// We need to rename the command buffer every time we record it
		if (globUtils.UsingDebugUtils())
		{
			vk::DebugUtilsObjectNameInfoEXT nameInfo{};
			nameInfo.objectHandle = (u64)(VkCommandBuffer)cmdBuffer;
			nameInfo.objectType = cmdBuffer.objectType;
			std::string name = std::string("Graphics viewport #") + std::to_string(viewportID) + std::string(" - CmdBuffer #") + std::to_string(resourceSetIndex);
			nameInfo.pObjectName = name.data();
			globUtils.debugUtils.setDebugUtilsObjectNameEXT(globUtils.device.handle, nameInfo);
		}

		char* ptr = (char*)apiData.test_mappedMem;
		ptr += (uSize)apiData.test_camUboOffset * globUtils.resourceSetCount * viewportID;
		ptr += apiData.test_camUboOffset * resourceSetIndex;

		std::memcpy(ptr, matrixPtr, 64);

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

		vk::DescriptorSet descrSet = apiData.test_cameraDescrSets[(uSize)viewportID * globUtils.resourceSetCount + resourceSetIndex];
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

	// Assumes that the mutex for viewportmanager is already locked.
	void HandleVirtualViewportEvents(
		GlobUtils const& globUtils,
		Std::Span<ViewportUpdateData const> viewportUpdates,
		ViewportManager& viewportManager)
	{
		vk::Result vkResult{};

		// First we handle any viewportManager that need to be initialized, not resized.
		for (uSize i = 0; i < viewportUpdates.Size(); i += 1)
		{
			Gfx::ViewportUpdateData updateData = viewportUpdates[i];
			// Find the viewport with the ID
			ViewportVkData* viewportPtr = nullptr;
			for (auto& viewportItem : viewportManager.viewportDatas)
			{
				if (viewportItem.a == updateData.id)
				{
					viewportPtr = &viewportItem.b;
					break;
				}
			}
			DENGINE_GFX_ASSERT(viewportPtr != nullptr);

			if (viewportPtr->renderTarget.img == vk::Image())
			{
				// We need to create this virtual viewport
				viewportPtr->renderTarget = Init::InitializeGfxViewport(globUtils, updateData.id, { updateData.width, updateData.height });
				ImGui_ImplVulkan_OverwriteTexture(
					viewportPtr->imguiTextureID,
					(VkImageView)viewportPtr->renderTarget.imgView,
					(VkImageLayout)vk::ImageLayout::eShaderReadOnlyOptimal);

				// make the command buffer stuff
				vk::CommandPoolCreateInfo cmdPoolInfo{};
				cmdPoolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
				cmdPoolInfo.queueFamilyIndex = 0;
				viewportPtr->cmdPool = globUtils.device.createCommandPool(cmdPoolInfo);
				vk::CommandBufferAllocateInfo cmdBufferAllocInfo{};
				cmdBufferAllocInfo.commandBufferCount = globUtils.resourceSetCount;
				cmdBufferAllocInfo.commandPool = viewportPtr->cmdPool;
				cmdBufferAllocInfo.level = vk::CommandBufferLevel::ePrimary;
				viewportPtr->cmdBuffers.Resize(cmdBufferAllocInfo.commandBufferCount);
				vkResult = globUtils.device.allocateCommandBuffers(cmdBufferAllocInfo, viewportPtr->cmdBuffers.Data());

				if (globUtils.UsingDebugUtils())
				{
					vk::DebugUtilsObjectNameInfoEXT nameInfo{};
					nameInfo.objectHandle = (u64)(VkCommandPool)viewportPtr->cmdPool;
					nameInfo.objectType = viewportPtr->cmdPool.objectType;
					std::string name = std::string("Graphics viewport #") + std::to_string(updateData.id) + " - Cmd Pool";
					nameInfo.pObjectName = name.data();
					globUtils.debugUtils.setDebugUtilsObjectNameEXT(globUtils.device.handle, nameInfo);
				}
			}
		}

		// Now we handle viewports that need to be resized, not initialized.
		// We have to wait for the entire graphics queue because ImGui might have some
		// extra OS windows that need to sample from the image later in the draw loop.
		// Ideally we would have a deferred job queue here so we wouldn't have to stall the queue like this...

		// First we check if any viewports needs to be resized
		// This tells us if we need to wait for the fences at all
		bool viewportNeedsResizing = false;
		for (uSize i = 0; i < viewportUpdates.Size(); i += 1)
		{
			Gfx::ViewportUpdateData updateData = viewportUpdates[i];
			ViewportVkData* viewportPtr = nullptr;
			// Search for the viewport this update is referring
			for (auto& [viewportID, viewport] : viewportManager.viewportDatas)
			{
				if (viewportID == updateData.id)
				{
					viewportPtr = &viewport;;
					break;
				}
			}
			DENGINE_GFX_ASSERT(viewportPtr != nullptr);
			vk::Extent2D currentExtent = viewportPtr->renderTarget.extent;
			if (updateData.width != currentExtent.width || updateData.height != currentExtent.height)
			{
				viewportNeedsResizing = true;
				break;
			}
		}
		if (viewportNeedsResizing)
		{
			globUtils.queues.graphics.waitIdle();
			for (uSize i = 0; i < viewportUpdates.Size(); i += 1)
			{
				Gfx::ViewportUpdateData updateData = viewportUpdates[i];
				ViewportVkData* viewportPtr = nullptr;
				// Search for the viewport this update is referring
				for (auto& [viewportID, viewport] : viewportManager.viewportDatas)
				{
					if (viewportID == updateData.id)
					{
						viewportPtr = &viewport;;
						break;
					}
				}
				DENGINE_GFX_ASSERT(viewportPtr != nullptr);

				GfxRenderTarget& renderTarget = viewportPtr->renderTarget;
				if (renderTarget.extent.width != updateData.width || renderTarget.extent.height != updateData.height)
				{
					globUtils.device.destroyFramebuffer(renderTarget.framebuffer);
					globUtils.device.destroyImageView(renderTarget.imgView);
					vmaDestroyImage(globUtils.vma, (VkImage)renderTarget.img, renderTarget.vmaAllocation);
					renderTarget = Init::InitializeGfxViewport(globUtils, updateData.id, { updateData.width, updateData.height });
					ImGui_ImplVulkan_OverwriteTexture(
						viewportPtr->imguiTextureID,
						(VkImageView)renderTarget.imgView,
						(VkImageLayout)vk::ImageLayout::eShaderReadOnlyOptimal);
				}
			}
		}
	}

	void HandleSwapchainResizeEvent(
		GlobUtils const& globUtils,
		GUIData& guiData,
		SurfaceInfo& surface,
		SwapchainData& swapchain)
	{
		globUtils.queues.graphics.waitIdle();

		Init::RecreateSwapchain(globUtils, surface, swapchain);

		globUtils.device.destroyFramebuffer(guiData.renderTarget.framebuffer);
		globUtils.device.destroyImageView(guiData.renderTarget.imgView);
		vmaDestroyImage(globUtils.vma, (VkImage)guiData.renderTarget.img, guiData.renderTarget.vmaAllocation);
		guiData.renderTarget = Init::CreateGUIRenderTarget(
			globUtils.device,
			globUtils.vma,
			globUtils.deletionQueue,
			globUtils.queues,
			swapchain.extents,
			swapchain.surfaceFormat.format,
			guiData.renderPass,
			globUtils.DebugUtilsPtr());

		Init::RecordSwapchainCmdBuffers(globUtils.device, swapchain, guiData.renderTarget.img);
	}
}

void DEngine::Gfx::Vk::Draw(Data& gfxData, Draw_Params const& drawParams, void* apiDataBuffer)
{
	vk::Result vkResult{};
	APIData& apiData = *(reinterpret_cast<APIData*>(apiDataBuffer));
	GlobUtils const& globUtils = apiData.globUtils;
	DevDispatch const& device = globUtils.device;

	std::lock_guard viewportManagerLockGuard{ apiData.viewportManager.viewportDataLock };

	HandleVirtualViewportEvents(
		globUtils, 
		drawParams.viewportUpdates.ToSpan(), 
		apiData.viewportManager);
	// Handle swapchain resize
	if (drawParams.resizeEvent)
		HandleSwapchainResizeEvent(
			globUtils, 
			apiData.guiData,
			apiData.surface, 
			apiData.swapchain);



	// I did it like this because I don't want the increment operation at the bottom of the function.
	std::uint_least8_t currentInFlightIndex = apiData.currentInFlightFrame;
	apiData.currentInFlightFrame = (apiData.currentInFlightFrame + 1) % apiData.globUtils.resourceSetCount;

	vkResult = apiData.globUtils.device.waitForFences({ apiData.mainFences[currentInFlightIndex] }, true, std::numeric_limits<std::uint64_t>::max());
	if (vkResult != vk::Result::eSuccess)
		throw std::runtime_error("Vulkan: Failed to wait for cmd buffer fence.");
	device.resetFences({ apiData.mainFences[currentInFlightIndex] });

	Std::StaticVector<vk::CommandBuffer, 10> cmdBuffersToSubmit{};

	for (auto const& viewportUpdate : drawParams.viewportUpdates)
	{
		//ViewportVkData const* viewport = apiData.viewportDatas[viewportUpdate.id];
		ViewportVkData const* viewport = nullptr;
		for (auto const& item : apiData.viewportManager.viewportDatas)
		{
			if (viewportUpdate.id == item.a)
			{
				viewport = &item.b;
				break;
			}
		}
		DENGINE_GFX_ASSERT(viewport != nullptr);
		
		vk::CommandBuffer cmdBuffer = viewport->cmdBuffers[currentInFlightIndex];
		cmdBuffersToSubmit.PushBack(cmdBuffer);
		RecordGraphicsCmdBuffer(
			apiData.globUtils,
			cmdBuffer,
			viewportUpdate.id,
			currentInFlightIndex,
			*viewport,
			apiData,
			viewportUpdate.transform.Data());
	}

	if (drawParams.presentMainWindow)
	{
		// First we re-record the GUI cmd buffer and submit it.
		RecordGUICmdBuffer(
			apiData.globUtils.device,
			apiData.guiData.cmdBuffers[currentInFlightIndex],
			apiData.guiData.renderTarget,
			apiData.guiData.renderPass,
			currentInFlightIndex,
			apiData.globUtils.DebugUtilsPtr());
		cmdBuffersToSubmit.PushBack(apiData.guiData.cmdBuffers[currentInFlightIndex]);
	}

	// Submit the command cmdBuffers
	vk::SubmitInfo submitInfo{};
	submitInfo.commandBufferCount = (u32)cmdBuffersToSubmit.Size();
	submitInfo.pCommandBuffers = cmdBuffersToSubmit.Data();
	apiData.globUtils.queues.graphics.submit({ submitInfo }, apiData.mainFences[currentInFlightIndex]);


	if (drawParams.presentMainWindow)
	{
		// We query the next available swapchain image, so we know which image we need to copy to
		PresentShit(apiData);
	}

	DeletionQueue::ExecuteCurrentTick(apiData.globUtils.deletionQueue);
}