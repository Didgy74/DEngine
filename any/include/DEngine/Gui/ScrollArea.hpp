#pragma once

#include <DEngine/Gui/Widget.hpp>

#include <DEngine/Std/Containers/Box.hpp>
#include <DEngine/Std/Containers/Variant.hpp>

namespace DEngine::Gui
{
	class ScrollArea : public Widget
	{
	public:
		using ParentType = Widget;

		u32 scrollbarPos = 0;
		u32 scrollbarThickness = 50;

		struct ScrollbarState_Normal {};
		struct ScrollbarState_Hovered
		{
			u8 pointerId;
		};
		struct ScrollbarState_Pressed
		{
			u8 pointerId;
			f32 pointerRelativePosY;
		};
		using ScrollbarState_T = Std::Variant<
			ScrollbarState_Normal,
			ScrollbarState_Hovered,
			ScrollbarState_Pressed>;
		ScrollbarState_T scrollbarState = ScrollbarState_Normal{};

		Std::Box<Widget> widget;

		[[nodiscard]] virtual SizeHint GetSizeHint(
			Context const& ctx) const override;

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

		[[nodiscard]] virtual bool CursorPress(
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
			Gui::TouchEvent event,
			bool occluded) override;

		virtual void InputConnectionLost() override;

		virtual void CharEnterEvent(
			Context& ctx) override;

		virtual void CharEvent(
			Context& ctx,
			u32 utfValue) override;

		virtual void CharRemoveEvent(
			Context& ctx) override;
	};
}