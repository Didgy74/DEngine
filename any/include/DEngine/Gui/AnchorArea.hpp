#pragma once

#include <DEngine/Gui/Widget.hpp>
#include <DEngine/Std/Containers/Box.hpp>

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
			u32 utfValue) override;

		virtual void CharRemoveEvent(
			Context& ctx) override;

		virtual void InputConnectionLost() override;


		virtual bool CursorPress(
			Context& ctx,
			WindowID windowId,
			Rect widgetRect,
			Rect visibleRect,
			Math::Vec2Int cursorPos,
			CursorPressEvent event) override;

		virtual bool CursorMove(
			Context& ctx,
			WindowID windowId,
			Rect widgetRect,
			Rect visibleRect,
			CursorMoveEvent event,
			bool occluded) override;

		virtual bool TouchPressEvent(
			Context& ctx,
			WindowID windowId,
			Rect widgetRect,
			Rect visibleRect,
			Gui::TouchPressEvent event) override;

		virtual bool TouchMoveEvent(
			Context& ctx,
			WindowID windowId,
			Rect widgetRect,
			Rect visibleRect,
			Gui::TouchMoveEvent event,
			bool occluded) override;
	};
}