#include "Vk.hpp"
#include <DEngine/Gfx/impl/Assert.hpp>

#include <DEngine/Math/LinearTransform3D.hpp>
#include <DEngine/Std/Utility.hpp>

#include "Init.hpp"

using namespace DEngine;
using namespace DEngine::Gfx;

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

	void RecordGUICmdBuffer(
		GlobUtils const& globUtils,
		GuiResourceManager const& guiResManager,
		ViewportManager const& viewportManager,
		WindowGuiData const& guiData,
		vk::CommandBuffer cmdBuffer,
		vk::Framebuffer framebuffer,
		NativeWindowUpdate const& windowUpdate,
		Std::Span<GuiDrawCmd const> guiDrawCmds,
		u8 inFlightIndex)
	{
		DeviceDispatch const& device = globUtils.device;

		{
			vk::RenderPassBeginInfo rpBegin{};
			rpBegin.framebuffer = framebuffer;
			rpBegin.renderPass = globUtils.guiRenderPass;
			rpBegin.renderArea.extent = guiData.extent;
			rpBegin.clearValueCount = 1;
			vk::ClearColorValue clearVal{};
			for (uSize i = 0; i < 4; i++)
				clearVal.float32[i] = windowUpdate.clearColor[i];
			vk::ClearValue test = clearVal;
			rpBegin.pClearValues = &test;

			device.cmdBeginRenderPass(cmdBuffer, rpBegin, vk::SubpassContents::eInline);

			{
				vk::Viewport viewport{};
				viewport.width = (float)guiData.extent.width;
				viewport.height = (float)guiData.extent.height;
				device.cmdSetViewport(cmdBuffer, 0, viewport);
				vk::Rect2D scissor{};
				scissor.extent = guiData.extent;
				device.cmdSetScissor(cmdBuffer, 0, scissor);
			}

			for (GuiDrawCmd const& drawCmd : guiDrawCmds)
			{
				switch (drawCmd.type)
				{
				case GuiDrawCmd::Type::Scissor:
				{
					vk::Rect2D scissor{};
					vk::Extent2D rotatedFramebufferExtent = guiData.extent;
					if (guiData.surfaceRotation == vk::SurfaceTransformFlagBitsKHR::eRotate90 ||
						guiData.surfaceRotation == vk::SurfaceTransformFlagBitsKHR::eRotate270)
						std::swap(rotatedFramebufferExtent.width, rotatedFramebufferExtent.height);
					scissor.extent.width = u32(Math::Round(drawCmd.rectExtent.x * rotatedFramebufferExtent.width));
					scissor.extent.height = u32(Math::Round(drawCmd.rectExtent.y * rotatedFramebufferExtent.height));
					i32 scissorPosX = i32(Math::Round(drawCmd.rectPosition.x * rotatedFramebufferExtent.width));
					i32 scissorPosY = i32(Math::Round(drawCmd.rectPosition.y * rotatedFramebufferExtent.height));
					if (guiData.surfaceRotation == vk::SurfaceTransformFlagBitsKHR::eIdentity)
					{
						scissor.offset.x = scissorPosX;
						scissor.offset.y = scissorPosY;
					}
					else if (guiData.surfaceRotation == vk::SurfaceTransformFlagBitsKHR::eRotate90)
					{
						scissor.offset.x = rotatedFramebufferExtent.height - scissor.extent.height - scissorPosY;
						scissor.offset.y = scissorPosX;
					}
					else if (guiData.surfaceRotation == vk::SurfaceTransformFlagBitsKHR::eRotate270)
					{
						scissor.offset.x = scissorPosY;
						scissor.offset.y = rotatedFramebufferExtent.width - scissor.extent.width - scissorPosX;
					}
					else if (guiData.surfaceRotation == vk::SurfaceTransformFlagBitsKHR::eRotate180)
					{
						scissor.offset.x = rotatedFramebufferExtent.width - scissor.extent.width - scissorPosX;
						scissor.offset.y = rotatedFramebufferExtent.height - scissor.extent.height - scissorPosY;
					}
					if (guiData.surfaceRotation == vk::SurfaceTransformFlagBitsKHR::eRotate90 ||
							guiData.surfaceRotation == vk::SurfaceTransformFlagBitsKHR::eRotate270)
						std::swap(scissor.extent.width, scissor.extent.height);
					device.cmdSetScissor(cmdBuffer, 0, scissor);
				}
				break;

				case GuiDrawCmd::Type::FilledMesh:
				{
					device.cmdBindPipeline(cmdBuffer, vk::PipelineBindPoint::eGraphics, guiResManager.filledMeshPipeline);
					GuiResourceManager::FilledMeshPushConstant pushConstant{};
					pushConstant.color = drawCmd.filledMesh.color;
					pushConstant.orientation = guiData.rotation;
					pushConstant.rectExtent = drawCmd.rectExtent;
					pushConstant.rectOffset = drawCmd.rectPosition;
					device.cmdPushConstants(
						cmdBuffer,
						guiResManager.filledMeshPipelineLayout,
						vk::ShaderStageFlagBits::eVertex,
						0,
						32,
						&pushConstant);
					device.cmdPushConstants(
						cmdBuffer,
						guiResManager.filledMeshPipelineLayout,
						vk::ShaderStageFlagBits::eFragment,
						32,
						sizeof(pushConstant.color),
						&pushConstant.color);
					device.cmdBindVertexBuffers(
						cmdBuffer,
						0,
						guiResManager.vtxBuffer,
						(guiResManager.vtxInFlightCapacity * inFlightIndex) + (drawCmd.filledMesh.mesh.vertexOffset * sizeof(GuiVertex)));
					device.cmdBindIndexBuffer(
						cmdBuffer,
						guiResManager.indexBuffer,
						(guiResManager.indexInFlightCapacity * inFlightIndex) + (drawCmd.filledMesh.mesh.indexOffset * sizeof(u32)),
						vk::IndexType::eUint32);
					device.cmdDrawIndexed(
						cmdBuffer,
						drawCmd.filledMesh.mesh.indexCount,
						1,
						0,
						0,
						0);
				}
					break;

				case GuiDrawCmd::Type::TextGlyph:
				{
					device.cmdBindPipeline(cmdBuffer, vk::PipelineBindPoint::eGraphics, guiResManager.font_pipeline);
					GuiResourceManager::FontPushConstant pushConstant{};
					pushConstant.color = drawCmd.textGlyph.color;
					pushConstant.orientation = guiData.rotation;
					pushConstant.rectExtent = drawCmd.rectExtent;
					pushConstant.rectOffset = drawCmd.rectPosition;
					device.cmdPushConstants(
						cmdBuffer,
						guiResManager.font_pipelineLayout,
						vk::ShaderStageFlagBits::eVertex,
						0,
						32,
						&pushConstant);
					device.cmdPushConstants(
						cmdBuffer,
						guiResManager.font_pipelineLayout,
						vk::ShaderStageFlagBits::eFragment,
						32,
						sizeof(pushConstant.color),
						&pushConstant.color);
					GuiResourceManager::GlyphData glyphData{};
					if (drawCmd.textGlyph.utfValue < GuiResourceManager::lowUtfGlyphDatasSize)
					{
						glyphData = guiResManager.lowUtfGlyphDatas[drawCmd.textGlyph.utfValue];
					}
					else
					{
						auto glyphDataIt = guiResManager.glyphDatas.find((u32)drawCmd.textGlyph.utfValue);
						if (glyphDataIt == guiResManager.glyphDatas.end())
							throw std::runtime_error("DEngine - Vulkan: Unable to find glyph.");
						glyphData = glyphDataIt->second;
					}

					
					device.cmdBindDescriptorSets(
						cmdBuffer,
						vk::PipelineBindPoint::eGraphics,
						guiResManager.font_pipelineLayout,
						0,
						glyphData.descrSet,
						nullptr);
					device.cmdDraw(
						cmdBuffer,
						6,
						1,
						0,
						0);
				}
					break;

				case GuiDrawCmd::Type::Viewport:
				{
					device.cmdBindPipeline(cmdBuffer, vk::PipelineBindPoint::eGraphics, guiResManager.viewportPipeline);
					GuiResourceManager::ViewportPushConstant pushConstant{};
					pushConstant.orientation = guiData.rotation;
					pushConstant.rectExtent = drawCmd.rectExtent;
					pushConstant.rectOffset = drawCmd.rectPosition;
					device.cmdPushConstants(
						cmdBuffer,
						guiResManager.viewportPipelineLayout,
						vk::ShaderStageFlagBits::eVertex,
						0,
						sizeof(pushConstant),
						&pushConstant);
					auto const& viewportData = *std::find_if(
						viewportManager.viewportNodes.begin(),
						viewportManager.viewportNodes.end(),
						[&drawCmd](decltype(viewportManager.viewportNodes)::value_type const& val) -> bool {
							return drawCmd.viewport.id == val.id; });
					device.cmdBindDescriptorSets(
						cmdBuffer,
						vk::PipelineBindPoint::eGraphics,
						guiResManager.viewportPipelineLayout,
						0,
						viewportData.viewport.descrSet,
						nullptr);
					device.cmdDraw(
						cmdBuffer,
						6,
						1,
						0,
						0);
				}
					break;
				}
			}

			device.cmdEndRenderPass(cmdBuffer);
		}
	}
	
	void RecordGraphicsCmdBuffer(
		GlobUtils const& globUtils, 
		vk::CommandBuffer cmdBuffer,
		ObjectDataManager const& objectDataManager,
		TextureManager const& textureManager,
		ViewportData const& viewportData,
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

void Vk::APIData::Draw(
	DrawParams const& drawParams)
{
	APIData& apiData = *this;

	std::unique_lock lock{ apiData.drawParamsLock };
	
	apiData.drawParamsCondVarProducer.wait(lock, [&apiData]() -> bool { return !apiData.drawParamsReady; });

	apiData.drawParams = drawParams;
	apiData.drawParamsReady = true;

	lock.unlock();
	apiData.drawParamsCondVarWorker.notify_one();
}

void Vk::APIData::InternalDraw(DrawParams const& drawParams)
{
	vk::Result vkResult{};
	APIData& apiData = *this;
	DevDispatch const& device = globUtils.device;

	NativeWindowManager::ProcessEvents(
		apiData.nativeWindowManager,
		globUtils,
		{ drawParams.nativeWindowUpdates.data(), drawParams.nativeWindowUpdates.size() });
	// Deletes and creates viewports when needed.
	ViewportManager::ProcessEvents(
		apiData.viewportManager,
		globUtils,
		{ drawParams.viewportUpdates.data(), drawParams.viewportUpdates.size() },
		apiData.guiResourceManager);
	// Resizes the buffer if the new size is too large.
	ObjectDataManager::HandleResizeEvent(
		apiData.objectDataManager,
		globUtils,
		drawParams.transforms.size());
	TextureManager::Update(
		apiData.textureManager,
		globUtils,
		drawParams,
		*apiData.test_textureAssetInterface);

	u8 const currentInFlightIndex = apiData.currInFlightIndex;

	// Wait for fences, so we know the resources are available.
	vkResult = globUtils.device.waitForFences(
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
			[&viewportUpdate](auto const& val) -> bool { return viewportUpdate.id == val.id; });
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
	std::vector<u32> swapchainIndices(drawParams.nativeWindowUpdates.size());
	std::vector<vk::SwapchainKHR> swapchains(drawParams.nativeWindowUpdates.size());
	std::vector<vk::Semaphore> swapchainImageReadySemaphores(drawParams.nativeWindowUpdates.size());
	std::vector<vk::PipelineStageFlags> swapchainImageReadyStages(drawParams.nativeWindowUpdates.size());
	for (auto& item : swapchainImageReadyStages)
		item = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	for (uSize i = 0; i < drawParams.nativeWindowUpdates.size(); i += 1)
	{
		// First assert that this ID doesn't already exist?

		auto const& windowUpdate = drawParams.nativeWindowUpdates[i];
		// First find the window in the database
		auto const windowDataIt = Std::FindIf(
			apiData.nativeWindowManager.nativeWindows.begin(),
			apiData.nativeWindowManager.nativeWindows.end(),
			[&windowUpdate](auto const& node) -> bool { return node.id == windowUpdate.id; });
		DENGINE_IMPL_GFX_ASSERT(windowDataIt != apiData.nativeWindowManager.nativeWindows.end());
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

		RecordGUICmdBuffer(
			globUtils,
			apiData.guiResourceManager,
			apiData.viewportManager,
			nativeWindow.gui,
			mainCmdBuffer,
			guiFramebuffer,
			windowUpdate,
			drawCmds,
			currentInFlightIndex);
	}

	if (cmdBufferBegun)
	{
		globUtils.device.endCommandBuffer(mainCmdBuffer);

		vk::SubmitInfo submitInfo = {};
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &mainCmdBuffer;
		submitInfo.pWaitDstStageMask = swapchainImageReadyStages.data();
		submitInfo.pWaitSemaphores = swapchainImageReadySemaphores.data();
		submitInfo.waitSemaphoreCount = (u32)swapchainImageReadySemaphores.size();
		globUtils.queues.graphics.submit(submitInfo, apiData.mainFences[currentInFlightIndex]);

		if (!swapchainIndices.empty())
		{
			vk::PresentInfoKHR presentInfo{};
			presentInfo.pImageIndices = swapchainIndices.data();
			presentInfo.pSwapchains = swapchains.data();
			presentInfo.swapchainCount = (u32)swapchains.size();
			vkResult = globUtils.queues.graphics.presentKHR(presentInfo);
			if (vkResult != vk::Result::eSuccess && vkResult != vk::Result::eSuboptimalKHR && vkResult != vk::Result::eErrorOutOfDateKHR)
				throw std::runtime_error("DEngine - Vulkan: Presentation submission did not return success result.");
		}
	}


	// Let the deletion-queue run it's tick.
	DeletionQueue::ExecuteTick(
		apiData.globUtils.delQueue,
		globUtils,
		currentInFlightIndex);

	// Increment the in-flight index.
	apiData.currInFlightIndex = (apiData.currInFlightIndex + 1) % apiData.globUtils.inFlightCount;
}

DEngine::Gfx::NativeWindowID DEngine::Gfx::Vk::APIData::NewNativeWindow(WsiInterface& wsiConnection)
{
	APIData& apiData = *this;

	return NativeWindowManager::PushCreateWindowJob(
		apiData.nativeWindowManager,
		wsiConnection);
}

