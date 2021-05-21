#pragma once

#include <DEngine/Gui/Widget.hpp>

#include <functional>
#include <string>

namespace DEngine::Gui
{
	class Dropdown : public Widget
	{
	public:
		u32 selectedItem = 0;

		std::vector<std::string> items;

		using SelectionChangedCallback = void(Dropdown&);
		std::function<SelectionChangedCallback> selectedItemChangedCallback;

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
		
	private:
		Context* menuWidgetCtx = nullptr;
		WindowID menuWidgetWindowId{};
		Widget* menuWidget = nullptr;

		void CreateDropdownMenu(
			Context& ctx,
			WindowID windowId,
			Math::Vec2Int menuPosition);
		void ClearDropdownMenu();
	};
}