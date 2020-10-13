#pragma once

#include <DEngine/Gui/Events.hpp>
#include <DEngine/Gui/Layout.hpp>
#include <DEngine/Gui/SizeHint.hpp>
#include <DEngine/Gui/WindowHandler.hpp>

#include <DEngine/Std/Containers/Array.hpp>
#include <DEngine/Std/Containers/Box.hpp>
#include <DEngine/FixedWidthTypes.hpp>
#include <DEngine/Math/Vector.hpp>

#include <vector>
#include <string_view>

// Definitely temporary
// Make an actual good interface instead
#include <DEngine/Gfx/Gfx.hpp>

// Maybe temporary?
// Could consider type-erasing the text manager
// and move it into it's own header+source file
// Or make an actually good public interface to use it.
#include <unordered_map>
#include <ft2build.h>
#include FT_FREETYPE_H



namespace DEngine::Gui::impl
{
	struct TextManager
	{
		Gfx::Context* gfxCtx = nullptr;

		FT_Library ftLib{};
		FT_Face face{};
		std::vector<u8> fontFileData;
		u32 lineheight = 0;
		u32 lineMinY = 0;
		struct GlyphData
		{
			bool hasBitmap{};
			u32 bitmapWidth{};
			u32 bitmapHeight{};

			i32 advanceX{};
			Math::Vec2Int posOffset{};
		};
		// Stores glyph-data that is not in the ascii-table
		std::unordered_map<u32, GlyphData> glyphDatas;
		static constexpr uSize lowGlyphTableSize = 256;
		Std::Array<GlyphData, lowGlyphTableSize> lowGlyphDatas;

		static void Initialize(
			TextManager& manager,
			Gfx::Context* gfxCtx);

		static void RenderText(
			TextManager& manager,
			std::string_view string,
			Math::Vec4 color,
			Rect widgetRect,
			DrawInfo& drawInfo);

		[[nodiscard]] static SizeHint GetSizeHint(
			TextManager& manager,
			std::string_view st);
	};

	struct WindowData
	{
		Rect rect{};
		Rect visibleRect{};
		bool isMinimized{};
		Math::Vec4 clearColor{ 0, 0, 0, 1 };
		u32 drawCmdOffset{};
		u32 drawCmdCount{};
		Std::Box<Layout> topLayout{};
	};

	struct WindowNode
	{
		WindowID id;
		WindowData data;
	};

	struct ImplData
	{
		ImplData() = default;
		ImplData(ImplData const&) = delete;

		WindowHandler* windowHandler = nullptr;

		Widget* inputConnectionWidget = nullptr;

		std::vector<WindowNode> windows;

		Math::Vec2Int cursorPosition{};

		TextManager textManager;
	};
}