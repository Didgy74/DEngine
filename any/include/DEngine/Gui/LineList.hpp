#pragma once

#include <DEngine/Gui/Widget.hpp>

#include <DEngine/Std/Containers/Opt.hpp>

#include <functional>
#include <string>
#include <vector>

namespace DEngine::Gui::impl { class LineListImpl; }

namespace DEngine::Gui
{
	class LineList : public Widget
	{
	public:
		static constexpr Math::Vec4 highlightOverlayColor = { 1.f, 1.f, 1.f, 0.5f };
		static constexpr Math::Vec4 hoverOverlayColor = { 1.f, 1.f, 1.f, 0.25f };
		static constexpr Math::Vec4 alternatingLineOverlayColor = {0.f, 0.f, 0.f, 0.1f };

		void RemoveLine(uSize index);
		
		Std::Opt<uSize> selectedLine;
		std::vector<std::string> lines;
		u32 textMargin = 0;

		using Callback = std::function<void(LineList&)>;
		Callback selectedLineChangedFn = nullptr;

		[[nodiscard]] virtual SizeHint GetSizeHint(Context const& ctx) const override;
	
		virtual void Render(
			Context const& ctx,
			Extent framebufferExtent,
			Rect widgetRect,
			Rect visibleRect,
			DrawInfo& drawInfo) const override;

		virtual bool CursorMove(
			Context& ctx,
			WindowID windowId,
			Rect widgetRect,
			Rect visibleRect,
			CursorMoveEvent event,
			bool occluded) override;

		virtual bool CursorPress(
			Context& ctx,
			WindowID windowId,
			Rect widgetRect,
			Rect visibleRect,
			Math::Vec2Int cursorPos,
			CursorClickEvent event) override;

		virtual bool TouchMoveEvent(
			Context& ctx,
			WindowID windowId,
			Rect widgetRect,
			Rect visibleRect,
			Gui::TouchMoveEvent event,
			bool occluded) override;

		virtual bool TouchPressEvent(
			Context& ctx,
			WindowID windowId,
			Rect widgetRect,
			Rect visibleRect,
			Gui::TouchPressEvent event) override;

	protected:
		friend impl::LineListImpl;

		struct PressedLine_T
		{
			u8 pointerId;
			// This is null-opt if we pressed an area outside
			// the possible lines. Meaning it should lead to
			// a deselect of any currently selected line.
			Std::Opt<uSize> lineIndex;
		};
		Std::Opt<PressedLine_T> currPressedLine;
		Std::Opt<uSize> lineCursorHover;
	};
}