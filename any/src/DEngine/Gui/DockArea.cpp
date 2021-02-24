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
	enum class DA_LayoutGizmo : char { Top, Bottom, Left, Right, COUNT };
	[[nodiscard]] constexpr DA_InnerLayoutGizmo DA_ToInnerLayoutGizmo(DA_LayoutGizmo in) noexcept
	{
		switch (in)
		{
		case DA_LayoutGizmo::Top: return DA_InnerLayoutGizmo::Top;
		case DA_LayoutGizmo::Bottom: return DA_InnerLayoutGizmo::Bottom;
		case DA_LayoutGizmo::Left: return DA_InnerLayoutGizmo::Left;
		case DA_LayoutGizmo::Right: return DA_InnerLayoutGizmo::Right;
		default:
			DENGINE_DETAIL_UNREACHABLE();
			return {};
		}
	}
	struct DA_WindowNode;
}

struct DA::Node
{
	virtual void Render(
		Context const& ctx,
		Extent framebufferExtent,
		Rect nodeRect,
		Rect visibleRect,
		DrawInfo& drawInfo) const = 0;

	// Returns true if the requested node was found.
	virtual bool RenderLayoutGizmo(
		Rect nodeRect,
		u32 gizmoSize,
		Math::Vec4 gizmoColor,
		Node const* hoveredWindow,
		DrawInfo& drawInfo) const = 0;

	// Returns true if successfully docked.
	[[nodiscard]] virtual bool DockNewNode(
		Node const* targetNode,
		impl::DA_InnerLayoutGizmo gizmo,
		Std::Box<Node>& insertNode) = 0;

	struct PointerPress_Params
	{
		Context* ctx = nullptr;
		WindowID windowId;
		DA* dockArea = nullptr;
		Rect nodeRect;
		Rect visibleRect;
		uSize layerIndex;
		Rect layerRect;

		u8 pointerId;
		Math::Vec2 pointerPos;
		bool pointerPressed;

		CursorClickEvent* cursorClickEvent = nullptr;
		Gui::TouchEvent* touchEvent = nullptr;
	};
	// Returns true if event was consumed
	struct PointerPress_Result
	{
		bool eventConsumed = false;
		struct DockingJob
		{
			impl::DA_InnerLayoutGizmo gizmo;
			impl::DA_WindowNode* targetNode;
		};
		Std::Opt<DockingJob> dockingJob;
	};
	[[nodiscard]] virtual PointerPress_Result PointerPress(PointerPress_Params const& in)
	{
		return {};
	}

	struct PointerMove_Params
	{
		Context* ctx = nullptr;
		WindowID windowId;
		DA* dockArea = nullptr;
		Rect visibleRect;
		uSize layerIndex;
		Rect nodeRect;
		Math::Vec2Int widgetPos;

		u8 pointerId;
		Math::Vec2 pointerPos;
		
		CursorMoveEvent* cursorMoveEvent = nullptr;
		Gui::TouchEvent* touchEvent = nullptr;
	};
	// Returns true when we don't want to iterate anymore.
	[[nodiscard]] virtual bool PointerMove(PointerMove_Params const& in)
	{
		return false;
	}

	virtual ~Node() {}
};

namespace DEngine::Gui::impl
{
	struct DA_WindowTab
	{
		std::string title;
		Math::Vec4 color{};
		Std::Box<Widget> widget;
	};

	struct DA_WindowNode : public DA::Node
	{
		uSize selectedTab = 0;
		std::vector<DA_WindowTab> tabs;

		virtual void Render(
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
			DrawInfo& drawInfo) const override;

		virtual bool DockNewNode(
			Node const* targetNode,
			impl::DA_InnerLayoutGizmo gizmo,
			Std::Box<Node>& newNode) override;

		[[nodiscard]] virtual PointerPress_Result PointerPress(PointerPress_Params const& in) override;

		[[nodiscard]] virtual bool PointerMove(PointerMove_Params const& in) override;
	};

	struct DA_SplitNode : public DA::Node
	{
		Std::Box<DA::Node> a;
		Std::Box<DA::Node> b;
		// In the range [0, 1]
		f32 split = 0.5f;
		enum class Direction : char { Horizontal, Vertical };
		Direction dir;

		virtual void Render(
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
			DrawInfo& drawInfo) const override;

		virtual bool DockNewNode(
			Node const* targetNode,
			impl::DA_InnerLayoutGizmo gizmo,
			Std::Box<Node>& newNode) override;

		[[nodiscard]] virtual PointerPress_Result PointerPress(PointerPress_Params const& in) override;

		[[nodiscard]] virtual bool PointerMove(PointerMove_Params const& in) override;
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

	[[nodiscard]] static bool DA_DockNewNode(
		Std::Box<DA::Node>& root,
		DA::Node const* targetNode,
		impl::DA_InnerLayoutGizmo gizmo,
		Std::Box<DA::Node>& insertNode)
	{
		using GizmoT = decltype(gizmo);

		bool returnVal = false;
		if (root == targetNode)
		{
			returnVal = true;
			if (gizmo != GizmoT::Center)
			{
				Std::Box<DA::Node> origNode = Std::Move(root);
				DA_SplitNode* newNode = new DA_SplitNode;
				root = Std::Box<DA::Node>{ newNode };
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
			using NodeT = Std::Trait::Cond<Std::Trait::isConst<T>, DA::Node const, DA::Node>;
			NodeT& rootNode;
			Rect layerRect{};
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
			Result returnVal{
				*layer.root.Get(),
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
	* endIndex is exclusive.
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
			
			SizeHint tabSizeHint = impl::TextManager::GetSizeHint(implData.textManager, tab.title);
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
			returnVal.currentIndex = tabs.Size() - 1;
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

	[[nodiscard]] Std::Opt<uSize> DA_CheckHitTab(
		Context const& ctx,
		Std::Span<DA_WindowTab const> tabs,
		Math::Vec2Int widgetPos,
		Math::Vec2 point)
	{
		Std::Opt<uSize> returnVal = {};
		auto const& tabItPair = DA_GetTabItPair(ctx, tabs, widgetPos);
		for (auto const& tab : tabItPair)
		{
			if (tab.rect.PointIsInside(point))
			{
				returnVal = tab.index;
				break;
			}
		}
		return returnVal;
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
				DENGINE_DETAIL_UNREACHABLE();
				break;
		}
		return returnVal;
	}

	[[nodiscard]] static Rect DA_GetLayoutGizmoRect(
		Rect layerRect,
		DA_LayoutGizmo in,
		u32 gizmoSize) noexcept
	{
		Rect returnVal = {};
		returnVal.position = layerRect.position;
		returnVal.extent = { gizmoSize, gizmoSize };
		switch (in)
		{
			case DA_LayoutGizmo::Top:
				returnVal.position.x += layerRect.extent.width / 2 - gizmoSize / 2;
				break;
			case DA_LayoutGizmo::Bottom:
				returnVal.position.x += layerRect.extent.width / 2 - gizmoSize / 2;
				returnVal.position.y += layerRect.extent.height - gizmoSize;
				break;
			case DA_LayoutGizmo::Left:
				returnVal.position.y += layerRect.extent.height / 2 - gizmoSize / 2;
				break;
			case DA_LayoutGizmo::Right:
				returnVal.position.x += layerRect.extent.width - gizmoSize;
				returnVal.position.y += layerRect.extent.height / 2 - gizmoSize / 2;
				break;
			default:
				DENGINE_DETAIL_UNREACHABLE();
				break;
		}
		return returnVal;
	}

	[[nodiscard]] static Std::Opt<DA_LayoutGizmo> DA_CheckHitLayoutGizmo(
		Rect nodeRect,
		u32 gizmoSize,
		Math::Vec2 point)
	{
		Std::Opt<DA_LayoutGizmo> gizmoHit;
		using GizmoT = DA_LayoutGizmo;
		for (GizmoT gizmo = {}; (int)gizmo < (int)GizmoT::COUNT; gizmo = GizmoT((int)gizmo + 1))
		{
			Rect gizmoRect = DA_GetLayoutGizmoRect(nodeRect, gizmo, gizmoSize);
			if (gizmoRect.PointIsInside(point))
			{
				gizmoHit = gizmo;
				break;
			}
		}
		return gizmoHit;
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
			DENGINE_DETAIL_UNREACHABLE();
			break;
		}
		return returnVal;
	}

	[[nodiscard]] static Std::Opt<DA_InnerLayoutGizmo> DA_CheckHitInnerLayoutGizmo(
		Rect nodeRect,
		u32 gizmoSize,
		Math::Vec2 point) noexcept
	{
		Std::Opt<DA_InnerLayoutGizmo> gizmoHit;
		using GizmoT = DA_InnerLayoutGizmo;
		for (GizmoT gizmo = {}; (int)gizmo < (int)GizmoT::COUNT; gizmo = GizmoT((int)gizmo + 1))
		{
			Rect gizmoRect = DA_GetInnerLayoutGizmoRect(nodeRect, gizmo, gizmoSize);
			if (gizmoRect.PointIsInside(point))
			{
				gizmoHit = gizmo;
				break;
			}
		}
		return gizmoHit;
	}

	struct DA_PointerPress_Params
	{
		Context* ctx = nullptr;
		WindowID windowId;
		DA* dockArea = nullptr;
		Rect widgetRect;
		Rect visibleRect;

		u8 pointerId;
		Math::Vec2 pointerPos;
		bool pointerPressed;

		CursorClickEvent* cursorClickEvent = nullptr;
		Gui::TouchEvent* touchEvent = nullptr;
	};
	static void DA_PointerPress(DA_PointerPress_Params const& in);

	struct DA_PointerMove_Params
	{
		Context* ctx = nullptr;
		WindowID windowId;
		DA* dockArea = nullptr;
		Rect widgetRect;
		Rect visibleRect;
		
		u8 pointerId;
		Math::Vec2 pointerPos;
		
		CursorMoveEvent* cursorMoveEvent = nullptr;
		Gui::TouchEvent* touchEvent = nullptr;
	};
	static void DA_PointerMove(DA_PointerMove_Params const& in);
}

bool impl::DA_WindowNode::DockNewNode(
	Node const* targetNode,
	impl::DA_InnerLayoutGizmo gizmo,
	Std::Box<Node>& insertNode)
{
	if (targetNode != this)
		return false;
	DENGINE_DETAIL_ASSERT(gizmo == impl::DA_InnerLayoutGizmo::Center);
	DENGINE_DETAIL_ASSERT(dynamic_cast<DA_WindowNode*>(insertNode.Get()));

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
	if (a.Get() == targetNode)
	{
		bool result = DA_DockNewNode(a, targetNode, gizmo, insertNode);
		if (result)
			return true;
	}

	if (b.Get() == targetNode)
	{
		bool result = DA_DockNewNode(b, targetNode, gizmo, insertNode);
		if (result)
			return true;
	}

	return false;
}

void impl::DA_WindowNode::Render(
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
	//drawInfo.PushFilledQuad(titlebarRect, activeTab.color);

	auto const tabItPair = DA_GetTabItPair(
		ctx,
		{ tabs.data(), tabs.size() },
		nodeRect.position);
	for (auto const& tabItem : tabItPair)
	{
		drawInfo.PushFilledQuad(tabItem.rect, tabItem.tab.color);
	}

	{
		u32 tabHorizontalOffset = 0;
		for (uSize i = 0; i < tabs.size(); i += 1)
		{
			auto const& tab = tabs[i];
			SizeHint tabTitleSizeHint = TextManager::GetSizeHint(implData.textManager, tab.title);
			// Render tabs and their text.
			Rect tabRect = titlebarRect;
			tabRect.position.x += tabHorizontalOffset;
			tabRect.extent.width = tabTitleSizeHint.preferred.width;
			drawInfo.PushFilledQuad(tabRect, tab.color);

			TextManager::RenderText(
				implData.textManager,
				tab.title,
				Math::Vec4{ 1.f, 1.f, 1.f, 1.f },
				tabRect,
				drawInfo);

			tabHorizontalOffset += tabRect.extent.width;
		}
	}

	Rect contentRect = nodeRect;
	contentRect.position.y += titlebarRect.extent.height;
	contentRect.extent.height -= titlebarRect.extent.height;

	Math::Vec4 contentBackgroundColor = activeTab.color;
	contentBackgroundColor *= 0.5f;
	drawInfo.PushFilledQuad(contentRect, contentBackgroundColor);

	if (activeTab.widget)
	{
		activeTab.widget->Render(
			ctx,
			framebufferExtent,
			contentRect,
			visibleRect,
			drawInfo);
	}
}

void impl::DA_SplitNode::Render(
	Context const& ctx,
	Extent framebufferExtent,
	Rect nodeRect,
	Rect visibleRect,
	DrawInfo& drawInfo) const
{
	DENGINE_IMPL_GUI_ASSERT(a);
	DENGINE_IMPL_GUI_ASSERT(b);

	Rect childRect = nodeRect;
	bool const isHoriz = dir == Direction::Horizontal;
	u32 const nodeExtent = isHoriz ? nodeRect.extent.width : nodeRect.extent.height;
	u32& childExtent = isHoriz ? childRect.extent.width : childRect.extent.height;
	i32& childPos = isHoriz ? childRect.position.x : childRect.position.y;
	childExtent = u32(nodeExtent * split);

	a->Render(
		ctx,
		framebufferExtent,
		childRect,
		visibleRect,
		drawInfo);

	childPos += childExtent;
	childExtent = nodeExtent - childExtent;

	b->Render(
		ctx,
		framebufferExtent,
		childRect,
		visibleRect,
		drawInfo);
}

bool impl::DA_WindowNode::RenderLayoutGizmo(
	Rect nodeRect,
	u32 gizmoSize,
	Math::Vec4 gizmoColor,
	Node const* hoveredWindow,
	DrawInfo& drawInfo) const
{
	if (this != hoveredWindow)
		return false;

	using GizmoT = DA_InnerLayoutGizmo;
	for (GizmoT gizmo = {}; (int)gizmo < (int)GizmoT::COUNT; gizmo = GizmoT((int)gizmo + 1))
	{
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
	DrawInfo& drawInfo) const
{
	DENGINE_IMPL_GUI_ASSERT(a);
	DENGINE_IMPL_GUI_ASSERT(b);

	Rect childRect = nodeRect;
	bool const isHoriz = dir == Direction::Horizontal;
	u32 const nodeExtent = isHoriz ? nodeRect.extent.width : nodeRect.extent.height;
	u32& childExtent = isHoriz ? childRect.extent.width : childRect.extent.height;
	i32& childPos = isHoriz ? childRect.position.x : childRect.position.y;
	childExtent = u32(nodeExtent * split);

	bool windowFound = a->RenderLayoutGizmo(childRect, gizmoSize, gizmoColor, hoveredWindow, drawInfo);
	if (windowFound)
		return true;

	childPos += childExtent;
	childExtent = nodeExtent - childExtent;

	windowFound = b->RenderLayoutGizmo(childRect, gizmoSize, gizmoColor, hoveredWindow, drawInfo);
	return windowFound;
}

DA::Node::PointerPress_Result impl::DA_WindowNode::PointerPress(PointerPress_Params const& in)
{
	PointerPress_Result returnVal{};

	bool cursorInside = in.nodeRect.PointIsInside(in.pointerPos) && in.visibleRect.PointIsInside(in.pointerPos);
	if (!cursorInside)
	{
		return returnVal;
	}

	if (in.dockArea->behaviorData.IsA<DA::BehaviorData_Normal>())
	{
		auto& implData = *static_cast<impl::ImplData*>(in.ctx->Internal_ImplData());

		Rect titlebarRect = in.nodeRect;
		titlebarRect.extent.height = implData.textManager.lineheight;

		// Check if we hit a tab that's not selected.

		if (in.layerIndex != in.dockArea->layers.size() - 1)
		{
			// and push this layer to the front, if it's not the rear-most layer.
			if (in.layerIndex != 0)
				DA_PushLayerToFront(*in.dockArea, in.layerIndex);

			// If we are within the titlebar, we want to transition the
			// dockarea widget into "moving" mode.
			
			if (in.pointerPressed && titlebarRect.PointIsInside(in.pointerPos))
			{
				DA::BehaviorData_Moving newBehavior{};
				newBehavior.pointerID = in.pointerId;
				Math::Vec2 temp = Math::Vec2{ (f32)in.layerRect.position.x, (f32)in.layerRect.position.y } - in.pointerPos;
				newBehavior.pointerOffset = temp;
				in.dockArea->behaviorData.Set(newBehavior);

				returnVal.eventConsumed = true;
				return returnVal;
			}
		}
		
		Rect contentRect = in.nodeRect;
		contentRect.position.y += titlebarRect.extent.height;
		contentRect.extent.height = in.nodeRect.extent.height - titlebarRect.extent.height;
		if (!returnVal.eventConsumed && contentRect.PointIsInside(in.pointerPos) && tabs[selectedTab].widget)
		{
			if (in.pointerId == DA::cursorPointerID)
			{
				tabs[selectedTab].widget->CursorClick(
					*in.ctx,
					in.windowId,
					contentRect,
					Rect::Intersection(contentRect, in.visibleRect),
					{ (i32)in.pointerPos.x, (i32)in.pointerPos.y },
					*in.cursorClickEvent);
			}
			else
			{
				tabs[selectedTab].widget->TouchEvent(
					*in.ctx,
					in.windowId,
					contentRect,
					Rect::Intersection(contentRect, in.visibleRect),
					*in.touchEvent);
			}
			returnVal.eventConsumed = true;
			return returnVal;
		}
	}
	else if (in.dockArea->behaviorData.IsA<DA::BehaviorData_Moving>())
	{
		auto& behaviorData = in.dockArea->behaviorData.Get<DA::BehaviorData_Moving>();
		if (in.pointerId != behaviorData.pointerID)
			return returnVal; // We don't want to do anything when we're in moving-state and it's not the active pointerID.

		// We want to iterate until we find out if we want to dock anywhere.
		if (!in.pointerPressed && in.layerIndex > 0)
		{
			// Check if we hit any of this window's layout gizmos.
			using GizmoT = DA_InnerLayoutGizmo;
			Std::Opt<GizmoT> gizmoHit = DA_CheckHitInnerLayoutGizmo(in.nodeRect, in.dockArea->gizmoSize, in.pointerPos);
			if (gizmoHit.HasValue())
			{
				returnVal.eventConsumed = true;
				PointerPress_Result::DockingJob dockJob{};
				dockJob.gizmo = gizmoHit.Value();
				dockJob.targetNode = this;
				returnVal.dockingJob = dockJob;
			}
		}
	}

	return returnVal;
}

DA::Node::PointerPress_Result impl::DA_SplitNode::PointerPress(PointerPress_Params const& in)
{
	DENGINE_IMPL_GUI_ASSERT(a);
	DENGINE_IMPL_GUI_ASSERT(b);

	Rect childRect = in.nodeRect;
	bool const isHoriz = dir == Direction::Horizontal;
	u32 const nodeExtent = isHoriz ? in.nodeRect.extent.width : in.nodeRect.extent.height;
	u32& childExtent = isHoriz ? childRect.extent.width : childRect.extent.height;
	i32& childPos = isHoriz ? childRect.position.x : childRect.position.y;
	childExtent = u32(nodeExtent * split);

	PointerPress_Params params = in;
	params.nodeRect = childRect;
	PointerPress_Result result = a->PointerPress(params);
	if (result.eventConsumed)
		return result;

	childPos += childExtent;
	childExtent = nodeExtent - childExtent;

	params.nodeRect = childRect;
	result = b->PointerPress(params);
	return result;
}

void impl::DA_PointerPress(DA_PointerPress_Params const& in)
{
	bool pointerInside = in.widgetRect.PointIsInside(in.pointerPos) && in.visibleRect.PointIsInside(in.pointerPos);
	if (!pointerInside)
		return;

	bool eventConsumed = false;

	bool runBackLayerDockingTest = 
		!eventConsumed &&
		in.dockArea->behaviorData.IsA<DA::BehaviorData_Moving>() &&
		in.dockArea->layers.size() >= 2 &&
		!in.pointerPressed;
	if (runBackLayerDockingTest)
	{
		// Get rect of rear-most layer.
		Rect backLayerRect = DA_GetLayerRect(
			{ in.dockArea->layers.data(), in.dockArea->layers.size() },
			in.widgetRect,
			in.dockArea->layers.size() - 1);
		auto gizmoHit = DA_CheckHitLayoutGizmo(backLayerRect, in.dockArea->gizmoSize, in.pointerPos);
		if (gizmoHit.HasValue())
		{
			in.dockArea->behaviorData.Set(DA::BehaviorData_Normal{});
			eventConsumed = true;

			Std::Box<DA::Node> origFrontNode = Std::Move(in.dockArea->layers.front().root);
			in.dockArea->layers.erase(in.dockArea->layers.begin());
			auto& backLayer = in.dockArea->layers.back();

			bool success = DA_DockNewNode(
				backLayer.root,
				backLayer.root.Get(),
				DA_ToInnerLayoutGizmo(gizmoHit.Value()),
				origFrontNode);
			DENGINE_IMPL_GUI_ASSERT(success);
		}
	}

	if (!eventConsumed)
	{
		DA::Node::PointerPress_Result pPressResult{};
		auto const layerItPair = impl::DA_GetLayerItPair(*in.dockArea, in.widgetRect);
		for (auto const& layerItem : layerItPair)
		{
			DA::Node::PointerPress_Params params = {};
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
			if (pPressResult.eventConsumed)
				break;
		}

		if (pPressResult.dockingJob.HasValue())
		{
			auto const& dockJob = pPressResult.dockingJob.Value();

			Std::Box<DA::Node> origFrontNode = Std::Move(in.dockArea->layers.front().root);
			in.dockArea->layers.erase(in.dockArea->layers.begin());

			auto const layerItPair = impl::DA_GetLayerItPair(*in.dockArea, in.widgetRect);
			bool success = false;
			for (auto const& layerItem : layerItPair)
			{
				success = DA_DockNewNode(
					in.dockArea->layers[layerItem.layerIndex].root,
					dockJob.targetNode,
					dockJob.gizmo,
					origFrontNode);
				if (success)
					break;
			}
			DENGINE_IMPL_GUI_ASSERT(success);
			DENGINE_IMPL_GUI_ASSERT(!origFrontNode.Get());
		}
		if (!in.pointerPressed && in.dockArea->behaviorData.IsA<DA::BehaviorData_Moving>())
		{
			in.dockArea->behaviorData.Set(DA::BehaviorData_Normal{});
		}
	}
}

bool impl::DA_WindowNode::PointerMove(PointerMove_Params const& in)
{
	bool eventConsumed = false;

	if (in.dockArea->behaviorData.IsA<DA::BehaviorData_Normal>())
	{
		auto& implData = *static_cast<impl::ImplData*>(in.ctx->Internal_ImplData());
		Rect titlebarRect = in.nodeRect;
		titlebarRect.extent.height = implData.textManager.lineheight;

		Rect contentRect = in.nodeRect;
		contentRect.position.y += titlebarRect.extent.height;
		contentRect.extent.height = in.nodeRect.extent.height - titlebarRect.extent.height;
		if (tabs[selectedTab].widget && contentRect.PointIsInside(in.pointerPos))
		{
			if (in.pointerId == DA::cursorPointerID)
			{
				tabs[selectedTab].widget->CursorMove(
					*in.ctx,
					in.windowId,
					in.nodeRect,
					Rect::Intersection(in.nodeRect, in.visibleRect),
					*in.cursorMoveEvent);
				eventConsumed = true;
			}
			else
			{
				tabs[selectedTab].widget->TouchEvent(
					*in.ctx,
					in.windowId,
					in.nodeRect,
					Rect::Intersection(in.nodeRect, in.visibleRect),
					*in.touchEvent);
				eventConsumed = true;
			}
		}
	}
	else if (in.dockArea->behaviorData.IsA<DA::BehaviorData_Moving>())
	{
		// If we only have 0-1 layer, we shouldn't be in the moving state
		// to begin with.
		DENGINE_IMPL_GUI_ASSERT(in.dockArea->layers.size() >= 2);
		auto& behaviorMoving = in.dockArea->behaviorData.Get<DA::BehaviorData_Moving>();

		// If we are the first layer, we just want to 
		// move the layer keep iterating to layers behind
		// us to see if we hovered over a window and display it's
		// layout gizmos
		if (in.pointerId == behaviorMoving.pointerID)
		{
			if (in.layerIndex == 0)
			{
				Math::Vec2 temp = in.pointerPos;
				temp -= { (f32)in.widgetPos.x, (f32)in.widgetPos.y };
				temp += behaviorMoving.pointerOffset;
				in.dockArea->layers[in.layerIndex].rect.position = { (i32)temp.x, (i32)temp.y };
			}
			else if (!eventConsumed && in.nodeRect.PointIsInside(in.pointerPos))
			{
				// If we are not the first layer,
				// we want to see if the mouse is hovering this node
				// and display gizmo layouts if we are,
				// then stop iterating through layers.
				eventConsumed = true;
				behaviorMoving.currWindowHovered = this;
			}
		}
	}
	return eventConsumed;
}

bool impl::DA_SplitNode::PointerMove(PointerMove_Params const& in)
{
	DENGINE_IMPL_GUI_ASSERT(a);
	DENGINE_IMPL_GUI_ASSERT(b);

	Rect childRect = in.nodeRect;
	bool const isHoriz = dir == Direction::Horizontal;
	u32 const nodeExtent = isHoriz ? in.nodeRect.extent.width : in.nodeRect.extent.height;
	u32& childExtent = isHoriz ? childRect.extent.width : childRect.extent.height;
	i32& childPos = isHoriz ? childRect.position.x : childRect.position.y;
	childExtent = u32(nodeExtent * split);

	PointerMove_Params params = in;
	params.nodeRect = childRect;
	bool eventConsumed = a->PointerMove(params);
	if (eventConsumed)
		return true;

	childPos += childExtent;
	childExtent = nodeExtent - childExtent;
	
	params.nodeRect = childRect;
	eventConsumed = b->PointerMove(params);
	return eventConsumed;
}

void impl::DA_PointerMove(DA_PointerMove_Params const& in)
{
	bool pointerInside = in.widgetRect.PointIsInside(in.pointerPos) && in.visibleRect.PointIsInside(in.pointerPos);
	if (!pointerInside)
		return;

	auto const layerItPair = impl::DA_GetLayerItPair(*in.dockArea, in.widgetRect);
	for (auto const& layerItem : layerItPair)
	{
		DA::Node::PointerMove_Params params = {};
		params.ctx = in.ctx;
		params.cursorMoveEvent = in.cursorMoveEvent;
		params.dockArea = in.dockArea;
		params.layerIndex = layerItem.layerIndex;
		params.nodeRect = layerItem.layerRect;
		params.pointerId = in.pointerId;
		params.pointerPos = in.pointerPos;
		params.touchEvent = in.touchEvent;
		params.widgetPos = in.widgetRect.position;
		params.windowId = in.windowId;
		bool eventConsumed = layerItem.rootNode.PointerMove(params);
		if (eventConsumed)
			break;
	}
}

DockArea::DockArea()
{
	behaviorData.Set(BehaviorData_Normal{});
}

void DockArea::AddWindow(
	std::string_view title,
	Math::Vec4 color,
	Std::Box<Widget>&& widget)
{
	layers.emplace(layers.begin(), DockArea::Layer{});
	DockArea::Layer& newLayer = layers.front();
	newLayer.rect = { { }, { 400, 400 } };
	impl::DA_WindowNode* node = new impl::DA_WindowNode;
	newLayer.root = Std::Box<DockArea::Node>{ node };
	node->tabs.push_back(impl::DA_WindowTab());
	auto& newWindow = node->tabs.back();
	newWindow.title = title;
	newWindow.color = color;
	newWindow.widget = static_cast<Std::Box<Widget>&&>(widget);
}

SizeHint DockArea::GetSizeHint(
	Context const& ctx) const
{
	SizeHint sizeHint{};
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
	auto const layerItPair = impl::DA_GetLayerItPair(*this, widgetRect).Reverse();
	for (auto const& layerItem : layerItPair)
	{
		layerItem.rootNode.Render(
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
		}
	}

	// If we are in the moving-state, we want to draw gizmos on top of everything else.
	if (behaviorData.IsA<DockArea::BehaviorData_Moving>())
	{
		auto& behaviorMoving = behaviorData.Get<DockArea::BehaviorData_Moving>();

		// If we are hovering a window, we want to find it
		// and display it's layout gizmos.
		if (behaviorMoving.currWindowHovered)
		{
			// Iterate from index 1 until the end
			auto const layerItPair = impl::DA_GetLayerItPair(*this, widgetRect, 1, layers.size());
			bool nodeFound = false;
			for (auto const& layerItem : layerItPair)
			{
				nodeFound = layerItem.rootNode.RenderLayoutGizmo(
					layerItem.layerRect,
					gizmoSize,
					resizeHandleColor,
					static_cast<DA::Node const*>(behaviorMoving.currWindowHovered),
					drawInfo);
				if (nodeFound)
					break;
			}
			DENGINE_IMPL_GUI_ASSERT(nodeFound);
		}

		Rect backLayerRect = widgetRect;
		// Iterate over each gizmo.
		using GizmoT = impl::DA_LayoutGizmo;
		for (GizmoT gizmo = {}; (int)gizmo < (int)GizmoT::COUNT; gizmo = GizmoT((int)gizmo + 1))
		{
			Rect gizmoRect = impl::DA_GetLayoutGizmoRect(
				backLayerRect,
				gizmo,
				gizmoSize);
			drawInfo.PushFilledQuad(gizmoRect, this->resizeHandleColor);
		}
	}
}

void DockArea::CursorClick(
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
	params.pointerId = DA::cursorPointerID;
	params.pointerPos = { (f32)cursorPos.x, (f32)cursorPos.y };
	params.pointerPressed = event.clicked;
	params.visibleRect = visibleRect;
	params.widgetRect = widgetRect;
	params.windowId = windowId;
	impl::DA_PointerPress(params);
}

void DockArea::CursorMove(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	CursorMoveEvent event)
{
	impl::DA_PointerMove_Params params = {};
	params.ctx = &ctx;
	params.cursorMoveEvent = &event;
	params.dockArea = this;
	params.pointerId = DockArea::cursorPointerID;
	params.pointerPos = { (f32)event.position.x, (f32)event.position.y };
	params.visibleRect = visibleRect;
	params.widgetRect = widgetRect;
	params.windowId = windowId;
	impl::DA_PointerMove(params);
}

void DockArea::TouchEvent(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	Gui::TouchEvent event) 
{
	if (event.type == TouchEventType::Down || event.type == TouchEventType::Up)
	{
		impl::DA_PointerPress_Params params = {};
		params.ctx = &ctx;
		params.dockArea = this;
		params.pointerId = event.id;
		params.pointerPos = event.position;
		params.pointerPressed = event.type == TouchEventType::Down;
		params.touchEvent = &event;
		params.visibleRect = visibleRect;
		params.widgetRect = widgetRect;
		params.windowId = windowId;
		impl::DA_PointerPress(params);
	}
	else if (event.type == TouchEventType::Moved)
	{
		impl::DA_PointerMove_Params params = {};
		params.ctx = &ctx;
		params.dockArea = this;
		params.pointerId = event.id;
		params.pointerPos = event.position;
		params.touchEvent = &event;
		params.visibleRect = visibleRect;
		params.widgetRect = widgetRect;
		params.windowId = windowId;
		impl::DA_PointerMove(params);
	}
	else
		DENGINE_DETAIL_UNREACHABLE();
}

void DockArea::InputConnectionLost() 
{
}

void DockArea::CharEnterEvent(
	Context& ctx) {}

void DockArea::CharEvent(
	Context& ctx,
	u32 utfValue) {}

void DockArea::CharRemoveEvent(
	Context& ctx)
{

}