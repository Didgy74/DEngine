#pragma once

#include <DEngine/Gui/Widget.hpp>

#include <DEngine/Std/Containers/Box.hpp>
#include <DEngine/Std/Containers/Opt.hpp>

namespace DEngine::Gui::impl { class SA_Impl; }

namespace DEngine::Gui
{
	class ScrollArea : public Widget
	{
	public:
		using ParentType = Widget;

		Std::Box<Widget> widget;

		static constexpr Math::Vec3 scrollbarHoverHighlight = { 0.1f, 0.1f, 0.1f };
		Math::Vec4 scrollbarInactiveColor = { 0.3f, 0.3f, 0.3f, 1.f };

		[[nodiscard]] virtual SizeHint GetSizeHint(
			Context const& ctx) const override;

		virtual SizeHint GetSizeHint2(
			GetSizeHint2_Params const& params) const override;

		virtual void BuildChildRects(
			BuildChildRects_Params const& params,
			Rect const& widgetRect,
			Rect const& visibleRect) const override;

		virtual void Render(
			Context const& ctx,
			Extent framebufferExtent,
			Rect widgetRect,
			Rect visibleRect,
			DrawInfo& drawInfo) const override;

		virtual void Render2(
			Render_Params const& params,
			Rect const& widgetRect,
			Rect const& visibleRect) const override;

		virtual bool CursorPress(
			Context& ctx,
			WindowID windowId,
			Rect widgetRect,
			Rect visibleRect,
			Math::Vec2Int cursorPos,
			CursorPressEvent event) override;

		virtual bool CursorMove(
			Context& ctx,
			WindowID windowId,
			Rect widgetRect,
			Rect visibleRect,
			CursorMoveEvent event,
			bool occluded) override;

		virtual bool TouchPressEvent(
			Context& ctx,
			WindowID windowId,
			Rect widgetRect,
			Rect visibleRect,
			Gui::TouchPressEvent event) override;

		virtual bool TouchMoveEvent(
			Context& ctx,
			WindowID windowId,
			Rect widgetRect,
			Rect visibleRect,
			Gui::TouchMoveEvent event,
			bool occluded) override;

		virtual void InputConnectionLost() override;

		virtual void CharEnterEvent(
			Context& ctx) override;

		virtual void CharEvent(
			Context& ctx,
			u32 utfValue) override;

		virtual void CharRemoveEvent(
			Context& ctx) override;

	private:
		friend impl::SA_Impl;

		u32 currScrollbarPos = 0;
		u32 scrollbarThickness = 50;

		struct Scrollbar_Pressed_T
		{
			u8 pointerId;
			f32 pointerRelativePosY;
		};
		Std::Opt<Scrollbar_Pressed_T> scrollbarHeldData;

		bool scrollbarHoveredByCursor = false;
	};
}