#pragma once

#include <DEngine/Gui/Widget.hpp>

#include <functional>
#include <string>
#include <vector>

namespace DEngine::Gui {
	class LineList : public Widget {
	public:
		static constexpr Math::Vec4 highlightOverlayColor = { 1.f, 1.f, 1.f, 0.5f };
		static constexpr Math::Vec4 hoverOverlayColor = { 1.f, 1.f, 1.f, 0.25f };
		static constexpr Math::Vec4 alternatingLineOverlayColor = {0.f, 0.f, 0.f, 0.1f };

		// Does not call line changed callbacks.
		void RemoveLine(uSize index);
		
		Std::Opt<uSize> selectedLine;
		std::vector<std::string> lines;

		struct Theme {
			Distance textMargin;
		};
		struct GetThemeParamsT {
			LineList const& widget;
			Std::ConstAnyRef const& appData;
		};
		using GetThemeFnT = Theme(GetThemeParamsT const&);
		std::function<GetThemeFnT> getThemeFn;

		using Callback = std::function<void(LineList&, WidgetEvent_DeferredJobQueue&)>;
		Callback selectedLineChangedFn = nullptr;

		[[nodiscard]] virtual GetSizeHint2_ReturnT GetSizeHint2(
			GetSizeHint2_Params const& params) const override;
		virtual void BuildChildRects(
			BuildChildRects_Params const& params,
			Rect const& widgetRect,
			Rect const& visibleRect,
			Std::Opt<RectCollection::Iter> const& rectCollIter) const override;
		virtual void Render2(
			Render_Params const& params,
			Std::Opt<RectCollection::Iter> const& rectCollIter) const override;
		virtual bool CursorMove(
			CursorMoveParams const& params,
			Std::Opt<RectCollection::Iter> const& rectCollIter,
			bool occluded) override;
		virtual bool CursorPress2(
			CursorPressParams const& params,
			Std::Opt<RectCollection::Iter> const& rectCollIter,
			bool consumed) override;
		virtual void CursorExit() override;
		virtual TouchEventConsumption WidgetEvent_TouchMove(
			WidgetEvent_TouchMoveParams const& params,
			Std::Opt<RectCollection::Iter> const& rectCollIter,
			bool occluded) override;
		virtual TouchEventConsumption WidgetEvent_TouchPress(
			WidgetEvent_TouchPressParams const& params,
			Std::Opt<RectCollection::Iter> const& rectCollIter,
			bool consumed) override;

	protected:
		struct Impl;
		friend Impl;

		struct PressedLine_T {
			u8 pointerId;
			// This is null-opt if we pressed an area outside
			// the possible lines. Meaning it should lead to
			// a deselect of any currently selected line.
			Std::Opt<uSize> lineIndex;
		};
		Std::Opt<PressedLine_T> currPressedLine;
		Std::Opt<uSize> lineCursorHover;
	};
}