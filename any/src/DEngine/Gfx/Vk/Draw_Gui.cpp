#include "Draw_Gui.hpp"

#include "GlobUtils.hpp"
#include "GuiResourceManager.hpp"
#include "NativeWindowManager.hpp"
#include "ViewportManager.hpp"

using namespace DEngine;
using namespace DEngine::Gfx;

void Gfx::Vk::RecordGuiCmds(
	RecordGuiCmds_Params const& params)
{
	auto const& cmdBuffer = params.cmdBuffer;
	DeviceDispatch const& device = params.globUtils.device;

	{
		vk::RenderPassBeginInfo rpBegin{};
		rpBegin.framebuffer = params.framebuffer;
		rpBegin.renderPass = params.globUtils.guiRenderPass;
		rpBegin.renderArea.extent = params.guiData.extent;
		rpBegin.clearValueCount = 1;
		vk::ClearColorValue clearVal{};
		for (uSize i = 0; i < 4; i++)
			clearVal.float32[i] = params.windowUpdate.clearColor[i];
		vk::ClearValue test = clearVal;
		rpBegin.pClearValues = &test;

		device.cmdBeginRenderPass(cmdBuffer, rpBegin, vk::SubpassContents::eInline);

		{
			vk::Viewport viewport{};
			viewport.width = (float)params.guiData.extent.width;
			viewport.height = (float)params.guiData.extent.height;
			device.cmdSetViewport(cmdBuffer, 0, viewport);
			vk::Rect2D scissor{};
			scissor.extent = params.guiData.extent;
			device.cmdSetScissor(cmdBuffer, 0, scissor);
		}

		for (auto const& drawCmd : params.guiDrawCmds)
		{
			switch (drawCmd.type)
			{
				case GuiDrawCmd::Type::Scissor:
				{
					vk::Rect2D scissor{};
					vk::Extent2D rotatedFramebufferExtent = params.guiData.extent;
					if (params.guiData.surfaceRotation == vk::SurfaceTransformFlagBitsKHR::eRotate90 ||
						params.guiData.surfaceRotation == vk::SurfaceTransformFlagBitsKHR::eRotate270)
						std::swap(rotatedFramebufferExtent.width, rotatedFramebufferExtent.height);
					scissor.extent.width = u32(Math::Round(drawCmd.rectExtent.x * rotatedFramebufferExtent.width));
					scissor.extent.height = u32(Math::Round(drawCmd.rectExtent.y * rotatedFramebufferExtent.height));
					i32 scissorPosX = i32(Math::Round(drawCmd.rectPosition.x * rotatedFramebufferExtent.width));
					i32 scissorPosY = i32(Math::Round(drawCmd.rectPosition.y * rotatedFramebufferExtent.height));
					if (params.guiData.surfaceRotation == vk::SurfaceTransformFlagBitsKHR::eIdentity)
					{
						scissor.offset.x = scissorPosX;
						scissor.offset.y = scissorPosY;
					}
					else if (params.guiData.surfaceRotation == vk::SurfaceTransformFlagBitsKHR::eRotate90)
					{
						scissor.offset.x = rotatedFramebufferExtent.height - scissor.extent.height - scissorPosY;
						scissor.offset.y = scissorPosX;
					}
					else if (params.guiData.surfaceRotation == vk::SurfaceTransformFlagBitsKHR::eRotate270)
					{
						scissor.offset.x = scissorPosY;
						scissor.offset.y = rotatedFramebufferExtent.width - scissor.extent.width - scissorPosX;
					}
					else if (params.guiData.surfaceRotation == vk::SurfaceTransformFlagBitsKHR::eRotate180)
					{
						scissor.offset.x = rotatedFramebufferExtent.width - scissor.extent.width - scissorPosX;
						scissor.offset.y = rotatedFramebufferExtent.height - scissor.extent.height - scissorPosY;
					}
					if (params.guiData.surfaceRotation == vk::SurfaceTransformFlagBitsKHR::eRotate90 ||
						params.guiData.surfaceRotation == vk::SurfaceTransformFlagBitsKHR::eRotate270)
						std::swap(scissor.extent.width, scissor.extent.height);
					device.cmdSetScissor(cmdBuffer, 0, scissor);
				}
					break;

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

				case GuiDrawCmd::Type::TextGlyph:
				{
					device.cmdBindPipeline(cmdBuffer, vk::PipelineBindPoint::eGraphics, params.guiResManager.font_pipeline);
					GuiResourceManager::FontPushConstant pushConstant{};
					pushConstant.color = drawCmd.textGlyph.color;
					pushConstant.orientation = params.guiData.rotation;
					pushConstant.rectExtent = drawCmd.rectExtent;
					pushConstant.rectOffset = drawCmd.rectPosition;
					device.cmdPushConstants(
						cmdBuffer,
						params.guiResManager.font_pipelineLayout,
						vk::ShaderStageFlagBits::eVertex,
						0,
						32,
						&pushConstant);
					device.cmdPushConstants(
						cmdBuffer,
						params.guiResManager.font_pipelineLayout,
						vk::ShaderStageFlagBits::eFragment,
						32,
						sizeof(pushConstant.color),
						&pushConstant.color);
					GuiResourceManager::GlyphData glyphData{};
					if (drawCmd.textGlyph.utfValue < GuiResourceManager::lowUtfGlyphDatasSize)
					{
						glyphData = params.guiResManager.lowUtfGlyphDatas[drawCmd.textGlyph.utfValue];
					}
					else
					{
						auto glyphDataIt = params.guiResManager.glyphDatas.find((u32)drawCmd.textGlyph.utfValue);
						if (glyphDataIt == params.guiResManager.glyphDatas.end())
							throw std::runtime_error("DEngine - Vulkan: Unable to find glyph.");
						glyphData = glyphDataIt->second;
					}


					device.cmdBindDescriptorSets(
						cmdBuffer,
						vk::PipelineBindPoint::eGraphics,
						params.guiResManager.font_pipelineLayout,
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
					device.cmdBindPipeline(cmdBuffer, vk::PipelineBindPoint::eGraphics, params.guiResManager.viewportPipeline);
					GuiResourceManager::ViewportPushConstant pushConstant{};
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
					auto const& viewportData = *std::find_if(
						params.viewportManager.viewportNodes.begin(),
						params.viewportManager.viewportNodes.end(),
						[&drawCmd](decltype(params.viewportManager.viewportNodes)::value_type const& val) {
							return drawCmd.viewport.id == val.id; });
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
			}
		}

		device.cmdEndRenderPass(cmdBuffer);
	}
}