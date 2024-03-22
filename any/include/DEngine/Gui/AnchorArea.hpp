#pragma once

#include <DEngine/Gui/Widget.hpp>
#include <DEngine/Std/Containers/Box.hpp>
#include <DEngine/Std/Containers/Opt.hpp>

#include <DEngine/Math/Vector.hpp>

#include <vector>

namespace DEngine::Gui
{
	class AnchorArea : public Widget
	{
	public:
		AnchorArea();

		enum class AnchorX {
			Left,
			Center,
			Right
		};

		enum class AnchorY {
			Top,
			Center,
			Bottom
		};

		struct Node {
			AnchorX anchorX;
			AnchorY anchorY;
			Std::Box<Widget> widget;
		};

		std::vector<Node> nodes;
		// If no widget, it falls back to the background color.
		Std::Box<Widget> backgroundWidget;
		static constexpr Math::Vec4 defaultBackgroundColor = { 0.5f, 0.5f, 0.5f, 1.f };
		Math::Vec4 backgroundColor = defaultBackgroundColor;

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
		virtual bool CursorMove(
			CursorMoveParams const& params,
			Rect const& widgetRect,
			Rect const& visibleRect,
			bool occluded) override;

		virtual bool TouchMove2(
			TouchMoveParams const& params,
			Rect const& widgetRect,
			Rect const& visibleRect,
			bool occluded) override;
		virtual bool TouchPress2(
			TouchPressParams const& params,
			Rect const& widgetRect,
			Rect const& visibleRect,
			bool consumed) override;

	};
}