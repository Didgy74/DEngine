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

		enum class AnchorX 
		{
			Left,
			Center,
			Right
		};

		enum class AnchorY
		{
			Top,
			Center,
			Bottom
		};

		struct Node
		{
			AnchorX anchorX;
			AnchorY anchorY;
			Extent extent;
			Std::Box<Widget> widget;
		};

		std::vector<Node> nodes;
		// If no widget, it falls back to the background color.
		Std::Box<Widget> backgroundWidget;
		Math::Vec4 backgroundColor;

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

		[[nodiscard]] virtual SizeHint GetSizeHint(
			Context const& ctx) const override;

		virtual void Render(
			Context const& ctx,
			Extent framebufferExtent,
			Rect widgetRect,
			Rect visibleRect,
			DrawInfo& drawInfo) const override;

		virtual void InputConnectionLost() override;
	};
}