#pragma once

#include <DEngine/Gui/Widget.hpp>

#include <DEngine/Std/Containers/Opt.hpp>

#include <functional>
#include <string>

namespace DEngine::Gui
{
	class LineEdit : public Widget
	{
	public:
		Math::Vec4 backgroundColor = { 0.3f, 0.3f, 0.3f, 1.f };
		u32 margin = 0;

		using TextChangedFn = void(LineEdit& widget);
		std::function<TextChangedFn> textChangedFn = nullptr;

		std::string text;

		virtual ~LineEdit();

		[[nodiscard]] constexpr bool HasInputSession() const;

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
		virtual bool CursorPress2(
			CursorPressParams const& params,
			Rect const& widgetRect,
			Rect const& visibleRect,
			bool consumed) override;
		virtual bool TouchPress2(
			TouchPressParams const& params,
			Rect const& widgetRect,
			Rect const& visibleRect,
			bool consumed) override;
		void TextInput(
			Context& ctx,
			AllocRef const& transientAlloc,
			TextInputEvent const& event) override;
		void TextSelection(
			Context& ctx,
			AllocRef const& transientAlloc,
			TextSelectionEvent const& event) override;
		void TextDelete(
			Context& ctx,
			AllocRef const& transientAlloc,
			WindowID windowId) override;
		void EndTextInputSession(
			Context& ctx,
			AllocRef const& transientAlloc,
			EndTextInputSessionEvent const& event) override;

	protected:
		// Holds the pointer ID if the widget is currently pressed.
		Std::Opt<u8> pointerId;

		Context* inputConnectionCtx = nullptr;

		void ClearInputConnection();

		struct Impl;
		friend Impl;
	};

	constexpr bool LineEdit::HasInputSession() const { return inputConnectionCtx; }
}