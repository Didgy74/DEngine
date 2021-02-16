#include "Vk.hpp"
#include <DEngine/Gfx/detail/Assert.hpp>

#include <DEngine/Math/LinearTransform3D.hpp>
#include <DEngine/Std/Utility.hpp>

#include "Init.hpp"

using namespace DEngine;
using namespace DEngine::Gfx;

namespace DEngine::Gfx::Vk
{
	void RecordGUICmdBuffer(
		GlobUtils const& globUtils,
		GuiResourceManager const& guiResManager,
		ViewportManager const& viewportManager,
		WindowGuiData const& guiData,
		vk::CommandBuffer cmdBuffer,
		NativeWindowUpdate const& windowUpdate,
		Std::Span<GuiDrawCmd const> guiDrawCmds,
		u8 inFlightIndex)
	{
		DeviceDispatch const& device = globUtils.device;

		// We need to name the command buffer every time we reset it
		if (globUtils.UsingDebugUtils())
		{
			std::string name;
			name += "Native Window #";
			name += std::to_string((u64)windowUpdate.id);
			name += " - GUI CmdBuffer #";
			name += std::to_string(inFlightIndex);
			globUtils.debugUtils.Helper_SetObjectName(device.handle, cmdBuffer, name.c_str());
		}

		{
			vk::CommandBufferBeginInfo beginInfo{};
			beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
			device.beginCommandBuffer(cmdBuffer, beginInfo);

			vk::RenderPassBeginInfo rpBegin{};
			rpBegin.framebuffer = guiData.framebuffer;
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

			device.endCommandBuffer(cmdBuffer);
		}
	}

	void RecordGizmoDrawCalls(
		GlobUtils const& globUtils,
		GizmoManager const& gizmoManager,
		ViewportData const& viewportData,
		ViewportUpdate::Gizmo gizmo,
		vk::CommandBuffer cmdBuffer,
		u8 inFlightIndex)
	{
		globUtils.device.cmdBindPipeline(cmdBuffer, vk::PipelineBindPoint::eGraphics, gizmoManager.arrowPipeline);

		globUtils.device.cmdBindVertexBuffers(cmdBuffer, 0, { gizmoManager.arrowVtxBuffer }, { 0 });
		Std::Array<vk::DescriptorSet, 1> descrSets = {
			viewportData.camDataDescrSets[inFlightIndex] };
		globUtils.device.cmdBindDescriptorSets(
			cmdBuffer,
			vk::PipelineBindPoint::eGraphics,
			gizmoManager.pipelineLayout,
			0,
			{ (u32)descrSets.Size(), descrSets.Data() },
			nullptr);

		// Draw X arrow
		{
			Math::Mat4 gizmoMatrix = Math::LinTran3D::Scale_Homo(gizmo.scale, gizmo.scale, gizmo.scale);
			gizmoMatrix = Math::LinTran3D::Rotate_Homo(Math::ElementaryAxis::Y, Math::pi / 2) * gizmoMatrix;
			Math::LinTran3D::SetTranslation(gizmoMatrix, gizmo.position);
			globUtils.device.cmdPushConstants(
				cmdBuffer,
				gizmoManager.pipelineLayout,
				vk::ShaderStageFlagBits::eVertex,
				0,
				sizeof(gizmoMatrix),
				&gizmoMatrix);
			Math::Vec4 color = { 1.f, 0.f, 0.f, 1.f };
			globUtils.device.cmdPushConstants(
				cmdBuffer,
				gizmoManager.pipelineLayout,
				vk::ShaderStageFlagBits::eFragment,
				sizeof(gizmoMatrix),
				sizeof(color),
				&color);

			globUtils.device.cmdDraw(
				cmdBuffer,
				gizmoManager.arrowVtxCount,
				1,
				0,
				0);
		}
		// Draw Y arrow
		{
			Math::Mat4 gizmoMatrix = Math::LinTran3D::Scale_Homo(gizmo.scale, gizmo.scale, gizmo.scale);
			gizmoMatrix = Math::LinTran3D::Rotate_Homo(Math::ElementaryAxis::X, -Math::pi / 2) * gizmoMatrix;
			Math::LinTran3D::SetTranslation(gizmoMatrix, gizmo.position);
			globUtils.device.cmdPushConstants(
				cmdBuffer,
				gizmoManager.pipelineLayout,
				vk::ShaderStageFlagBits::eVertex,
				0,
				sizeof(gizmoMatrix),
				&gizmoMatrix);
			Math::Vec4 color = { 0.f, 1.f, 0.f, 1.f };
			globUtils.device.cmdPushConstants(
				cmdBuffer,
				gizmoManager.pipelineLayout,
				vk::ShaderStageFlagBits::eFragment,
				sizeof(gizmoMatrix),
				sizeof(color),
				&color);
			globUtils.device.cmdDraw(
				cmdBuffer,
				gizmoManager.arrowVtxCount,
				1,
				0,
				0);
		}

		// Draw the floating quad thing
		{
			globUtils.device.cmdBindPipeline(cmdBuffer, vk::PipelineBindPoint::eGraphics, gizmoManager.quadPipeline);

			Math::Mat4 gizmoMatrix = Math::LinTran3D::Scale_Homo(gizmo.quadScale, gizmo.quadScale, gizmo.quadScale);
			Math::Vec3 translation = gizmo.position;
			translation += Math::Vec3{ 1.f, 1.f, 0.f } * gizmo.quadOffset;

			Math::LinTran3D::SetTranslation(gizmoMatrix, translation);
			globUtils.device.cmdPushConstants(
				cmdBuffer,
				gizmoManager.pipelineLayout,
				vk::ShaderStageFlagBits::eVertex,
				0,
				sizeof(gizmoMatrix),
				&gizmoMatrix);
			Math::Vec4 color = { 1.f, 1.f, 0.f, 0.75f };
			globUtils.device.cmdPushConstants(
				cmdBuffer,
				gizmoManager.pipelineLayout,
				vk::ShaderStageFlagBits::eFragment,
				sizeof(gizmoMatrix),
				sizeof(color),
				&color);
			globUtils.device.cmdDraw(
				cmdBuffer,
				4,
				1,
				0,
				0);
		}
	}
	
	// Assumes the resource set is available
	void RecordGraphicsCmdBuffer(
		GlobUtils const& globUtils, 
		ObjectDataManager const& objectDataManager,
		TextureManager const& textureManager,
		ViewportData const& viewportData,
		ViewportUpdate const& viewportUpdate,
		DrawParams const& drawParams,
		u8 inFlightIndex,
		APIData& test_apiData)
	{
		vk::CommandBuffer cmdBuffer = viewportData.cmdBuffers[inFlightIndex];

		// We need to rename the command buffer every time we record it
		if (globUtils.UsingDebugUtils())
		{
			std::string name;
			name += "Graphics viewport #";
			name += std::to_string((u64)viewportUpdate.id);
			name += std::string(" - CmdBuffer #");
			name += std::to_string(inFlightIndex);
			globUtils.debugUtils.Helper_SetObjectName(
				globUtils.
				device.handle, 
				cmdBuffer, name.c_str());
		}

		vk::CommandBufferBeginInfo beginInfo{};
		beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
		globUtils.device.beginCommandBuffer(cmdBuffer, beginInfo);

		vk::RenderPassBeginInfo rpBegin{};
		rpBegin.framebuffer = viewportData.renderTarget.framebuffer;
		rpBegin.renderPass = globUtils.gfxRenderPass;
		rpBegin.renderArea.extent = viewportData.renderTarget.extent;
		rpBegin.clearValueCount = 1;
		vk::ClearColorValue clearVal{};
		clearVal.setFloat32({ 0.f, 0.f, 0.5f, 1.f });
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
			globUtils.device.cmdBindPipeline(
				cmdBuffer,
				vk::PipelineBindPoint::eGraphics,
				test_apiData.gizmoManager.linePipeline);

			Std::Array<vk::DescriptorSet, 1> descrSets = {
				viewportData.camDataDescrSets[inFlightIndex] };
			globUtils.device.cmdBindDescriptorSets(
				cmdBuffer,
				vk::PipelineBindPoint::eGraphics,
				test_apiData.gizmoManager.pipelineLayout,
				0,
				{ (u32)descrSets.Size(), descrSets.Data() },
				nullptr);

			Math::Mat4 gizmoMatrix = Math::Mat4::Identity();
			globUtils.device.cmdPushConstants(
				cmdBuffer,
				test_apiData.gizmoManager.pipelineLayout,
				vk::ShaderStageFlagBits::eVertex,
				0,
				sizeof(gizmoMatrix),
				&gizmoMatrix);

			uSize vertexOffset = 0;
			for (auto const& drawCmd : drawParams.lineDrawCmds)
			{
				// Push the color to the push-constant
				globUtils.device.cmdPushConstants(
					cmdBuffer,
					test_apiData.gizmoManager.pipelineLayout,
					vk::ShaderStageFlagBits::eFragment,
					sizeof(gizmoMatrix),
					sizeof(drawCmd.color),
					&drawCmd.color);

				// Bind the vertex-array
				vk::Buffer vertexBuffer = test_apiData.gizmoManager.lineVtxBuffer;
				uSize vertexBufferOffset = 0;
				// Offset into the right in-flight part of the buffer
				vertexBufferOffset += test_apiData.gizmoManager.lineVtxBufferCapacity * test_apiData.gizmoManager.lineVtxElementSize * inFlightIndex;
				// Index into the correct vertex
				vertexBufferOffset += test_apiData.gizmoManager.lineVtxElementSize * vertexOffset;
				globUtils.device.cmdBindVertexBuffers(
					cmdBuffer,
					0,
					vertexBuffer,
					vertexBufferOffset);

				globUtils.device.cmdDraw(
					cmdBuffer,
					drawCmd.vertCount,
					1,
					0,
					0);

				vertexOffset += drawCmd.vertCount;
			}
		}









		// Draw the gizmo for this viewport
		if (viewportUpdate.gizmo.HasValue())
		{
			ViewportUpdate::Gizmo gizmo = viewportUpdate.gizmo.Value();

			RecordGizmoDrawCalls(
				globUtils,
				test_apiData.gizmoManager,
				viewportData,
				gizmo,
				cmdBuffer,
				inFlightIndex);
		}
		

		globUtils.device.cmdEndRenderPass(cmdBuffer);

		// We need a barrier for the render-target if we are going to sample from it in the gui pass
		if (globUtils.editorMode)
		{
			vk::ImageMemoryBarrier imgBarrier{};
			imgBarrier.image = viewportData.renderTarget.img;
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

	NativeWindowManager::Update(
		apiData.nativeWindowManager,
		globUtils,
		{ drawParams.nativeWindowUpdates.data(), drawParams.nativeWindowUpdates.size() });
	// Deletes and creates viewports when needed.
	ViewportManager::HandleEvents(
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
		5000000000); // Added a 5s timeout for testing purposes
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

	std::vector<vk::CommandBuffer> graphicsCmdBuffers{};
	for (auto const& viewportUpdate : drawParams.viewportUpdates)
	{
		auto viewportDataIt = Std::FindIf(
			apiData.viewportManager.viewportNodes.begin(),
			apiData.viewportManager.viewportNodes.end(),
			[&viewportUpdate](decltype(apiData.viewportManager.viewportNodes)::value_type const& val) -> bool {
				return viewportUpdate.id == val.id; });
		ViewportData const& viewport = viewportDataIt->viewport;
		vk::CommandBuffer cmdBuffer = viewport.cmdBuffers[currentInFlightIndex];
		graphicsCmdBuffers.push_back(cmdBuffer);
		
		RecordGraphicsCmdBuffer(
			globUtils,
			apiData.objectDataManager,
			apiData.textureManager,
			viewport,
			viewportUpdate,
			drawParams,
			currentInFlightIndex,
			apiData);
	}

	// Record all the GUI shit.
	std::vector<NativeWindowData const*> windowsToPresent;
	for (auto& windowUpdate : drawParams.nativeWindowUpdates)
	{
		auto& manager = apiData.nativeWindowManager;
		// First find the window in the database
		auto windowDataIt = std::find_if(
			manager.nativeWindows.begin(),
			manager.nativeWindows.end(),
			[&windowUpdate](NativeWindowManager::Node const& node) -> bool { return node.id == windowUpdate.id; });
		auto const& nativeWindow = *windowDataIt;
		windowsToPresent.push_back(&nativeWindow.windowData);

		vk::CommandBuffer cmdBuffer = nativeWindow.gui.cmdBuffers[currentInFlightIndex];
		graphicsCmdBuffers.push_back(cmdBuffer);

		Std::Span<GuiDrawCmd const> drawCmds;
		if (!drawParams.guiDrawCmds.empty())
		{
			DENGINE_DETAIL_GFX_ASSERT(windowUpdate.drawCmdOffset + windowUpdate.drawCmdCount <= drawParams.guiDrawCmds.size());
			drawCmds = { &drawParams.guiDrawCmds[windowUpdate.drawCmdOffset], windowUpdate.drawCmdCount };
		}

		RecordGUICmdBuffer(
			globUtils,
			apiData.guiResourceManager,
			apiData.viewportManager,
			nativeWindow.gui,
			cmdBuffer,
			windowUpdate,
			drawCmds,
			currentInFlightIndex);
	}

	// Submit the graphics cmdBuffers
	if (!graphicsCmdBuffers.empty())
	{
		vk::SubmitInfo submitInfo{};
		submitInfo.commandBufferCount = (u32)graphicsCmdBuffers.size();
		submitInfo.pCommandBuffers = graphicsCmdBuffers.data();
		globUtils.queues.graphics.submit(submitInfo, vk::Fence());
	}

	// Setup swapchain copy cmd buffers.
	std::vector<u32> presentIndices;
	std::vector<vk::SwapchainKHR> swapchains;
	std::vector<vk::CommandBuffer> swapchainCopyCmdBuffers;
	std::vector<vk::Semaphore> swapchainImageReadySemaphores;
	std::vector<vk::PipelineStageFlags> swapchainCopyWaitStages;
	for (uSize i = 0; i < windowsToPresent.size(); i += 1)
	{
		vk::ResultValue<u32> acquireResult = device.acquireNextImageKHR(
			windowsToPresent[i]->swapchain,
			std::numeric_limits<u64>::max(),
			windowsToPresent[i]->swapchainImageReady,
			vk::Fence());
		if (acquireResult.result != vk::Result::eSuccess)
			throw std::runtime_error("DEngine - Vulkan: Acquiring next swapchain image did not return success result.");
		u32 index = acquireResult.value;

		presentIndices.push_back(index);
		swapchains.push_back(windowsToPresent[i]->swapchain);
		swapchainCopyCmdBuffers.push_back(windowsToPresent[i]->copyCmdBuffers[index]);
		swapchainImageReadySemaphores.push_back(windowsToPresent[i]->swapchainImageReady);
		swapchainCopyWaitStages.push_back(vk::PipelineStageFlagBits::eTransfer);
	}

	vk::SubmitInfo submitInfo{};
	submitInfo.commandBufferCount = (u32)swapchainCopyCmdBuffers.size();
	submitInfo.pCommandBuffers = swapchainCopyCmdBuffers.data();
	submitInfo.pWaitDstStageMask = swapchainCopyWaitStages.data();
	submitInfo.pWaitSemaphores = swapchainImageReadySemaphores.data();
	submitInfo.waitSemaphoreCount = (u32)swapchainImageReadySemaphores.size();

	globUtils.queues.graphics.submit(submitInfo, apiData.mainFences[currentInFlightIndex]);

	if (!presentIndices.empty())
	{
		vk::PresentInfoKHR presentInfo{};
		presentInfo.pImageIndices = presentIndices.data();
		presentInfo.pSwapchains = swapchains.data();
		presentInfo.swapchainCount = (u32)swapchains.size();
		vkResult = globUtils.queues.graphics.presentKHR(presentInfo);
		if (vkResult != vk::Result::eSuccess && vkResult != vk::Result::eSuboptimalKHR)
			throw std::runtime_error("DEngine - Vulkan: Presentation submission did not return success result.");
	}

	DeletionQueue::ExecuteCurrentTick(
		apiData.globUtils.deletionQueue,
		globUtils,
		currentInFlightIndex);
	apiData.currInFlightIndex = (apiData.currInFlightIndex + 1) % apiData.globUtils.inFlightCount;
}

DEngine::Gfx::NativeWindowID DEngine::Gfx::Vk::APIData::NewNativeWindow(WsiInterface& wsiConnection)
{
	APIData& apiData = *this;

	return NativeWindowManager::PushCreateWindowJob(
		apiData.nativeWindowManager,
		wsiConnection);
}

