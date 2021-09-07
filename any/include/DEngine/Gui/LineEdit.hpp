#pragma once

#include <DEngine/Gui/Widget.hpp>

#include <DEngine/Std/Containers/Opt.hpp>

#include <functional>
#include <string>

namespace DEngine::Gui::impl { struct LineEditImpl; }

namespace DEngine::Gui
{
	class LineEdit : public Widget
	{
	public:
		enum class Type
		{
			Float,
			Integer,
			UnsignedInteger,
		};
		Type type = Type::Float;

		Math::Vec4 backgroundColor = { 0.25f, 0.25f, 0.25f, 1.f };
		u32 margin = 0;

		using TextChangedFn = void(LineEdit& widget);
		std::function<TextChangedFn> textChangedFn = nullptr;

		std::string text;

		virtual ~LineEdit();

		[[nodiscard]] constexpr bool CurrentlyBeingEdited() const;

		[[nodiscard]] virtual SizeHint GetSizeHint(Context const& ctx) const override;

		virtual void Render(
			Context const& ctx,
			Extent framebufferExtent,
			Rect widgetRect,
			Rect visibleRect,
			DrawInfo& drawInfo) const override;

		virtual void CharEnterEvent(
			Context& ctx) override;

		virtual void CharEvent(
			Context& ctx,
			u32 charEvent) override;

		virtual void CharRemoveEvent(
			Context& ctx) override;

		virtual void InputConnectionLost() override;

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

	protected:
		// Holds the pointer ID if the widget is currently pressed.
		Std::Opt<u8> pointerId;

		Context* inputConnectionCtx = nullptr;

		void ClearInputConnection();

		friend impl::LineEditImpl;
	};

	constexpr bool LineEdit::CurrentlyBeingEdited() const { return inputConnectionCtx; }
}