#pragma once

#include <DEngine/Gui/Widget.hpp>
#include <DEngine/Gui/StackLayout.hpp>
#include <DEngine/Gui/Button.hpp>

#include <DEngine/Std/Containers/Box.hpp>

namespace DEngine::Gui
{
	class CollapsingHeader : public Widget
	{
	public:
		CollapsingHeader(bool collapsed = false);

		[[nodiscard]] StackLayout& GetChildStackLayout() { return *childStackLayoutPtr; }
		[[nodiscard]] StackLayout const& GetChildStackLayout() const { return *childStackLayoutPtr; }
		void SetCollapsed(bool newVal)
		{

		}

		[[nodiscard]] virtual SizeHint GetSizeHint(
			Context const& ctx) const override
		{
			return mainStackLayout.GetSizeHint(ctx);
		}

		virtual void Render(
			Context const& ctx,
			Extent framebufferExtent,
			Rect widgetRect,
			Rect visibleRect,
			DrawInfo& drawInfo) const override
		{
			return mainStackLayout.Render(
				ctx,
				framebufferExtent,
				widgetRect,
				visibleRect,
				drawInfo);
		}

		virtual void CharEnterEvent(
			Context& ctx) override
		{
			return mainStackLayout.CharEnterEvent(ctx);
		}

		virtual void CharEvent(
			Context& ctx,
			u32 utfValue) override
		{
			return mainStackLayout.CharEvent(ctx, utfValue);
		}

		virtual void CharRemoveEvent(
			Context& ctx) override
		{
			return mainStackLayout.CharRemoveEvent(ctx);
		}
		
		virtual void InputConnectionLost(
			Context& ctx) override
		{
			return mainStackLayout.InputConnectionLost(ctx);
		}

		virtual void CursorMove(
			Context& ctx,
			WindowID windowId,
			Rect widgetRect,
			Rect visibleRect,
			CursorMoveEvent event) override
		{
			return mainStackLayout.CursorMove(
				ctx,
				windowId,
				widgetRect,
				visibleRect,
				event);
		}

		virtual void CursorClick(
			Context& ctx,
			WindowID windowId,
			Rect widgetRect,
			Rect visibleRect,
			Math::Vec2Int cursorPos,
			CursorClickEvent event) override
		{
			return mainStackLayout.CursorClick(
				ctx,
				windowId,
				widgetRect,
				visibleRect,
				cursorPos,
				event);
		}

		virtual void TouchEvent(
			Context& ctx,
			WindowID windowId,
			Rect widgetRect,
			Rect visibleRect,
			Gui::TouchEvent event) override
		{
			return mainStackLayout.TouchEvent(
				ctx,
				windowId,
				widgetRect,
				visibleRect,
				event);
		}

	private:
		StackLayout mainStackLayout = StackLayout(StackLayout::Direction::Vertical);
		Std::Box<StackLayout> childStackLayoutBox;
		StackLayout* childStackLayoutPtr = nullptr;
	};
}