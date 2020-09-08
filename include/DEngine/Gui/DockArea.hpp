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
		Math::Vec2Int windowMovedRelativePos{};
		Std::Opt<Rect> highlightRect;
		Node const* holdingTab = nullptr;
		u32 tabDisconnectRadius = 25;
		Node const* showLayoutNodePtr = nullptr;
		u32 gizmoSize = 25;
		u32 gizmoPadding = 10;
		u32 resizeAreaRect = 10;
		enum class ResizeSide { Top, Bottom, Left, Right, };
		ResizeSide resizeSide{};
		bool resizingSplit = false;
		Node const* resizeSplitNode = nullptr;

		enum class Behavior
		{
			Normal,
			Moving,
			Resizing,
			HoldingTab,
		};
		Behavior currentBehavior = Behavior::Normal;


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

		virtual void Tick(
			Context& ctx,
			Rect widgetRect,
			Rect visibleRect) override;

		virtual void CursorMove(
			Context& ctx,
			Rect widgetRect,
			Rect visibleRect,
			CursorMoveEvent event) override;

		virtual void CursorClick(
			Context& ctx,
			Rect widgetRect,
			Rect visibleRect,
			Math::Vec2Int cursorPos,
			CursorClickEvent event) override;
	};
}