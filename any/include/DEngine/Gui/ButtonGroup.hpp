#pragma once

#include <DEngine/FixedWidthTypes.hpp>
#include <DEngine/Gui/Widget.hpp>

#include <DEngine/Std/Containers/Opt.hpp>

#include <vector>
#include <string>
#include <functional>

namespace DEngine::Gui
{
	class ButtonGroup : public Widget
	{
	public:
		u32 activeIndex = 0;
		struct InternalButton
		{
			std::string title;
		};
		std::vector<InternalButton> buttons;

		struct HeldPointerData
		{
			u8 buttonIndex = 0;
			u8 pointerId = 0;
		};
		Std::Opt<HeldPointerData> heldPointerId;

		using ActiveChangedCallbackT = void(ButtonGroup& widget, u32 newIndex);
		std::function<ActiveChangedCallbackT> activeChangedCallback;

		void AddButton(std::string const& title);
		[[nodiscard]] u32 GetButtonCount() const;
		[[nodiscard]] u32 GetActiveButtonIndex() const;

		[[nodiscard]] virtual SizeHint GetSizeHint(Context const& ctx) const override;

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
	};
}