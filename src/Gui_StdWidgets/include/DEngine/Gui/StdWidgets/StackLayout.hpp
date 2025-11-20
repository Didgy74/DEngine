#pragma once

#include <DEngine/Gui/Widget.hpp>


#include <vector>
#include <functional>

namespace DEngine::Gui
{
	// It is illegal to add/remove Widgets from the StackLayout in the middle of event dispatching.
	class StackLayout : public Widget
	{
	public:
		using ParentType = Widget;

		enum class Direction {
			Horizontal,
			Vertical,
		};
		using Dir = Direction;

		explicit StackLayout(Direction direction = Direction::Horizontal) :
			direction(direction)
		{}

		virtual ~StackLayout() override;

		Direction direction{};
		Math::Vec4 color{ 1.f, 1.f, 1.f, 0.f };

		struct Theme {
			Distance spacing;
		};
		struct GetThemeParamsT {
			StackLayout const& widget;
			Std::ConstAnyRef const& appData;
		};
		using GetThemeFnT = Theme(GetThemeParamsT const&);
		std::function<GetThemeFnT> getThemeFn;

		bool expandNonDirection = false;

		[[nodiscard]] uSize ChildCount() const;
		[[nodiscard]] Widget& At(uSize index);
		[[nodiscard]] Widget const& At(uSize index) const;
		void AddWidget(Std::Box<Widget>&& in);
		Std::Box<Widget> ExtractChild(uSize index);
		void InsertWidget(uSize index, Std::Box<Widget>&& in);
		void RemoveItem(uSize index);
		void ClearChildren();

		virtual GetSizeHint2_ReturnT GetSizeHint2(
			GetSizeHint2_Params const& params) const override;
		virtual void BuildChildRects(
			BuildChildRects_Params const& params,
			Rect const& widgetRect,
			Rect const& visibleRect,
			Std::Opt<RectCollection::Iter> const& rectCollIter) const override;
		virtual void Render2(
			Render_Params const& params,
			Std::Opt<RectCollection::Iter> const& rectCollIter) const override;
		virtual void CursorExit() override;
		virtual void WidgetEvent_TextInput(
			AllocRef const& transientAlloc,
			WidgetEvent_TextInputParams const& event) override;
		virtual void WidgetEvent_TextSelection(
			AllocRef const& transientAlloc,
			WidgetEvent_TextSelectionParams const& event) override;
		virtual void WidgetEvent_EndTextInputSession(
			AllocRef const& transientAlloc,
			WidgetEvent_EndTextInputSessionParams const& event) override;

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

		void AccessibilityTest(
			AccessibilityTest_Params const& params,
			Std::Opt<RectCollection::Iter> const& rectColIter) const override;

		virtual void WidgetEvent_Navigation(
			Widget_NavigationParams const& params,
			Std::Opt<RectCollection::Iter> const& rectCollIter,
			Widget_NavigationState&) override;

	protected:
		std::vector<Std::Box<Widget>> children;

		bool currentlyIterating = false;

		struct Impl;
		friend Impl;
	};
}