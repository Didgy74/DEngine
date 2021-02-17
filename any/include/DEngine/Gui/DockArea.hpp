#pragma once

#include <DEngine/Gui/Widget.hpp>

#include <DEngine/Std/Containers/Box.hpp>
#include <DEngine/Std/Containers/Opt.hpp>
#include <DEngine/Std/Containers/Variant.hpp>

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

	class DockArea : public Widget
	{
	public:
		static constexpr Math::Vec4 resizeHandleColor = { 1.f, 1.f, 1.f, 0.75f };

		DockArea();

		struct Node;
		struct Window
		{
			std::string title{};
			Math::Vec4 titleBarColor{ 0.5f, 0.5f, 0.5f, 1.f };
			Std::Box<Widget> widget;
		};
		struct Split
		{
			Std::Box<Node> a{};
			Std::Box<Node> b{};
			// In the range of 0 - 1
			f32 split = 0.5f;
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
		struct Layer
		{
			static constexpr Rect fullSizeRect{};
			Rect rect{};
			Std::Box<Node> node{};
		};
		std::vector<Layer> layers;
		
		u32 tabDisconnectRadius = 25;
		u32 gizmoSize = 75;
		u32 gizmoPadding = 15;
		u32 resizeAreaThickness = 20;
		u32 resizeHandleLength = 75;
		

		enum class ResizeSide { Top, Bottom, Left, Right, };

		struct BehaviorData_Normal
		{
		};
		struct BehaviorData_Moving
		{
			Math::Vec2Int windowMovedRelativePos;
			Node const* showLayoutNodePtr;
			bool useHighlightGizmo;
			impl::DockArea_LayoutGizmo highlightGizmo;
		};
		struct BehaviorData_Resizing
		{
			ResizeSide resizeSide;
			bool resizingSplit;
			Node const* resizeSplitNode;
			bool resizingSplitIsFrontNode;
		};
		struct BehaviorData_HoldingTab
		{
			bool holdingFrontWindow;
			Node const* holdingTab;
			Math::Vec2Int cursorPosRelative;
		};	
		Std::Variant<
			BehaviorData_Normal, 
			BehaviorData_Moving,
			BehaviorData_Resizing,
			BehaviorData_HoldingTab> behaviorData{};

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

		virtual void InputConnectionLost() override;

		virtual void CharEnterEvent(
			Context& ctx) override;

		virtual void CharEvent(
			Context& ctx,
			u32 utfValue) override;

		virtual void CharRemoveEvent(
			Context& ctx) override;
	};
}