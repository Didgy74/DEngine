#pragma once

#include <DEngine/Gui/Widget.hpp>

#include <DEngine/Std/Containers/Opt.hpp>

#include <functional>
#include <string>
#include <vector>

namespace DEngine::Gui
{
	class LineList : public Widget
	{
	public:
		static constexpr Math::Vec4 highlightOverlayColor = { 1.f, 1.f, 1.f, 0.5f };
		static constexpr Math::Vec4 hoverOverlayColor = { 1.f, 1.f, 1.f, 0.25f };
		static constexpr Math::Vec4 alternatingLineOverlayColor = {0.f, 0.f, 0.f, 0.1f };

		void RemoveLine(uSize index);
		
		Std::Opt<uSize> selectedLine;
		std::vector<std::string> lines;
		u32 textMargin = 0;

		using Callback = std::function<void(LineList&, Context* ctx)>;
		Callback selectedLineChangedFn = nullptr;

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

	protected:
		struct Impl;
		friend Impl;

		struct PressedLine_T
		{
			u8 pointerId;
			// This is null-opt if we pressed an area outside
			// the possible lines. Meaning it should lead to
			// a deselect of any currently selected line.
			Std::Opt<uSize> lineIndex;
		};
		Std::Opt<PressedLine_T> currPressedLine;
		Std::Opt<uSize> lineCursorHover;
	};
}