#pragma once

#include <DEngine/Gui/Widget.hpp>


#include <vector>
#include <functional>

namespace DEngine::Gui
{
	class Grid : public Widget
	{
	public:
		struct Theme {
			Distance spacing;
		};
		struct GetThemeParamsT {
			Grid const& widget;
			Std::ConstAnyRef const& appData;
		};
		using GetThemeFnT = Theme(GetThemeParamsT const&);
		std::function<GetThemeFnT> getThemeFn;

		void SetDimensions(int width, int height);
		void SetWidth(int newWidth);
		[[nodiscard]] int GetWidth() const noexcept;
		void SetHeight(int newHeight);
		[[nodiscard]] int GetHeight() const noexcept;

		int PushBackColumn();
		int PushBackRow();

		void SetChild(int x, int y, Std::Box<Widget>&& in);
		Widget* GetChild(int x, int y);
		Widget const* GetChild(int x, int y) const;

		virtual GetSizeHint2_ReturnT GetSizeHint2(
			GetSizeHint2_Params const& params) const override;
		virtual void BuildChildRects(
			BuildChildRects_Params const& params,
			Rect const& widgetRect,
			Rect const& visibleRect,
			Std::Opt<RectCollection::Iter> const& rectCollIter) const override;
		virtual bool CursorMove(
			CursorMoveParams const& params,
			Std::Opt<RectCollection::Iter> const& rectCollIter,
			bool occluded) override;
		virtual bool CursorPress2(
			CursorPressParams const& params,
			Std::Opt<RectCollection::Iter> const& rectCollIter,
			bool consumed) override;
		virtual TouchEventConsumption WidgetEvent_TouchMove(
			WidgetEvent_TouchMoveParams const& params,
			Std::Opt<RectCollection::Iter> const& rectCollIter,
			bool occluded) override;
		virtual TouchEventConsumption WidgetEvent_TouchPress(
			WidgetEvent_TouchPressParams const& params,
			Std::Opt<RectCollection::Iter> const& rectCollIter,
			bool consumed) override;
		virtual void WidgetEvent_TextInput(
			AllocRef const& transientAlloc,
			WidgetEvent_TextInputParams const& event) override;
		virtual void WidgetEvent_EndTextInputSession(
			AllocRef const& transientAlloc,
			WidgetEvent_EndTextInputSessionParams const& event) override;
		virtual void Render2(
			Render_Params const& params,
			Std::Opt<RectCollection::Iter> const& rectCollIter) const override;
		virtual void AccessibilityTest(
			AccessibilityTest_Params const& params,
			Std::Opt<RectCollection::Iter> const& rectCollIter) const override;

	protected:
		// Elements are stored row-major.
		std::vector<Std::Box<Widget>> children;
		int width = 0;
		int height = 0;

		struct Impl;
		friend Impl;
	};
}