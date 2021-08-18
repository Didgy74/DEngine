#pragma once

#include <DEngine/Gui/Widget.hpp>

#include <DEngine/Std/Containers/Opt.hpp>

#include <functional>
#include <string>
#include <vector>

namespace DEngine::Gui
{
	class Dropdown : public Widget
	{
	public:
		u32 selectedItem = 0;

		std::vector<std::string> items;

		Std::Opt<u8> heldPointerId;

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

		virtual bool TouchPressEvent(
			Context& ctx,
			WindowID windowId,
			Rect widgetRect,
			Rect visibleRect,
			Gui::TouchPressEvent event) override;
	};
}