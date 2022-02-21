#pragma once

#include <DEngine/Gui/Widget.hpp>

#include <DEngine/Std/Containers/Opt.hpp>
#include <DEngine/Math/Vector.hpp>

#include <functional>
#include <string>
#include <vector>

namespace DEngine::Gui::impl { class DropdownImpl; }

namespace DEngine::Gui
{
	class Dropdown : public Widget
	{
	public:
		static constexpr Math::Vec4 defaultBoxColor = { 1.f, 1.f, 1.f, 0.25f };
		static constexpr u32 defaultTextMargin = 10;

		Math::Vec4 boxColor = defaultBoxColor;

		u32 textMargin = defaultTextMargin;

		u32 selectedItem = 0;

		std::vector<std::string> items;

		using SelectionChangedCallback = void(Dropdown&);
		std::function<SelectionChangedCallback> selectionChangedCallback;

		Dropdown();
		virtual ~Dropdown() override;

		virtual SizeHint GetSizeHint2(
			GetSizeHint2_Params const& params) const override;

		virtual void Render2(
			Render_Params const& params,
			Rect const& widgetRect,
			Rect const& visibleRect) const override;

		virtual bool CursorPress2(
			CursorPressParams const& params,
			Rect const& widgetRect,
			Rect const& visibleRect,
			bool consumed) override;


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
			CursorPressEvent event) override;

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