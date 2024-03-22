#pragma once

#include <DEngine/Gfx/Gfx.hpp>
#include "ForwardDeclarations.hpp"
#include "VulkanIncluder.hpp"

#include <DEngine/FixedWidthTypes.hpp>
#include <DEngine/Std/Containers/Span.hpp>

namespace DEngine::Gfx::Vk
{
	struct RecordGuiCmds_Params {
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
	void RecordGuiCmds(
		RecordGuiCmds_Params const& params);
}