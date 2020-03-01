#include "Vk.hpp"
#include "../VkInterface.hpp"

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
		vk::Result vkResult{};

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
				throw std::runtime_error("Vulkan: Queue presentation submit failed.");
		}
		else if (imageIndexOpt.result == vk::Result::eSuboptimalKHR)
			throw std::runtime_error("Vulkan: Encountered suboptimal surface.");
		else
			throw std::runtime_error("Vulkan: vkAcquireNextImageKHR failed.");
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
		u8 viewportID,
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
		globUtils.device.cmdDraw(cmdBuffer, 3, 1, 0, 0);

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

	void HandleVirtualViewportEvents(
		GlobUtils const& globUtils,
		Cont::Span<ViewportUpdateData const> viewportUpdates,
		Cont::Span<ViewportVkData> viewports)
	{
		vk::Result vkResult{};

		{
			bool needToWaitForQueue = false;
			// If we have to resize (not initialize!) a virtual viewport,
			// We have to wait for the entire queue because the ImGui 
			// secondary viewports' command buffers may still be using the virtual viewport.
			for (auto const& item : viewportUpdates)
			{
				ViewportVkData const& viewport = viewports[item.id];
				if ((viewport.renderTarget.extent.width != item.width || viewport.renderTarget.extent.height != item.height) &&
					viewport.renderTarget.img != vk::Image())
				{
					needToWaitForQueue = true;
					break;
				}
			}
			if (needToWaitForQueue)
			{
				globUtils.queues.graphics.waitIdle();
			}
		}

		for (uSize i = 0; i < viewportUpdates.Size(); i += 1)
		{
			Gfx::ViewportUpdateData updateData = viewportUpdates[i];
			ViewportVkData& viewport = viewports[updateData.id];

			if (!viewport.inUse)
				throw std::runtime_error("DEngine - Vulkan: Tried to update a virtual viewport that as not been \
					initialized by .NewViewport().");

			if (viewport.renderTarget.img == vk::Image())
			{
				// We need to create this virtual viewport
				viewport.renderTarget = Init::InitializeGfxViewport(globUtils, updateData.id, { updateData.width, updateData.height });
				ImGui_ImplVulkan_OverwriteTexture(
					viewport.imguiTextureID,
					viewport.renderTarget.imgView,
					(VkImageLayout)vk::ImageLayout::eShaderReadOnlyOptimal);

				// make the command buffer stuff
				vk::CommandPoolCreateInfo cmdPoolInfo{};
				cmdPoolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
				cmdPoolInfo.queueFamilyIndex = 0;
				viewport.cmdPool = globUtils.device.createCommandPool(cmdPoolInfo);
				vk::CommandBufferAllocateInfo cmdBufferAllocInfo{};
				cmdBufferAllocInfo.commandBufferCount = globUtils.resourceSetCount;
				cmdBufferAllocInfo.commandPool = viewport.cmdPool;
				cmdBufferAllocInfo.level = vk::CommandBufferLevel::ePrimary;
				viewport.cmdBuffers.Resize(cmdBufferAllocInfo.commandBufferCount);
				vkResult = globUtils.device.allocateCommandBuffers(cmdBufferAllocInfo, viewport.cmdBuffers.Data());

				if (globUtils.UsingDebugUtils())
				{
					vk::DebugUtilsObjectNameInfoEXT nameInfo{};
					nameInfo.objectHandle = (u64)(VkCommandPool)viewport.cmdPool;
					nameInfo.objectType = viewport.cmdPool.objectType;
					std::string name = std::string("Graphics viewport #") + std::to_string(updateData.id) + " - Cmd Pool";
					nameInfo.pObjectName = name.data();
					globUtils.debugUtils.setDebugUtilsObjectNameEXT(globUtils.device.handle, nameInfo);
				}
			}
			else if (viewport.renderTarget.extent.width != updateData.width || viewport.renderTarget.extent.height != updateData.height)
			{
				// We need to resize this viewport
				// We have already waited for the queue, no need to synchronize stuff here.

				// First we destroy the old handles
				globUtils.device.destroyFramebuffer(viewport.renderTarget.framebuffer);
				globUtils.device.destroyImageView(viewport.renderTarget.imgView);
				vmaDestroyImage(globUtils.vma, viewport.renderTarget.img, viewport.renderTarget.vmaAllocation);

				viewport.renderTarget = Init::InitializeGfxViewport(globUtils, updateData.id, { updateData.width, updateData.height });
				ImGui_ImplVulkan_OverwriteTexture(
					viewport.imguiTextureID,
					viewport.renderTarget.imgView,
					(VkImageLayout)vk::ImageLayout::eShaderReadOnlyOptimal);
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
		vmaDestroyImage(globUtils.vma, guiData.renderTarget.img, guiData.renderTarget.vmaAllocation);
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

	HandleVirtualViewportEvents(globUtils, drawParams.viewports.ToSpan(), apiData.viewportDatas.ToSpan());
	// Handle swapchain resize
	if (drawParams.resizeEvent)
		HandleSwapchainResizeEvent(globUtils, apiData.guiData, apiData.surface, apiData.swapchain);



	// I did it like this because I don't want the increment operation at the bottom of the function.
	std::uint_least8_t currentResourceSet = apiData.currentResourceSet;
	apiData.currentResourceSet = (apiData.currentResourceSet + 1) % apiData.globUtils.resourceSetCount;

	vkResult = apiData.globUtils.device.waitForFences({ apiData.mainFences[currentResourceSet] }, true, std::numeric_limits<std::uint64_t>::max());
	if (vkResult != vk::Result::eSuccess)
		throw std::runtime_error("Vulkan: Failed to wait for cmd buffer fence.");
	device.resetFences({ apiData.mainFences[currentResourceSet] });

	Cont::FixedVector<vk::CommandBuffer, 10> cmdBuffersToSubmit{};

	for (auto const& viewportUpdate : drawParams.viewports)
	{
		ViewportVkData const& viewport = apiData.viewportDatas[viewportUpdate.id];
		vk::CommandBuffer cmdBuffer = viewport.cmdBuffers[currentResourceSet];
		cmdBuffersToSubmit.PushBack(cmdBuffer);
		RecordGraphicsCmdBuffer(
			apiData.globUtils,
			cmdBuffer,
			viewportUpdate.id,
			currentResourceSet,
			viewport,
			apiData,
			viewportUpdate.transform.Data());
	}

	if (drawParams.presentMainWindow)
	{
		// First we re-record the GUI cmd buffer and submit it.
		RecordGUICmdBuffer(
			apiData.globUtils.device,
			apiData.guiData.cmdBuffers[currentResourceSet],
			apiData.guiData.renderTarget,
			apiData.guiData.renderPass,
			currentResourceSet,
			apiData.globUtils.DebugUtilsPtr());
		cmdBuffersToSubmit.PushBack(apiData.guiData.cmdBuffers[currentResourceSet]);
	}

	// Submit the command cmdBuffers
	vk::SubmitInfo submitInfo{};
	submitInfo.commandBufferCount = (u32)cmdBuffersToSubmit.Size();
	submitInfo.pCommandBuffers = cmdBuffersToSubmit.Data();
	apiData.globUtils.queues.graphics.submit({ submitInfo }, apiData.mainFences[currentResourceSet]);


	if (drawParams.presentMainWindow)
	{
		// We query the next available swapchain image, so we know which image we need to copy to
		PresentShit(apiData);
	}

	DeletionQueue::ExecuteCurrentTick(apiData.globUtils.deletionQueue);
}