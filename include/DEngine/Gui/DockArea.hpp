#pragma once

#include <DEngine/Gui/Layout.hpp>
#include <DEngine/Gui/Widget.hpp>

#include <DEngine/Std/Containers/Box.hpp>
#include <DEngine/Std/Containers/Opt.hpp>

#include <DEngine/Math/Vector.hpp>

#include <vector>
#include <string>
#include <string_view>

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
		u32 gizmoSize = 75;
		u32 gizmoPadding = 15;
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

		struct Internal_Behavior_Normal
		{
		};
		struct Internal_Behavior_Moving
		{
			Math::Vec2Int windowMovedRelativePos;
			Node const* showLayoutNodePtr;
			bool useHighlightGizmo;
			impl::DockArea_LayoutGizmo highlightGizmo;
		};
		struct Internal_Behavior_Resizing
		{
			ResizeSide resizeSide;
			bool resizingSplit;
			Node const* resizeSplitNode;
			bool resizingSplitIsFrontNode;
		};
		struct Internal_Behavior_HoldingTab
		{
			bool holdingFrontWindow;
			Node const* holdingTab;
			Math::Vec2Int cursorPosRelative;
		};	
		union BehaviorData
		{
			Internal_Behavior_Normal normal;
			Internal_Behavior_Moving moving;
			Internal_Behavior_Resizing resizing;
			Internal_Behavior_HoldingTab holdingTab;
		};
		BehaviorData behaviorData{};

		void AddWindow(
			Rect windowRect,
			std::string_view title,
			Std::Box<Widget> widget);

		[[nodiscard]] virtual SizeHint GetSizeHint(
			Context const& ctx) const override;

		virtual void Render(
			Context const& ctx,
			Extent framebufferExtent,
			Rect widgetRect,
			Rect visibleRect,
			DrawInfo& drawInfo) const override;

		virtual void CursorMove(
			Context& ctx,
			WindowID windowId,
			Rect widgetRect,
			Rect visibleRect,
			CursorMoveEvent event) override;

		virtual void CursorClick(
			Context& ctx,
			WindowID windowId,
			Rect widgetRect,
			Rect visibleRect,
			Math::Vec2Int cursorPos,
			CursorClickEvent event) override;

		virtual void TouchEvent(
			Context& ctx,
			WindowID windowId,
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