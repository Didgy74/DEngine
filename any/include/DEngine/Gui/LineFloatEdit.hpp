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
		f64 min = defaultMin;
		f64 max = defaultMax;

		using TextChangedFnT = void(LineFloatEdit& widget, f64 newValue);
		std::function<TextChangedFnT> valueChangedFn;

		Math::Vec4 backgroundColor = { 0.0f, 0.0f, 0.0f, 0.25f };

		[[nodiscard]] inline bool CurrentlyBeingEdited() const noexcept { return inputConnectionCtx; }

		virtual ~LineFloatEdit() override;

		[[nodiscard]] virtual SizeHint GetSizeHint(
			Context const& ctx) const override;

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
		Std::Opt<u8> pointerId;
		Context* inputConnectionCtx = nullptr;
		std::string text = "0.0";
		f64 currentValue = 0.0;
		u8 decimalPoints = 3;

		void EndEditingSession();
		void StartInputConnection(Context& ctx);
		void ClearInputConnection();

		friend impl::LineFloatEditImpl;
	};
}