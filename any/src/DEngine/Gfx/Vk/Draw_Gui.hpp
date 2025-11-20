#pragma once

#include <DEngine/Gfx/Gfx.hpp>
#include "ForwardDeclarations.hpp"
#include "VulkanIncluder.hpp"

import DEngine.FixedWidthTypes;

namespace DEngine::Gfx::Vk {
	struct RecordGuiRenderPass_Params {
		GlobUtils const& globUtils;
		GuiResourceManager const& guiResManager;
		ViewportManager const& viewportManager;
		NativeWindowUpdate const& windowUpdate;
		vk::CommandBuffer cmdBuffer;
		vk::Framebuffer framebuffer;
		Std::Span<GuiDrawCmd const> guiDrawCmds;
		Std::Span<u32 const> utfValues;
		Std::Span<GlyphRect const> glyphRects;
		Gfx::WindowRotation rotation;
		vk::Extent2D windowExtent;
		u8 inFlightIndex;
	};
	void RecordGuiRenderPass(
		RecordGuiRenderPass_Params const& params);

	struct RecordGuiDrawCmds_Params {
		GlobUtils const& globUtils;
		GuiResourceManager const& guiResManager;
		ViewportManager const& viewportManager;
		vk::CommandBuffer cmdBuffer;
		Std::Span<vk::DescriptorSet const> descrSets;
		Std::Span<u32 const> descrDynamicOffsets;
		Std::Span<GuiDrawCmd const> guiDrawCmds;
		Std::Span<u32 const> utfValues;
		Std::Span<GlyphRect const> glyphRects;
		Gfx::WindowRotation rotation;
		vk::Extent2D windowExtent;
		u8 inFlightIndex;
		u8& stencilReference;
	};
	void RecordGuiDrawCmds(
		RecordGuiDrawCmds_Params const& params);
}