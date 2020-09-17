#pragma once

#include <DEngine/Gui/Layout.hpp>
#include <DEngine/Gui/Widget.hpp>

#include <DEngine/Containers/Box.hpp>

namespace DEngine::Gui
{
	class ScrollArea : public Layout
	{
	public:
		using ParentType = Layout;

		u32 scrollBarCursorRelativePosY = 0;
		u32 scrollBarPos = 0;
		u32 scrollBarWidthPixels = 50;
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

		[[nodiscard]] virtual Gui::SizeHint SizeHint(
			Context const& ctx) const override;

		[[nodiscard]] virtual Gui::SizeHint SizeHint_Tick(
			Context const& ctx) override;

		virtual void Render(
			Context const& ctx,
			Extent framebufferExtent,
			Rect widgetRect,
			Rect visibleRect,
			DrawInfo& drawInfo) const override;

		virtual void Tick(
			Context& ctx,
			Rect widgetRect,
			Rect visibleRect) override;

		virtual void CursorMove(
			Test& test,
			Rect widgetRect,
			Rect visibleRect,
			CursorMoveEvent event) override;

		virtual void CursorClick(
			Context& ctx,
			Rect widgetRect,
			Rect visibleRect,
			Math::Vec2Int cursorPos,
			CursorClickEvent event) override;

		virtual void TouchEvent(
			Context& ctx,
			Rect widgetRect,
			Rect visibleRect,
			Gui::TouchEvent event) override;
	};
}