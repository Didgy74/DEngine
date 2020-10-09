#pragma once

#include <DEngine/Gui/Layout.hpp>
#include <DEngine/Gui/Widget.hpp>

#include <DEngine/Containers/Box.hpp>
#include <DEngine/Containers/Opt.hpp>

#include <DEngine/Math/Vector.hpp>

#include <vector>
#include <string>

namespace DEngine::Gui
{
	namespace impl
	{
		enum class DockArea_LayoutGizmo { Top, Bottom, Left, Right, Center };
	}

	class DockArea : public Layout
	{
	public:
		DockArea();

		struct Node;
		struct Window
		{
			std::string title{};
			Math::Vec4 titleBarColor{ 0.5f, 0.5f, 0.5f, 1.f };
			Std::Box<Widget> widget;
			Std::Box<Layout> layout;
		};
		struct Split
		{
			Std::Box<Node> a{};
			Std::Box<Node> b{};
			f32 split = 0.5f; // ???
		};
		struct Node
		{
			enum class Type : char
			{
				Window,
				HoriSplit,
				VertSplit,
			};
			Type type{};
			std::vector<Window> windows;
			uSize selectedWindow = 0;
			Split split{};
		};
		struct TopLevelNode
		{
			static constexpr Rect fullSizeRect{};
			Rect rect{};
			Std::Box<Node> node{};
		};
		std::vector<TopLevelNode> topLevelNodes;
		
		u32 tabDisconnectRadius = 25;
		u32 gizmoSize = 50;
		u32 gizmoPadding = 10;
		u32 resizeAreaThickness = 30;
		u32 resizeHandleLength = 50;

		enum class ResizeSide { Top, Bottom, Left, Right, };

		enum class Behavior
		{
			Normal,
			Moving,
			Resizing,
			HoldingTab,
		};
		Behavior behavior = Behavior::Normal;
		union BehaviorData
		{
			template<Behavior>
			struct Internal;
			template<>
			struct Internal<Behavior::Normal>
			{
			};
			template<>
			struct Internal<Behavior::Moving>
			{
				Math::Vec2Int windowMovedRelativePos;
				Node const* showLayoutNodePtr;
				bool useHighlightGizmo;
				impl::DockArea_LayoutGizmo highlightGizmo;
			};
			template<>
			struct Internal<Behavior::Resizing>
			{
				ResizeSide resizeSide;
				bool resizingSplit;
				Node const* resizeSplitNode;
				bool resizingSplitIsFrontNode;
			};
			template<>
			struct Internal<Behavior::HoldingTab>
			{
				bool holdingFrontWindow;
				Node const* holdingTab;
				Math::Vec2Int cursorPosRelative;
			};
			Internal<Behavior::Normal> normal;
			Internal<Behavior::Moving> moving;
			Internal<Behavior::Resizing> resizing;
			Internal<Behavior::HoldingTab> holdingTab;
		};
		BehaviorData behaviorData{};

		[[nodiscard]] virtual Gui::SizeHint SizeHint(
			Context const& ctx) const override;

		[[nodiscard]] virtual Gui::SizeHint SizeHint_Tick(
			Context const& ctx) override;

		virtual void Render(
			Context const& ctx,
			Extent framebufferExtent,
			Rect widgetRect,
			Rect visibleRect,
			DrawInfo& drawInfo) const override;

		virtual void CursorMove(
			Test& test,
			Rect widgetRect,
			Rect visibleRect,
			CursorMoveEvent event) override;

		virtual void CursorClick(
			Context& ctx,
			Rect widgetRect,
			Rect visibleRect,
			Math::Vec2Int cursorPos,
			CursorClickEvent event) override;

		virtual void TouchEvent(
			Context& ctx,
			Rect widgetRect,
			Rect visibleRect,
			Gui::TouchEvent event) override;

		virtual void CharEnterEvent(
			Context& ctx) override;

		virtual void CharEvent(
			Context& ctx,
			u32 utfValue) override;

		virtual void CharRemoveEvent(
			Context& ctx) override;
	};
}