#pragma once

#include <DEngine/Gui/Widget.hpp>

#include <DEngine/Std/Containers/Opt.hpp>

#include <functional>
#include <string>

namespace DEngine::Gui
{
	class Context;

	class Button : public Widget
	{
	public:
		using ParentType = Widget;

		static constexpr Math::Vec3 hoverOverlayColor = { 0.1f, 0.1f, 0.1f };

		Button();
		virtual ~Button() override {}

		std::string text;
		u32 textMargin = 10;
		
		enum class Type
		{
			Push,
			Toggle
		};
		Type type = Type::Push;

		struct Colors
		{
			Math::Vec4 normal = { 0.3f, 0.3f, 0.3f, 1.f };
			Math::Vec4 normalText = Math::Vec4::One();
			Math::Vec4 toggled = { 0.6f, 0.6f, 0.6f, 1.f };
			Math::Vec4 toggledText = Math::Vec4::One();
			Math::Vec4 pressed = { 1.f, 1.f, 1.f, 1.f };
			Math::Vec4 pressedText = Math::Vec4::Zero();
		};
		Colors colors = {};

		using ActivateCallback = void(Button& btn);
		std::function<ActivateCallback> activateFn = nullptr;

		void SetToggled(bool toggled);
		[[nodiscard]] bool GetToggled() const;


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

		virtual void CursorExit(
			Context& ctx) override;



		virtual bool TouchMoveEvent(
			Context& ctx,
			WindowID windowId,
			Rect widgetRect,
			Rect visibleRect,
			Gui::TouchMoveEvent event,
			bool occluded) override;

		virtual bool TouchPressEvent(
			Context& ctx,
			WindowID windowId,
			Rect widgetRect,
			Rect visibleRect,
			Gui::TouchPressEvent event) override;

	protected:
		bool toggled = false;
		Std::Opt<u8> heldPointerId;
		bool hoveredByCursor = false;
		
		void Activate();

		struct Impl;
		friend Impl;
	};
}