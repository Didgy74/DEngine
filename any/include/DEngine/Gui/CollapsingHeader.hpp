#pragma once

#include <DEngine/Gui/Widget.hpp>
#include <DEngine/Gui/StackLayout.hpp>
#include <DEngine/Gui/Button.hpp>

#include <DEngine/Std/Containers/Box.hpp>
#include <DEngine/Std/Containers/Opt.hpp>
#include <DEngine/Std/Containers/Str.hpp>

#include <functional>
#include <string>

namespace DEngine::Gui::impl { struct CH_Impl; }

namespace DEngine::Gui
{
	class CollapsingHeader : public Widget
	{
	public:

		CollapsingHeader();

		Std::Box<Widget> child;
		bool collapsed = true;
		std::string title = "Title";
		u32 titleMargin = 0;

		using CollapseFnT = void(CollapsingHeader& widget);
		std::function<CollapseFnT> collapseFn;

		[[nodiscard]] virtual SizeHint GetSizeHint(
			Context const& ctx) const override;

		virtual void Render(
			Context const& ctx,
			Extent framebufferExtent,
			Rect widgetRect,
			Rect visibleRect,
			DrawInfo& drawInfo) const override;

		virtual void CharEnterEvent(
			Context& ctx) override;

		virtual void CharEvent(
			Context& ctx,
			u32 utfValue) override;

		virtual void CharRemoveEvent(
			Context& ctx) override;
		
		virtual void InputConnectionLost() override;

		virtual bool CursorPress(
			Context& ctx,
			WindowID windowId,
			Rect widgetRect,
			Rect visibleRect,
			Math::Vec2Int cursorPos,
			CursorClickEvent event) override;

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

	protected:
		friend impl::CH_Impl;

		Std::Opt<u8> headerPointerId;
		bool hoveredByCursor = false;
	};
}