#pragma once

#include <DEngine/Gui/Widget.hpp>

#include <DEngine/Std/Containers/Box.hpp>
#include <DEngine/Std/Containers/Opt.hpp>

namespace DEngine::Gui::impl { class SA_Impl; }

namespace DEngine::Gui
{
	class ScrollArea : public Widget
	{
	public:
		using ParentType = Widget;

		Std::Box<Widget> child;
		static constexpr bool defaultExpandChild = true;
		bool expandChild = defaultExpandChild;

		static constexpr Math::Vec3 scrollbarHoverHighlight = { 0.1f, 0.1f, 0.1f };
		Math::Vec4 scrollbarInactiveColor = { 0.4f, 0.4f, 0.4f, 1.f };

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

		virtual void CursorExit(
			Context& ctx) override;

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

		virtual void TextInput(
			Context& ctx,
			Std::FrameAlloc& transientAlloc,
			TextInputEvent const& event) override;




		[[nodiscard]] virtual SizeHint GetSizeHint(
			Context const& ctx) const override;

		virtual void Render(
			Context const& ctx,
			Extent framebufferExtent,
			Rect widgetRect,
			Rect visibleRect,
			DrawInfo& drawInfo) const override;

		virtual void InputConnectionLost() override;

		virtual void CharRemoveEvent(
			Context& ctx,
			Std::FrameAlloc& transientAlloc) override;

	private:
		friend impl::SA_Impl;

		u32 currScrollbarPos = 0;
		u32 scrollbarThickness = 50;

		struct Scrollbar_Pressed_T
		{
			u8 pointerId;
			f32 pointerRelativePosY;
		};
		Std::Opt<Scrollbar_Pressed_T> scrollbarHoldData;

		bool scrollbarHoveredByCursor = false;
	};
}