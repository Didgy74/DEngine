#pragma once

#include <DEngine/Gui/Widget.hpp>

#include <DEngine/Std/Containers/Opt.hpp>

#include <functional>
#include <string>
#include <vector>

namespace DEngine::Gui
{
	class LineList : public Widget
	{
	public:
		static constexpr Math::Vec4 highlightColor = { 1.f, 1.f, 1.f, 0.5f };
		static constexpr Math::Vec4 alternatingLineColor = { 0.f, 0.f, 0.f, 0.1f };
		static constexpr u32 horizontalPadding = 20;

		void RemoveLine(uSize index);
		
		Std::Opt<uSize> selectedLine;
		struct PressedLine_T
		{
			u8 pointerId;
			// This is null-opt if we pressed an area outside
			// the possible lines. Meaning it should lead to
			// a deselect of any currently selected line.
			Std::Opt<uSize> lineIndex;
		};
		Std::Opt<PressedLine_T> currPressedLine;
		std::vector<std::string> lines;

		using Callback = std::function<void(LineList&)>;
		Callback selectedLineChangedCallback = nullptr;

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
	};
}