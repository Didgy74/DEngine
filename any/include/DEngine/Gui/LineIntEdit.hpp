#pragma once

#include <DEngine/Gui/Widget.hpp>

#include <DEngine/Std/Containers/Opt.hpp>
#include <DEngine/Std/Limits.hpp>

#include <string>
#include <functional>

namespace DEngine::Gui
{
	class LineIntEdit : public Widget
	{
	public:
		static constexpr i64 defaultMin = Std::Limits<i64>::lowest;
		static constexpr i64 defaultMax = Std::Limits<i64>::highest;
		// Set minimum to 0.0 or higher to get an unsigned text input session.
		i64 min = defaultMin;
		i64 max = defaultMax;

		using ValueChangedFnT = void(LineIntEdit& widget, i64 newValue);
		std::function<ValueChangedFnT> valueChangedFn;

		Math::Vec4 backgroundColor = { 0.3f, 0.3f, 0.3f, 1.f };
		u32 margin = 0;

		[[nodiscard]] bool HasInputSession() const noexcept { return inputConnectionCtx; }
		void SetValue(i64 in);

		virtual ~LineIntEdit() override;

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
		virtual void TextInput(
			Context& ctx,
			AllocRef const& transientAlloc,
			TextInputEvent const& event) override;
		virtual void EndTextInputSession(
			Context& ctx,
			AllocRef const& transientAlloc,
			EndTextInputSessionEvent const& event) override;

		struct Impl;
		friend Impl;

	protected:
		void ClearInputConnection();

		Std::Opt<u8> pointerId;
		Context* inputConnectionCtx = nullptr;
		std::string text = "0";
		i64 value = 0;
	};
}