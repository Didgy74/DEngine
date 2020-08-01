#pragma once

#include <DEngine/Gui/Events.hpp>
#include <DEngine/Gui/Layout.hpp>
#include <DEngine/Gui/WindowID.hpp>

#include <DEngine/Containers/Box.hpp>
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
	struct Event
	{
		enum class Type
		{
			Window,
			Input
		};
		Type type;

		struct Window
		{
			enum class Type
			{
				CursorEnter,
				Minimize,
				Move,
				Resize,
			};
			Type type;
			union
			{
				WindowCursorEnterEvent cursorEnter;
				WindowMinimizeEvent minimize;
				WindowMoveEvent move;
				WindowResizeEvent resize;
			};
		};

		struct Input
		{
			enum class Type
			{
				CursorMove,
				CursorClick,
				Touch,
				Char,
				CharRemove,
			};
			Type type;
			union
			{
				CursorMoveEvent cursorMove;
				CursorClickEvent cursorClick;
				TouchEvent touch;
				CharEvent charEvent;
				CharRemoveEvent charRemove;
			};
		};

		union
		{
			Window window;
			Input input;
		};
	};

	struct TextManager
	{
		Gfx::Context* gfxCtx = nullptr;

		FT_Library ftLib{};
		FT_Face face{};
		std::vector<u8> fontFileData;
		u32 dpi = 0;
		u32 lineHeight = 0;
		struct GlyphData
		{
			u32 bitmapWidth{};
			u32 bitmapHeight{};
			u32 advance{};
			i32 horizBearingX{};
			i32 horizBearingY{};
			u32 hasBitmap{};
		};
		std::unordered_map<u32, GlyphData> glyphDatas;

		static void RenderText(
			TextManager& manager,
			std::string_view string,
			Math::Vec4 color,
			Gui::Extent framebufferExtent,
			Gui::Rect widgetRect,
			DrawInfo const& drawInfo);
	};

	struct WindowData
	{
		Gui::Rect rect{};
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
		std::vector<Event> eventQueue;

		std::vector<WindowNode> windows;

		Math::Vec2Int cursorPosition{};

		TextManager textManager;
	};
}