#pragma once

#include <DEngine/FixedWidthTypes.hpp>
#include <DEngine/Gui/Widget.hpp>

#include <DEngine/Std/Containers/Opt.hpp>
#include <DEngine/Math/Vector.hpp>

#include <vector>
#include <string>
#include <functional>

namespace DEngine::Gui
{
	class ButtonGroup : public Widget
	{
	public:
		u32 activeIndex = 0;
		struct InternalButton {
			std::string title;
		};
		std::vector<InternalButton> buttons;

		u32 margin = 0;

		using ActiveChangedCallbackT = void(ButtonGroup& widget, u32 newIndex);
		std::function<ActiveChangedCallbackT> activeChangedCallback;

		struct Colors
		{
			Math::Vec4 inactiveColor = { 0.3f, 0.3f, 0.3f, 1.f };
			Math::Vec4 hoveredColor = { 0.4f, 0.4f, 0.4f, 1.f };
			Math::Vec4 activeColor = { 0.6f, 0.6f, 0.6f, 1.f };
		};
		Colors colors = {};

		void AddButton(std::string const& title);
		[[nodiscard]] u32 GetButtonCount() const;
		[[nodiscard]] u32 GetActiveButtonIndex() const;

		virtual SizeHint GetSizeHint2(
			GetSizeHint2_Params const& params) const override;
		virtual void BuildChildRects(
			BuildChildRects_Params const& params,
			Rect const& widgetRect,
			Rect const& visibleRect) const override;
		virtual void Render2(
			Render_Params const& params,
			Rect const& widgetRect,
			Rect const& visibleRect) const override;

		virtual void CursorExit(
			Context& ctx) override;

		virtual bool CursorMove(
			CursorMoveParams const& params,
			Rect const& widgetRect,
			Rect const& visibleRect,
			bool occluded) override;

		virtual bool CursorPress2(
			CursorPressParams const& params,
			Rect const& widgetRect,
			Rect const& visibleRect,
			bool consumed) override;

		virtual bool TouchMove2(
			TouchMoveParams const& params,
			Rect const& widgetRect,
			Rect const& visibleRect,
			bool occluded) override;

		virtual bool TouchPress2(
			TouchPressParams const& params,
			Rect const& widgetRect,
			Rect const& visibleRect,
			bool consumed) override;


		struct Impl;

	protected:
		struct HeldPointerData
		{
			uSize buttonIndex = 0;
			u8 pointerId = 0;
		};
		Std::Opt<HeldPointerData> heldPointerData;
		Std::Opt<uSize> cursorHoverIndex;


		friend Impl;
	};
}