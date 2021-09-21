#pragma once

#include <DEngine/Gui/Widget.hpp>

#include <DEngine/Std/Containers/Opt.hpp>

#include <functional>
#include <string>
#include <vector>

namespace DEngine::Gui::impl { class DropdownImpl; }

namespace DEngine::Gui
{
	class Dropdown : public Widget
	{
	public:
		u32 textMargin = 0;

		u32 selectedItem = 0;

		std::vector<std::string> items;

		using SelectionChangedCallback = void(Dropdown&);
		std::function<SelectionChangedCallback> selectionChangedCallback;

		Dropdown();
		virtual ~Dropdown() override;

		[[nodiscard]] virtual SizeHint GetSizeHint(
			Context const& ctx) const override;

		virtual void Render(
			Context const& ctx,
			Extent framebufferExtent,
			Rect widgetRect,
			Rect visibleRect,
			DrawInfo& drawInfo) const override;

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
			bool cursorOccluded) override;

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

	private:
		friend impl::DropdownImpl;

		Std::Opt<u8> heldPointerId;
	};
}