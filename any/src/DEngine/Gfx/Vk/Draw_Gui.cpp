#include "Draw_Gui.hpp"

#include "GlobUtils.hpp"
#include "GuiResourceManager.hpp"
#include "ViewportManager.hpp"

using namespace DEngine;
using namespace DEngine::Gfx;

void Gfx::Vk::RecordGuiCmds(
	RecordGuiCmds_Params const& params)
{
	auto const& device = params.globUtils.device;
	auto const& guiResMgr = params.guiResManager;
	auto const& cmdBuffer = params.cmdBuffer;
	auto const& viewportMan = params.viewportManager;
	auto const& windowId = params.windowUpdate.id;
	auto inFlightIndex = params.inFlightIndex;
	auto windowRotation = params.rotation;
	auto windowExtent = params.windowExtent;


	{
		vk::RenderPassBeginInfo rpBegin{};
		rpBegin.framebuffer = params.framebuffer;
		rpBegin.renderPass = params.globUtils.guiRenderPass;
		rpBegin.renderArea.extent = windowExtent;
		rpBegin.clearValueCount = 1;
		vk::ClearColorValue clearVal{};
		for (uSize i = 0; i < 4; i++)
			clearVal.float32[i] = params.windowUpdate.clearColor[i];
		vk::ClearValue test = clearVal;
		rpBegin.pClearValues = &test;

		device.cmdBeginRenderPass(cmdBuffer, rpBegin, vk::SubpassContents::eInline);

		{
			vk::Viewport viewport{};
			viewport.width = (float)windowExtent.width;
			viewport.height = (float)windowExtent.height;
			device.cmdSetViewport(cmdBuffer, 0, viewport);
			vk::Rect2D scissor{};
			scissor.extent = windowExtent;
			device.cmdSetScissor(cmdBuffer, 0, scissor);
		}


		// Bind the window-specific GUI descriptorset
		auto perWindowDescrSet =
			GuiResourceManager::GetPerWindowDescrSet(guiResMgr, windowId, inFlightIndex);

		for (auto const& drawCmd : params.guiDrawCmds) {
			switch (drawCmd.type) {
				case GuiDrawCmd::Type::Scissor: {
					GuiResourceManager::PerformGuiDrawCmd_Scissor_Params temp = {
						.rectExtent = drawCmd.rectExtent,
					  	.rectPos = drawCmd.rectPosition,
						.rotation = windowRotation,
						.resolutionX = (int)windowExtent.width,
						.resolutionY = (int)windowExtent.height, };
					GuiResourceManager::PerformGuiDrawCmd_Scissor(
						guiResMgr,
						device,
						cmdBuffer,
						temp);
					break;
				}

				case GuiDrawCmd::Type::Rectangle: {
					GuiResourceManager::RenderRectangle(
						guiResMgr,
						device,
						perWindowDescrSet,
						cmdBuffer,
						drawCmd.rectangle);
					break;
				}

				case GuiDrawCmd::Type::Text: {
					GuiResourceManager::PerformGuiDrawCmd_Text(
						guiResMgr,
						device,
						cmdBuffer,
						perWindowDescrSet,
						drawCmd.text,
						params.utfValues,
						params.glyphRects);
					break;
				}

				case GuiDrawCmd::Type::Viewport: {

					GuiResourceManager::PerformGuiDrawCmd_Viewport(
						guiResMgr,
						device,
						cmdBuffer,
						perWindowDescrSet,
						viewportMan.GetViewportData(drawCmd.viewport.id).imgDescrSet,
						windowRotation,
						drawCmd.rectPosition,
						drawCmd.rectExtent);
					break;
				}


				/*
				case GuiDrawCmd::Type::FilledMesh:
				{
					device.cmdBindPipeline(cmdBuffer, vk::PipelineBindPoint::eGraphics, params.guiResManager.filledMeshPipeline);
					GuiResourceManager::FilledMeshPushConstant pushConstant{};
					pushConstant.color = drawCmd.filledMesh.color;
					pushConstant.orientation = params.guiData.rotation;
					pushConstant.rectExtent = drawCmd.rectExtent;
					pushConstant.rectOffset = drawCmd.rectPosition;
					device.cmdPushConstants(
						cmdBuffer,
						params.guiResManager.filledMeshPipelineLayout,
						vk::ShaderStageFlagBits::eVertex,
						0,
						32,
						&pushConstant);
					device.cmdPushConstants(
						cmdBuffer,
						params.guiResManager.filledMeshPipelineLayout,
						vk::ShaderStageFlagBits::eFragment,
						32,
						sizeof(pushConstant.color),
						&pushConstant.color);
					device.cmdBindVertexBuffers(
						cmdBuffer,
						0,
						params.guiResManager.vtxBuffer,
						(params.guiResManager.vtxInFlightCapacity * params.inFlightIndex) + (drawCmd.filledMesh.mesh.vertexOffset * sizeof(GuiVertex)));
					device.cmdBindIndexBuffer(
						cmdBuffer,
						params.guiResManager.indexBuffer,
						(params.guiResManager.indexInFlightCapacity * params.inFlightIndex) + (drawCmd.filledMesh.mesh.indexOffset * sizeof(u32)),
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


				 */


				/*
				case GuiDrawCmd::Type::Viewport: {
					device.cmdBindPipeline(cmdBuffer, vk::PipelineBindPoint::eGraphics, params.guiResManager.viewportPipeline);
					GuiResourceManager::ViewportPushConstant pushConstant = {};
					pushConstant.orientation = params.guiData.rotation;
					pushConstant.rectExtent = drawCmd.rectExtent;
					pushConstant.rectOffset = drawCmd.rectPosition;
					device.cmdPushConstants(
						cmdBuffer,
						params.guiResManager.viewportPipelineLayout,
						vk::ShaderStageFlagBits::eVertex,
						0,
						sizeof(pushConstant),
						&pushConstant);
					auto const* viewportDataPtr = ViewportMan::FindNode(viewportMan, drawCmd.viewport.id);
					DENGINE_IMPL_GFX_ASSERT(viewportDataPtr);
					auto const& viewportData = *viewportDataPtr;
					DENGINE_IMPL_GFX_ASSERT_MSG(
						viewportData.IsInitialized(),
						"Attempted to render a viewport that is not initialized.");
					device.cmdBindDescriptorSets(
						cmdBuffer,
						vk::PipelineBindPoint::eGraphics,
						params.guiResManager.viewportPipelineLayout,
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
				 */
			}
		}

		device.cmdEndRenderPass(cmdBuffer);
	}
}