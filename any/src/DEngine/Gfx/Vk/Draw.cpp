#include "Vk.hpp"
#include <DEngine/Gfx/impl/Assert.hpp>

#include <DEngine/Std/Containers/Defer.hpp>
#include <DEngine/Math/LinearTransform3D.hpp>

#include "Draw_Gui.hpp"
#include "NativeWindowManager.hpp"

#include <iostream>

#if defined(DENGINE_TRACY_LINKED)
#include <tracy/Tracy.hpp>
#include <tracy/TracyC.h>
#endif

import DEngine.Std.Vec;

using namespace DEngine;
using namespace DEngine::Gfx;

void Vk::APIData::Draw(
	DrawParams const& drawParams)
{
	APIData& apiData = *this;

	if constexpr (Gfx::enableDedicatedThread)
	{
		std::unique_lock lock{ apiData.threadLock };

		auto& threadData = apiData.thread;
		thread.drawParamsCondVarProducer.wait(
			lock,
			[&threadData]() { return !threadData.nextJobReady; });

		threadData.drawParams = drawParams;
		threadData.nextJobReady = true;
		threadData.nextJobFn = [](APIData& apiData) {
			APIData::InternalDraw(apiData, apiData.thread.drawParams);
		};
		lock.unlock();
		thread.drawParamsCondVarWorker.notify_one();
	}
	else {
		APIData::InternalDraw(apiData, drawParams);
	}
}

namespace DEngine::Gfx::Vk
{
	void BeginRecordingMainCmdBuffer(
		DeviceDispatch const& device,
		vk::CommandPool cmdPool,
		vk::CommandBuffer cmdBuffer,
		u8 inFlightIndex,
		DebugUtilsDispatch const* debugUtils)
	{
		device.resetCommandPool(cmdPool);

		vk::CommandBufferBeginInfo beginInfo{};
		beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

		device.beginCommandBuffer(cmdBuffer, beginInfo);

		if (debugUtils) {
			std::string name = "Main CmdBuffer #";
			name += std::to_string(inFlightIndex);
			debugUtils->Helper_SetObjectName(device.handle, cmdBuffer, name.c_str());
		}
	}

	static void RecordGraphicsCmdBuffer(
		GlobUtils const& globUtils, 
		vk::CommandBuffer cmdBuffer,
		ObjectDataManager const& objectDataManager,
		TextureManager const& textureManager,
		ViewportManager const& viewportManager, // TODO: Re-architecture so we don't need this
		GuiResourceManager const& guiResManager,
		ViewportMgr_ViewportData const& viewportData,
		ViewportUpdate const& viewportUpdate,
		DrawParams const& drawParams,
		u8 inFlightIndex,
		APIData& test_apiData)
	{
		auto const& device = globUtils.device;

		vk::ClearColorValue colorAttachmentClearVal = {};
		for (uSize i = 0; i < 4; i++)
			colorAttachmentClearVal.float32[i] = viewportUpdate.clearColor[i];

		vk::ClearDepthStencilValue depthStencilAttachmentClearVal = {};
		depthStencilAttachmentClearVal.stencil = 64;

		// Must appear in the same order as list of attachments in the vk::RenderPass
		Std::Array<vk::ClearValue, 2> clearValues = {
			colorAttachmentClearVal,
			depthStencilAttachmentClearVal };

		vk::RenderPassBeginInfo rpBegin = {};
		rpBegin.framebuffer = viewportData.renderTarget.framebuffer.Handle();
		rpBegin.renderPass = globUtils.gfxRenderPass;
		rpBegin.renderArea.extent = viewportData.renderTarget.extent;
		rpBegin.clearValueCount = clearValues.Size();
		rpBegin.pClearValues = clearValues.Data();

		device.cmdBeginRenderPass(cmdBuffer, rpBegin, vk::SubpassContents::eInline);

		vk::Viewport viewport = {};
		viewport.width = static_cast<f32>(viewportData.renderTarget.extent.width);
		viewport.height = static_cast<f32>(viewportData.renderTarget.extent.height);
		device.cmdSetViewport(cmdBuffer, 0, viewport);

		device.cmdBindPipeline(cmdBuffer, vk::PipelineBindPoint::eGraphics, test_apiData.testPipeline);

		for (uSize drawIndex = 0; drawIndex < drawParams.textureIDs.size(); drawIndex += 1) {
			Std::Array<vk::DescriptorSet, 3> descrSets = {
				viewportData.camDataDescrSets[inFlightIndex],
				objectDataManager.GetObjectDataUniformDescrSet(),
				textureManager.database.at(drawParams.textureIDs[drawIndex]).descrSet };

			uSize objectDataBufferOffset = objectDataManager
				.GetObjectDataUniformBufferOffset(inFlightIndex, drawIndex);

			device.cmdBindDescriptorSets(
				cmdBuffer,
				vk::PipelineBindPoint::eGraphics,
				test_apiData.testPipelineLayout,
				0,
				{ (u32)descrSets.Size(), descrSets.Data() },
				(u32)objectDataBufferOffset);
			device.cmdDraw(cmdBuffer, 4, 1, 0, 0);
		}

		// TODO: Iterate over all our GUI planes and draw them!!
		u8 stencilReference = 64;
		device.cmdSetStencilReference(cmdBuffer, vk::StencilFaceFlagBits::eFrontAndBack, stencilReference);
		viewport.width = static_cast<f32>(viewportData.renderTarget.extent.width);
		viewport.height = static_cast<f32>(viewportData.renderTarget.extent.height);
		device.cmdSetViewport(cmdBuffer, 0, viewport);
		vk::Rect2D scissor = {};
		scissor.extent = viewportData.renderTarget.extent;
		device.cmdSetScissor(cmdBuffer, 0, scissor);
		for (int i = 0; i < drawParams.guiPlanes.size(); i += 1) {

			auto const& guiPlane = drawParams.guiPlanes[i];

			DENGINE_IMPL_GFX_ASSERT(guiPlane.drawCmdOffset + guiPlane.drawCmdCount <= drawParams.guiDrawCmds.size());
			auto guiDrawCmds = Std::Span{
				drawParams.guiDrawCmds.data() + guiPlane.drawCmdOffset,
				guiPlane.drawCmdCount };
			auto guiUtfValues = Std::Span{ drawParams.guiUtfValues.data(), drawParams.guiUtfValues.size() };
			auto guiGlyphRects = Std::Span{ drawParams.guiTextGlyphRects.data(), drawParams.guiTextGlyphRects.size() };

			Std::Array<vk::DescriptorSet, 3> descrSets = {
				viewportData.camDataDescrSets[inFlightIndex],
				objectDataManager.GetGuiPlaneObjectDataUniformDescrSet(),
				objectDataManager.GetGuiPlaneDescrSet(inFlightIndex, i)};
			Std::Array<u32, 1> descrOffsets = {
				objectDataManager.GetGuiPlaneObjectDataUniformOffset(inFlightIndex, i)
			};

			RecordGuiDrawCmds_Params guiDrawParams = {
				.globUtils = globUtils,
				.guiResManager = guiResManager,
				.viewportManager = viewportManager,
				.cmdBuffer = cmdBuffer,
				.descrSets = descrSets.ToSpan(),
				.descrDynamicOffsets = descrOffsets.ToSpan(),
				.guiDrawCmds = guiDrawCmds,
				.utfValues = guiUtfValues,
				.glyphRects = guiGlyphRects,
				.rotation = Gfx::WindowRotation::e0,
				.windowExtent = { guiPlane.extentPx.width, guiPlane.extentPx.height },
				.stencilReference = stencilReference,
			};
			RecordGuiDrawCmds(guiDrawParams);
		}

		// Draw our lines
		if (!drawParams.lineDrawCmds.empty()) {
			GizmoManager::DebugLines_RecordDrawCalls(
				test_apiData.gizmoManager,
				globUtils,
				viewportData,
				{ drawParams.lineDrawCmds.data(),drawParams.lineDrawCmds.size() },
				cmdBuffer,
				inFlightIndex);
		}

		// Draw the gizmo for this viewport
		if (viewportUpdate.gizmoOpt.HasValue()) {
			ViewportUpdate::Gizmo gizmo = viewportUpdate.gizmoOpt.Value();

			GizmoManager::Gizmo_RecordDrawCalls(
				test_apiData.gizmoManager,
				globUtils,
				viewportData,
				gizmo,
				cmdBuffer,
				inFlightIndex);
		}
		
		globUtils.device.cmdEndRenderPass(cmdBuffer);
	}
}

void Vk::APIData::InternalDraw(APIData& apiData, DrawParams const& drawParams)
{
#ifdef DENGINE_TRACY_LINKED
	TracyCZoneNS(tracy_draw, "Rendering thread tick", 10, true);
#endif

	vk::Result vkResult = {};
	auto const& globUtils = apiData.m_globUtils;
	auto const& device = globUtils.device;
	auto& delQueue = apiData.delQueue;
	auto const& transientAlloc = Std::AllocRef{ apiData.frameAllocator };

	auto& vma = apiData.m_globUtils.vma;

	auto const debugUtils = globUtils.DebugUtilsPtr();
	auto& nativeWinMgr = apiData.nativeWindowManager;
	auto& viewportMan = apiData.viewportManager;
	auto& guiResourceMan = apiData.guiResourceManager;
	auto& objectDataMan = apiData.objectDataManager;

	Std::Span nativeWindowUpdates = {
		drawParams.nativeWindowUpdates.data(),
		drawParams.nativeWindowUpdates.size() };
	Std::Span transforms = {
		drawParams.transforms.data(),
		drawParams.transforms.size() };
	Std::Span viewportUpdates = {
		drawParams.viewportUpdates.data(),
		drawParams.viewportUpdates.size() };

	// Increment the in-flight index.
	Std::Defer _incrementIndex { [&apiData] {
		apiData.IncrementInFlightIndex();
		apiData.IncrementMainFenceIndex();
		apiData.frameAllocator.Reset();
	} };

	auto inFlightIndex = apiData.currInFlightIndex;
	auto mainFenceIndex = apiData.MainFenceIndex();

	// Let the deletion-queue run its tick.
	Std::Defer delQueueCleanup { [&] {
		DeletionQueue::ExecuteTick(
			apiData.delQueue,
			globUtils,
			inFlightIndex);
	} };

	auto mainFence = apiData.mainFences[mainFenceIndex].Handle();
	auto& stagingBufferAlloc = apiData.mainStagingBufferAllocs[inFlightIndex];
	StagingBufferAlloc::Reset(stagingBufferAlloc);

	auto mainCmdPool = apiData.mainCmdPools[inFlightIndex].Handle();
	auto mainCmdBuffer = apiData.mainCmdBuffers[inFlightIndex];
	BeginRecordingMainCmdBuffer(
		device,
		mainCmdPool,
		mainCmdBuffer,
		inFlightIndex,
		debugUtils);

	// Events that can happen right away?...
	{
		//ZoneScopedNS("Renderer manager updates", 10);
		NativeWinMgr::ProcessEvents(
			nativeWinMgr,
			globUtils,
			delQueue,
			transientAlloc,
			nativeWindowUpdates);
		auto rangeFn = [&nativeWinMgr](int i) {
			auto const& windowNode = nativeWinMgr.GetWindowData(i);
			GuiResourceManager::UpdateWindowUniforms_Params::WindowInfo out = {};
			out.windowId = windowNode.id;
			out.orientation = windowNode.windowData.GfxRotation();
			out.resolutionWidth = (i32)windowNode.windowData.extent.width;
			out.resolutionHeight = (i32)windowNode.windowData.extent.height;
			return out;
		};
		GuiResourceManager::UpdateWindowUniforms_Params temp = {
			.device = device,
			.delQueue = delQueue,
			.vma = vma,
			.stagingBufferAlloc = stagingBufferAlloc,
			.transientAlloc = transientAlloc,
			.debugUtils = debugUtils,
			.stagingCmdBuffer = mainCmdBuffer,
			.windowRangeRef = { nativeWinMgr.WindowCount(), rangeFn },
			.inFlightIndex = inFlightIndex };
		GuiResourceManager::UpdateWindowUniforms(guiResourceMan, temp);

		ViewportManager::ProcessEvents(
			viewportMan,
			{
				.globUtils = globUtils,
				.cmdBuffer = mainCmdBuffer,
				.delQueue = delQueue,
				.transientAlloc = transientAlloc,
				.viewportUpdates = viewportUpdates,
				.inFlightIndex = inFlightIndex,
				.debugUtils = debugUtils });

		ObjectDataManager::Update(
			objectDataMan,
			device,
			vma,
			transforms,
			{ drawParams.guiPlanes.data(), drawParams.guiPlanes.size() },
			mainCmdBuffer,
			delQueue,
			inFlightIndex,
			transientAlloc,
			debugUtils);

		TextureManager::Update(
			apiData.textureManager,
			globUtils,
			delQueue,
			stagingBufferAlloc,
			mainCmdBuffer,
			drawParams,
			*apiData.test_textureAssetInterface,
			transientAlloc);
	}

	for (auto const& viewportUpdate : drawParams.viewportUpdates) {
		auto const viewportDataIt = Std::FindIf(
			apiData.viewportManager.viewportNodes.begin(),
			apiData.viewportManager.viewportNodes.end(),
			[&viewportUpdate](auto const& val) { return viewportUpdate.id == val.id; });
		DENGINE_IMPL_GFX_ASSERT(viewportDataIt != apiData.viewportManager.viewportNodes.end());
		auto const& viewportNode = *viewportDataIt;

		RecordGraphicsCmdBuffer(
			globUtils,
			mainCmdBuffer,
			apiData.objectDataManager,
			apiData.textureManager,
			apiData.viewportManager,
			apiData.guiResourceManager,
			viewportNode.viewport,
			viewportUpdate,
			drawParams,
			inFlightIndex,
			apiData);
	}

	StagingBufferAlloc::Finalize(stagingBufferAlloc, vma);

	// Wait for the main fences

	{
		// Wait for main fence, so we know the resources are available.
		vkResult = device.waitForFences(
			mainFence,
			true,
			3000000000); // Added a 3s timeout for testing purposes
		if (vkResult == vk::Result::eTimeout)
			throw std::runtime_error("Vulkan: Failed to wait for cmd buffer fence. Timed out.");
		else if (vkResult != vk::Result::eSuccess)
			throw std::runtime_error("Vulkan: Failed to wait for cmd buffer fence. Unknown error.");
		device.resetFences(mainFence);
	}

	// Record all the GUI shit.
	auto windowUpdateCount = (int)drawParams.nativeWindowUpdates.size();
	auto swapchainIndices = Std::NewVec_Reserve<u32>(transientAlloc, windowUpdateCount);
	auto swapchains = Std::NewVec_Reserve<vk::SwapchainKHR>(transientAlloc, windowUpdateCount);
	auto swapchainImageReadySemaphores = Std::NewVec_Reserve<vk::Semaphore>(transientAlloc, windowUpdateCount);
	auto renderFinishedSemaphores = Std::NewVec_Reserve<vk::Semaphore>(transientAlloc, windowUpdateCount);
	auto swapchainImageReadyStages = Std::NewVec_Fill<vk::PipelineStageFlags>(
		transientAlloc,
		windowUpdateCount,
		vk::PipelineStageFlagBits::eColorAttachmentOutput);

	for (int i = 0; i < windowUpdateCount; i += 1) {
		auto const& windowUpdate = drawParams.nativeWindowUpdates[i];
		auto const& nativeWindow = nativeWinMgr.GetWindowData(windowUpdate.id);

		auto const acquireResult = device.acquireNextImageKHR(
			nativeWindow.swapchain,
			std::numeric_limits<u64>::max(),
			nativeWindow.swapchainImgReadySems[mainFenceIndex],
			vk::Fence());
		if (acquireResult.result == vk::Result::eErrorOutOfDateKHR) {
            // Do we skip this presentation and schedule a recreation for next frame?
            NativeWinMgr::TagSwapchainOutOfDate(nativeWinMgr, windowUpdate.id);
            static int count = 0;
            std::cout << "Swapchain out of date " << count++ << std::endl;
            break;
        } else if (acquireResult.result == vk::Result::eSuboptimalKHR) {
            // Do nothing
		} else if (acquireResult.result != vk::Result::eSuccess) {
			// TODO: On Android debugging we can often get timeout error, we should probably be able
			// to handle that and stop rendering for a while in those cases.
			throw std::runtime_error("DEngine - Vulkan: Acquiring next swapchain image did not return success result.");
		}

		u32 swapchainIndex = acquireResult.value;

		// Set the presentation stuff
		swapchainIndices.PushBack(swapchainIndex);
		swapchains.PushBack(nativeWindow.swapchain);
		swapchainImageReadySemaphores.PushBack(nativeWindow.swapchainImgReadySems[mainFenceIndex]);
		renderFinishedSemaphores.PushBack(nativeWindow.renderFinishedSems[swapchainIndex]);

		auto guiFramebuffer = nativeWindow.framebuffers[swapchainIndex];

		Std::Span<GuiDrawCmd const> drawCmds;
		if (!drawParams.guiDrawCmds.empty()) {
			DENGINE_IMPL_GFX_ASSERT((u64)windowUpdate.drawCmdOffset + (u64)windowUpdate.drawCmdCount <= drawParams.guiDrawCmds.size());
			drawCmds = { &drawParams.guiDrawCmds[windowUpdate.drawCmdOffset], windowUpdate.drawCmdCount };
		}

		RecordGuiRenderPass_Params recordGuiParams = {
			.globUtils = globUtils,
			.guiResManager = guiResourceMan,
			.viewportManager = apiData.viewportManager,
			.windowUpdate = windowUpdate, };
		recordGuiParams.cmdBuffer = mainCmdBuffer;
		recordGuiParams.guiDrawCmds = drawCmds;
		recordGuiParams.framebuffer = guiFramebuffer;
		recordGuiParams.inFlightIndex = inFlightIndex;
		recordGuiParams.utfValues = { drawParams.guiUtfValues.data(), drawParams.guiUtfValues.size() };
		recordGuiParams.glyphRects = { drawParams.guiTextGlyphRects.data(), drawParams.guiTextGlyphRects.size() };
		recordGuiParams.rotation = nativeWindow.GfxRotation();
		recordGuiParams.windowExtent = nativeWindow.extent;
		RecordGuiRenderPass(recordGuiParams);
	}

	device.endCommandBuffer(mainCmdBuffer);

	vk::SubmitInfo submitInfo = {};
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &mainCmdBuffer;
	submitInfo.pWaitDstStageMask = swapchainImageReadyStages.Data();
	submitInfo.pWaitSemaphores = swapchainImageReadySemaphores.Data();
	submitInfo.waitSemaphoreCount = (u32)swapchainImageReadySemaphores.Size();
	submitInfo.pSignalSemaphores = renderFinishedSemaphores.Data();
	submitInfo.signalSemaphoreCount = (u32)renderFinishedSemaphores.Size();
	globUtils.queues.graphics.submit(submitInfo, mainFence);

	if (!swapchainIndices.Empty()) {
		vk::PresentInfoKHR presentInfo = {};
		presentInfo.pImageIndices = swapchainIndices.Data();
		presentInfo.pSwapchains = swapchains.Data();
		presentInfo.swapchainCount = (u32)swapchains.Size();
		presentInfo.pWaitSemaphores = renderFinishedSemaphores.Data();
		presentInfo.waitSemaphoreCount = (u32)renderFinishedSemaphores.Size();
		vkResult = globUtils.queues.graphics.presentKHR(presentInfo);
		if (vkResult != vk::Result::eSuccess && vkResult != vk::Result::eSuboptimalKHR && vkResult != vk::Result::eErrorOutOfDateKHR)
			throw std::runtime_error("DEngine - Vulkan: Presentation submission did not return success result.");
	}

	apiData.tickCount++;

	#ifdef DENGINE_TRACY_LINKED
		TracyCZoneEnd(tracy_draw)
	#endif
}

void Vk::APIData::NewNativeWindow(NativeWindowID windowId)
{
	APIData& apiData = *this;

	// TODO: Should the public API pass down the size hint?
	return NativeWinMgr_PushCreateWindowJob(
		apiData.nativeWindowManager,
		windowId,
		Std::nullOpt,
		Std::nullOpt);
}

void Vk::APIData::DeleteNativeWindow(NativeWindowID windowId)
{
	APIData& apiData = *this;

	return NativeWinMgr_PushDeleteWindowJob(
		apiData.nativeWindowManager,
		windowId);
}

