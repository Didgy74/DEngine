#pragma once

#include <DEngine/Gui/DockArea.hpp>
#include <DEngine/Gui/AllocRef.hpp>
#include <DEngine/Std/Containers/FnRef.hpp>
#include <DEngine/Std/Containers/Vec.hpp>

#include <DEngine/Std/Containers/Array.hpp>

#include "Iterators.inl"

struct DEngine::Gui::DockArea::Impl
{
	using Layer = DockArea::Layer;
	using StateDataT = DockArea::StateDataT;
	using State_Normal = DockArea::State_Normal;
	using State_Moving = DockArea::State_Moving;
	using State_ResizingSplit = DockArea::State_ResizingSplit;
	using State_HoldingTab = DockArea::State_HoldingTab;

	[[nodiscard]] static auto& GetLayers(DockArea& in) { return in.layers; }
	[[nodiscard]] static auto const& GetLayers(DockArea const& in) { return in.layers; }
	[[nodiscard]] static auto& GetStateData(DockArea& in) { return in.stateData; }
	[[nodiscard]] static auto const& GetStateData(DockArea const& in) { return in.stateData; }
};

namespace DEngine::Gui::impl
{
	using DA_Layer = DockArea::Impl::Layer;
	[[nodiscard]] inline auto& DA_GetLayers(DockArea& in) { return DockArea::Impl::GetLayers(in); }
	[[nodiscard]] inline auto const& DA_GetLayers(DockArea const& in) { return DockArea::Impl::GetLayers(in); }
	[[nodiscard]] inline auto& DA_GetStateData(DockArea& in) { return DockArea::Impl::GetStateData(in); }
	[[nodiscard]] inline auto const& DA_GetStateData(DockArea const& in) { return DockArea::Impl::GetStateData(in); }
	using DA_StateData = DockArea::Impl::StateDataT;
	using DA_State_Normal = DockArea::Impl::State_Normal;
	using DA_State_Moving = DockArea::Impl::State_Moving;
	using DA_State_ResizingSplit = DockArea::Impl::State_ResizingSplit;
	using DA_State_HoldingTab = DockArea::Impl::State_HoldingTab;

	static constexpr auto cursorPointerId = static_cast<u8>(-1);

	enum class NodeType : u8 { Window, Split };
	struct DA_Node
	{
		DA_Node() = default;
		DA_Node(DA_Node const&) = delete;
		DA_Node(DA_Node&&) = default;
		[[nodiscard]] virtual NodeType GetNodeType() const = 0;
		virtual ~DA_Node() {}
	};

	struct DA_WindowTab
	{
		std::string title;
		Math::Vec4 color = {};
		Std::Box<Widget> widget;
	};
	struct DA_WindowNode : public DA_Node
	{
		uSize activeTabIndex = 0;
		std::vector<DA_WindowTab> tabs;

		[[nodiscard]] virtual NodeType GetNodeType() const override { return NodeType::Window; }
		// Erases tab and updates the active tab index.
		void EraseTab(uSize index) {
			DENGINE_IMPL_GUI_ASSERT(index < tabs.size());
			tabs.erase(tabs.begin() + index);
			if (index <= activeTabIndex && index != 0)
				activeTabIndex -= 1;
		}
	};
	[[nodiscard]] Rect DA_Layer_FindWindowNodeRect(
		DA_Node const& rootNode,
		Rect const& layerRect,
		DA_WindowNode const* targetNode,
		AllocRef transientAlloc);
	[[nodiscard]] Rect DA_FindNodeRect(
		DockArea const& dockArea,
		Rect const& widgetRect,
		DA_Node const* targetNode,
		AllocRef transientAlloc);

	struct CheckHitWindowNode_ReturnT {
		DA_WindowNode* window;
		Rect rect;
	};
	[[nodiscard]] CheckHitWindowNode_ReturnT CheckHitWindowNode(
		DA_Node& rootNode,
		Rect const& layerRect,
		Math::Vec2 point,
		AllocRef transientAlloc);
	Std::Vec<Rect, AllocRef> DA_WindowNode_BuildTabRects(
		Rect const& titlebarRect,
		Std::Span<DA_WindowTab const> tabs,
		u32 textMargin,
		TextManager& textManager,
		AllocRef transientAlloc);
	// Checks if a point overlaps with any tabs
	// Returns the index of the tab if it overlapped.
	[[nodiscard]] Std::Opt<uSize> DA_CheckHitTab(
		Rect const& titlebarRect,
		Std::Span<DA_WindowTab const> tabs,
		u32 textMargin,
		TextManager& textManager,
		Math::Vec2 point,
		AllocRef const& transientAlloc);
	void Render_WindowNode(
		DA_WindowNode const& windowNode,
		DockArea const& dockArea,
		Rect const& nodeRect,
		Rect const& visibleRect,
		Widget::Render_Params const& params);

	struct DA_WindowNodePrimaryRects
	{
		Rect titlebarRect;
		Rect contentRect;
	};
	[[nodiscard]] DA_WindowNodePrimaryRects DA_WindowNode_BuildPrimaryRects(
		Rect const& nodeRect,
		u32 totalLineheight);

	struct DA_SplitNode : public DA_Node
	{
		[[nodiscard]] virtual NodeType GetNodeType() const override { return NodeType::Split; }


		DA_Node* a = nullptr;
		DA_Node* b = nullptr;
		virtual ~DA_SplitNode() override
		{
			if (a) delete a;
			if (b) delete b;
		}

		// In the range [0, 1]
		f32 split = 0.5f;
		enum class Direction : char { Horizontal, Vertical };
		Direction dir;

		[[nodiscard]] Std::Array<Rect, 2> BuildChildRects(Rect const& nodeRect) const;
	};

	// Returns the pointer to the node of the resize handle that we hit
	// Returns nullptr if no handle was hit
	[[nodiscard]] DA_SplitNode* DA_Layer_CheckHitResizeHandle(
		DockArea& dockArea,
		DA_Node& rootNode,
		Rect const& layerRect,
		AllocRef transientAlloc,
		Math::Vec2 pos);
	[[nodiscard]] Std::Opt<Rect> FindSplitNodeRect(
		DA_Node const& rootNode,
		Rect const& layerRect,
		DA_SplitNode const* targetNode,
		AllocRef transientAlloc);
	[[nodiscard]] Rect DA_BuildSplitNodeResizeHandleRect(
		Rect nodeRect,
		u32 thickness,
		u32 length,
		f32 splitOffset,
		DA_SplitNode::Direction dir) noexcept;

	[[nodiscard]] int FindLayerIndex(
		DockArea const& dockArea,
		DA_Node const* targetNode,
		AllocRef transientAlloc);
	[[nodiscard]] DA_Node** DA_FindParentsPtrToNode(
		DockArea& dockArea,
		DA_Node const& targetNode,
		AllocRef transientAlloc);



	[[nodiscard]] Rect DA_BuildLayerRect(
		Std::Span<DA_Layer const> layers,
		uSize layerIndex,
		Rect const& widgetRect);
	// This should not be done immediately. Do it after iteration.
	void DA_PushLayerToFront(DockArea& dockArea, uSize indexToPush);


	enum class DA_ResizeSide : char { Top, Bottom, Left, Right, COUNT };

	enum class GizmoType : char { InnerDocking, OuterDocking, Delete };
	enum class DA_InnerDockingGizmo : char { Center, Top, Bottom, Left, Right, COUNT };
	enum class DA_OuterLayoutGizmo : char { Top, Bottom, Left, Right, COUNT };
	[[nodiscard]] DA_InnerDockingGizmo DA_ToInnerLayoutGizmo(DA_OuterLayoutGizmo in) noexcept;
	[[nodiscard]] Rect DA_BuildOuterLayoutGizmoRect(
		Rect layerRect,
		DA_OuterLayoutGizmo in,
		u32 gizmoSize) noexcept;
	[[nodiscard]] Std::Opt<DA_OuterLayoutGizmo> DA_CheckHitOuterLayoutGizmo(
		Rect const& nodeRect,
		u32 gizmoSize,
		Math::Vec2 point) noexcept;

	[[nodiscard]] Rect DA_BuildDeleteGizmoRect(
		Rect layerRect,
		u32 gizmoSize);

	[[nodiscard]] bool DA_CheckHitDeleteGizmo(
		Rect layerRect,
		u32 gizmoSize,
		Math::Vec2 point) noexcept;

	[[nodiscard]] Rect DA_BuildInnerDockingGizmoRect(
		Rect const& nodeRect,
		DA_InnerDockingGizmo in,
		u32 gizmoSize) noexcept;
	[[nodiscard]] Std::Opt<DA_InnerDockingGizmo> DA_CheckHitInnerDockingGizmo(
		Rect const& nodeRect,
		u32 gizmoSize,
		Math::Vec2 const& point) noexcept;

	struct DA_Layer_CheckHitInnerDockingGizmo_ReturnT
	{
		DA_WindowNode* node;
		DA_InnerDockingGizmo gizmo;
	};
	// Returns if any docking gizmo was hit on this layer.
	[[nodiscard]] DA_Layer_CheckHitInnerDockingGizmo_ReturnT DA_Layer_CheckHitInnerDockingGizmo(
		DA_Node& rootNode,
		Rect const& layerRect,
		u32 gizmoSize,
		Math::Vec2 point,
		AllocRef transientAlloc);

	[[nodiscard]] Rect DA_BuildDockingHighlightRect(
		Rect nodeRect,
		DA_InnerDockingGizmo gizmo);
	[[nodiscard]] Rect DA_BuildDockingHighlightRect(
		Rect nodeRect,
		DA_OuterLayoutGizmo gizmo);

	// We want to dock the entire front layer into this target
	// node.
	struct DockingJob
	{
		DA_OuterLayoutGizmo outerGizmo;
		bool dockIntoOuter;
		DA_InnerDockingGizmo innerGizmo;
		// This is the node that we will be docking into.
		// There are two options, either
		// we create a split node in its stead and drop
		// the front layer into one of the halves.
		// Or we grab all the front layer tabs and drop it into the target node.
		// We can only dock on a window node.
		DA_WindowNode* targetNode;
	};
	void DockNode(
		DockArea& dockArea,
		DockingJob const& dockingJob,
		AllocRef transientAlloc);



	using DA_ChildDispatchFnT = Std::FnRef<bool(Widget&, Rect const&, Rect const&, bool)>;
	struct DA_PointerMove_Pointer
	{
		u8 id;
		Math::Vec2 pos;
	};
	struct DA_PointerMove_Params2
	{
		DockArea& dockArea;
		RectCollection const& rectCollection;
		TextManager& textManager;
		AllocRef transientAlloc;
		Rect const& widgetRect;
		Rect const& visibleRect;
		DA_PointerMove_Pointer const& pointer;
	};
	[[nodiscard]] bool DA_PointerMove(
		DA_PointerMove_Params2 const& params,
		bool pointerOccluded,
		DA_ChildDispatchFnT const& childDispatchFn);

	struct DA_PointerPress_Pointer
	{
		u8 id;
		Math::Vec2 pos;
		bool pressed;
	};
	struct DA_PointerPress_Params2
	{
		DockArea& dockArea;
		AllocRef const& transientAlloc;
		RectCollection const& rectCollection;
		TextManager& textManager;
		Rect const& widgetRect;
		Rect const& visibleRect;
		DA_PointerPress_Pointer const& pointer;
		DA_ChildDispatchFnT const& childDispatchFn;
	};
	[[nodiscard]] bool DA_PointerPress(
		DA_PointerPress_Params2 const& params,
		bool eventConsumed);
}

#include "Iterators_Impl.inl"