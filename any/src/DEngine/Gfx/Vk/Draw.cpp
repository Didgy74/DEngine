#include "Vk.hpp"
#include <DEngine/Gfx/impl/Assert.hpp>

#include <DEngine/Std/Containers/Defer.hpp>
#include <DEngine/Std/Containers/Vec.hpp>
#include <DEngine/Math/LinearTransform3D.hpp>
#include <DEngine/Std/Utility.hpp>

#include "Draw_Gui.hpp"

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
			[&threadData]() { return !threadData.drawParamsReady; });

		threadData.drawParams = drawParams;
		threadData.drawParamsReady = true;
		threadData.nextCmd = APIData::Thread::NextCmd::Draw;

		thread.drawParamsCondVarWorker.notify_one();
	}
	else
	{
		apiData.InternalDraw(drawParams);
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
		if (debugUtils)
		{
			std::string name = "Main CmdBuffer #";
			name += std::to_string(inFlightIndex);
			debugUtils->Helper_SetObjectName(device.handle, cmdBuffer, name.c_str());
		}

		device.resetCommandPool(cmdPool);

		vk::CommandBufferBeginInfo beginInfo{};
		beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

		device.beginCommandBuffer(cmdBuffer, beginInfo);
	}

	void RecordGraphicsCmdBuffer(
		GlobUtils const& globUtils, 
		vk::CommandBuffer cmdBuffer,
		ObjectDataManager const& objectDataManager,
		TextureManager const& textureManager,
		ViewportMgr_ViewportData const& viewportData,
		ViewportUpdate const& viewportUpdate,
		DrawParams const& drawParams,
		u8 inFlightIndex,
		APIData& test_apiData)
	{
		vk::RenderPassBeginInfo rpBegin = {};
		rpBegin.framebuffer = viewportData.renderTarget.framebuffer;
		rpBegin.renderPass = globUtils.gfxRenderPass;
		rpBegin.renderArea.extent = viewportData.renderTarget.extent;
		rpBegin.clearValueCount = 1;

		vk::ClearColorValue clearVal = {};
		clearVal.setFloat32({
			viewportUpdate.clearColor.x,
			viewportUpdate.clearColor.y,
			viewportUpdate.clearColor.z,
			viewportUpdate.clearColor.w });
		vk::ClearValue clear = clearVal;
		rpBegin.pClearValues = &clear;
		globUtils.device.cmdBeginRenderPass(cmdBuffer, rpBegin, vk::SubpassContents::eInline);

		vk::Viewport viewport{};
		viewport.width = static_cast<f32>(viewportData.renderTarget.extent.width);
		viewport.height = static_cast<f32>(viewportData.renderTarget.extent.height);
		globUtils.device.cmdSetViewport(cmdBuffer, 0, viewport);

		globUtils.device.cmdBindPipeline(cmdBuffer, vk::PipelineBindPoint::eGraphics, test_apiData.testPipeline);

		for (uSize drawIndex = 0; drawIndex < drawParams.textureIDs.size(); drawIndex += 1)
		{
			Std::Array<vk::DescriptorSet, 3> descrSets = {
				viewportData.camDataDescrSets[inFlightIndex],
				objectDataManager.descrSet,
				textureManager.database.at(drawParams.textureIDs[drawIndex]).descrSet };

			uSize objectDataBufferOffset = 0;
			objectDataBufferOffset += objectDataManager.capacity * objectDataManager.elementSize * inFlightIndex;
			objectDataBufferOffset += objectDataManager.elementSize * drawIndex;

			globUtils.device.cmdBindDescriptorSets(
				cmdBuffer,
				vk::PipelineBindPoint::eGraphics,
				test_apiData.testPipelineLayout,
				0,
				{ (u32)descrSets.Size(), descrSets.Data() },
				(u32)objectDataBufferOffset);
			globUtils.device.cmdDraw(cmdBuffer, 4, 1, 0, 0);
		}

		// Draw our lines
		if (!drawParams.lineDrawCmds.empty())
		{
			GizmoManager::DebugLines_RecordDrawCalls(
				test_apiData.gizmoManager,
				globUtils,
				viewportData,
				{ drawParams.lineDrawCmds.data(),drawParams.lineDrawCmds.size() },
				cmdBuffer,
				inFlightIndex);
		}

		// Draw the gizmo for this viewport
		if (viewportUpdate.gizmoOpt.HasValue())
		{
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

void Vk::APIData::InternalDraw(DrawParams const& drawParams)
{
	vk::Result vkResult = {};
	APIData& apiData = *this;
	auto const& device = globUtils.device;
	auto& delQueue = apiData.delQueue;
	auto const& transientAlloc = Std::AllocRef{ apiData.frameAllocator };
	Std::Defer allocCleanup { [&apiData]() {
		apiData.frameAllocator.Reset();
	}};

	NativeWinMgr::ProcessEvents(
		apiData.nativeWindowManager,
		globUtils,
		delQueue,
		transientAlloc,
		{ drawParams.nativeWindowUpdates.data(), drawParams.nativeWindowUpdates.size() });
	// Deletes and creates viewports when needed.
	ViewportManager::ProcessEvents(
		apiData.viewportManager,
		globUtils,
		delQueue,
		transientAlloc,
		{ drawParams.viewportUpdates.data(), drawParams.viewportUpdates.size() },
		apiData.guiResourceManager);
	// Resizes the buffer if the new size is too large.
	ObjectDataManager::HandleResizeEvent(
		apiData.objectDataManager,
		globUtils,
		delQueue,
		drawParams.transforms.size());
	TextureManager::Update(
		apiData.textureManager,
		globUtils,
		delQueue,
		drawParams,
		*apiData.test_textureAssetInterface,
		transientAlloc);

	u8 const currentInFlightIndex = apiData.currInFlightIndex;

	// Wait for main fence, so we know the resources are available.
	vkResult = device.waitForFences(
		apiData.mainFences[currentInFlightIndex],
		true,
		3000000000); // Added a 3s timeout for testing purposes
	if (vkResult != vk::Result::eSuccess)
		throw std::runtime_error("Vulkan: Failed to wait for cmd buffer fence.");
	device.resetFences(apiData.mainFences[currentInFlightIndex]);

	// Update object-data
	ObjectDataManager::Update(
		apiData.objectDataManager,
		{ drawParams.transforms.data(), drawParams.transforms.size() },
		currentInFlightIndex);
	// Update GUI vertices and indices
	GuiResourceManager::Update(
		apiData.guiResourceManager,
		globUtils,
		{ drawParams.guiVertices.data(), drawParams.guiVertices.size() },
		{ drawParams.guiIndices.data(), drawParams.guiIndices.size() },
		currentInFlightIndex);
	ViewportManager::UpdateCameras(
		apiData.viewportManager,
		apiData.globUtils,
		{ drawParams.viewportUpdates.data(), drawParams.viewportUpdates.size() },
		currentInFlightIndex);
	GizmoManager::UpdateLineVtxBuffer(
		apiData.gizmoManager,
		globUtils,
		currentInFlightIndex,
		{ drawParams.lineVertices.data(), drawParams.lineVertices.size() });

	vk::CommandBuffer mainCmdBuffer = apiData.mainCmdBuffers[currentInFlightIndex];
	bool cmdBufferBegun = false;

	for (auto const& viewportUpdate : drawParams.viewportUpdates)
	{
		auto const viewportDataIt = Std::FindIf(
			apiData.viewportManager.viewportNodes.begin(),
			apiData.viewportManager.viewportNodes.end(),
			[&viewportUpdate](auto const& val) { return viewportUpdate.id == val.id; });
		DENGINE_IMPL_GFX_ASSERT(viewportDataIt != apiData.viewportManager.viewportNodes.end());
		ViewportManager::Node const& viewportNode = *viewportDataIt;
		
		if (!cmdBufferBegun)
		{
			BeginRecordingMainCmdBuffer(
				globUtils.device,
				apiData.mainCmdPools[currentInFlightIndex],
				mainCmdBuffer,
				currentInFlightIndex,
				globUtils.DebugUtilsPtr());
			cmdBufferBegun = true;
		}

		RecordGraphicsCmdBuffer(
			globUtils,
			mainCmdBuffer,
			apiData.objectDataManager,
			apiData.textureManager,
			viewportNode.viewport,
			viewportUpdate,
			drawParams,
			currentInFlightIndex,
			apiData);
	}

	// Record all the GUI shit.
	auto swapchainIndices = Std::NewVec<u32>(transientAlloc);
	swapchainIndices.Resize(drawParams.nativeWindowUpdates.size());
	auto swapchains = Std::NewVec<vk::SwapchainKHR>(transientAlloc);
	swapchains.Resize(drawParams.nativeWindowUpdates.size());
	auto swapchainImageReadySemaphores = Std::NewVec<vk::Semaphore>(transientAlloc);
	swapchainImageReadySemaphores.Resize(drawParams.nativeWindowUpdates.size());
	auto swapchainImageReadyStages = Std::NewVec<vk::PipelineStageFlags>(transientAlloc);
	swapchainImageReadyStages.Resize(drawParams.nativeWindowUpdates.size());
	for (auto& item : swapchainImageReadyStages)
		item = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	for (uSize i = 0; i < drawParams.nativeWindowUpdates.size(); i += 1)
	{
		auto const& windowUpdate = drawParams.nativeWindowUpdates[i];
		// First find the window in the database
		auto const windowDataIt = Std::FindIf(
			apiData.nativeWindowManager.main.nativeWindows.begin(),
			apiData.nativeWindowManager.main.nativeWindows.end(),
			[&windowUpdate](auto const& node) { return node.id == windowUpdate.id; });
		DENGINE_IMPL_GFX_ASSERT(windowDataIt != apiData.nativeWindowManager.main.nativeWindows.end());
		auto const& nativeWindow = *windowDataIt;

		if (!cmdBufferBegun)
		{
			BeginRecordingMainCmdBuffer(
				globUtils.device,
				apiData.mainCmdPools[currentInFlightIndex],
				mainCmdBuffer,
				currentInFlightIndex,
				globUtils.DebugUtilsPtr());
			cmdBufferBegun = true;
		}

		Std::Span<GuiDrawCmd const> drawCmds;
		if (!drawParams.guiDrawCmds.empty())
		{
			DENGINE_IMPL_GFX_ASSERT((u64)windowUpdate.drawCmdOffset + (u64)windowUpdate.drawCmdCount <= drawParams.guiDrawCmds.size());
			drawCmds = { &drawParams.guiDrawCmds[windowUpdate.drawCmdOffset], windowUpdate.drawCmdCount };
		}

		auto const acquireResult = device.acquireNextImageKHR(
			nativeWindow.windowData.swapchain,
			std::numeric_limits<u64>::max(),
			nativeWindow.windowData.swapchainImgReadySem,
			vk::Fence());
		if (acquireResult.result != vk::Result::eSuccess)
			throw std::runtime_error("DEngine - Vulkan: Acquiring next swapchain image did not return success result.");

		u32 index = acquireResult.value;
		swapchainIndices[i] = index;
		swapchains[i] = nativeWindow.windowData.swapchain;
		swapchainImageReadySemaphores[i] = nativeWindow.windowData.swapchainImgReadySem;

		vk::Framebuffer guiFramebuffer = nativeWindow.windowData.framebuffers[index];

		RecordGuiCmds_Params recordGuiParams = {
			.globUtils = globUtils,
			.guiResManager = apiData.guiResourceManager,
			.viewportManager = apiData.viewportManager,
			.guiData = nativeWindow.gui,
			.windowUpdate = windowUpdate };
		recordGuiParams.cmdBuffer = mainCmdBuffer;
		recordGuiParams.guiDrawCmds = drawCmds;
		recordGuiParams.framebuffer = guiFramebuffer;
		recordGuiParams.inFlightIndex = currentInFlightIndex;
		RecordGuiCmds(recordGuiParams);
	}

	if (cmdBufferBegun)
	{
		globUtils.device.endCommandBuffer(mainCmdBuffer);

		vk::SubmitInfo submitInfo = {};
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &mainCmdBuffer;
		submitInfo.pWaitDstStageMask = swapchainImageReadyStages.Data();
		submitInfo.pWaitSemaphores = swapchainImageReadySemaphores.Data();
		submitInfo.waitSemaphoreCount = (u32)swapchainImageReadySemaphores.Size();
		globUtils.queues.graphics.submit(submitInfo, apiData.mainFences[currentInFlightIndex]);

		if (!swapchainIndices.Empty())
		{
			vk::PresentInfoKHR presentInfo{};
			presentInfo.pImageIndices = swapchainIndices.Data();
			presentInfo.pSwapchains = swapchains.Data();
			presentInfo.swapchainCount = (u32)swapchains.Size();
			vkResult = globUtils.queues.graphics.presentKHR(presentInfo);
			if (vkResult != vk::Result::eSuccess && vkResult != vk::Result::eSuboptimalKHR && vkResult != vk::Result::eErrorOutOfDateKHR)
				throw std::runtime_error("DEngine - Vulkan: Presentation submission did not return success result.");
		}
	}


	// Let the deletion-queue run its tick.
	DeletionQueue::ExecuteTick(
		apiData.delQueue,
		globUtils,
		currentInFlightIndex);

	// Increment the in-flight index.
	apiData.currInFlightIndex = (apiData.currInFlightIndex + 1) % apiData.globUtils.inFlightCount;
}

void Vk::APIData::NewNativeWindow(NativeWindowID windowId)
{
	APIData& apiData = *this;

	return NativeWinMgr_PushCreateWindowJob(
		apiData.nativeWindowManager,
		windowId);
}

void Vk::APIData::DeleteNativeWindow(NativeWindowID windowId)
{
	APIData& apiData = *this;

	return NativeWinMgr_PushDeleteWindowJob(
		apiData.nativeWindowManager,
		windowId);
}

