#pragma once

#include <DEngine/Gui/Widget.hpp>

#include <DEngine/Std/Containers/Opt.hpp>

#include <functional>
#include <string>

namespace DEngine::Gui::impl
{
	class BtnImpl;
}

namespace DEngine::Gui
{
	class Context;

	class Button : public Widget
	{
	public:
		using ParentType = Widget;

		Button();
		virtual ~Button() override {}

		std::string text;
		u32 textMargin = 0;
		
		enum class Type
		{
			Push,
			Toggle
		};
		Type type = Type::Push;
		
		Math::Vec4 normalColor = { 0.3f, 0.3f, 0.3f, 1.f };
		Math::Vec4 normalTextColor = Math::Vec4::One();
		Math::Vec4 toggledColor = { 0.6f, 0.6f, 0.6f, 1.f };
		Math::Vec4 toggledTextColor = Math::Vec4::One();
		Math::Vec4 hoverColor = { 0.4f, 0.4f, 0.4f, 1.f };
		Math::Vec4 hoverTextColor = Math::Vec4::One();
		Math::Vec4 pressedColor = { 1.f, 1.f, 1.f, 1.f };
		Math::Vec4 pressedTextColor = Math::Vec4::Zero();

		using ActivateCallback = void(Button& btn);
		std::function<ActivateCallback> activateFn = nullptr;

		void SetToggled(bool toggled);
		[[nodiscard]] bool GetToggled() const;

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

		virtual bool CursorMove(
			Context& ctx,
			WindowID windowId,
			Rect widgetRect,
			Rect visibleRect,
			CursorMoveEvent event,
			bool cursorOccluded) override;

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
		enum class State
		{
			Normal,
			Hovered,
			Pressed,
			Toggled,
		};
		State state = State::Normal;
		Std::Opt<u8> pointerId;
		
		void Activate(
			Context& ctx);

		friend impl::BtnImpl;
	};
}