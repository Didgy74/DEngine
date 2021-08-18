#include <DEngine/Gui/DockArea.hpp>

#include <DEngine/Gui/Context.hpp>

#include "ImplData.hpp"

#include <DEngine/Std/Containers/StackVec.hpp>
#include <DEngine/Std/Utility.hpp>

// We have a shorthand for DockArea called DA in this file.

using namespace DEngine;
using namespace DEngine::Gui;
using DA = DockArea;

namespace DEngine::Gui::impl
{
	enum class DA_InnerLayoutGizmo : char { Center, Top, Bottom, Left, Right, COUNT };
	enum class DA_OuterLayoutGizmo : char { Top, Bottom, Left, Right, COUNT };
	[[nodiscard]] constexpr DA_InnerLayoutGizmo DA_ToInnerLayoutGizmo(DA_OuterLayoutGizmo in) noexcept
	{
		switch (in)
		{
			case DA_OuterLayoutGizmo::Top: return DA_InnerLayoutGizmo::Top;
			case DA_OuterLayoutGizmo::Bottom: return DA_InnerLayoutGizmo::Bottom;
			case DA_OuterLayoutGizmo::Left: return DA_InnerLayoutGizmo::Left;
			case DA_OuterLayoutGizmo::Right: return DA_InnerLayoutGizmo::Right;
			default:
				DENGINE_IMPL_UNREACHABLE();
				return {};
		}
	}
	struct DA_WindowNode;
	struct DA_WindowTab
	{
		std::string title;
		Math::Vec4 color = {};
		Std::Box<Widget> widget;
	};
}

namespace DEngine::Gui::impl
{
	enum class NodeType : u8
	{
		Window,
		Split
	};

	struct Node : public DA::NodeBase
	{
		[[nodiscard]] virtual NodeType GetNodeType() const = 0;

		virtual void Render(
			DA const* dockArea,
			Context const& ctx,
			Extent framebufferExtent,
			Rect nodeRect,
			Rect visibleRect,
			DrawInfo& drawInfo) const = 0;

		// Returns true if the requested node was found.
		[[nodiscard]] virtual bool RenderLayoutGizmo(
			Rect nodeRect,
			u32 gizmoSize,
			Math::Vec4 gizmoColor,
			Node const* hoveredWindow,
			bool ignoreCenter,
			DrawInfo& drawInfo) const = 0;

		[[nodiscard]] virtual bool RenderDockingHighlight(
			Rect nodeRect,
			Math::Vec4 highlightColor,
			Node const* hoveredWindow,
			impl::DA_InnerLayoutGizmo gizmo,
			DrawInfo& drawInfo) const = 0;

		// Returns true if successfully docked.
		[[nodiscard]] virtual bool DockNewNode(
			Node const* targetNode,
			impl::DA_InnerLayoutGizmo gizmo,
			Std::Box<Node>& insertNode) = 0;

		struct PointerPress_Params
		{
			Context* ctx = {};
			WindowID windowId = {};
			DA* dockArea = {};
			Rect nodeRect = {};
			Rect visibleRect = {};
			uSize layerIndex = {};
			Rect layerRect = {};
			bool hasSplitNodeParent = {};

			u8 pointerId = {};
			Math::Vec2 pointerPos = {};
			bool pointerPressed = {};
			bool occluded = {};

			CursorClickEvent* cursorClickEvent = nullptr;
			Gui::TouchPressEvent* touchEvent = nullptr;
		};
		// Returns true if event was consumed
		struct PointerPress_Result
		{
			bool eventConsumed = false;
			struct DockingJob
			{
				uSize layerIndex;
				impl::DA_InnerLayoutGizmo gizmo;
				impl::DA_WindowNode* targetNode;
			};
			Std::Opt<DockingJob> dockingJobOpt;
		};
		virtual PointerPress_Result PointerPress(PointerPress_Params const& in)
		{
			return {};
		}

		struct PointerMove_Params
		{
			Context* ctx = {};
			WindowID windowId = {};
			DA* dockArea = {};
			Rect visibleRect = {};
			uSize layerIndex = {};
			Rect nodeRect = {};
			Math::Vec2Int widgetPos = {};
			bool hasSplitNodeParent = {};

			u8 pointerId = {};
			Math::Vec2 pointerPos = {};
			bool pointerOccluded = {};

			CursorMoveEvent* cursorMoveEvent = nullptr;
			Gui::TouchMoveEvent* touchEvent = nullptr;
		};
		struct PointerMove_Result
		{
			bool stopIterating = false;
			struct UndockJob
			{
				impl::DA_WindowTab undockedTab;
				struct RemoveSplitNodeJob
				{
					uSize layerIndex;
					impl::DA_WindowNode const* windowNodePtr = nullptr;
				};
				Std::Opt<RemoveSplitNodeJob> removeSplitNodeJobOpt;
				DockArea::State_Moving movingState = {};
			};
			Std::Opt<UndockJob> undockJobOpt;
		};
		// Returns true when we don't want to iterate anymore.
		virtual PointerMove_Result PointerMove(PointerMove_Params const& in)
		{
			return {};
		}

		virtual void InputConnectionLost() {}

		virtual void CharEnterEvent(
			Context& ctx) {}
		virtual void CharEvent(
			Context& ctx,
			u32 utfValue) {}
		virtual void CharRemoveEvent(
			Context& ctx) {}

		virtual ~Node() {}
	};

	struct DA_WindowNode : public Node
	{
		uSize selectedTab = 0;
		std::vector<DA_WindowTab> tabs;

		virtual NodeType GetNodeType() const override { return NodeType::Window; }

		virtual void Render(
			DA const* dockArea,
			Context const& ctx,
			Extent framebufferExtent,
			Rect nodeRect,
			Rect visibleRect,
			DrawInfo& drawInfo) const override;

		virtual bool RenderLayoutGizmo(
			Rect nodeRect,
			u32 gizmoSize,
			Math::Vec4 gizmoColor,
			Node const* hoveredWindow,
			bool ignoreCenter,
			DrawInfo& drawInfo) const override;

		[[nodiscard]] virtual bool RenderDockingHighlight(
			Rect nodeRect,
			Math::Vec4 highlightColor,
			Node const* hoveredWindow,
			impl::DA_InnerLayoutGizmo gizmo,
			DrawInfo& drawInfo) const override;

		virtual bool DockNewNode(
			Node const* targetNode,
			impl::DA_InnerLayoutGizmo gizmo,
			Std::Box<Node>& newNode) override;

		[[nodiscard]] PointerPress_Result PointerPress_StateNormal(PointerPress_Params const& in);
		[[nodiscard]] PointerPress_Result PointerPress_StateMoving(PointerPress_Params const& in);
		[[nodiscard]] virtual PointerPress_Result PointerPress(PointerPress_Params const& in) override;

		[[nodiscard]] PointerMove_Result PointerMove_StateNormal(PointerMove_Params const& in);
		[[nodiscard]] PointerMove_Result PointerMove_StateHoldingTab(PointerMove_Params const& in);
		[[nodiscard]] PointerMove_Result PointerMove_StateMoving(PointerMove_Params const& in);
		[[nodiscard]] virtual PointerMove_Result PointerMove(PointerMove_Params const& in) override;
		
		virtual void InputConnectionLost() override;
		virtual void CharEnterEvent(
			Context& ctx) override;
		virtual void CharEvent(
			Context& ctx,
			u32 utfValue) override;
		virtual void CharRemoveEvent(
			Context& ctx) override;
	};

	struct DA_SplitNode : public Node
	{
		Std::Box<Node> a;
		Std::Box<Node> b;
		// In the range [0, 1]
		f32 split = 0.5f;
		enum class Direction : char { Horizontal, Vertical };
		Direction dir;

		virtual NodeType GetNodeType() const override { return NodeType::Split; }

		virtual void Render(
			DA const* dockArea,
			Context const& ctx,
			Extent framebufferExtent,
			Rect nodeRect,
			Rect visibleRect,
			DrawInfo& drawInfo) const override;

		virtual bool RenderLayoutGizmo(
			Rect nodeRect,
			u32 gizmoSize,
			Math::Vec4 gizmoColor,
			Node const* hoveredWindow,
			bool ignoreCenter,
			DrawInfo& drawInfo) const override;

		[[nodiscard]] virtual bool RenderDockingHighlight(
			Rect nodeRect,
			Math::Vec4 highlightColor,
			Node const* hoveredWindow,
			impl::DA_InnerLayoutGizmo gizmo,
			DrawInfo& drawInfo) const override;

		virtual bool DockNewNode(
			Node const* targetNode,
			impl::DA_InnerLayoutGizmo gizmo,
			Std::Box<Node>& newNode) override;

		[[nodiscard]] virtual PointerPress_Result PointerPress(PointerPress_Params const& in) override;

		[[nodiscard]] virtual PointerMove_Result PointerMove(PointerMove_Params const& in) override;

		virtual void InputConnectionLost() override;
		virtual void CharEnterEvent(
			Context& ctx) override;
		virtual void CharEvent(
			Context& ctx,
			u32 utfValue) override;
		virtual void CharRemoveEvent(
			Context& ctx) override;
	};

	static void DA_PushLayerToFront(
		DockArea& dockArea,
		uSize indexToPush)
	{
		DENGINE_IMPL_GUI_ASSERT(indexToPush < dockArea.layers.size());
		if (indexToPush == 0)
			return;

		// First remove the element and store it in a temporary
		DockArea::Layer temp = Std::Move(dockArea.layers[indexToPush]);
		dockArea.layers.erase(dockArea.layers.begin() + indexToPush);

		// Insert it at front
		dockArea.layers.insert(dockArea.layers.begin(), Std::Move(temp));
	}

	[[nodiscard]] static Rect DA_GetLayerRect(
		Std::Span<DA::Layer const> layers,
		Rect widgetRect,
		uSize layerIndex) noexcept
	{
		DENGINE_IMPL_GUI_ASSERT(layerIndex < layers.Size());

		Rect layerRect = layers[layerIndex].rect;
		layerRect.position += widgetRect.position;
		// If we're on the rear-most layer, we want it to have
		// the full widget size.
		if (layerIndex == layers.Size() - 1)
			layerRect = widgetRect;

		return layerRect;
	}

	[[nodiscard]] static Rect DA_GetLayerRect(
		std::vector<DA::Layer> const& layers,
		Rect widgetRect,
		uSize layerIndex) noexcept
	{
		return DA_GetLayerRect({ layers.data(), layers.size() }, widgetRect, layerIndex);
	}

	[[nodiscard]] static bool DA_DockNewNode(
		Std::Box<Node>& root,
		Node const* targetNode,
		impl::DA_InnerLayoutGizmo gizmo,
		Std::Box<Node>& insertNode)
	{
		using GizmoT = decltype(gizmo);

		bool returnVal = false;
		if (root == targetNode && gizmo != GizmoT::Center)
		{
			returnVal = true;

			Std::Box<Node> origNode = Std::Move(root);
			DA_SplitNode* newNode = new DA_SplitNode;
			root = Std::Box{ newNode };
			newNode->dir = gizmo == GizmoT::Top || gizmo == GizmoT::Bottom ?
				DA_SplitNode::Direction::Vertical :
				DA_SplitNode::Direction::Horizontal;
			bool temp = gizmo == GizmoT::Top || gizmo == GizmoT::Left;
			newNode->a = Std::Move(temp ? insertNode : origNode);
			newNode->b = Std::Move(!temp ? insertNode : origNode);
		}
		else
		{
			returnVal = root->DockNewNode(targetNode, gizmo, insertNode);
		}
		// If we reported success, then we check that insertNode
		// has been moved from.
		DENGINE_IMPL_GUI_ASSERT(!returnVal || !insertNode.Get());
		return returnVal;
	}

	// Returns true if successfully removed.
	[[nodiscard]] static bool DA_RemoveSplitNode(
		Std::Box<Node>& root,
		DA_WindowNode const* target)
	{
		Std::Box<Node> temp;
		auto const nodeType = root->GetNodeType();
		if (nodeType != NodeType::Split)
			return false;

		DA_SplitNode* splitNode = static_cast<DA_SplitNode*>(root.Get());

		if (splitNode->a == target)
		{
			// Move the root into temp so we don't delete it immediately.
			temp = Std::Move(root);
			root = Std::Move(splitNode->b);
			return true;
		}
		else
		{
			bool result = DA_RemoveSplitNode(
				splitNode->a,
				target);
			if (result)
				return true;
		}

		if (splitNode->b == target)
		{
			temp = Std::Move(root);
			root = Std::Move(splitNode->a);
			return true;
		}
		else
		{
			bool result = DA_RemoveSplitNode(
				splitNode->b,
				target);
			if (result)
				return true;
		}

		return false;
	}

	// DO NOT USE DIRECTLY
	template<typename T>
	struct DA_LayerIt
	{
		Std::Span<T> layers;
		Rect widgetRect{};
		uSize layerIndex = 0;
		bool reverse = false;

		// For lack of a better name...
		[[nodiscard]] uSize GetActualIndex() const noexcept { return !reverse ? layerIndex : layerIndex - 1; }

		// DO NOT USE DIRECTLY
		struct Result
		{
			using NodeT = Std::Trait::Cond<Std::Trait::isConst<T>, impl::Node const, impl::Node>;
			NodeT& rootNode;
			Rect layerRect = {};
			uSize layerIndex = 0;
		};
		[[nodiscard]] Result operator*() const noexcept
		{
			uSize actualIndex = GetActualIndex();
			
			Rect layerRect = DA_GetLayerRect(
				layers,
				widgetRect,
				actualIndex);

			auto& layer = layers[actualIndex];
			DENGINE_IMPL_GUI_ASSERT(layer.root);
			Result returnVal = {
				*static_cast<typename Result::NodeT*>(layer.root.Get()),
				layerRect,
				actualIndex };

			return returnVal;
		}

		[[nodiscard]] bool operator!=(DA_LayerIt const& other) const noexcept
		{
			DENGINE_IMPL_GUI_ASSERT(layers.Data() == other.layers.Data());
			DENGINE_IMPL_GUI_ASSERT(reverse == other.reverse);
			return layerIndex != other.layerIndex;
		}

		DA_LayerIt& operator++() noexcept
		{
			if (!reverse)
				layerIndex += 1;
			else
				layerIndex -= 1;
			return *this;
		}
	};

	// DO NOT USE DIRECTLY
	template<typename T>
	struct DA_LayerItPair
	{
		Std::Span<T> layers = {};
		Rect widgetRect{};
		uSize startIndex = 0;
		uSize endIndex = 0;
		bool reverse = false;

		[[nodiscard]] DA_LayerItPair<T> Reverse() const noexcept
		{
			DA_LayerItPair returnVal = *this;
			returnVal.reverse = !reverse;
			return returnVal;
		}

		[[nodiscard]] DA_LayerIt<T> begin() const noexcept
		{
			DA_LayerIt<T> returnVal{};
			returnVal.layers = layers;
			returnVal.widgetRect = widgetRect;
			returnVal.reverse = reverse;
			if (!reverse)
				returnVal.layerIndex = startIndex;
			else
				returnVal.layerIndex = endIndex;
			return returnVal;
		}

		[[nodiscard]] DA_LayerIt<T> end() const noexcept
		{
			DA_LayerIt<T> returnVal{};
			returnVal.layers = layers;
			returnVal.widgetRect = widgetRect;
			returnVal.reverse = reverse;
			if (!reverse)
				returnVal.layerIndex = endIndex;
			else
				returnVal.layerIndex = startIndex;
			return returnVal;
		}
	};

	[[nodiscard]] static DA_LayerItPair<DA::Layer> DA_GetLayerItPair(
		DA& dockArea,
		Rect widgetRect) noexcept
	{
		DA_LayerItPair<DA::Layer> returnVal{};
		returnVal.layers = { dockArea.layers.data(), dockArea.layers.size() };
		returnVal.widgetRect = widgetRect;
		returnVal.startIndex = 0;
		returnVal.endIndex = dockArea.layers.size();
		return returnVal;
	}

	[[nodiscard]] static DA_LayerItPair<DA::Layer const> DA_GetLayerItPair(
		DA const& dockArea, 
		Rect widgetRect) noexcept
	{
		DA_LayerItPair<DA::Layer const> returnVal{};
		returnVal.layers = { dockArea.layers.data(), dockArea.layers.size() };
		returnVal.widgetRect = widgetRect;
		returnVal.startIndex = 0;
		returnVal.endIndex = dockArea.layers.size();
		return returnVal;
	}

	/*
	*	startIndex is inclusive.
	*	endIndex is exclusive.
	*/
	[[nodiscard]] static DA_LayerItPair<DA::Layer const> DA_GetLayerItPair(
		DA const& dockArea,
		Rect widgetRect,
		uSize startIndex,
		uSize endIndex) noexcept
	{
		DENGINE_IMPL_GUI_ASSERT(startIndex <= endIndex);
		DA_LayerItPair<DA::Layer const> returnVal{};
		returnVal.layers = { dockArea.layers.data(), dockArea.layers.size() };
		returnVal.widgetRect = widgetRect;
		returnVal.startIndex = startIndex;
		returnVal.endIndex = endIndex;
		return returnVal;
	}

	template<typename T>
	struct DA_TabIt
	{
		Context const* ctx = nullptr;
		Std::Span<T> tabs;
		Math::Vec2Int widgetPos;
		u32 tabHeight;
		u32 horizontalOffset = 0;
		uSize currentIndex;

		struct Result
		{
			T& tab;
			Rect rect;
			uSize index;
		};
		Result operator*() noexcept
		{
			auto& tab = tabs[currentIndex];
			auto& implData = *static_cast<impl::ImplData*>(ctx->Internal_ImplData());
			
			SizeHint tabSizeHint = impl::TextManager::GetSizeHint(
				implData.textManager, 
				{ tab.title.data(), tab.title.size() });
			Rect tabRect = {};
			tabRect.position = widgetPos;
			tabRect.position.x += horizontalOffset;
			tabRect.extent.width = tabSizeHint.preferred.width;
			tabRect.extent.height = tabHeight;

			Result returnVal{
				tabs[currentIndex],
				tabRect,
				currentIndex};

			horizontalOffset += tabRect.extent.width;

			return returnVal;
		}

		[[nodiscard]] bool operator!=(DA_TabIt const& other) const noexcept
		{
			DENGINE_IMPL_GUI_ASSERT(tabs.Data() == other.tabs.Data());
			return currentIndex != other.currentIndex;
		}

		DA_TabIt& operator++() noexcept
		{
			currentIndex += 1;
			return *this;
		}
	};

	// DO NOT USE DIRECTLY
	template<typename T>
	struct DA_TabItPair
	{
		Context const* ctx = nullptr;
		Std::Span<T> tabs;
		Math::Vec2Int widgetPos;

		[[nodiscard]] DA_TabIt<T> begin() const noexcept
		{
			auto& implData = *static_cast<impl::ImplData*>(ctx->Internal_ImplData());

			DA_TabIt<T> returnVal = {};
			returnVal.ctx = ctx;
			returnVal.currentIndex = 0;
			returnVal.tabHeight = implData.textManager.lineheight;
			returnVal.tabs = tabs;
			returnVal.widgetPos = widgetPos;
			return returnVal;
		}

		[[nodiscard]] DA_TabIt<T> end() const noexcept
		{
			auto& implData = *static_cast<impl::ImplData*>(ctx->Internal_ImplData());

			DA_TabIt<T> returnVal = {};
			returnVal.ctx = ctx;
			returnVal.currentIndex = tabs.Size();
			returnVal.tabHeight = implData.textManager.lineheight;
			returnVal.tabs = tabs;
			returnVal.widgetPos = widgetPos;
			return returnVal;
		}
	};

	[[nodiscard]] static DA_TabItPair<DA_WindowTab> DA_GetTabItPair(
		Context const& ctx,
		Std::Span<DA_WindowTab> tabs,
		Math::Vec2Int widgetPos) noexcept
	{
		DA_TabItPair<DA_WindowTab> returnVal = {};
		returnVal.ctx = &ctx;
		returnVal.tabs = tabs;
		returnVal.widgetPos = widgetPos;
		return returnVal;
	}

	[[nodiscard]] static DA_TabItPair<DA_WindowTab const> DA_GetTabItPair(
		Context const& ctx,
		Std::Span<DA_WindowTab const> tabs,
		Math::Vec2Int widgetPos) noexcept
	{
		DA_TabItPair<DA_WindowTab const> returnVal = {};
		returnVal.ctx = &ctx;
		returnVal.tabs = tabs;
		returnVal.widgetPos = widgetPos;
		return returnVal;
	}

	[[nodiscard]] static DA_TabItPair<DA_WindowTab const> DA_GetTabItPair(
		Context const& ctx,
		std::vector<DA_WindowTab> const& tabs,
		Math::Vec2Int widgetPos) noexcept 
	{
		return DA_GetTabItPair(
			ctx, 
			{ tabs.data(), tabs.size() }, 
			widgetPos);
	}

	struct DA_TabHit
	{
		uSize index;
		Rect rect;
	};
	[[nodiscard]] Std::Opt<DA_TabHit> DA_CheckHitTab(
		Context const& ctx,
		Std::Span<DA_WindowTab const> tabs,
		Math::Vec2Int widgetPos,
		Math::Vec2 point)
	{
		Std::Opt<DA_TabHit> returnVal = {};
		auto const& tabItPair = DA_GetTabItPair(ctx, tabs, widgetPos);
		for (auto const& tab : tabItPair)
		{
			if (tab.rect.PointIsInside(point))
			{
				DA_TabHit hit = {};
				hit.index = tab.index;
				hit.rect = tab.rect;
				returnVal = hit;
				break;
			}
		}
		return returnVal;
	}

	[[nodiscard]] Std::Opt<DA_TabHit> DA_CheckHitTab(
		Context const& ctx,
		std::vector<DA_WindowTab> const& tabs,
		Math::Vec2Int widgetPos,
		Math::Vec2 point)
	{
		return DA_CheckHitTab(
			ctx,
			{ tabs.data(), tabs.size() },
			widgetPos,
			point);
	}

	enum class DA_ResizeSide : char { Top, Bottom, Left, Right, COUNT };
	[[nodiscard]] Rect DA_GetResizeHandleRect(
		Rect layerRect,
		DA_ResizeSide in,
		u32 handleThickness,
		u32 handleLength) noexcept
	{
		Rect returnVal = layerRect;
		switch (in)
		{
			case DA_ResizeSide::Top:
			{
				returnVal.position.x += layerRect.extent.width / 2 - handleLength / 2;
				returnVal.position.y -= handleThickness / 2;
				returnVal.extent = { handleLength, handleThickness };
			}
			break;
			case DA_ResizeSide::Bottom:
			{
				returnVal.position.x += layerRect.extent.width / 2 - handleLength / 2;
				returnVal.position.y += layerRect.extent.height - handleThickness / 2;
				returnVal.extent = { handleLength, handleThickness };
			}
			break;
			case DA_ResizeSide::Left:
			{
				returnVal.position.x -= handleThickness / 2;
				returnVal.position.y += layerRect.extent.height / 2 - handleLength / 2;
				returnVal.extent = { handleThickness, handleLength };
			}
			break;
			case DA_ResizeSide::Right:
			{
				returnVal.position.x += layerRect.extent.width - handleThickness / 2;
				returnVal.position.y += layerRect.extent.height / 2 - handleLength / 2;
				returnVal.extent = { handleThickness, handleLength };
			}
			break;
			default:
				DENGINE_IMPL_UNREACHABLE();
				break;
		}
		return returnVal;
	}

	[[nodiscard]] static Std::Array<Rect, 2> DA_GetSplitNodeChildRects(
		Rect nodeRect,
		f32 splitOffset,
		DA_SplitNode::Direction dir) noexcept
	{
		bool const isHoriz = dir == DA_SplitNode::Direction::Horizontal;
		u32 const nodeExtent = isHoriz ? nodeRect.extent.width : nodeRect.extent.height;

		Std::Array<Rect, 2> returnVal = {};

		Rect childRect = nodeRect;

		u32& childExtent = isHoriz ? childRect.extent.width : childRect.extent.height;
		i32& childPos = isHoriz ? childRect.position.x : childRect.position.y;

		childExtent = u32(nodeExtent * splitOffset);
		returnVal[0] = childRect;

		childPos += childExtent;
		childExtent = nodeExtent - childExtent;
		returnVal[1] = childRect;

		return returnVal;
	}

	[[nodiscard]] static Rect DA_GetSplitNodeResizeHandleRect(
		Rect nodeRect,
		u32 thickness,
		u32 length,
		f32 splitOffset,
		DA_SplitNode::Direction dir) noexcept
	{
		bool const isHoriz = dir == DA_SplitNode::Direction::Horizontal;
		
		Rect returnVal = nodeRect;
		returnVal.extent.width = isHoriz ? thickness : length;
		returnVal.extent.height = isHoriz ? length : thickness;

		if (isHoriz)
		{
			returnVal.position.x += u32(nodeRect.extent.width * splitOffset);
			returnVal.position.x -= returnVal.extent.width / 2;
			returnVal.position.y += nodeRect.extent.height / 2;
			returnVal.position.y -= returnVal.extent.height / 2;
		}
		else
		{
			returnVal.position.x += nodeRect.extent.width / 2;
			returnVal.position.x -= returnVal.extent.width / 2;
			returnVal.position.y += u32(nodeRect.extent.height * splitOffset);
			returnVal.position.y -= returnVal.extent.height / 2;
		}

		return returnVal;
	}


	[[nodiscard]] static Rect DA_GetOuterLayoutGizmoRect(
		Rect layerRect,
		DA_OuterLayoutGizmo in,
		u32 gizmoSize) noexcept
	{
		Rect returnVal = {};
		returnVal.position = layerRect.position;
		returnVal.extent = { gizmoSize, gizmoSize };
		switch (in)
		{
			case DA_OuterLayoutGizmo::Top:
				returnVal.position.x += layerRect.extent.width / 2 - gizmoSize / 2;
				break;
			case DA_OuterLayoutGizmo::Bottom:
				returnVal.position.x += layerRect.extent.width / 2 - gizmoSize / 2;
				returnVal.position.y += layerRect.extent.height - gizmoSize;
				break;
			case DA_OuterLayoutGizmo::Left:
				returnVal.position.y += layerRect.extent.height / 2 - gizmoSize / 2;
				break;
			case DA_OuterLayoutGizmo::Right:
				returnVal.position.x += layerRect.extent.width - gizmoSize;
				returnVal.position.y += layerRect.extent.height / 2 - gizmoSize / 2;
				break;
			default:
				DENGINE_IMPL_UNREACHABLE();
				break;
		}
		return returnVal;
	}

	[[nodiscard]] static Std::Opt<DA_OuterLayoutGizmo> DA_CheckHitOuterLayoutGizmo(
		Rect nodeRect,
		u32 gizmoSize,
		Math::Vec2 point) noexcept
	{
		Std::Opt<DA_OuterLayoutGizmo> gizmoHit;
		using GizmoT = DA_OuterLayoutGizmo;
		for (GizmoT gizmo = {}; (int)gizmo < (int)GizmoT::COUNT; gizmo = GizmoT((int)gizmo + 1))
		{
			Rect gizmoRect = DA_GetOuterLayoutGizmoRect(nodeRect, gizmo, gizmoSize);
			if (gizmoRect.PointIsInside(point))
			{
				gizmoHit = gizmo;
				break;
			}
		}
		return gizmoHit;
	}

	[[nodiscard]] static Rect DA_GetDeleteGizmoRect(
		Rect layerRect,
		u32 gizmoSize)
	{
		Rect returnVal = layerRect;
		returnVal.extent = { gizmoSize, gizmoSize };
		returnVal.position.x += layerRect.extent.width / 2 - gizmoSize / 2;
		returnVal.position.y += layerRect.extent.height - gizmoSize * 2;
		return returnVal;
	}

	[[nodiscard]] static bool DA_CheckHitDeleteGizmo(
		Rect layerRect,
		u32 gizmoSize,
		Math::Vec2 point) noexcept
	{
		return DA_GetDeleteGizmoRect(layerRect, gizmoSize).PointIsInside(point);
	}

	[[nodiscard]] static Rect DA_GetInnerLayoutGizmoRect(
		Rect nodeRect,
		DA_InnerLayoutGizmo in,
		u32 gizmoSize) noexcept
	{
		// We initialize it with the top-left corner of the center gizmo
		Rect returnVal = {};
		returnVal.position = nodeRect.position;
		returnVal.position.x += nodeRect.extent.width / 2 - gizmoSize / 2;
		returnVal.position.y += nodeRect.extent.height / 2 - gizmoSize / 2;
		returnVal.extent = { gizmoSize, gizmoSize };
		switch (in)
		{
		case DA_InnerLayoutGizmo::Center:
			// Do nothing
			break;
		case DA_InnerLayoutGizmo::Top:
			returnVal.position.y -= gizmoSize;
			break;
		case DA_InnerLayoutGizmo::Bottom:
			returnVal.position.y += gizmoSize;
			break;
		case DA_InnerLayoutGizmo::Left:
			returnVal.position.x -= gizmoSize;
			break;
		case DA_InnerLayoutGizmo::Right:
			returnVal.position.x += gizmoSize;
			break;
		default:
			DENGINE_IMPL_UNREACHABLE();
			break;
		}
		return returnVal;
	}

	[[nodiscard]] static Std::Opt<DA_InnerLayoutGizmo> DA_CheckHitInnerLayoutGizmo(
		Rect nodeRect,
		u32 gizmoSize,
		bool ignoreCenter,
		Math::Vec2 point) noexcept
	{
		Std::Opt<DA_InnerLayoutGizmo> gizmoHit;
		using GizmoT = DA_InnerLayoutGizmo;
		for (GizmoT gizmo = {}; (int)gizmo < (int)GizmoT::COUNT; gizmo = GizmoT((int)gizmo + 1))
		{
			if (ignoreCenter && gizmo == GizmoT::Center)
				continue;
			Rect gizmoRect = DA_GetInnerLayoutGizmoRect(nodeRect, gizmo, gizmoSize);
			if (gizmoRect.PointIsInside(point))
			{
				gizmoHit = gizmo;
				break;
			}
		}
		return gizmoHit;
	}

	[[nodiscard]] static Rect DA_GetDockingHighlightRect(
		Rect nodeRect,
		DA_InnerLayoutGizmo gizmo)
	{
		Rect returnVal = nodeRect;
		switch (gizmo)
		{
			case DA_InnerLayoutGizmo::Top:
				returnVal.extent.height = nodeRect.extent.height / 2;
				break;
			case DA_InnerLayoutGizmo::Left:
				returnVal.extent.width = nodeRect.extent.width / 2;
				break;
			case DA_InnerLayoutGizmo::Bottom:
				returnVal.extent.height = nodeRect.extent.height / 2;
				returnVal.position.y += nodeRect.extent.height / 2;
				break;
			case DA_InnerLayoutGizmo::Right:
				returnVal.extent.width = nodeRect.extent.width / 2;
				returnVal.position.x += nodeRect.extent.width / 2;
				break;
			case DA_InnerLayoutGizmo::Center:
				break;
			default:
				DENGINE_IMPL_UNREACHABLE();
				break;
		}
		return returnVal;
	}

	static constexpr auto cursorPointerId = static_cast<u8>(-1);
	struct DA_PointerPress_Params
	{
		Context* ctx = {};
		WindowID windowId = {};
		DA* dockArea = {};
		Rect widgetRect = {};
		Rect visibleRect = {};

		u8 pointerId = {};
		Math::Vec2 pointerPos = {};
		bool pointerPressed = {};

		CursorClickEvent* cursorClickEvent = {};
		Gui::TouchPressEvent* touchEvent = {};
	};
	static bool DA_PointerPress(DA_PointerPress_Params const& in);

	struct DA_PointerMove_Params
	{
		Context* ctx = {};
		WindowID windowId = {};
		DA* dockArea = {};
		Rect widgetRect = {};
		Rect visibleRect = {};
		
		u8 pointerId = {};
		Math::Vec2 pointerPos = {};
		bool pointerOccluded = {};
		
		CursorMoveEvent* cursorMoveEvent = {};
		Gui::TouchMoveEvent* touchEvent = {};
	};
	static bool DA_PointerMove(DA_PointerMove_Params const& in);
}

bool impl::DA_WindowNode::DockNewNode(
	Node const* targetNode,
	impl::DA_InnerLayoutGizmo gizmo,
	Std::Box<Node>& insertNode)
{
	if (targetNode != this)
		return false;
	DENGINE_DETAIL_ASSERT(gizmo == impl::DA_InnerLayoutGizmo::Center);
	DENGINE_DETAIL_ASSERT(insertNode->GetNodeType() == NodeType::Window);

	Std::Box<Node> oldBox = Std::Move(insertNode);

	DA_WindowNode* oldNode = static_cast<DA_WindowNode*>(oldBox.Get());

	selectedTab = tabs.size() + oldNode->selectedTab;

	tabs.reserve(tabs.size() + oldNode->tabs.size());
	for (auto& oldTab : oldNode->tabs)
	{
		tabs.push_back(Std::Move(oldTab));
	}

	return true;
}

bool impl::DA_SplitNode::DockNewNode(
	Node const* targetNode,
	impl::DA_InnerLayoutGizmo gizmo,
	Std::Box<Node>& insertNode)
{
	DENGINE_IMPL_GUI_ASSERT(a && b);
	bool result = DA_DockNewNode(a, targetNode, gizmo, insertNode);
	if (result)
		return true;

	result = DA_DockNewNode(b, targetNode, gizmo, insertNode);
	if (result)
		return true;

	return false;
}

void impl::DA_WindowNode::Render(
	DA const* dockArea,
	Context const& ctx,
	Extent framebufferExtent,
	Rect nodeRect,
	Rect visibleRect,
	DrawInfo& drawInfo) const
{
	DENGINE_IMPL_GUI_ASSERT(!tabs.empty());
	auto& activeTab = tabs[selectedTab];

	auto& implData = *static_cast<impl::ImplData*>(ctx.Internal_ImplData());

	Rect titlebarRect = nodeRect;
	titlebarRect.extent.height = implData.textManager.lineheight;
	// Render the titlebar background
	drawInfo.PushFilledQuad(titlebarRect, activeTab.color);

	auto const tabItPair = DA_GetTabItPair(
		ctx,
		{ tabs.data(), tabs.size() },
		nodeRect.position);
	for (auto const& tabItem : tabItPair)
	{
		Math::Vec4 tabColor = tabItem.tab.color;
		if (tabItem.index != selectedTab)
		{
			tabColor *= 0.75f;
			tabColor.w = 1.f;
		}
		drawInfo.PushFilledQuad(tabItem.rect, tabColor);

		if (tabItem.index == selectedTab)
		{
			drawInfo.PushFilledQuad(tabItem.rect, { 1.f, 1.f, 1.f, 0.1f });
		}

		Math::Vec4 textColor = { 1.f, 1.f, 1.f, 1.f };
		if (tabItem.index != selectedTab)
		{
			textColor *= 0.75f;
			textColor.w = 1.f;
		}

		TextManager::RenderText(
			implData.textManager,
			{ tabItem.tab.title.data(), tabItem.tab.title.size() },
			textColor,
			tabItem.rect,
			drawInfo);
	}

	Rect contentRect = nodeRect;
	contentRect.position.y += titlebarRect.extent.height;
	contentRect.extent.height -= titlebarRect.extent.height;

	Math::Vec4 contentBackgroundColor = activeTab.color;
	contentBackgroundColor *= 0.5f;
	drawInfo.PushFilledQuad(contentRect, contentBackgroundColor);

	if (activeTab.widget)
	{
		auto scissor = DrawInfo::ScopedScissor(drawInfo, contentRect);
		activeTab.widget->Render(
			ctx,
			framebufferExtent,
			contentRect,
			Rect::Intersection(contentRect, visibleRect),
			drawInfo);
	}
}

void impl::DA_SplitNode::Render(
	DA const* dockArea,
	Context const& ctx,
	Extent framebufferExtent,
	Rect nodeRect,
	Rect visibleRect,
	DrawInfo& drawInfo) const
{
	DENGINE_IMPL_GUI_ASSERT(a);
	DENGINE_IMPL_GUI_ASSERT(b);

	auto childRects = DA_GetSplitNodeChildRects(nodeRect, split, dir);

	a->Render(
		dockArea,
		ctx,
		framebufferExtent,
		childRects[0],
		visibleRect,
		drawInfo);

	b->Render(
		dockArea,
		ctx,
		framebufferExtent,
		childRects[1],
		visibleRect,
		drawInfo);

	auto handleRect = DA_GetSplitNodeResizeHandleRect(
		nodeRect,
		dockArea->resizeHandleThickness,
		dockArea->resizeHandleLength, 
		split,
		dir);
	drawInfo.PushFilledQuad(handleRect, dockArea->resizeHandleColor);
}

bool impl::DA_WindowNode::RenderDockingHighlight(
	Rect nodeRect,
	Math::Vec4 highlightColor,
	Node const* hoveredWindow,
	DA_InnerLayoutGizmo gizmo,
	DrawInfo& drawInfo) const
{
	if (this != hoveredWindow)
		return false;

	Rect highlightRect = DA_GetDockingHighlightRect(nodeRect, gizmo);
	drawInfo.PushFilledQuad(highlightRect, highlightColor);

	return true;
}

bool impl::DA_SplitNode::RenderDockingHighlight(
	Rect nodeRect,
	Math::Vec4 highlightColor,
	Node const* hoveredWindow,
	DA_InnerLayoutGizmo gizmo,
	DrawInfo& drawInfo) const
{
	DENGINE_IMPL_GUI_ASSERT(a);
	DENGINE_IMPL_GUI_ASSERT(b);

	auto childRects = DA_GetSplitNodeChildRects(nodeRect, split, dir);

	bool windowFound = a->RenderDockingHighlight(
		childRects[0],
		highlightColor,
		hoveredWindow,
		gizmo,
		drawInfo);
	if (windowFound)
		return true;

	windowFound = b->RenderDockingHighlight(
		childRects[1],
		highlightColor,
		hoveredWindow,
		gizmo,
		drawInfo);
	return windowFound;

	return true;
}

bool impl::DA_WindowNode::RenderLayoutGizmo(
	Rect nodeRect,
	u32 gizmoSize,
	Math::Vec4 gizmoColor,
	Node const* hoveredWindow,
	bool ignoreCenter,
	DrawInfo& drawInfo) const
{
	if (this != hoveredWindow)
		return false;

	using GizmoT = DA_InnerLayoutGizmo;
	for (GizmoT gizmo = {}; (int)gizmo < (int)GizmoT::COUNT; gizmo = GizmoT((int)gizmo + 1))
	{
		if (ignoreCenter && gizmo == GizmoT::Center)
			continue;

		Rect gizmoRect = DA_GetInnerLayoutGizmoRect(nodeRect, gizmo, gizmoSize);
		drawInfo.PushFilledQuad(gizmoRect, gizmoColor);
	}
	return true;
}

bool impl::DA_SplitNode::RenderLayoutGizmo(
	Rect nodeRect,
	u32 gizmoSize,
	Math::Vec4 gizmoColor,
	Node const* hoveredWindow,
	bool ignoreCenter,
	DrawInfo& drawInfo) const
{
	DENGINE_IMPL_GUI_ASSERT(a);
	DENGINE_IMPL_GUI_ASSERT(b);

	auto childRects = DA_GetSplitNodeChildRects(nodeRect, split, dir);

	bool windowFound = a->RenderLayoutGizmo(
		childRects[0], 
		gizmoSize, 
		gizmoColor,
		hoveredWindow,
		ignoreCenter,
		drawInfo);
	if (windowFound)
		return true;

	windowFound = b->RenderLayoutGizmo(
		childRects[1], 
		gizmoSize, 
		gizmoColor, 
		hoveredWindow, 
		ignoreCenter,
		drawInfo);
	return windowFound;
}

impl::Node::PointerPress_Result impl::DA_WindowNode::PointerPress_StateNormal(PointerPress_Params const& in)
{
	PointerPress_Result returnVal = {};

	if (!in.nodeRect.PointIsInside(in.pointerPos) && in.pointerPressed)
	{
		returnVal.eventConsumed = false;
		return returnVal;
	}
	
	// If we down-pressed, on a layer that is not the front-most and not the rear-most
	// then we push this layer to front.
	if (in.pointerPressed && in.layerIndex != 0 && in.layerIndex != in.dockArea->layers.size() - 1)
		DA_PushLayerToFront(*in.dockArea, in.layerIndex);

	auto& implData = *static_cast<impl::ImplData*>(in.ctx->Internal_ImplData());

	Rect titlebarRect = in.nodeRect;
	titlebarRect.extent.height = implData.textManager.lineheight;
	
	// Handle titlebar related behavior.
	if (titlebarRect.PointIsInside(in.pointerPos) && in.pointerPressed)
	{
		// Since we hit the titlebar, we want to consume the event.
		returnVal.eventConsumed = true;

		// If we only have one tab, we just want to go into moving-state even if we hit a tab.
		Std::Opt<DA_TabHit> tabHit = DA_CheckHitTab(*in.ctx, tabs, titlebarRect.position, in.pointerPos);
		if (tabHit.HasValue() && (tabs.size() > 1 || in.hasSplitNodeParent))
		{
			selectedTab = tabHit.Value().index;

			// Now we want to transition into the "HoldingTab" state
			DA::State_HoldingTab newBehavior = {};
			newBehavior.windowBeingHeld = this;
			newBehavior.pointerId = in.pointerId;
			Math::Vec2 tabPos = { (f32)tabHit.Value().rect.position.x, (f32)tabHit.Value().rect.position.y };
			newBehavior.pointerOffset = tabPos - in.pointerPos;
			in.dockArea->stateData = newBehavior;
		}
		else
		{
			// If we're on the back-most layer, we don't want to do anything?
			// If we're not, we want to go into moving mode
			if (in.layerIndex != in.dockArea->layers.size() - 1)
			{
				DA::State_Moving newBehavior{};
				newBehavior.movingSplitNode = in.hasSplitNodeParent;
				newBehavior.pointerId = in.pointerId;
				Math::Vec2 temp = Math::Vec2{ (f32)in.layerRect.position.x, (f32)in.layerRect.position.y } - in.pointerPos;
				newBehavior.pointerOffset = temp;
				in.dockArea->stateData = newBehavior;
			}
		}
	}

	// Handle content-area events.
	Rect contentRect = in.nodeRect;
	contentRect.position.y += titlebarRect.extent.height;
	contentRect.extent.height = in.nodeRect.extent.height - titlebarRect.extent.height;
	bool dispatchEvent = 
		!in.pointerPressed || 
		(in.pointerPressed && !returnVal.eventConsumed && contentRect.PointIsInside(in.pointerPos));
	auto& tab = tabs[selectedTab];
	if (tab.widget && dispatchEvent)
	{
		if (in.pointerId == cursorPointerId)
		{
			tab.widget->CursorPress(
				*in.ctx,
				in.windowId,
				contentRect,
				Rect::Intersection(contentRect, in.visibleRect),
				{ (i32)in.pointerPos.x, (i32)in.pointerPos.y },
				*in.cursorClickEvent);
		}
		else
		{
			tab.widget->TouchPressEvent(
				*in.ctx,
				in.windowId,
				contentRect,
				Rect::Intersection(contentRect, in.visibleRect),
				*in.touchEvent);
		}
		returnVal.eventConsumed = true;
		return returnVal;
	}

	// We know we hit inside the layer somewhere,
	// so we want to consume the event.
	returnVal.eventConsumed = true;
	return returnVal;
}

impl::Node::PointerPress_Result impl::DA_WindowNode::PointerPress_StateMoving(PointerPress_Params const& in)
{
	PointerPress_Result returnVal = {};

	auto& behaviorData = in.dockArea->stateData.Get<DA::State_Moving>();
	if (in.pointerId != behaviorData.pointerId)
		return returnVal; // We don't want to do anything when we're in moving-state and it's not the active pointerID.

	// We want to iterate until we find out if we want to dock anywhere.
	if (!in.pointerPressed && in.layerIndex > 0)
	{
		// Check if we hit any of this window's layout gizmos.
		using GizmoT = DA_InnerLayoutGizmo;
		Std::Opt<GizmoT> gizmoHit = DA_CheckHitInnerLayoutGizmo(
			in.nodeRect,
			in.dockArea->gizmoSize,
			behaviorData.movingSplitNode, // Ignore center gizmo
			in.pointerPos);
		if (gizmoHit.HasValue())
		{
			returnVal.eventConsumed = true;
			PointerPress_Result::DockingJob dockJob{};
			dockJob.layerIndex = in.layerIndex;
			dockJob.gizmo = gizmoHit.Value();
			dockJob.targetNode = this;
			returnVal.dockingJobOpt = dockJob;
		}
	}

	return returnVal;
}

impl::Node::PointerPress_Result impl::DA_WindowNode::PointerPress(PointerPress_Params const& in)
{
	PointerPress_Result returnVal{};

	bool cursorInside = in.nodeRect.PointIsInside(in.pointerPos) && in.visibleRect.PointIsInside(in.pointerPos);
	if (!cursorInside && in.pointerPressed)
		return returnVal;

	if (in.dockArea->stateData.IsA<DA::State_Normal>())
	{
		returnVal = PointerPress_StateNormal(in);
	}
	else if (in.dockArea->stateData.IsA<DA::State_Moving>())
	{
		returnVal = PointerPress_StateMoving(in);
	}

	return returnVal;
}

impl::Node::PointerPress_Result impl::DA_SplitNode::PointerPress(PointerPress_Params const& in)
{
	DENGINE_IMPL_GUI_ASSERT(a);
	DENGINE_IMPL_GUI_ASSERT(b);

	if (in.dockArea->stateData.IsA<DA::State_Normal>())
	{
		auto resizeHandleRect = DA_GetSplitNodeResizeHandleRect(
			in.nodeRect,
			in.dockArea->resizeHandleThickness,
			in.dockArea->resizeHandleLength,
			split,
			dir);
		if (resizeHandleRect.PointIsInside(in.pointerPos) && in.pointerPressed)
		{
			DA::State_ResizingSplitNode newState = {};
			newState.layerIndex = in.layerIndex;
			newState.pointerId = in.pointerId;
			newState.splitNode = this;
			in.dockArea->stateData = newState;

			PointerPress_Result result = {};
			result.eventConsumed = true;
			return result;
		}
	}

	auto childRects = DA_GetSplitNodeChildRects(in.nodeRect, split, dir);

	PointerPress_Params params = in;
	params.hasSplitNodeParent = true;
	params.nodeRect = childRects[0];
	PointerPress_Result resultA = a->PointerPress(params);
	if (resultA.eventConsumed && in.pointerPressed)
		return resultA;

	params.nodeRect = childRects[1];
	PointerPress_Result resultB = b->PointerPress(params);

	// We can't have 2 docking jobs simultaneously.
	DENGINE_IMPL_GUI_ASSERT(!(resultA.dockingJobOpt.HasValue() && resultB.dockingJobOpt.HasValue()));
	PointerPress_Result result = {};
	result.eventConsumed = resultA.eventConsumed || resultB.eventConsumed;
	if (resultA.dockingJobOpt.HasValue())
		result.dockingJobOpt = resultA.dockingJobOpt;
	else if (resultB.dockingJobOpt.HasValue())
		result.dockingJobOpt = resultB.dockingJobOpt;

	return result;
}

bool impl::DA_PointerPress(DA_PointerPress_Params const& in)
{
	bool pointerInside = in.widgetRect.PointIsInside(in.pointerPos) && in.visibleRect.PointIsInside(in.pointerPos);
	if (!pointerInside && in.pointerPressed)
		return false;

	bool eventConsumed = false;
	
	bool runBackLayerDockingTest = !eventConsumed;
	// We only want to check for background docking if we unpressed.
	runBackLayerDockingTest = runBackLayerDockingTest && !in.pointerPressed;
	// We only want to check for background docking if we are in the moving state
	// and the pointerId that is moving the window is the same as the pointerId inputted.
	if (auto ptr = in.dockArea->stateData.ToPtr<DA::State_Moving>())
		runBackLayerDockingTest = runBackLayerDockingTest && ptr->pointerId == in.pointerId;
	else
		runBackLayerDockingTest = false;

	if (runBackLayerDockingTest)
	{
		DENGINE_IMPL_GUI_ASSERT(in.dockArea->layers.size() >= 2);

		// Get rect of rear-most layer.
		Rect backLayerRect = DA_GetLayerRect(
			{ in.dockArea->layers.data(), in.dockArea->layers.size() },
			in.widgetRect,
			in.dockArea->layers.size() - 1);

		// This checks if we hit the gizmo that deletes the layer currently being moved.
		bool deleteGizmoHit = DA_CheckHitDeleteGizmo(backLayerRect, in.dockArea->gizmoSize, in.pointerPos);
		if (!eventConsumed && deleteGizmoHit)
		{
			in.dockArea->stateData = DA::State_Normal{};
			eventConsumed = true;
			in.dockArea->layers.erase(in.dockArea->layers.begin());
		}

		auto gizmoHit = DA_CheckHitOuterLayoutGizmo(backLayerRect, in.dockArea->gizmoSize, in.pointerPos);
		if (!eventConsumed && gizmoHit.HasValue())
		{
			in.dockArea->stateData = DA::State_Normal{};
			eventConsumed = true;

			auto const tempPtr = in.dockArea->layers.front().root.Release();
			Std::Box<Node> origFrontNode = Std::Box{ static_cast<Node*>(tempPtr) };
			in.dockArea->layers.erase(in.dockArea->layers.begin());
			auto& backLayer = in.dockArea->layers.back();

			// The function takes a Box<Node>, but we store a Box<NodeBase> for implementation hiding.
			// So we need to move it into a temp Box and move it back when we're done.
			Std::Box<Node> temp = Std::Box{ static_cast<Node*>(backLayer.root.Release()) };
			[[maybe_unused]] bool success = DA_DockNewNode(
				temp,
				temp.Get(),
				DA_ToInnerLayoutGizmo(gizmoHit.Value()),
				origFrontNode);
			DENGINE_IMPL_GUI_ASSERT(success);
			backLayer.root = Std::Move(temp);
		}
	}

	if (!eventConsumed)
	{
		// Iterate over each layer and dispatch the event to each one.
		impl::Node::PointerPress_Result pPressResult = {};
		auto const layerItPair = impl::DA_GetLayerItPair(*in.dockArea, in.widgetRect);
		for (auto const& layerItem : layerItPair)
		{
			impl::Node::PointerPress_Params params = {};
			params.ctx = in.ctx;
			params.cursorClickEvent = in.cursorClickEvent;
			params.dockArea = in.dockArea;
			params.layerIndex = layerItem.layerIndex;
			params.layerRect = layerItem.layerRect;
			params.nodeRect = layerItem.layerRect;
			params.pointerId = in.pointerId;
			params.pointerPos = in.pointerPos;
			params.pointerPressed = in.pointerPressed;
			params.touchEvent = in.touchEvent;
			params.visibleRect = in.visibleRect;
			params.windowId = in.windowId;

			pPressResult = layerItem.rootNode.PointerPress(params);
			if (pPressResult.eventConsumed && in.pointerPressed)
			{
				eventConsumed = true;
				break;
			}
		}

		if (auto dockJob = pPressResult.dockingJobOpt.ToPtr())
		{
			// We can't dock in the front-most node, it's one we're docking!
			DENGINE_IMPL_GUI_ASSERT(dockJob->layerIndex > 0);

			auto const tempPtr = in.dockArea->layers.front().root.Release();
			Std::Box<Node> origFrontNode = Std::Box{ static_cast<Node*>(tempPtr) };
			auto& layer = in.dockArea->layers[dockJob->layerIndex];

			// The function takes a Box<Node>, but we store a Box<NodeBase> for implementation hiding.
			// So we need to move it into a temp Box and move it back when we're done.
			Std::Box<Node> temp = Std::Box{ static_cast<Node*>(layer.root.Release()) };
			[[maybe_unused]] bool success = DA_DockNewNode(
				temp,
				dockJob->targetNode,
				dockJob->gizmo,
				origFrontNode);
			DENGINE_IMPL_GUI_ASSERT(success);
			DENGINE_IMPL_GUI_ASSERT(!origFrontNode.Get());
			layer.root = Std::Move(temp);
			in.dockArea->layers.erase(in.dockArea->layers.begin());
		}

		// Are these even necessary?
		if (auto movingState = in.dockArea->stateData.ToPtr<DA::State_Moving>())
		{
			if (movingState->pointerId == in.pointerId && !in.pointerPressed)
				in.dockArea->stateData = DA::State_Normal{};
		}
		else if (auto holdingTabState = in.dockArea->stateData.ToPtr<DA::State_HoldingTab>())
		{
			if (holdingTabState->pointerId == in.pointerId && !in.pointerPressed)
				in.dockArea->stateData = DA::State_Normal{};
		}
		else if (auto resizingSplitNodeState = in.dockArea->stateData.ToPtr<DA::State_ResizingSplitNode>())
		{
			if (resizingSplitNodeState->pointerId == in.pointerId && !in.pointerPressed)
				in.dockArea->stateData = DA::State_Normal{};
		}
	}

	// We know the pointer was inside the DockArea as a whole, so we
	// want to consume the press event.
	return pointerInside;
}

impl::Node::PointerMove_Result impl::DA_WindowNode::PointerMove_StateNormal(PointerMove_Params const& in)
{
	PointerMove_Result returnVal = {};

	auto& implData = *static_cast<impl::ImplData*>(in.ctx->Internal_ImplData());
	Rect titlebarRect = in.nodeRect;
	titlebarRect.extent.height = implData.textManager.lineheight;

	Rect contentRect = in.nodeRect;
	contentRect.position.y += titlebarRect.extent.height;
	contentRect.extent.height = in.nodeRect.extent.height - titlebarRect.extent.height;
	auto& tab = tabs[selectedTab];
	if (tab.widget)
	{
		if (in.pointerId == cursorPointerId)
		{
			tab.widget->CursorMove(
				*in.ctx,
				in.windowId,
				contentRect,
				Rect::Intersection(contentRect, in.visibleRect),
				*in.cursorMoveEvent,
				in.pointerOccluded);
			
		}
		else
		{
			tab.widget->TouchMoveEvent(
				*in.ctx,
				in.windowId,
				contentRect,
				Rect::Intersection(contentRect, in.visibleRect),
				*in.touchEvent,
				in.pointerOccluded);
		}
	}

	return returnVal;
}

impl::Node::PointerMove_Result impl::DA_WindowNode::PointerMove_StateHoldingTab(PointerMove_Params const& in)
{
	DENGINE_IMPL_GUI_ASSERT(in.dockArea->stateData.IsA<DA::State_HoldingTab>());

	PointerMove_Result returnVal = {};

	auto& stateData = in.dockArea->stateData.Get<DA::State_HoldingTab>();
	// If we are too far away from the tab, we dislodge it into it's own layer.
	if (stateData.windowBeingHeld == this && stateData.pointerId == in.pointerId)
	{
		// Compare pointer position Y to titlebar rect Y and dislodge if we are too far
		i32 titlebarPosY = in.nodeRect.position.y;
		// Then skip to the middle of the titlebar
		auto& implData = *static_cast<impl::ImplData*>(in.ctx->Internal_ImplData());
		titlebarPosY += implData.textManager.lineheight / 2;

		if (Math::Abs(in.pointerPos.y - titlebarPosY) >= implData.textManager.lineheight)
		{
			PointerMove_Result::UndockJob undockingJob = {};
			undockingJob.undockedTab = Std::Move(tabs[selectedTab]);

			tabs.erase(tabs.begin() + selectedTab);
			if (selectedTab == tabs.size())
				selectedTab = tabs.size() - 1;

			if (tabs.empty())
			{
				PointerMove_Result::UndockJob::RemoveSplitNodeJob removeSplitNodeJob = {};
				removeSplitNodeJob.layerIndex = in.layerIndex;
				removeSplitNodeJob.windowNodePtr = this;
				undockingJob.removeSplitNodeJobOpt = removeSplitNodeJob;
			}

			DA::State_Moving newBehavior = {};
			newBehavior.movingSplitNode = false;
			newBehavior.pointerId = stateData.pointerId;
			newBehavior.pointerOffset = stateData.pointerOffset;
			//in.dockArea->stateData = newBehavior;
			undockingJob.movingState = newBehavior;

			returnVal.undockJobOpt = Std::Move(undockingJob);

			returnVal.stopIterating = true;
		}
	}

	return returnVal;
}

impl::Node::PointerMove_Result impl::DA_WindowNode::PointerMove_StateMoving(PointerMove_Params const& in)
{
	PointerMove_Result returnVal = {};

	// If we only have 0-1 layers, we shouldn't be in the moving state
	// to begin with.
	DENGINE_IMPL_GUI_ASSERT(in.dockArea->layers.size() >= 2);
	auto& stateMoving = in.dockArea->stateData.Get<DA::State_Moving>();

	// If we are the first layer, we just want to 
	// move the layer and keep iterating to layers behind
	// us to see if we hovered over a window and display it's
	// layout gizmos
	if (in.pointerId == stateMoving.pointerId)
	{
		if (in.layerIndex == 0)
		{
			// Move the window
			Math::Vec2 temp = in.pointerPos;
			temp -= { (f32)in.widgetPos.x, (f32)in.widgetPos.y };
			temp += stateMoving.pointerOffset;
			in.dockArea->layers[0].rect.position = { (i32)temp.x, (i32)temp.y };
			returnVal.stopIterating = false;
		}
		else if (in.nodeRect.PointIsInside(in.pointerPos))
		{
			// If we are not the first layer,
			// we want to see if the mouse is hovering this node
			// and display gizmo layouts if we are,
			// then stop iterating through layers.
			DA::State_Moving::HoveredWindow hoveredWindow = {};
			hoveredWindow.layerIndex = in.layerIndex;
			hoveredWindow.windowNode = this;

			// Then we check if are hovering any of layout gizmos,
			// because if so we want to display the gizmo layout highlight
			Std::Opt<DA_InnerLayoutGizmo> gizmoHitOpt = DA_CheckHitInnerLayoutGizmo(
				in.nodeRect,
				in.dockArea->gizmoSize,
				stateMoving.movingSplitNode, // ignore center
				in.pointerPos);
			if (gizmoHitOpt.HasValue())
			{
				hoveredWindow.gizmoHighlightOpt = (int)gizmoHitOpt.Value();
			}
			
			stateMoving.hoveredWindowOpt = hoveredWindow;
			returnVal.stopIterating = true;
		}
	}

	return returnVal;
}

impl::Node::PointerMove_Result impl::DA_WindowNode::PointerMove(PointerMove_Params const& in)
{
	PointerMove_Result returnVal = {};

	if (in.dockArea->stateData.IsA<DA::State_Normal>())
	{
		returnVal = PointerMove_StateNormal(in);
	}
	else if (in.dockArea->stateData.IsA<DA::State_HoldingTab>())
	{
		returnVal = PointerMove_StateHoldingTab(in);
	}
	else if (in.dockArea->stateData.IsA<DA::State_Moving>())
	{
		returnVal = PointerMove_StateMoving(in);
	}
	return returnVal;
}

void impl::DA_WindowNode::InputConnectionLost()
{
	DENGINE_IMPL_GUI_ASSERT(!tabs.empty());
	DENGINE_IMPL_GUI_ASSERT(selectedTab < tabs.size());
	auto& tab = tabs[selectedTab];
	if (tab.widget)
		tab.widget->InputConnectionLost();
}

void impl::DA_WindowNode::CharEnterEvent(Context& ctx)
{
	DENGINE_IMPL_GUI_ASSERT(!tabs.empty());
	DENGINE_IMPL_GUI_ASSERT(selectedTab < tabs.size());
	auto& tab = tabs[selectedTab];
	if (tab.widget)
		tab.widget->CharEnterEvent(ctx);
}

void impl::DA_WindowNode::CharEvent(Context& ctx, u32 utfValue)
{
	DENGINE_IMPL_GUI_ASSERT(!tabs.empty());
	DENGINE_IMPL_GUI_ASSERT(selectedTab < tabs.size());
	auto& tab = tabs[selectedTab];
	if (tab.widget)
		tab.widget->CharEvent(ctx, utfValue);
}

void impl::DA_WindowNode::CharRemoveEvent(Context& ctx)
{
	DENGINE_IMPL_GUI_ASSERT(!tabs.empty());
	DENGINE_IMPL_GUI_ASSERT(selectedTab < tabs.size());
	auto& tab = tabs[selectedTab];
	if (tab.widget)
		tab.widget->CharRemoveEvent(ctx);
}

impl::Node::PointerMove_Result impl::DA_SplitNode::PointerMove(PointerMove_Params const& in)
{
	DENGINE_IMPL_GUI_ASSERT(a);
	DENGINE_IMPL_GUI_ASSERT(b);

	if (auto resizingState = in.dockArea->stateData.ToPtr<DA::State_ResizingSplitNode>())
	{
		if (resizingState->splitNode == this && resizingState->pointerId == in.pointerId)
		{
			// Translate pointer-position into [0,1] for direction of the splitnode.
			f32 normalizedPos = 0.f;
			Math::Vec2Int tempPointerPos = { (i32)in.pointerPos.x, (i32)in.pointerPos.y };
			if (dir == Direction::Horizontal)
				normalizedPos = f32(tempPointerPos.x - in.nodeRect.position.x) / in.nodeRect.extent.width;
			else
				normalizedPos = f32(tempPointerPos.y - in.nodeRect.position.y) / in.nodeRect.extent.height;

			split = Math::Clamp(normalizedPos, 0.1f, 0.9f);

			PointerMove_Result result = {};
			return result;
		}
	}

	auto childRects = DA_GetSplitNodeChildRects(in.nodeRect, split, dir);

	PointerMove_Params params = in;
	params.hasSplitNodeParent = true;
	params.nodeRect = childRects[0];
	params.pointerOccluded = in.pointerOccluded;
	PointerMove_Result resultA = a->PointerMove(params);
	if (resultA.stopIterating)
		return resultA;

	params.nodeRect = childRects[1];
	PointerMove_Result resultB = b->PointerMove(params);

	PointerMove_Result result = {};
	//result.pointerOccluded = resultA.pointerOccluded || resultB.pointerOccluded;
	result.stopIterating = resultA.stopIterating || resultB.stopIterating;
	// Both results can't have a undocking job simultaneously.
	DENGINE_IMPL_GUI_ASSERT(!(resultA.undockJobOpt.HasValue() && resultB.undockJobOpt.HasValue()));
	if (resultA.undockJobOpt.HasValue())
		result.undockJobOpt = Std::Move(resultA.undockJobOpt);
	else if (resultB.undockJobOpt.HasValue())
		result.undockJobOpt = Std::Move(resultB.undockJobOpt);

	return result;
}

void impl::DA_SplitNode::InputConnectionLost()
{
	DENGINE_IMPL_GUI_ASSERT(a);
	DENGINE_IMPL_GUI_ASSERT(b);

	a->InputConnectionLost();
	b->InputConnectionLost();
}

void DEngine::Gui::impl::DA_SplitNode::CharEnterEvent(Context& ctx)
{
	DENGINE_IMPL_GUI_ASSERT(a);
	DENGINE_IMPL_GUI_ASSERT(b);
	a->CharEnterEvent(ctx);
	b->CharEnterEvent(ctx);
}

void impl::DA_SplitNode::CharEvent(Context& ctx, u32 utfValue)
{
	DENGINE_IMPL_GUI_ASSERT(a);
	DENGINE_IMPL_GUI_ASSERT(b);
	a->CharEvent(ctx, utfValue);
	b->CharEvent(ctx, utfValue);
}

void impl::DA_SplitNode::CharRemoveEvent(Context& ctx)
{
	DENGINE_IMPL_GUI_ASSERT(a);
	DENGINE_IMPL_GUI_ASSERT(b);
	a->CharRemoveEvent(ctx);
	b->CharRemoveEvent(ctx);
}

bool impl::DA_PointerMove(DA_PointerMove_Params const& in)
{
	bool pointerInside = in.widgetRect.PointIsInside(in.pointerPos) && in.visibleRect.PointIsInside(in.pointerPos);

	// If we're inside the widget, we want to occlude the cursor.
	bool returnVal = pointerInside;

	impl::Node::PointerMove_Result result = {};
	auto const layerItPair = impl::DA_GetLayerItPair(*in.dockArea, in.widgetRect);
	bool layerOccluded = false;
	for (auto const& layerItem : layerItPair)
	{
		impl::Node::PointerMove_Params params = {};
		params.ctx = in.ctx;
		params.cursorMoveEvent = in.cursorMoveEvent;
		params.dockArea = in.dockArea;
		params.hasSplitNodeParent = false;
		params.layerIndex = layerItem.layerIndex;
		params.nodeRect = layerItem.layerRect;
		params.pointerId = in.pointerId;
		params.pointerOccluded = in.pointerOccluded || layerOccluded;
		params.pointerPos = in.pointerPos;
		params.touchEvent = in.touchEvent;
		params.visibleRect = in.visibleRect;
		params.widgetPos = in.widgetRect.position;
		params.windowId = in.windowId;
		result = layerItem.rootNode.PointerMove(params);

		if (params.nodeRect.PointIsInside(in.pointerPos) && params.visibleRect.PointIsInside(in.pointerPos))
			layerOccluded = true;

		if (result.stopIterating)
			break;
	}

	if (auto tabUndockingJob = result.undockJobOpt.ToPtr())
	{
		if (auto splitNodeRemoveJob = tabUndockingJob->removeSplitNodeJobOpt.ToPtr())
		{
			auto& layer = in.dockArea->layers[splitNodeRemoveJob->layerIndex];

			// The function takes a Box<Node>, but we store a Box<NodeBase> for implementation hiding.
			// So we need to move it into a temp Box and move it back when we're done.
			Std::Box<Node> temp = Std::Box{ static_cast<Node*>(layer.root.Release()) };
			[[maybe_unused]] bool result = DA_RemoveSplitNode(
				temp,
				(DA_WindowNode const*)splitNodeRemoveJob->windowNodePtr);
			DENGINE_IMPL_GUI_ASSERT(result);
			layer.root = Std::Move(temp);
		}

		// Update our new behavior state
		in.dockArea->stateData = Std::Move(tabUndockingJob->movingState);

		// We want to create a new front layer with a WindowNode, with this tab.
		in.dockArea->layers.emplace(in.dockArea->layers.begin(), DockArea::Layer{});
		DockArea::Layer& newLayer = in.dockArea->layers.front();
		newLayer.rect = { { }, { 400, 400 } };
		impl::DA_WindowNode* node = new impl::DA_WindowNode;
		newLayer.root = Std::Box{ node };
		node->tabs.emplace_back(Std::Move(tabUndockingJob->undockedTab));

		// The current implementation requires we're already in moving state.
		DENGINE_IMPL_GUI_ASSERT(in.dockArea->stateData.IsA<DA::State_Moving>());
		auto& behaviorMoving = in.dockArea->stateData.Get<DA::State_Moving>();
		Math::Vec2 temp = in.pointerPos;
		temp -= { (f32)in.widgetRect.position.x, (f32)in.widgetRect.position.y };
		temp += behaviorMoving.pointerOffset;
		newLayer.rect.position = { (i32)temp.x, (i32)temp.y };
	}

	if (auto movingState = in.dockArea->stateData.ToPtr<DA::State_Moving>())
	{
		DENGINE_IMPL_GUI_ASSERT(in.dockArea->layers.size() >= 2);
		auto gizmoHit = DA_CheckHitOuterLayoutGizmo(in.widgetRect, in.dockArea->gizmoSize, in.pointerPos);
		if (gizmoHit.HasValue())
		{
			movingState->backOuterGizmoHighlightOpt = (int)gizmoHit.Value();
			if (auto hoveredWindow = movingState->hoveredWindowOpt.ToPtr())
			{
				hoveredWindow->gizmoHighlightOpt = Std::nullOpt;
			}
		}
		else
		{
			movingState->backOuterGizmoHighlightOpt = Std::nullOpt;
		}
	}

	return true;
}

DockArea::DockArea()
{
}

void DockArea::AddWindow(
	Std::Str title,
	Math::Vec4 color,
	Std::Box<Widget>&& widget)
{
	layers.emplace(layers.begin(), DockArea::Layer{});
	DockArea::Layer& newLayer = layers.front();
	newLayer.rect = { { }, { 400, 400 } };
	impl::DA_WindowNode* node = new impl::DA_WindowNode;
	newLayer.root = Std::Box{ node };
	node->tabs.push_back(impl::DA_WindowTab());
	auto& newWindow = node->tabs.back();
	newWindow.title = { title.Data(), title.Size() };
	newWindow.color = color;
	newWindow.widget = static_cast<Std::Box<Widget>&&>(widget);
}

SizeHint DockArea::GetSizeHint(
	Context const& ctx) const
{
	SizeHint sizeHint = {};
	sizeHint.expandX = true;
	sizeHint.expandY = true;
	sizeHint.preferred = { 400, 400 };
	return sizeHint;
}

void DockArea::Render(
	Context const& ctx,
	Extent framebufferExtent,
	Rect widgetRect,
	Rect visibleRect,
	DrawInfo& drawInfo) const 
{
	auto scopedScissor = DrawInfo::ScopedScissor(drawInfo, Rect::Intersection(widgetRect, visibleRect));

	auto const layerItPair = impl::DA_GetLayerItPair(*this, widgetRect).Reverse();
	for (auto const& layerItem : layerItPair)
	{
		layerItem.rootNode.Render(
			this,
			ctx,
			framebufferExtent,
			layerItem.layerRect,
			visibleRect,
			drawInfo);

		if (layerItem.layerIndex != layers.size() - 1)
		{
			/*
			auto const resizeHandleItPair = impl::DockArea_GetLayerResizeItPair(
				layerItem.layerRect,
				resizeHandleThickness,
				resizeHandleLength);
			for (auto const& resizeHandle : resizeHandleItPair)
			{
				drawInfo.PushFilledQuad(resizeHandle.rect, resizeHandleColor);
			}
			*/
			
			/*
			// TESTING DRAWING BOT RIGHT TRIANGLE
			Gfx::GuiVertex vertices[3];
			vertices[0].position = { 1.f, 1.f };
			vertices[1].position = { 1.f, -1.f };
			vertices[2].position = { -1.f, 1.f };

			Gfx::GuiDrawCmd drawCmd{};
			drawCmd.type = Gfx::GuiDrawCmd::Type::FilledMesh;
			drawCmd.filledMesh.mesh.vertexOffset = (u32)drawInfo.vertices.size();
			drawCmd.filledMesh.mesh.indexOffset = (u32)drawInfo.indices.size();
			drawCmd.filledMesh.mesh.indexCount = 3;
			drawCmd.filledMesh.color = resizeHandleColor;
			Extent fbExtent = drawInfo.GetFramebufferExtent();
			Math::Vec2Int positionInt = layerItem.layerRect.position;
			positionInt.x += layerItem.layerRect.extent.width - resizeHandleThickness;
			positionInt.y += layerItem.layerRect.extent.height - resizeHandleThickness;
			drawCmd.rectPosition = { (f32)positionInt.x / fbExtent.width, (f32)positionInt.y / fbExtent.height };
			drawCmd.rectExtent = { (f32)resizeHandleThickness / fbExtent.width, (f32)resizeHandleThickness / fbExtent.height };
			drawInfo.drawCmds.push_back(drawCmd);
			
			drawInfo.vertices.push_back(vertices[0]);
			drawInfo.vertices.push_back(vertices[1]);
			drawInfo.vertices.push_back(vertices[2]);
			drawInfo.indices.push_back(0);
			drawInfo.indices.push_back(1);
			drawInfo.indices.push_back(2);
			*/
		}
	}

	// If we are in the moving-state, we want to draw gizmos on top of everything else.
	if (auto stateMoving = stateData.ToPtr<DockArea::State_Moving>())
	{
		// We can't highlight the outer gizmo AND the window-gizmo
		DENGINE_IMPL_GUI_ASSERT(!(
			stateMoving->backOuterGizmoHighlightOpt.HasValue() &&
			stateMoving->hoveredWindowOpt.HasValue() &&
			stateMoving->hoveredWindowOpt.Value().gizmoHighlightOpt.HasValue()));

		if (auto hoveredWindow = stateMoving->hoveredWindowOpt.ToPtr())
		{
			if (auto highlight = hoveredWindow->gizmoHighlightOpt.ToPtr())
			{
				auto const& layer = layers[hoveredWindow->layerIndex];
				auto const& root = *static_cast<impl::Node const*>(layer.root.Get());
				bool nodeFound = root.RenderDockingHighlight(
					impl::DA_GetLayerRect(layers, widgetRect, hoveredWindow->layerIndex),
					dockingHighlightColor,
					static_cast<impl::Node const*>(hoveredWindow->windowNode),
					(impl::DA_InnerLayoutGizmo)*highlight,
					drawInfo);
				DENGINE_IMPL_GUI_ASSERT(nodeFound);
			}
		}

		Rect backLayerRect = widgetRect;
		if (auto backHighlight = stateMoving->backOuterGizmoHighlightOpt.ToPtr())
		{
			Rect highlightRect = impl::DA_GetDockingHighlightRect(
				backLayerRect,
				impl::DA_ToInnerLayoutGizmo((impl::DA_OuterLayoutGizmo)*backHighlight));
			drawInfo.PushFilledQuad(highlightRect, dockingHighlightColor);
		}
		// Iterate over each outer gizmo.
		using GizmoT = impl::DA_OuterLayoutGizmo;
		for (GizmoT gizmo = {}; (int)gizmo < (int)GizmoT::COUNT; gizmo = GizmoT((int)gizmo + 1))
		{
			Rect gizmoRect = impl::DA_GetOuterLayoutGizmoRect(
				backLayerRect,
				gizmo,
				gizmoSize);
			drawInfo.PushFilledQuad(gizmoRect, this->resizeHandleColor);
		}
		// Draw the delete gizmo
		Rect deleteGizmoRect = impl::DA_GetDeleteGizmoRect(backLayerRect, gizmoSize);
		drawInfo.PushFilledQuad(deleteGizmoRect, deleteLayerGizmoColor);

		// If we are hovering a window, we want to find it
		// and display it's layout gizmos.
		if (auto hoveredWindow = stateMoving->hoveredWindowOpt.ToPtr())
		{
			auto const& layer = layers[hoveredWindow->layerIndex];
			auto const& root = *static_cast<impl::Node const*>(layer.root.Get());
			bool nodeFound = root.RenderLayoutGizmo(
				impl::DA_GetLayerRect(layers, widgetRect, hoveredWindow->layerIndex),
				gizmoSize,
				resizeHandleColor,
				static_cast<impl::Node const*>(hoveredWindow->windowNode),
				stateMoving->movingSplitNode, // ignore center
				drawInfo);
			DENGINE_IMPL_GUI_ASSERT(nodeFound);
		}
	}
}

bool DockArea::CursorPress(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	Math::Vec2Int cursorPos,
	CursorClickEvent event)
{
	impl::DA_PointerPress_Params params = {};
	params.ctx = &ctx;
	params.cursorClickEvent = &event;
	params.dockArea = this;
	params.pointerId = impl::cursorPointerId;
	params.pointerPos = { (f32)cursorPos.x, (f32)cursorPos.y };
	params.pointerPressed = event.clicked;
	params.visibleRect = visibleRect;
	params.widgetRect = widgetRect;
	params.windowId = windowId;
	return impl::DA_PointerPress(params);
}

bool DockArea::CursorMove(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	CursorMoveEvent event,
	bool occluded)
{
	impl::DA_PointerMove_Params params = {};
	params.ctx = &ctx;
	params.cursorMoveEvent = &event;
	params.dockArea = this;
	params.pointerId = impl::cursorPointerId;
	params.pointerOccluded = occluded;
	params.pointerPos = { (f32)event.position.x, (f32)event.position.y };
	params.visibleRect = visibleRect;
	params.widgetRect = widgetRect;
	params.windowId = windowId;
	return impl::DA_PointerMove(params);
}

bool DockArea::TouchPressEvent(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	Gui::TouchPressEvent event)
{
	impl::DA_PointerPress_Params params = {};
	params.ctx = &ctx;
	params.dockArea = this;
	params.pointerId = event.id;
	params.pointerPos = event.position;
	params.pointerPressed = event.pressed;
	params.touchEvent = &event;
	params.visibleRect = visibleRect;
	params.widgetRect = widgetRect;
	params.windowId = windowId;
	return impl::DA_PointerPress(params);
}

bool DockArea::TouchMoveEvent(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	Gui::TouchMoveEvent event,
	bool occluded)
{
	impl::DA_PointerMove_Params params = {};
	params.ctx = &ctx;
	params.dockArea = this;
	params.pointerId = event.id;
	params.pointerOccluded = occluded;
	params.pointerPos = event.position;
	params.touchEvent = &event;
	params.visibleRect = visibleRect;
	params.widgetRect = widgetRect;
	params.windowId = windowId;
	return impl::DA_PointerMove(params);
}

void DockArea::InputConnectionLost() 
{
	auto const layerItPair = impl::DA_GetLayerItPair(*this, Rect());
	for (auto const& layer : layerItPair)
	{
		layer.rootNode.InputConnectionLost();
	}
}

void DockArea::CharEnterEvent(
	Context& ctx) 
{
	auto const layerItPair = impl::DA_GetLayerItPair(*this, Rect());
	for (auto const& layer : layerItPair)
	{
		layer.rootNode.CharEnterEvent(ctx);
	}
}

void DockArea::CharEvent(
	Context& ctx,
	u32 utfValue) 
{
	auto const layerItPair = impl::DA_GetLayerItPair(*this, Rect());
	for (auto const& layer : layerItPair)
	{
		layer.rootNode.CharEvent(ctx, utfValue);
	}
}

void DockArea::CharRemoveEvent(
	Context& ctx)
{
	auto const layerItPair = impl::DA_GetLayerItPair(*this, Rect());
	for (auto const& layer : layerItPair)
	{
		layer.rootNode.CharRemoveEvent(ctx);
	}
}