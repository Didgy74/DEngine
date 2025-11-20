#pragma once

#include <DEngine/Gui/Widget.hpp>


#include <functional>
#include <string>

namespace DEngine::Gui
{
	class Button : public Widget
	{
	public:
		using ParentType = Widget;

		static constexpr Math::Vec3 hoverOverlayColor = { 0.1f, 0.1f, 0.1f };

		Button();
		virtual ~Button() override {}

		std::string text;

		struct Theme {
			Distance margin;
			Distance cornerRadius;
		};
		struct GetThemeParamsT {
			Button const& widget;
			Std::ConstAnyRef const& appData;
		};
		using GetThemeFnT = Theme(GetThemeParamsT const&);
		std::function<GetThemeFnT> getThemeFn;

		enum class Type {
			Push,
			Toggle
		};
		Type type = Type::Push;

		struct Colors {
			Math::Vec4 normal = { 0.3f, 0.3f, 0.3f, 1.f };
			Math::Vec4 normalText = Math::Vec4::One();
			Math::Vec4 toggled = { 0.6f, 0.6f, 0.6f, 1.f };
			Math::Vec4 toggledText = Math::Vec4::One();
			Math::Vec4 pressed = { 1.f, 1.f, 1.f, 1.f };
			Math::Vec4 pressedText = Math::Vec4::Zero();
		};
		Colors colors = {};

		struct ActivateParams {
			Button& btn;
			Std::AnyRef appData;
		};
		using ActivateCallback = void(ActivateParams const&);
		// TODO: Clarify whether this invoked in the midst of dispatch or not.
		std::function<ActivateCallback> activateFn = nullptr;

		void SetToggled(bool toggled);
		[[nodiscard]] bool GetToggled() const;


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
		virtual void AccessibilityTest(
			AccessibilityTest_Params const& params,
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

		virtual void WidgetEvent_Navigation(
			Widget_NavigationParams const& param,
			Std::Opt<RectCollection::Iter> const& rectCollIterParam,
			Widget_NavigationState& event) override
		{
			Widget_DefaultNavigationHandler(*this, param, rectCollIterParam, event);
		}

		virtual bool WidgetEvent_Activate(
			WidgetEvent_ActivateParams const&,
			Std::Opt<RectCollection::Iter> const&) override;

	protected:
		bool m_toggled = false;
		Std::Opt<u8> heldPointerId;
		bool hoveredByCursor = false;
		
		void Activate(Std::AnyRef customData);

		class Impl;
		friend Impl;
	};
}