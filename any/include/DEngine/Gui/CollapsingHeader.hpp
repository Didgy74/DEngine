#pragma once

#include <DEngine/Gui/Widget.hpp>
#include <DEngine/Gui/StackLayout.hpp>
#include <DEngine/Gui/Button.hpp>

#include <DEngine/Std/Containers/Box.hpp>
#include <DEngine/Std/Containers/Opt.hpp>

#include <functional>
#include <string>

namespace DEngine::Gui
{
	class CollapsingHeader : public Widget
	{
	public:
		CollapsingHeader();

		Std::Box<Widget> child;
		bool collapsed = true;
		std::string title = "Title";
		u32 titleMargin = 0;

		Math::Vec4 collapsedColor = { 0.3f, 0.3f, 0.3f, 1.f };
		Math::Vec4 expandedColor = { 0.6f, 0.6f, 0.6f, 1.f };

		using CollapseFnT = void(CollapsingHeader& widget);
		std::function<CollapseFnT> collapseFn;


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
		virtual void CharRemoveEvent(
			Context& ctx,
			Std::FrameAlloc& transientAlloc) override;
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


	protected:
		struct Impl;
		friend Impl;

		Std::Opt<u8> headerPointerId;
		bool hoveredByCursor = false;
	};
}