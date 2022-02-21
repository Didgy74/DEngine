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
		enum class Type
		{
			Float,
			Integer,
			UnsignedInteger,
		};
		Type type = Type::Float;

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


		[[nodiscard]] virtual SizeHint GetSizeHint(Context const& ctx) const override;

		virtual void Render(
			Context const& ctx,
			Extent framebufferExtent,
			Rect widgetRect,
			Rect visibleRect,
			DrawInfo& drawInfo) const override;

		virtual void CharRemoveEvent(
			Context& ctx,
			Std::FrameAlloc& transientAlloc) override;

		virtual void InputConnectionLost() override;

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

		struct Impl;
		friend Impl;
	};

	constexpr bool LineEdit::HasInputSession() const { return inputConnectionCtx; }
}