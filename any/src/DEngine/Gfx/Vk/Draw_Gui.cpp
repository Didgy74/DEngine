#include "Draw_Gui.hpp"

#include "DEngine/Gui/StdWidgets/AnchorArea.hpp"
#include "GlobUtils.hpp"
#include "GuiResourceManager.hpp"
#include "ViewportManager.hpp"

import DEngine.Std.StackVec;

using namespace DEngine;
using namespace DEngine::Gfx;

void Gfx::Vk::RecordGuiRenderPass(
	RecordGuiRenderPass_Params const& params)
{
	auto const& device = params.globUtils.device;
	auto const& guiResMgr = params.guiResManager;
	auto const& cmdBuffer = params.cmdBuffer;
	auto const& viewportMan = params.viewportManager;
	auto const& windowId = params.windowUpdate.id;
	auto inFlightIndex = params.inFlightIndex;
	auto windowRotation = params.rotation;
	auto windowExtent = params.windowExtent;

	u8 stencilReference = 64;

	{
		vk::ClearColorValue colorAttachmentClearVal = {};
		for (uSize i = 0; i < 4; i++)
			colorAttachmentClearVal.float32[i] = params.windowUpdate.clearColor[i];

		vk::ClearDepthStencilValue depthStencilAttachmentClearVal = {};
		depthStencilAttachmentClearVal.stencil = stencilReference;

		// Must appear in the same order as list of attachments in the vk::RenderPass
		Std::Array<vk::ClearValue, 2> clearValues = {
			colorAttachmentClearVal,
			depthStencilAttachmentClearVal };

		vk::RenderPassBeginInfo rpBegin = {};
		rpBegin.framebuffer = params.framebuffer;
		rpBegin.renderPass = params.globUtils.guiRenderPass;
		rpBegin.renderArea.extent = windowExtent;
		rpBegin.clearValueCount = clearValues.Size();
		rpBegin.pClearValues = clearValues.Data();

		device.cmdBeginRenderPass(cmdBuffer, rpBegin, vk::SubpassContents::eInline);

		{
			vk::Viewport viewport = {};
			viewport.width = (float)windowExtent.width;
			viewport.height = (float)windowExtent.height;
			device.cmdSetViewport(cmdBuffer, 0, viewport);
			vk::Rect2D scissor = {};
			scissor.extent = windowExtent;
			device.cmdSetScissor(cmdBuffer, 0, scissor);
			device.cmdSetStencilReference(cmdBuffer, vk::StencilFaceFlagBits::eFrontAndBack, stencilReference);
		}

		// Bind the window-specific GUI descriptorset
		auto perWindowDescrSet =
			GuiResourceManager::GetPerWindowDescrSet(guiResMgr, windowId, inFlightIndex);

		Std::StackVec<vk::DescriptorSet, 5> descrSets = { {
			guiResMgr.m_dummyCameraObjectUniforms.cameraDescrSet,
			guiResMgr.m_dummyCameraObjectUniforms.objectDescrSet,
			perWindowDescrSet
		} };
		Std::StackVec<u32, 5> descrDynamicOffsets = { { 0 } };

		RecordGuiDrawCmds_Params drawParams = {
			.globUtils = params.globUtils,
			.guiResManager = guiResMgr,
			.viewportManager = viewportMan,
			.cmdBuffer = cmdBuffer,
			.descrSets = descrSets.ToSpan(),
			.descrDynamicOffsets = descrDynamicOffsets.ToSpan(),
			.guiDrawCmds = params.guiDrawCmds,
			.utfValues = params.utfValues,
			.glyphRects = params.glyphRects,
			.rotation = windowRotation,
			.windowExtent = windowExtent,
			.inFlightIndex = inFlightIndex,
			.stencilReference = stencilReference,
		};
		RecordGuiDrawCmds(drawParams);

		device.cmdEndRenderPass(cmdBuffer);
	}
}

void Gfx::Vk::RecordGuiDrawCmds(
	RecordGuiDrawCmds_Params const& params)
{
	auto& device = params.globUtils.device;
	auto& guiResMgr = params.guiResManager;
	// TODO: We can't access this if we're in a 3D scene!!!
	auto& viewportMan = params.viewportManager;

	auto cmdBuffer = params.cmdBuffer;
	auto descrSets = params.descrSets;
	auto descrDynamicOffsets = params.descrDynamicOffsets;
	auto windowRotation = params.rotation;
	auto windowExtent = params.windowExtent;

	auto& stencilReference = params.stencilReference;

	for (auto const& drawCmd : params.guiDrawCmds) {
		switch (drawCmd.type) {
			/*
			case GuiDrawCmd::Type::Scissor: {
				// TODO: We should do something different if we're in 3D scene...
				GuiResourceManager::PerformGuiDrawCmd_Scissor_Params temp = {
					.rectExtentPx = drawCmd.rectExtentPx,
					.rectPosPx = drawCmd.rectPosPx,
					.rotation = windowRotation,
					.resolutionX = (int)windowExtent.width,
					.resolutionY = (int)windowExtent.height, };
				GuiResourceManager::PerformGuiDrawCmd_Scissor(
					guiResMgr,
					device,
					cmdBuffer,
					temp);
				break;
			}*/

			case GuiDrawCmd::Type::Gradient: {
				GuiResourceManager::RenderGradient(
					guiResMgr,
					device,
					descrSets,
					descrDynamicOffsets,
					cmdBuffer,
					drawCmd.gradient,
					drawCmd.rectPosPx,
					drawCmd.rectExtentPx);
				break;
			}

			case GuiDrawCmd::Type::RectangleStencil: {
				GuiResourceManager::RenderRectangleStencil(
					guiResMgr,
					device,
					descrSets,
					descrDynamicOffsets,
					cmdBuffer,
					drawCmd.rectangleStencil,
					stencilReference,
					drawCmd.rectPosPx,
					drawCmd.rectExtentPx);
				break;
			}

			case GuiDrawCmd::Type::Rectangle: {
				GuiResourceManager::RenderRectangle(
					guiResMgr,
					device,
					descrSets,
					descrDynamicOffsets,
					cmdBuffer,
					drawCmd.rectangle,
					drawCmd.rectPosPx,
					drawCmd.rectExtentPx);
				break;
			}

			case GuiDrawCmd::Type::RectangleShadow: {
				GuiResourceManager::RenderRectangleShadow(
					guiResMgr,
					device,
					descrSets,
					descrDynamicOffsets,
					cmdBuffer,
					drawCmd.rectangleShadow,
					drawCmd.rectPosPx,
					drawCmd.rectExtentPx);
				break;
			}

			case GuiDrawCmd::Type::Text: {
				GuiResourceManager::PerformGuiDrawCmd_Text(
					guiResMgr,
					device,
					cmdBuffer,
					descrSets,
					descrDynamicOffsets,
					drawCmd.text,
					params.utfValues,
					params.glyphRects,
					drawCmd.rectPosPx);
				break;
			}

			// TODO: We can't do this in a 3D scene!!!
			case GuiDrawCmd::Type::Viewport: {
				GuiResourceManager::PerformGuiDrawCmd_Viewport(
					guiResMgr,
					device,
					cmdBuffer,
					descrSets,
					descrDynamicOffsets,
					viewportMan.GetViewportData(drawCmd.viewport.id).imgDescrSet,
					windowRotation,
					drawCmd.rectPosPx,
					drawCmd.rectExtentPx);
				break;
			}

			case GuiDrawCmd::Type::FilledMesh: {
				DENGINE_IMPL_GFX_UNREACHABLE();
				break;
			}
		}
	}
}