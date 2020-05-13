#include "Vk.hpp"
#include "../Assert.hpp"

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
				//ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), VkCommandBuffer(cmdBuffer));
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
			apiData.globUtils.queues.graphics.submit(copyImageSubmitInfo, vk::Fence());

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
		ViewportData const& viewportInfo,
		ViewportUpdateData const& viewportUpdate,
		DrawParams const& drawParams,
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
		globUtils.device.cmdSetViewport(cmdBuffer, 0, viewport);

		globUtils.device.cmdBindPipeline(cmdBuffer, vk::PipelineBindPoint::eGraphics, apiData.testPipeline);

		for (uSize drawIndex = 0; drawIndex < drawParams.textureIDs.size(); drawIndex += 1)
		{
			Std::Array<vk::DescriptorSet, 3> descrSets = {
				viewportInfo.camDataDescrSets[resourceSetIndex],
				apiData.objectDataManager.descrSet,
				apiData.textureManager.database.at(drawParams.textureIDs[drawIndex]).descrSet };

			uSize objectDataBufferOffset = 0;
			objectDataBufferOffset += apiData.objectDataManager.capacity * apiData.objectDataManager.elementSize * resourceSetIndex;
			objectDataBufferOffset += apiData.objectDataManager.elementSize * drawIndex;

			globUtils.device.cmdBindDescriptorSets(
				cmdBuffer,
				vk::PipelineBindPoint::eGraphics,
				apiData.testPipelineLayout,
				0,
				{ (u32)descrSets.Size(), descrSets.Data() },
				(u32)objectDataBufferOffset);
			globUtils.device.cmdDraw(cmdBuffer, 4, 1, 0, 0);
		}

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
				nullptr,
				nullptr,
				imgBarrier);
		}

		globUtils.device.endCommandBuffer(cmdBuffer);
	}

	void HandleSwapchainResizeEvent(
		u32 width,
		u32 height,
		IWsi& wsiInterface,
		GlobUtils const& globUtils,
		GUIData& guiData,
		SurfaceInfo& surface,
		SwapchainData& swapchain)
	{
		globUtils.device.WaitIdle();

		surface = Init::BuildSurfaceInfo(
			globUtils.instance,
			globUtils.physDevice.handle,
			surface.handle,
			globUtils.logger);

		SwapchainSettings spSettings = Init::BuildSwapchainSettings(
			globUtils.instance,
			globUtils.physDevice.handle,
			surface,
			width,
			height,
			globUtils.logger);
		Init::RecreateSwapchain(globUtils, spSettings, swapchain);

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
		u32 width,
		u32 height,
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
		vk::Result surfaceCreateResult = (vk::Result)wsiInterface.CreateVkSurface(
			(u64)(VkInstance)globUtils.instance.handle, 
			nullptr, 
			*reinterpret_cast<u64*>(&surface));
		if (surfaceCreateResult != vk::Result::eSuccess)
			throw std::runtime_error("Unable to create VkSurfaceKHR object during initialization.");

		surfaceInfo = Init::BuildSurfaceInfo(
			globUtils.instance,
			globUtils.physDevice.handle,
			surface,
			globUtils.logger);
		
		swapchainData.handle = vk::SwapchainKHR();

		HandleSwapchainResizeEvent(
			width,
			height,
			wsiInterface,
			globUtils,
			guiData,
			surfaceInfo,
			swapchainData);
	}
}

#include <unordered_map>
#include <set>

#include "Texas/Texas.hpp"
#include "Texas/VkTools.hpp"
#include "DEngine/Application.hpp"

void DEngine::Gfx::Vk::APIData::Draw(Data& gfxData, DrawParams const& drawParams)
{
	vk::Result vkResult{};
	APIData& apiData = *this;
	GlobUtils const& globUtils = apiData.globUtils;
	DevDispatch const& device = globUtils.device;

	// Handle surface recreation
	if (drawParams.restoreEvent)
		HandleSurfaceRecreationEvent(
			drawParams.swapchainWidth,
			drawParams.swapchainHeight,
			globUtils,
			apiData.surface,
			*apiData.wsiInterface,
			apiData.swapchain,
			apiData.guiData);
	// Handle swapchain resize
	else if (drawParams.swapchainWidth != apiData.swapchain.extents.width || drawParams.swapchainHeight != apiData.swapchain.extents.height)
		HandleSwapchainResizeEvent(
			drawParams.swapchainWidth,
			drawParams.swapchainHeight,
			*apiData.wsiInterface,
			globUtils, 
			apiData.guiData,
			apiData.surface, 
			apiData.swapchain);

	// Deletes and creates viewports when needed.
	ViewportManager::HandleEvents(
		apiData.viewportManager,
		globUtils,
		{ drawParams.viewportUpdates.data(), drawParams.viewportUpdates.size() });
	// Resizes the buffer if the new size is too large.
	ObjectDataManager::HandleResizeEvent(
		apiData.objectDataManager,
		globUtils,
		drawParams.transforms.size());
	TextureManager::Update(
		apiData.textureManager,
		apiData.globUtils,
		drawParams,
		*apiData.test_textureAssetInterface);



	u8 currentInFlightIndex = apiData.currentInFlightFrame;

	vkResult = apiData.globUtils.device.waitForFences(
		apiData.mainFences[currentInFlightIndex], 
		true, 
		static_cast<u64>(-1));
	if (vkResult != vk::Result::eSuccess)
		throw std::runtime_error("Vulkan: Failed to wait for cmd buffer fence.");
	device.resetFences(apiData.mainFences[currentInFlightIndex]);


	// Update object-data
	ObjectDataManager::Update(
		apiData.objectDataManager,
		{ drawParams.transforms.data(), drawParams.transforms.size() },
		currentInFlightIndex);


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
		ViewportData const& viewport = apiData.viewportManager.viewportData[viewportIndex].b;
		vk::CommandBuffer cmdBuffer = viewport.cmdBuffers[currentInFlightIndex];
		cmdBuffersToSubmit.push_back(cmdBuffer);
		RecordGraphicsCmdBuffer(
			apiData.globUtils,
			viewport,
			viewportUpdate,
			drawParams,
			currentInFlightIndex,
			apiData);
	}
	


	// First we re-record the GUI cmd buffer and submit it.
	RecordGUICmdBuffer(
		apiData.globUtils.device,
		apiData.guiData.cmdBuffers[currentInFlightIndex],
		apiData.guiData.renderTarget,
		globUtils.guiRenderPass,
		currentInFlightIndex,
		apiData.globUtils.DebugUtilsPtr());
	cmdBuffersToSubmit.push_back(apiData.guiData.cmdBuffers[currentInFlightIndex]);

	// Submit the command cmdBuffers
	vk::SubmitInfo submitInfo{};
	submitInfo.commandBufferCount = (u32)cmdBuffersToSubmit.size();
	submitInfo.pCommandBuffers = cmdBuffersToSubmit.data();
	apiData.globUtils.queues.graphics.submit(submitInfo, apiData.mainFences[currentInFlightIndex]);


	// We query the next available swapchain image, so we know which image we need to copy to
	PresentShit(apiData);



	DeletionQueue::ExecuteCurrentTick(apiData.globUtils.deletionQueue);
	apiData.currentInFlightFrame = (apiData.currentInFlightFrame + 1) % apiData.globUtils.resourceSetCount;
	apiData.globUtils.SetCurrentResourceSetIndex(apiData.currentInFlightFrame);
}

