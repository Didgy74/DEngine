#pragma once

#include <DEngine/Gui/Layout.hpp>
#include <DEngine/Gui/Widget.hpp>

#include <DEngine/Std/Containers/Box.hpp>

namespace DEngine::Gui
{
	class ScrollArea : public Layout
	{
	public:
		using ParentType = Layout;

		u32 scrollBarCursorRelativePosY = 0;
		mutable u32 scrollBarPos = 0;
		u32 scrollBarThickness = 50;
		enum class ScrollBarState
		{
			Normal,
			Hovered,
			Pressed
		};
		ScrollBarState scrollBarState = ScrollBarState::Normal;
		Std::Opt<u8> scrollBarTouchIndex;

		enum class ChildType
		{
			None,
			Layout,
			Widget,
		};
		ChildType childType = ChildType::None;
		Std::Box<Layout> layout;
		Std::Box<Widget> widget;

		[[nodiscard]] virtual SizeHint GetSizeHint(
			Context const& ctx) const override;

		virtual void Render(
			Context const& ctx,
			Extent framebufferExtent,
			Rect widgetRect,
			Rect visibleRect,
			DrawInfo& drawInfo) const override;

		virtual void CursorMove(
			Context& ctx,
			WindowID windowId,
			Rect widgetRect,
			Rect visibleRect,
			CursorMoveEvent event) override;

		virtual void CursorClick(
			Context& ctx,
			WindowID windowId,
			Rect widgetRect,
			Rect visibleRect,
			Math::Vec2Int cursorPos,
			CursorClickEvent event) override;

		virtual void TouchEvent(
			Context& ctx,
			WindowID windowId,
			Rect widgetRect,
			Rect visibleRect,
			Gui::TouchEvent event) override;

		virtual void CharEnterEvent(
			Context& ctx) override;

		virtual void CharEvent(
			Context& ctx,
			u32 utfValue) override;

		virtual void CharRemoveEvent(
			Context& ctx) override;
	};
}