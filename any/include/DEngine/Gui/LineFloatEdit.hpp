#pragma once

#include <DEngine/Gui/Widget.hpp>

#include <DEngine/Std/Containers/Opt.hpp>
#include <DEngine/Std/Limits.hpp>

#include <string>
#include <functional>

namespace DEngine::Gui::impl { struct LineFloatEditImpl; }

namespace DEngine::Gui
{
	class LineFloatEdit : public Widget
	{
	public:
		static constexpr f64 defaultMin = Std::Limits<f64>::lowest;
		static constexpr f64 defaultMax = Std::Limits<f64>::highest;
		// Set minimum to 0.0 or higher to get an unsigned text input session.
		f64 min = defaultMin;
		f64 max = defaultMax;

		using ValueChangedFnT = void(LineFloatEdit& widget, f64 newValue);
		std::function<ValueChangedFnT> valueChangedFn;

		Math::Vec4 backgroundColor = { 0.3f, 0.3f, 0.3f, 1.f };
		u32 margin = 0;

		[[nodiscard]] bool HasInputSession() const noexcept { return inputConnectionCtx; }
		void SetValue(f64 in);

		virtual ~LineFloatEdit() override;

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
		virtual void CharRemoveEvent(
			Context& ctx,
			Std::FrameAlloc& transientAlloc) override;
		virtual void TextInput(
			Context& ctx,
			Std::FrameAlloc& transientAlloc,
			TextInputEvent const& event) override;


		[[nodiscard]] virtual SizeHint GetSizeHint(
			Context const& ctx) const override;

		struct Impl;
		friend Impl;

	protected:
		Std::Opt<u8> pointerId;
		Context* inputConnectionCtx = nullptr;
		std::string text = "0.0";
		f64 value = 0.0;
		u8 decimalPoints = 3;
	};
}