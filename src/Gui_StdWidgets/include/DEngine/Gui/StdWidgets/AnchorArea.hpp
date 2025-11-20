#pragma once

#include <DEngine/Gui/Widget.hpp>


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
			Distance offsetX;
			Distance offsetY;
			Std::Box<Widget> widget;
		};

		std::vector<Node> nodes;
		// If no widget, it falls back to the background color.
		Std::Box<Widget> backgroundWidget;
		static constexpr Math::Vec4 defaultBackgroundColor = { 0.5f, 0.5f, 0.5f, 1.f };
		Math::Vec4 backgroundColor = defaultBackgroundColor;

		virtual GetSizeHint2_ReturnT GetSizeHint2(
			GetSizeHint2_Params const& params) const override;
		virtual void BuildChildRects(
			BuildChildRects_Params const& params,
			Rect const& widgetRect,
			Rect const& visibleRect,
			Std::Opt<RectCollection::Iter> const& rectCollIter) const override;
		virtual void Render2(
			Render_Params const& params,
			Std::Opt<RectCollection::Iter> const& rectCollIter) const override;

		virtual bool CursorPress2(
			CursorPressParams const& params,
			Std::Opt<RectCollection::Iter> const& rectCollIter,
			bool consumed) override;
		virtual bool CursorMove(
			CursorMoveParams const& params,
			Std::Opt<RectCollection::Iter> const& rectCollIter,
			bool occluded) override;

		virtual TouchEventConsumption WidgetEvent_TouchMove(
			WidgetEvent_TouchMoveParams const& params,
			Std::Opt<RectCollection::Iter> const& rectCollIter,
			bool occluded) override;
		virtual TouchEventConsumption WidgetEvent_TouchPress(
			WidgetEvent_TouchPressParams const& params,
			Std::Opt<RectCollection::Iter> const& rectCollIter,
			bool consumed) override;

	};
}