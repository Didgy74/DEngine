#include "Impl.hpp"

#include <DEngine/Gui/TextManager.hpp>

using namespace DEngine;
using namespace DEngine::Gui;

namespace DEngine::Gui::impl
{
	// At the time of writing, there is no decent container for a function
	// This is just a simple implementation that fits our needs and works with
	// the GUIs transient allocator.
	template<class ReturnT, class... ArgsT>
	class DA_TransientFn;
	template<class ReturnT, class... ArgsT>
	class DA_TransientFn<ReturnT(ArgsT...)>
	{
	public:
		DA_TransientFn(AllocRef allocRef) : allocRef{ allocRef } {}
		DA_TransientFn(DA_TransientFn const&) = delete;
		DA_TransientFn(DA_TransientFn&& in) noexcept :
			allocRef{ in.allocRef },
			actualFn{ in.actualFn },
			wrapperFn{ in.wrapperFn }
		{
			in.Nullify();
		}

		template<class Callable>
		void operator=(Callable const& in) {
			Clear();
			Set(in);
		}

		void operator=(DA_TransientFn const&) = delete;
		void operator=(DA_TransientFn&& in) noexcept {
			allocRef = in.allocRef;
			actualFn = in.actualFn;
			wrapperFn = in.wrapperFn;
			in.Nullify();
		}

		ReturnT operator()(ArgsT... args) const {
			return Invoke(args...);
		}

		template<class Callable>
		void Set(Callable const& in) {
			auto* newMem = (Callable*)allocRef.Alloc(sizeof(Callable), alignof(Callable));
			new(newMem) Callable(in);

			actualFn = newMem;
			wrapperFn = [](void* fnIn, ArgsT... args) {
				auto& myFn = *reinterpret_cast<Callable*>(fnIn);
				return myFn(args...);
			};
		}

		[[nodiscard]] bool IsValid() const { return actualFn != nullptr; }

		void Clear() {
			if (actualFn) {
				allocRef.Free(actualFn);
			}
			Nullify();
		}

		ReturnT Invoke(ArgsT... args) const {
			DENGINE_IMPL_GUI_ASSERT(actualFn);
			return wrapperFn(actualFn, args...);
		}

		~DA_TransientFn() {
			if (actualFn) {
				allocRef.Free(actualFn);
			}
		}

	private:
		void Nullify() {
			wrapperFn = nullptr;
			actualFn = nullptr;
		}

		using WrapperFnT = ReturnT(*)(void*, ArgsT...);
		WrapperFnT wrapperFn = nullptr;
		void* actualFn = nullptr;
		AllocRef allocRef;
	};

	// Returns true if event was consumed
	struct DA_PointerPress_Result
	{
		bool eventConsumed = false;
		Std::Opt<DockingJob> dockingJobOpt;
		Std::Opt<DockArea::Impl::StateDataT> newStateJob;
		Std::Opt<uSize> pushLayerToFrontJob;
		DA_TransientFn<void()> job;
	};

	DA_PointerPress_Result DA_PointerPress_StateNormal(
		DA_PointerPress_Params2 const& params,
		bool eventConsumed)
	{
		auto& dockArea = params.dockArea;
		auto& textManager = params.textManager;
		auto const& rectCollection = params.rectCollection;
		auto const& widgetRect = params.widgetRect;
		auto const& visibleRect = params.visibleRect;
		auto& transientAlloc = params.transientAlloc;
		auto const& pointer = params.pointer;

		auto const textheight = textManager.GetLineheight();
		auto const totalTabHeight = textheight + dockArea.tabTextMargin * 2;

		bool pointerInWidget = PointIsInAll(pointer.pos, { widgetRect, visibleRect });

		DA_PointerPress_Result returnValue = { .job = transientAlloc };
		returnValue.eventConsumed = eventConsumed;

		for (auto const layerIt : DA_BuildLayerItPair(dockArea))
		{
			auto const& layerRect = layerIt.BuildLayerRect(widgetRect);
			auto const& pointerInLayer =
				layerRect.PointIsInside(pointer.pos) &&
				pointerInWidget;

			// If the event is not consumed and we down-pressed, on a layer that is not the front-most and not the rear-most
			// then we push this layer to front. AND we have not already found a layer to push to front
			auto const pushLayerToFront =
				!returnValue.pushLayerToFrontJob.HasValue() &&
				!returnValue.eventConsumed &&
				pointer.pressed &&
				pointerInLayer &&
				layerIt.layerIndex != 0 &&
				layerIt.layerIndex != layerIt.layerCount - 1;
			if (pushLayerToFront)
				returnValue.pushLayerToFrontJob = layerIt.layerIndex;

			// First we check if we hit a resize handle.
			if (pointer.pressed && pointerInLayer && !returnValue.eventConsumed)
			{
				// Pointer to the node of the resize handle that we hit.
				auto resizeHandleHitNodePtr = DA_Layer_CheckHitResizeHandle(
					dockArea,
					layerIt.node,
					layerRect,
					transientAlloc,
					pointer.pos);
				if (resizeHandleHitNodePtr)
				{
					returnValue.eventConsumed = true;
					DA_State_ResizingSplit newStateJob = {};
					newStateJob.splitNode = resizeHandleHitNodePtr;
					newStateJob.pointerId = pointer.id;
					newStateJob.resizingBack = layerIt.layerIndex == (layerIt.layerCount - 1);
					returnValue.newStateJob = newStateJob;
				}
			}

			// Then iterate over each window.
			// See if we should now be dragging a window,
			// Remember to always dispatch the press to the child.
			for (auto const& nodeIt : DA_BuildNodeItPair(
				layerIt.node,
				layerRect,
				transientAlloc))
			{
				if (nodeIt.node.GetNodeType() != NodeType::Window)
					continue;
				auto& windowNode = static_cast<DA_WindowNode&>(nodeIt.node);
				auto [titlebarRect, contentRect] = DA_WindowNode_BuildPrimaryRects(
					nodeIt.rect,
					totalTabHeight);
				auto const pointerInsideTitlebar =
					titlebarRect.PointIsInside(pointer.pos) &&
					pointerInLayer;

				// Check if we hit any of the tabs.
				// We only want to enter the holding tab stat
				// If:
				// The event is not consumed
				// The point is inside a tabs rect
				// The pointer was pressed down
				//
				// AND:
				// We have multiple tabs per windownode,
				// OR the windownode has one tab but also has a parent node. (Always splitnode)
				bool temp =
					windowNode.tabs.size() > 1 ||
					nodeIt.hasSplitNodeParent;
				bool checkForHitTab =
					!returnValue.eventConsumed &&
					pointerInsideTitlebar &&
					pointer.pressed &&
					temp;
				if (checkForHitTab)
				{
					auto tabRects = DA_WindowNode_BuildTabRects(
						titlebarRect,
						{ windowNode.tabs.data(), windowNode.tabs.size() },
						dockArea.tabTextMargin,
						textManager,
						transientAlloc);
					auto hitTabOpt = PointIsInside(pointer.pos, tabRects.ToSpan());
					if (hitTabOpt.HasValue())
					{
						returnValue.eventConsumed = true;
						// We hit the tab, enter holding tab state and change active tab on this
						// windownode.
						returnValue.job = [&windowNode, hitTabOpt]() {
							windowNode.activeTabIndex = hitTabOpt.Value();
						};
						DA_State_HoldingTab newState = {};
						newState.windowBeingHeld = &windowNode;
						newState.pointerId = pointer.id;
						newState.tabIndex = hitTabOpt.Value();
						auto const& tabPos = tabRects[hitTabOpt.Value()].position;
						newState.pointerOffsetFromTab = pointer.pos - Math::Vec2{ (f32)tabPos.x, (f32)tabPos.y };
						returnValue.newStateJob = newState;
					}
				}

				auto goToMovingState =
					!returnValue.eventConsumed &&
					pointerInsideTitlebar &&
					pointer.pressed &&
					layerIt.layerIndex != layerIt.layerCount - 1;
				if (goToMovingState)
				{
					returnValue.eventConsumed = true;
					DA_State_Moving newState = {};
					newState.pointerId = pointer.id;
					auto layerPos = Math::Vec2{
						(f32)layerRect.position.x,
						(f32)layerRect.position.y };
					newState.pointerOffset = pointer.pos - layerPos;
					returnValue.newStateJob = newState;
				}
			}

			returnValue.eventConsumed = returnValue.eventConsumed || pointerInLayer;
		}

		return returnValue;
	}

	DA_PointerPress_Result DA_PointerPress_StateMoving(
		DA_PointerPress_Params2 const& params,
		bool eventConsumed)
	{
		auto& dockArea = params.dockArea;
		auto const& widgetRect = params.widgetRect;
		auto const& visibleRect = params.visibleRect;
		auto& transientAlloc = params.transientAlloc;
		auto const& pointer = params.pointer;

		bool pointerInsideWidget =
			widgetRect.PointIsInside(pointer.pos) &&
			visibleRect.PointIsInside(pointer.pos);

		// If we only have 0-1 layers, we shouldn't be in the moving state
		// to begin with.
		DENGINE_IMPL_GUI_ASSERT(DA_GetLayers(dockArea).size() >= 2);
		auto& stateData = DA_GetStateData(dockArea).Get<DA_State_Moving>();

		DA_PointerPress_Result returnValue = { .job = transientAlloc };
		returnValue.eventConsumed = eventConsumed;

		// Check if we hit the outer docking gizmo

		auto hitOuterGizmo =
			DA_CheckHitOuterLayoutGizmo(widgetRect, dockArea.gizmoSize, pointer.pos);
		bool undockIntoOuterGizmo =
			!returnValue.dockingJobOpt.HasValue() &&
			!pointer.pressed &&
			stateData.pointerId == pointer.id &&
			pointerInsideWidget &&
			hitOuterGizmo.Has();
		if (undockIntoOuterGizmo)
		{
			// We hit something
			DockingJob dockingJob = {};
			dockingJob.outerGizmo = hitOuterGizmo.Get();
			dockingJob.dockIntoOuter = true;
			returnValue.dockingJobOpt = dockingJob;
		}

		for (auto const& layerIt : DA_BuildLayerItPair(dockArea))
		{
			auto layerRect = layerIt.BuildLayerRect(widgetRect);
			auto const pointerInsideLayer =
				layerRect.PointIsInside(pointer.pos) &&
				pointerInsideWidget;

			// If we unpressed, then we want to exit this moving state.
			if (layerIt.layerIndex == 0 && pointer.id == stateData.pointerId && !pointer.pressed)
			{
				DA_State_Normal newState = {};
				returnValue.newStateJob = newState;
			}

			// Check if we hit any of the window docking gizmos
			bool checkForDockingGizmo =
				!returnValue.dockingJobOpt.HasValue() &&
				!pointer.pressed &&
				layerIt.layerIndex != 0 &&
				pointerInsideLayer &&
				stateData.pointerId == pointer.id;
			if (checkForDockingGizmo)
			{
				auto hitResult = DA_Layer_CheckHitInnerDockingGizmo(
					layerIt.node,
					layerRect,
					dockArea.gizmoSize,
					pointer.pos,
					transientAlloc);
				if (hitResult.node)
				{
					// We hit something
					DockingJob dockingJob = {};
					dockingJob.innerGizmo = hitResult.gizmo;
					dockingJob.dockIntoOuter = false;
					dockingJob.targetNode = hitResult.node;
					returnValue.dockingJobOpt = dockingJob;
					break;
				}
			}

			returnValue.eventConsumed = returnValue.eventConsumed || pointerInsideLayer;
		}

		return returnValue;
	}

	DA_PointerPress_Result DA_PointerPress_StateResizingSplit(
		DA_PointerPress_Params2 const& params,
		bool eventConsumed)
	{
		auto& dockArea = params.dockArea;
		auto& widgetRect = params.widgetRect;
		auto& visibleRect = params.visibleRect;
		auto& transientAlloc = params.transientAlloc;
		auto& pointer = params.pointer;

		auto& stateData = DA_GetStateData(dockArea).Get<DA_State_ResizingSplit>();

		DA_PointerPress_Result returnValue = { .job = transientAlloc };
		returnValue.eventConsumed = eventConsumed;

		if (!pointer.pressed && pointer.id == stateData.pointerId)
		{
			DA_State_Normal newStateData = {};
			returnValue.newStateJob = newStateData;
		}

		return returnValue;
	}

	DA_PointerPress_Result DA_PointerPress_State_HoldingTab(
		DA_PointerPress_Params2 const& params,
		bool eventConsumed)
	{
		auto& dockArea = params.dockArea;
		auto const& pointer = params.pointer;
		auto& transientAlloc = params.transientAlloc;

		DENGINE_IMPL_GUI_ASSERT(DA_GetStateData(dockArea).IsA<DA_State_HoldingTab>());
		auto& stateData = DA_GetStateData(dockArea).Get<DA_State_HoldingTab>();

		DA_PointerPress_Result returnValue = { .job = transientAlloc };
		returnValue.eventConsumed = eventConsumed;

		// If our pointer that is holding the tab was released, we no longer want
		// to be in holding state.
		if (pointer.id == stateData.pointerId && !pointer.pressed) {
			returnValue.newStateJob = DA_State_Normal{};
		}

		return returnValue;
	}

	bool DA_PointerPress_DispatchChild(
		DA_PointerPress_Params2 const& params,
		bool eventConsumed)
	{
		auto& dockArea = params.dockArea;
		auto const& widgetRect = params.widgetRect;
		auto const& visibleRect = params.visibleRect;
		auto const& pointer = params.pointer;
		auto& textManager = params.textManager;
		auto& transientAlloc = params.transientAlloc;
		auto& childDispatchFn = params.childDispatchFn;

		u32 textHeight = textManager.GetLineheight();
		u32 totalLineHeight = textHeight + dockArea.tabTextMargin * 2;

		bool newEventConsumed = eventConsumed;
		for (auto const layerIt : DA_BuildLayerItPair(dockArea))
		{
			auto const layerRect = layerIt.BuildLayerRect(widgetRect);
			auto const pointerInsideLayer =
				layerRect.PointIsInside(pointer.pos) &&
				visibleRect.PointIsInside(pointer.pos);

			// Consume the event if we hit any resize handles on this layer.
			newEventConsumed =
				newEventConsumed ||
				(DA_Layer_CheckHitResizeHandle(
					dockArea,
					layerIt.node,
					layerRect,
					transientAlloc,
					pointer.pos) != nullptr);

			for (auto const nodeIt : DA_BuildNodeItPair(
				layerIt.node,
				layerRect,
				transientAlloc))
			{
				if (nodeIt.node.GetNodeType() != NodeType::Window)
					continue;
				auto& windowNode = static_cast<DA_WindowNode&>(nodeIt.node);
				auto [titlebarRect, contentRect] =
					DA_WindowNode_BuildPrimaryRects(nodeIt.rect, totalLineHeight);
				bool titlebarConsumes =
					titlebarRect.PointIsInside(pointer.pos) &&
					pointerInsideLayer &&
					pointer.pressed;
				newEventConsumed = newEventConsumed || titlebarConsumes;

				auto& widget = windowNode.tabs[windowNode.activeTabIndex].widget;
				if (widget)
				{
					childDispatchFn(
						*widget,
						contentRect,
						Intersection(contentRect, visibleRect),
						newEventConsumed);
				}
			}

			newEventConsumed = newEventConsumed || (pointerInsideLayer && pointer.pressed);
		}

		return newEventConsumed;
	}
}

bool Gui::impl::DA_PointerPress(
	DA_PointerPress_Params2 const& params,
	bool eventConsumed)
{
	auto& dockArea = params.dockArea;
	auto& stateData = DA_GetStateData(dockArea);
	auto& transientAlloc = params.transientAlloc;

	auto newConsumed = eventConsumed;

	DA_PointerPress_Result temp = { .job = transientAlloc, };
	if (stateData.IsA<DA_State_Normal>())
		temp = DA_PointerPress_StateNormal(params, newConsumed);
	else if (stateData.IsA<DA_State_Moving>())
		temp = DA_PointerPress_StateMoving(params, newConsumed);
	else if (stateData.IsA<DA_State_ResizingSplit>())
		temp = DA_PointerPress_StateResizingSplit(params, newConsumed);
	else if (stateData.IsA<DA_State_HoldingTab>())
		temp = DA_PointerPress_State_HoldingTab(params, newConsumed);
	else
		DENGINE_IMPL_GUI_UNREACHABLE();

	DA_PointerPress_DispatchChild(params, newConsumed);

	newConsumed = temp.eventConsumed;

	if (temp.job.IsValid()) {
		temp.job();
	}

	if (temp.dockingJobOpt.HasValue())
	{
		DockNode(
			dockArea,
			temp.dockingJobOpt.Value(),
			transientAlloc);
	}

	if (temp.pushLayerToFrontJob.HasValue())
	{
		DA_PushLayerToFront(dockArea, temp.pushLayerToFrontJob.Value());
	}

	if (temp.newStateJob.HasValue())
	{
		auto& newState = temp.newStateJob.Value();
		stateData = Std::Move(newState);
	}

	return newConsumed;
}





namespace DEngine::Gui::impl
{
	struct DA_PointerMove_Return
	{
		bool pointerOccluded;
		Std::Opt<Math::Vec2Int> frontLayerNewPos;
		Std::Opt<DA_StateData> newStateJob;
		DA_TransientFn<void()> job;
	};

	DA_PointerMove_Return DA_PointerMove_StateNormal(
		DA_PointerMove_Params2 const& params,
		bool pointerOccluded)
	{
		auto& dockArea = params.dockArea;
		auto const& widgetRect = params.widgetRect;
		auto const& visibleRect = params.visibleRect;
		auto& textManager = params.textManager;
		auto& transientAlloc = params.transientAlloc;
		auto const& pointer = params.pointer;

		DA_PointerMove_Return returnValue = { .job = transientAlloc, };
		returnValue.pointerOccluded = pointerOccluded;

		auto const totalTabHeight = textManager.GetLineheight() + dockArea.tabTextMargin * 2;

		for (auto const layerIt : DA_BuildLayerItPair(dockArea))
		{
			auto const layerRect = layerIt.BuildLayerRect(widgetRect);

			for (auto const& nodeIt : DA_BuildNodeItPair(
				layerIt.node,
				layerRect,
				transientAlloc))
			{
				if (nodeIt.node.GetNodeType() != NodeType::Window)
					continue;

				auto& node = static_cast<DA_WindowNode&>(nodeIt.node);
				auto& activeTab = node.tabs[node.activeTabIndex];
				auto& nodeRect = nodeIt.rect;
				auto const& mainWindowNodeRects =
					DA_WindowNode_BuildPrimaryRects(nodeRect, totalTabHeight);
				auto const& titlebarRect = mainWindowNodeRects.titlebarRect;
				auto const& contentRect = mainWindowNodeRects.contentRect;

				// Check if we are inside the titlebar
				auto const pointerInsideTitlebar =
					titlebarRect.PointIsInside(pointer.pos) &&
					visibleRect.PointIsInside(pointer.pos);
				returnValue.pointerOccluded =
					returnValue.pointerOccluded || pointerInsideTitlebar;
			}

			auto const pointerInsideLayer =
				layerRect.PointIsInside(pointer.pos) &&
				visibleRect.PointIsInside(pointer.pos);
			returnValue.pointerOccluded = returnValue.pointerOccluded || pointerInsideLayer;
		}

		auto const pointerInsideWidget =
			widgetRect.PointIsInside(pointer.pos) &&
			visibleRect.PointIsInside(pointer.pos);
		returnValue.pointerOccluded = returnValue.pointerOccluded || pointerInsideWidget;

		return returnValue;
	}

	DA_PointerMove_Return DA_PointerMove_StateMoving(
		DA_PointerMove_Params2 const& params,
		bool pointerOccluded)
	{
		auto& dockArea = params.dockArea;
		auto& transientAlloc = params.transientAlloc;
		auto& textManager = params.textManager;
		auto const& widgetRect = params.widgetRect;
		auto const& visibleRect = params.visibleRect;
		auto const& pointer = params.pointer;

		auto const pointerInWidget = PointIsInAll(pointer.pos, { widgetRect, visibleRect });

		// If we only have 0-1 layers, we shouldn't be in the moving state
		// to begin with.
		DENGINE_IMPL_GUI_ASSERT(DA_GetLayers(dockArea).size() > 1);
		auto& stateData = DA_GetStateData(dockArea).Get<DA_State_Moving>();

		DA_PointerMove_Return returnValue = { .job = transientAlloc, };
		returnValue.pointerOccluded = pointerOccluded;

		// Reset stuff
		stateData.hoveredGizmoOpt = Std::nullOpt;

		// Before we do anything, check if we are hovering any of the outer docking gizmos.
		if (pointerInWidget) {
			auto hitOuterGizmoOpt = DA_CheckHitOuterLayoutGizmo(
				widgetRect,
				dockArea.gizmoSize,
				pointer.pos);
			if (hitOuterGizmoOpt.HasValue()) {
				DA_State_Moving::HoveredWindow newHoveredGizmo = {};
				newHoveredGizmo.windowNode = nullptr;
				newHoveredGizmo.gizmo = (int)hitOuterGizmoOpt.Value();
				newHoveredGizmo.gizmoIsInner = false;
				stateData.hoveredGizmoOpt = newHoveredGizmo;
			}
		}


		for (auto const& layerIt : DA_BuildLayerItPair(dockArea))
		{
			auto const& layerRect = layerIt.BuildLayerRect(widgetRect);
			auto const& pointerInsideLayer =
				layerRect.PointIsInside(pointer.pos) &&
				visibleRect.PointIsInside(pointer.pos);

			// If we are the first layer, we just want to
			// move the layer and keep iterating to layers behind
			// us to see if we hovered over a window and display it's
			// layout gizmos
			if (layerIt.layerIndex == 0 && pointer.id == stateData.pointerId)
			{
				// Move the window
				auto const temp = pointer.pos - stateData.pointerOffset;
				Math::Vec2Int a = { (i32)Math::Round(temp.x), (i32)Math::Round(temp.y) };
				a -= widgetRect.position;
				// Clamp to widget-size.
				a.x = Math::Max(a.x, 0);
				a.x = Math::Min(a.x, (i32)(widgetRect.extent.width - layerRect.extent.width));
				a.y = Math::Max(a.y, 0);
				a.y = Math::Min(a.y, (i32)(widgetRect.extent.height - layerRect.extent.height));
				returnValue.frontLayerNewPos = a;
				returnValue.pointerOccluded = true;
			}

			// We want to see which window we are hovering, but only
			// if we are:
			// Dealing with the pointer that is doing the moving.
			// We are not the front-most layer.
			// Haven't already found a window we are hovering during this iteration.
			// Our pointer is inside this particular layer.
			bool checkForHoveredWindow =
				pointer.id == stateData.pointerId &&
				layerIt.layerIndex != 0 &&
				pointerInsideLayer &&
				!stateData.hoveredGizmoOpt.HasValue();
			if (checkForHoveredWindow)
			{
				auto hitWindowNode = CheckHitWindowNode(
					layerIt.node,
					layerRect,
					pointer.pos,
					transientAlloc);
				if (hitWindowNode.window)
				{
					auto const& nodeRect = hitWindowNode.rect;
					DA_State_Moving::HoveredWindow newHoveredWindow = {};
					newHoveredWindow.windowNode = hitWindowNode.window;
					// We hit a window, now check if we hit a docking gizmo inside that window.
					auto hitGizmoOpt =
						DA_CheckHitInnerDockingGizmo(nodeRect, dockArea.gizmoSize, pointer.pos);
					if (hitGizmoOpt.HasValue())
						newHoveredWindow.gizmo = (int)hitGizmoOpt.Value();
					newHoveredWindow.gizmoIsInner = true;
					stateData.hoveredGizmoOpt = newHoveredWindow;
				}
			}

			returnValue.pointerOccluded = returnValue.pointerOccluded || pointerInsideLayer;
		}


		returnValue.pointerOccluded = returnValue.pointerOccluded || pointerInWidget;

		return returnValue;
	}

	DA_PointerMove_Return DA_PointerMove_StateResizingSplit(
		DA_PointerMove_Params2 const& params,
		bool pointerOccluded)
	{
		auto& dockArea = params.dockArea;
		auto& transientAlloc = params.transientAlloc;
		auto& textManager = params.textManager;
		auto& widgetRect = params.widgetRect;
		auto& visibleRect = params.visibleRect;
		auto& pointer = params.pointer;

		// If we only have 0-1 layers, we shouldn't be in the moving state
		// to begin with.
		auto& stateData = DA_GetStateData(dockArea).Get<DA_State_ResizingSplit>();
		auto& targetSplitNode = *static_cast<DA_SplitNode*>(stateData.splitNode);

		DA_PointerMove_Return returnValue = { .job = transientAlloc, };
		returnValue.pointerOccluded = pointerOccluded;

		{
			// Apply resize
			auto& layers = DA_GetLayers(dockArea);
			auto const layerIndex = stateData.resizingBack ? layers.size() - 1 : 0;
			auto const layerRect = DA_BuildLayerRect(
				{ layers.data(), layers.size() },
				layerIndex,
				widgetRect);

			auto nodeRectOpt = FindSplitNodeRect(
				*layers[layerIndex].root,
				layerRect,
				&targetSplitNode,
				transientAlloc);
			// It is an error if we can't find the rect for this split node.
			DENGINE_IMPL_GUI_ASSERT(nodeRectOpt.HasValue());
			auto const& nodeRect = nodeRectOpt.Value();

			// Apply new split
			auto const normalizedPointerPos = Math::Vec2{
				(pointer.pos.x - (f32)nodeRect.position.x) / (f32)nodeRect.extent.width,
				(pointer.pos.y - (f32)nodeRect.position.y) / (f32)nodeRect.extent.height };
			targetSplitNode.split =
				targetSplitNode.dir == DA_SplitNode::Direction::Horizontal ?
				normalizedPointerPos.x :
				normalizedPointerPos.y;

			targetSplitNode.split = Math:: Clamp(targetSplitNode.split, 0.1f, 0.9f);
		}

		return returnValue;
	}

	// Returns the parent of a split node
	[[nodiscard]] DA_SplitNode* DA_FindWindowNodeParent(
		DockArea& dockArea,
		DA_WindowNode* srcWindowNode,
		AllocRef transientAlloc)
	{
		for (auto const layerIt : DA_BuildLayerItPair(dockArea))
		{
			for (auto const nodeIt : DA_BuildNodeItPair(
				layerIt.ppParentPtrToNode,
				transientAlloc))
			{
				if (nodeIt.node.GetNodeType() != NodeType::Split)
					continue;
				auto& splitNode = static_cast<DA_SplitNode&>(nodeIt.node);
				if (splitNode.a == srcWindowNode || splitNode.b == srcWindowNode)
					return &splitNode;
			}
		}
		return nullptr;
	}

	void DA_Untab(
		DockArea& dockArea,
		DA_WindowNode* srcWindowNode,
		Math::Vec2Int newLayerPos,
		AllocRef transientAlloc)
	{
		auto& layers = DA_GetLayers(dockArea);

		auto* newWindowNode = new DA_WindowNode;
		// Extract the tab
		newWindowNode->tabs.push_back(Std::Move(srcWindowNode->tabs[srcWindowNode->activeTabIndex]));
		// Then erase the element that has been moved from
		srcWindowNode->EraseTab(srcWindowNode->activeTabIndex);

		DA_Layer newLayer = {};
		newLayer.root = newWindowNode;
		newLayer.rect.extent = DockArea::defaultLayerExtent;
		newLayer.rect.position = newLayerPos;
		layers.insert(layers.begin(), Std::Move(newLayer));

		if (srcWindowNode->tabs.empty())
		{
			// The windownode is now empty.
			// The only way this can happen is if we have a parent split node.
			// We want to find that parent split node, and replace it such that
			// it points directly to the windownode that remains under the splitnode.
			auto* splitNodeParent = DA_FindWindowNodeParent(dockArea, srcWindowNode, transientAlloc);
			DENGINE_IMPL_GUI_ASSERT(splitNodeParent);
			auto** ppGrandparentPtr = DA_FindParentsPtrToNode(
				dockArea,
				*splitNodeParent,
				transientAlloc);
			DENGINE_IMPL_GUI_ASSERT(ppGrandparentPtr);

			// Set our grandparents pointer to point to the window node
			// that is NOT our recently emptied one.
			if (splitNodeParent->a == srcWindowNode) {
				(*ppGrandparentPtr) = splitNodeParent->b;
				splitNodeParent->b = nullptr;
			} else {
				(*ppGrandparentPtr) = splitNodeParent->a;
				splitNodeParent->a = nullptr;
			}

			// Now we can destroy the emptied windowNode and our SplitNode
			delete splitNodeParent;
		}
	}

	DA_PointerMove_Return DA_PointerMove_State_HoldingTab(
		DA_PointerMove_Params2 const& params,
		bool pointerOccluded)
	{
		auto& dockArea = params.dockArea;
		auto const& widgetRect = params.widgetRect;
		auto const& pointer = params.pointer;
		auto& textManager = params.textManager;
		auto& transientAlloc = params.transientAlloc;

		DENGINE_IMPL_GUI_ASSERT(DA_GetStateData(dockArea).IsA<DA_State_HoldingTab>());
		auto& stateData = DA_GetStateData(dockArea).Get<DA_State_HoldingTab>();
		auto& layers = DA_GetLayers(dockArea);

		DA_PointerMove_Return returnValue = { .job = transientAlloc, };
		returnValue.pointerOccluded = pointerOccluded;

		// If this event is for a pointer that we are not currently interacting with,
		// just skip it. It will get dispatched to the children later instead.
		if (pointer.id != stateData.pointerId)
			return returnValue;

		auto& windowNode = *stateData.windowBeingHeld;

		// We want to see if we moved our pointer a small distance away from the tab itself, so we know we want
		// to untab it.

		// Start by finding the windownodes rect, so we can determine where the pointer is in relation to the
		// tab rect.
		auto const windowNodeRect = DA_FindNodeRect(
			dockArea,
			widgetRect,
			&windowNode,
			transientAlloc);

		auto textHeight = textManager.GetLineheight();
		auto totalLineheight = textHeight + dockArea.tabTextMargin * 2;

		auto [titlebarRect, contentRect] = DA_WindowNode_BuildPrimaryRects(windowNodeRect, totalLineheight);
		auto tabRects = DA_WindowNode_BuildTabRects(
			titlebarRect,
			{ windowNode.tabs.data(), windowNode.tabs.size() },
			dockArea.tabTextMargin,
			textManager,
			transientAlloc);

		// If we move up or down by N amount, we undock the tab and turn it into the front layer.
		// If this windowNode only has one tab, we need to terminate the parent split node.
		// If the windownode doesn't have a splitnode as parent, we shouldn't even holding a tab
		// to begin with.
		// When we undock this tab, we go into the moving-layer state.
		bool undockTab =
			pointer.pos.y < (f32)(titlebarRect.position.y - (i32)titlebarRect.extent.height) ||
			pointer.pos.y > (f32)(titlebarRect.position.y + (i32)titlebarRect.extent.height * 2);
		if (undockTab)
		{
			// Create a new layer that is a window node, with this tab.
			// Then enter moving state.
			returnValue.job = [=, &dockArea, &windowNode]() {
				auto const oldState = stateData;

				auto temp = pointer.pos + oldState.pointerOffsetFromTab;
				Math::Vec2Int b = { (i32)temp.x, (i32)temp.y };
				Math::Vec2Int newLayerPos = widgetRect.position + b;
				DA_Untab(dockArea, &windowNode, newLayerPos, transientAlloc);

				auto& state = DA_GetStateData(dockArea);
				DA_State_Moving newState = {};
				newState.pointerId = params.pointer.id;
				newState.pointerOffset = oldState.pointerOffsetFromTab;
				state = newState;
			};
		}




		return returnValue;
	}

	void DA_PointerMove_DispatchChild(
		DA_PointerMove_Params2 const& params,
		bool pointerOccluded,
		DA_ChildDispatchFnT const& childDispatchFn)
	{
		auto& dockArea = params.dockArea;
		auto const& widgetRect = params.widgetRect;
		auto const& visibleRect = params.visibleRect;
		auto const& pointer = params.pointer;
		auto& transientAlloc = params.transientAlloc;
		auto& textManager = params.textManager;

		auto totalTabHeight = textManager.GetLineheight() + dockArea.tabTextMargin * 2;

		bool newPointerOccluded = pointerOccluded;
		for (auto const& layerIt : DA_BuildLayerItPair(dockArea))
		{
			auto const& layerRect = layerIt.BuildLayerRect(widgetRect);
			bool const pointerInsideLayer =
				layerRect.PointIsInside(pointer.pos) &&
				visibleRect.PointIsInside(pointer.pos);

			for (auto const& nodeIt : DA_BuildNodeItPair(layerIt.ppParentPtrToNode, layerRect, transientAlloc))
			{
				if (nodeIt.node.GetNodeType() != NodeType::Window)
					continue;
				auto& windowNode = static_cast<DA_WindowNode&>(nodeIt.node);
				auto [titlebarRect, contentRect] = DA_WindowNode_BuildPrimaryRects(nodeIt.rect, totalTabHeight);
				bool const pointerInsideTitlebar =
					pointerInsideLayer &&
					titlebarRect.PointIsInside(pointer.pos);

				newPointerOccluded = newPointerOccluded || pointerInsideTitlebar;

				auto& widgetBox = windowNode.tabs[windowNode.activeTabIndex].widget;
				if (widgetBox)
				{
					childDispatchFn(
						*widgetBox,
						contentRect,
						Intersection(contentRect, visibleRect),
						newPointerOccluded);
				}
			}

			newPointerOccluded = newPointerOccluded || pointerInsideLayer;
		}
	}
}

bool Gui::impl::DA_PointerMove(
	DA_PointerMove_Params2 const& params,
	bool pointerOccluded,
	DA_ChildDispatchFnT const& childDispatchFn)
{
	auto& dockArea = params.dockArea;
	auto const& widgetRect = params.widgetRect;
	auto& transientAlloc = params.transientAlloc;
	auto& stateData = DA_GetStateData(dockArea);

	DA_PointerMove_DispatchChild(params, pointerOccluded, childDispatchFn);

	DA_PointerMove_Return temp = { .job = transientAlloc, };

	if (stateData.IsA<DA_State_Normal>())
		temp = DA_PointerMove_StateNormal(params, pointerOccluded);
	else if (stateData.IsA<DA_State_Moving>())
		temp = DA_PointerMove_StateMoving(params, pointerOccluded);
	else if (stateData.IsA<DA_State_ResizingSplit>())
		temp = DA_PointerMove_StateResizingSplit(params, pointerOccluded);
	else if (stateData.IsA<DA_State_HoldingTab>())
		temp = DA_PointerMove_State_HoldingTab(params, pointerOccluded);
	else
		DENGINE_IMPL_GUI_UNREACHABLE();



	if (temp.job.IsValid())
	{
		temp.job.Invoke();
	}

	if (temp.frontLayerNewPos.HasValue())
	{
		auto const& newFrontPos = temp.frontLayerNewPos.Value();
		auto& frontLayerRect = DA_GetLayers(dockArea).front().rect;
		frontLayerRect.position = newFrontPos;
	}

	return temp.pointerOccluded;
}