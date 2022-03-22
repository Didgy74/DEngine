#include <DEngine/Gui/DockArea.hpp>
#include <DEngine/Gui/DrawInfo.hpp>
#include <DEngine/Gui/TextManager.hpp>
#include <DEngine/Std/Utility.hpp>

#include <DEngine/Std/Containers/Array.hpp>
#include <DEngine/Std/Containers/Vec.hpp>

#include <new>

// We have a shorthand for DockArea called DA in this file.

using namespace DEngine;
using namespace DEngine::Gui;
using DA = DockArea;

namespace DEngine::Gui::impl
{
	enum class DA_InnerDockingGizmo : char { Center, Top, Bottom, Left, Right, COUNT };
	enum class DA_OuterLayoutGizmo : char { Top, Bottom, Left, Right, COUNT };
	[[nodiscard]] constexpr DA_InnerDockingGizmo DA_ToInnerLayoutGizmo(DA_OuterLayoutGizmo in) noexcept
	{
		switch (in)
		{
			case DA_OuterLayoutGizmo::Top: return DA_InnerDockingGizmo::Top;
			case DA_OuterLayoutGizmo::Bottom: return DA_InnerDockingGizmo::Bottom;
			case DA_OuterLayoutGizmo::Left: return DA_InnerDockingGizmo::Left;
			case DA_OuterLayoutGizmo::Right: return DA_InnerDockingGizmo::Right;
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

	struct DA_Node
	{
		[[nodiscard]] virtual NodeType GetNodeType() const = 0;

		virtual void Render(
			DA const& dockArea,
			Rect const& nodeRect,
			Rect const& visibleRect,
			Widget::Render_Params const& params) const {}

		// Returns true if the requested node was found.
		[[nodiscard]] virtual bool RenderLayoutGizmo(
			Rect nodeRect,
			u32 gizmoSize,
			Math::Vec4 gizmoColor,
			DA_Node const* hoveredWindow,
			bool ignoreCenter,
			DrawInfo& drawInfo) const = 0;

		[[nodiscard]] virtual bool RenderDockingHighlight(
			Rect nodeRect,
			Math::Vec4 highlightColor,
			DA_Node const* hoveredWindow,
			impl::DA_InnerDockingGizmo gizmo,
			DrawInfo& drawInfo) const = 0;

		// Returns true if successfully docked.
		[[nodiscard]] virtual bool DockNewNode(
			DA_Node const* targetNode,
			impl::DA_InnerDockingGizmo gizmo,
			Std::Box<DA_Node>& insertNode) = 0;

		struct PointerPress_Params
		{
			Context* ctx = {};
			TextManager* textManager = {};
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

			CursorPressEvent* cursorClickEvent = nullptr;
			Gui::TouchPressEvent* touchEvent = nullptr;
		};

		virtual void InputConnectionLost() {}

		virtual void CharEnterEvent(
			Context& ctx) {}
		virtual void CharEvent(
			Context& ctx,
			u32 utfValue) {}
		virtual void CharRemoveEvent(
			Context& ctx) {}

		virtual ~DA_Node() {}
	};

	struct DA_WindowNode : public DA_Node
	{
		uSize activeTabIndex = 0;
		std::vector<DA_WindowTab> tabs;

		[[nodiscard]] virtual NodeType GetNodeType() const override { return NodeType::Window; }

		virtual void Render(
			DA const& dockArea,
			Rect const& nodeRect,
			Rect const& visibleRect,
			Widget::Render_Params const& params) const override;

		virtual bool RenderLayoutGizmo(
			Rect nodeRect,
			u32 gizmoSize,
			Math::Vec4 gizmoColor,
			DA_Node const* hoveredWindow,
			bool ignoreCenter,
			DrawInfo& drawInfo) const override;

		[[nodiscard]] virtual bool RenderDockingHighlight(
			Rect nodeRect,
			Math::Vec4 highlightColor,
			DA_Node const* hoveredWindow,
			impl::DA_InnerDockingGizmo gizmo,
			DrawInfo& drawInfo) const override;

		virtual bool DockNewNode(
			DA_Node const* targetNode,
			impl::DA_InnerDockingGizmo gizmo,
			Std::Box<DA_Node>& newNode) override;
		
		virtual void InputConnectionLost() override;
	};

	struct DA_SplitNode : public DA_Node
	{
		DA_Node* a = nullptr;
		DA_Node* b = nullptr;
		virtual ~DA_SplitNode() override
		{
			DENGINE_IMPL_GUI_ASSERT(a);
			DENGINE_IMPL_GUI_ASSERT(b);
			delete a;
			delete b;
		}

		// In the range [0, 1]
		f32 split = 0.5f;
		enum class Direction : char { Horizontal, Vertical };
		Direction dir;

		[[nodiscard]] virtual NodeType GetNodeType() const override { return NodeType::Split; }

		virtual void Render(
			DA const& dockArea,
			Rect const& nodeRect,
			Rect const& visibleRect,
			Widget::Render_Params const& params) const override;

		virtual bool RenderLayoutGizmo(
			Rect nodeRect,
			u32 gizmoSize,
			Math::Vec4 gizmoColor,
			DA_Node const* hoveredWindow,
			bool ignoreCenter,
			DrawInfo& drawInfo) const override;

		[[nodiscard]] virtual bool RenderDockingHighlight(
			Rect nodeRect,
			Math::Vec4 highlightColor,
			DA_Node const* hoveredWindow,
			impl::DA_InnerDockingGizmo gizmo,
			DrawInfo& drawInfo) const override;

		virtual bool DockNewNode(
			DA_Node const* targetNode,
			impl::DA_InnerDockingGizmo gizmo,
			Std::Box<DA_Node>& newNode) override;

		virtual void InputConnectionLost() override;
		virtual void CharEnterEvent(
			Context& ctx) override;
		virtual void CharEvent(
			Context& ctx,
			u32 utfValue) override;
		virtual void CharRemoveEvent(
			Context& ctx) override;
	};

	template<typename T>
	struct DA_TabIt
	{
		TextManager* textManager = nullptr;
		Std::Span<T> tabs;
		Math::Vec2Int widgetPos;
		u32 tabHeight;
		u32 textMargin;
		u32 horizontalOffset = 0;
		uSize currentIndex;

		struct Result
		{
			T& tab;
			Rect rectOuter;
			Rect textRect;
			uSize index;
		};
		Result operator*() noexcept
		{
			auto& tab = tabs[currentIndex];

			DENGINE_IMPL_GUI_UNREACHABLE();
			SizeHint tabSizeHint = {};

			Rect tabRect = {};
			tabRect.position = widgetPos;
			tabRect.position.x += horizontalOffset;
			tabRect.extent.width = tabSizeHint.minimum.width + (textMargin * 2);
			tabRect.extent.height = tabHeight + (textMargin * 2);

			Rect textRect = tabRect;
			textRect.position.x += textMargin;
			textRect.position.y += textMargin;
			textRect.extent.width -= textMargin * 2;
			textRect.extent.height -= textMargin * 2;

			Result returnVal = {
				tabs[currentIndex],
				tabRect,
				textRect,
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
		TextManager* textManager = nullptr;
		Std::Span<T> tabs;
		Math::Vec2Int widgetPos;
		u32 textMargin;

		[[nodiscard]] DA_TabIt<T> begin() const noexcept
		{
			DA_TabIt<T> returnVal = {};
			returnVal.currentIndex = 0;
			returnVal.tabHeight = textManager->GetLineheight();
			returnVal.tabs = tabs;
			returnVal.textMargin = textMargin;
			returnVal.widgetPos = widgetPos;
			return returnVal;
		}

		[[nodiscard]] DA_TabIt<T> end() const noexcept
		{
			DA_TabIt<T> returnVal = {};
			returnVal.currentIndex = tabs.Size();
			returnVal.tabHeight = textManager->GetLineheight();
			returnVal.tabs = tabs;
			returnVal.textMargin = textMargin;
			returnVal.widgetPos = widgetPos;
			return returnVal;
		}
	};

	[[nodiscard]] static DA_TabItPair<DA_WindowTab> DA_GetTabItPair(
		Std::Span<DA_WindowTab> tabs,
		u32 textMargin,
		Math::Vec2Int widgetPos) noexcept
	{
		DA_TabItPair<DA_WindowTab> returnVal = {};
		returnVal.tabs = tabs;
		returnVal.textMargin = textMargin;
		returnVal.widgetPos = widgetPos;
		return returnVal;
	}

	[[nodiscard]] static DA_TabItPair<DA_WindowTab const> DA_GetTabItPair(
		Std::Span<DA_WindowTab const> tabs,
		u32 textMargin,
		Math::Vec2Int widgetPos) noexcept
	{
		DA_TabItPair<DA_WindowTab const> returnVal = {};
		returnVal.tabs = tabs;
		returnVal.textMargin = textMargin;
		returnVal.widgetPos = widgetPos;
		return returnVal;
	}

	[[nodiscard]] static DA_TabItPair<DA_WindowTab const> DA_GetTabItPair(
		std::vector<DA_WindowTab> const& tabs,
		u32 textMargin,
		Math::Vec2Int widgetPos) noexcept 
	{
		return DA_GetTabItPair(
			{ tabs.data(), tabs.size() },
			textMargin,
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
		u32 textMargin,
		Math::Vec2Int widgetPos,
		Math::Vec2 point)
	{
		Std::Opt<DA_TabHit> returnVal = {};
		/*auto const& tabItPair = DA_GetTabItPair(tabs, textMargin, widgetPos);
		for (auto const& tab : tabItPair)
		{
			if (tab.rectOuter.PointIsInside(point))
			{
				DA_TabHit hit = {};
				hit.index = tab.index;
				hit.rect = tab.rectOuter;
				returnVal = hit;
				break;
			}
		}*/
		return returnVal;
	}

	[[nodiscard]] Std::Opt<DA_TabHit> DA_CheckHitTab(
		Context const& ctx,
		std::vector<DA_WindowTab> const& tabs,
		u32 textMargin,
		Math::Vec2Int widgetPos,
		Math::Vec2 point)
	{
		return DA_CheckHitTab(
			ctx,
			{ tabs.data(), tabs.size() },
			textMargin,
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

	[[nodiscard]] static Std::Array<Rect, 2> DA_BuildSplitNodeChildRects(
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

	[[nodiscard]] static Rect DA_BuildInnerDockingGizmoRect(
		Rect const& nodeRect,
		DA_InnerDockingGizmo in,
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
		case DA_InnerDockingGizmo::Center:
			// Do nothing
			break;
		case DA_InnerDockingGizmo::Top:
			returnVal.position.y -= gizmoSize;
			break;
		case DA_InnerDockingGizmo::Bottom:
			returnVal.position.y += gizmoSize;
			break;
		case DA_InnerDockingGizmo::Left:
			returnVal.position.x -= gizmoSize;
			break;
		case DA_InnerDockingGizmo::Right:
			returnVal.position.x += gizmoSize;
			break;
		default:
			DENGINE_IMPL_UNREACHABLE();
			break;
		}
		return returnVal;
	}

	static constexpr auto cursorPointerId = static_cast<u8>(-1);
}

bool Gui::impl::DA_WindowNode::DockNewNode(
	DA_Node const* targetNode,
	impl::DA_InnerDockingGizmo gizmo,
	Std::Box<DA_Node>& insertNode)
{
	if (targetNode != this)
		return false;
	DENGINE_IMPL_ASSERT(gizmo == impl::DA_InnerDockingGizmo::Center);
	DENGINE_IMPL_ASSERT(insertNode->GetNodeType() == NodeType::Window);

	Std::Box<DA_Node> oldBox = Std::Move(insertNode);

	DA_WindowNode* oldNode = static_cast<DA_WindowNode*>(oldBox.Get());

	activeTabIndex = tabs.size() + oldNode->activeTabIndex;

	tabs.reserve(tabs.size() + oldNode->tabs.size());
	for (auto& oldTab : oldNode->tabs)
	{
		tabs.push_back(Std::Move(oldTab));
	}

	return true;
}

bool Gui::impl::DA_SplitNode::DockNewNode(
	DA_Node const* targetNode,
	impl::DA_InnerDockingGizmo gizmo,
	Std::Box<DA_Node>& insertNode)
{
/*	DENGINE_IMPL_GUI_ASSERT(a && b);
	bool result = DA_DockNewNode(a, targetNode, gizmo, insertNode);
	if (result)
		return true;

	result = DA_DockNewNode(b, targetNode, gizmo, insertNode);
	if (result)
		return true;*/

	return false;
}

bool Gui::impl::DA_WindowNode::RenderDockingHighlight(
	Rect nodeRect,
	Math::Vec4 highlightColor,
	DA_Node const* hoveredWindow,
	DA_InnerDockingGizmo gizmo,
	DrawInfo& drawInfo) const
{
	/*
	if (this != hoveredWindow)
		return false;

	Rect highlightRect = DA_BuildDockingHighlightRect(nodeRect, gizmo);
	drawInfo.PushFilledQuad(highlightRect, highlightColor);
	*/
	return true;
}

bool Gui::impl::DA_SplitNode::RenderDockingHighlight(
	Rect nodeRect,
	Math::Vec4 highlightColor,
	DA_Node const* hoveredWindow,
	DA_InnerDockingGizmo gizmo,
	DrawInfo& drawInfo) const
{
	DENGINE_IMPL_GUI_ASSERT(a);
	DENGINE_IMPL_GUI_ASSERT(b);

	auto childRects = DA_BuildSplitNodeChildRects(nodeRect, split, dir);

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

bool Gui::impl::DA_WindowNode::RenderLayoutGizmo(
	Rect nodeRect,
	u32 gizmoSize,
	Math::Vec4 gizmoColor,
	DA_Node const* hoveredWindow,
	bool ignoreCenter,
	DrawInfo& drawInfo) const
{
	if (this != hoveredWindow)
		return false;

	using GizmoT = DA_InnerDockingGizmo;
	for (GizmoT gizmo = {}; (int)gizmo < (int)GizmoT::COUNT; gizmo = GizmoT((int)gizmo + 1))
	{
		if (ignoreCenter && gizmo == GizmoT::Center)
			continue;

		Rect gizmoRect = DA_BuildInnerDockingGizmoRect(nodeRect, gizmo, gizmoSize);
		drawInfo.PushFilledQuad(gizmoRect, gizmoColor);
	}
	return true;
}

bool Gui::impl::DA_SplitNode::RenderLayoutGizmo(
	Rect nodeRect,
	u32 gizmoSize,
	Math::Vec4 gizmoColor,
	DA_Node const* hoveredWindow,
	bool ignoreCenter,
	DrawInfo& drawInfo) const
{
	DENGINE_IMPL_GUI_ASSERT(a);
	DENGINE_IMPL_GUI_ASSERT(b);

	auto childRects = DA_BuildSplitNodeChildRects(nodeRect, split, dir);

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

void Gui::impl::DA_WindowNode::InputConnectionLost()
{
	DENGINE_IMPL_GUI_ASSERT(!tabs.empty());
	DENGINE_IMPL_GUI_ASSERT(activeTabIndex < tabs.size());
	auto& tab = tabs[activeTabIndex];
	if (tab.widget)
		tab.widget->InputConnectionLost();
}

void Gui::impl::DA_SplitNode::InputConnectionLost()
{
	DENGINE_IMPL_GUI_ASSERT(a);
	DENGINE_IMPL_GUI_ASSERT(b);

	a->InputConnectionLost();
	b->InputConnectionLost();
}

void Gui::impl::DA_SplitNode::CharEnterEvent(Context& ctx)
{
	DENGINE_IMPL_GUI_ASSERT(a);
	DENGINE_IMPL_GUI_ASSERT(b);
	a->CharEnterEvent(ctx);
	b->CharEnterEvent(ctx);
}

void Gui::impl::DA_SplitNode::CharEvent(Context& ctx, u32 utfValue)
{
	DENGINE_IMPL_GUI_ASSERT(a);
	DENGINE_IMPL_GUI_ASSERT(b);
	a->CharEvent(ctx, utfValue);
	b->CharEvent(ctx, utfValue);
}

void Gui::impl::DA_SplitNode::CharRemoveEvent(Context& ctx)
{
	DENGINE_IMPL_GUI_ASSERT(a);
	DENGINE_IMPL_GUI_ASSERT(b);
	a->CharRemoveEvent(ctx);
	b->CharRemoveEvent(ctx);
}

DockArea::DockArea()
{
}

void DockArea::AddWindow(
	Std::Str title,
	Math::Vec4 color,
	Std::Box<Widget>&& widget)
{
	Math::Vec2Int spawnPos = {};
	if (layers.size() >= 2)
	{
		if (layers.front().rect.position == spawnPos)
		{
			spawnPos.x += 150;
			spawnPos.y += 150;
		}
	}

	layers.emplace(layers.begin(), DockArea::Layer{});
	DockArea::Layer& newLayer = layers.front();

	newLayer.rect = { spawnPos, { 400, 400 } };

	auto* node = new impl::DA_WindowNode;
	newLayer.root = node;

	node->tabs.emplace_back(impl::DA_WindowTab());

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
	sizeHint.minimum = { 400, 400 };
	return sizeHint;
}



















namespace DEngine::Gui::impl
{
	[[nodiscard]] static Std::Opt<DA_InnerDockingGizmo> DA_CheckHitInnerDockingGizmo(
		Rect const& nodeRect,
		u32 gizmoSize,
		bool ignoreCenter,
		Math::Vec2 const& point) noexcept
	{
		Std::Opt<DA_InnerDockingGizmo> gizmoHit;
		using GizmoT = DA_InnerDockingGizmo;
		for (GizmoT gizmo = {}; (int)gizmo < (int)GizmoT::COUNT; gizmo = GizmoT((int)gizmo + 1))
		{
			if (ignoreCenter && gizmo == GizmoT::Center)
				continue;
			Rect gizmoRect = DA_BuildInnerDockingGizmoRect(nodeRect, gizmo, gizmoSize);
			if (gizmoRect.PointIsInside(point))
			{
				gizmoHit = gizmo;
				break;
			}
		}
		return gizmoHit;
	}

	[[nodiscard]] static Std::Opt<DA_InnerDockingGizmo> DA_CheckHitInnerDockingGizmo(
		Rect const& nodeRect,
		u32 gizmoSize,
		Math::Vec2 const& point) noexcept
	{
		return DA_CheckHitInnerDockingGizmo(nodeRect, gizmoSize, false, point);
	}

	[[nodiscard]] static Rect DA_BuildDockingHighlightRect(
		Rect nodeRect,
		DA_InnerDockingGizmo gizmo)
	{
		Rect returnVal = nodeRect;
		switch (gizmo)
		{
			case DA_InnerDockingGizmo::Top:
				returnVal.extent.height = nodeRect.extent.height / 2;
				break;
			case DA_InnerDockingGizmo::Left:
				returnVal.extent.width = nodeRect.extent.width / 2;
				break;
			case DA_InnerDockingGizmo::Bottom:
				returnVal.extent.height = nodeRect.extent.height / 2;
				returnVal.position.y += nodeRect.extent.height / 2;
				break;
			case DA_InnerDockingGizmo::Right:
				returnVal.extent.width = nodeRect.extent.width / 2;
				returnVal.position.x += nodeRect.extent.width / 2;
				break;
			case DA_InnerDockingGizmo::Center:
				break;
			default:
				DENGINE_IMPL_UNREACHABLE();
				break;
		}
		return returnVal;
	}


	struct DA_WindowNodePrimaryRects
	{Rect titlebarRect;
		Rect contentRect;
	};
	[[nodiscard]] static auto BuildWindowNodePrimaryRects(
		Rect const& nodeRect,
		u32 totalLineheight)
	{
		DA_WindowNodePrimaryRects returnValue = {};

		returnValue.titlebarRect = nodeRect;
		returnValue.titlebarRect.extent.height = totalLineheight;

		returnValue.contentRect = nodeRect;
		returnValue.contentRect.position.y += (i32)returnValue.titlebarRect.extent.height;
		returnValue.contentRect.extent.height -= returnValue.titlebarRect.extent.height;

		return returnValue;
	}
}

void Gui::impl::DA_WindowNode::Render(
	DA const& dockArea,
	Rect const& nodeRect,
	Rect const& visibleRect,
	Widget::Render_Params const& params) const
{
	auto& textManager = params.textManager;
	auto& transientAlloc = params.transientAlloc;
	auto& drawInfo = params.drawInfo;

	auto const lineheight = textManager.GetLineheight();
	auto const totalTabHeight = lineheight + dockArea.tabTextMargin * 2;

	auto const& mainWindowNodeRects = impl::BuildWindowNodePrimaryRects(
		nodeRect,
		totalTabHeight);

	DENGINE_IMPL_GUI_ASSERT(!tabs.empty());
	auto& activeTab = tabs[activeTabIndex];

	Math::Vec4 contentBackgroundColor = activeTab.color;
	contentBackgroundColor *= 0.5f;
	contentBackgroundColor.w = 1.f;
	drawInfo.PushFilledQuad(mainWindowNodeRects.contentRect, contentBackgroundColor);

	auto const visibleIntersection = Rect::Intersection(mainWindowNodeRects.contentRect, visibleRect);
	if (activeTab.widget && !visibleIntersection.IsNothing())
	{
		auto const& child = *activeTab.widget;
		child.Render2(
			params,
			mainWindowNodeRects.contentRect,
			visibleIntersection);
	}

	// Render the titlebar background
	drawInfo.PushFilledQuad(mainWindowNodeRects.titlebarRect, activeTab.color);

	/*int const tabCount = (int)tabs.size();

	auto tabWidths = Std::MakeVec<u32>(transientAlloc);
	tabWidths.Resize(tabs.size());
	for (int i = 0; i < tabCount; i += 1)
	{

	}*/
}

void Gui::impl::DA_SplitNode::Render(
	DA const& dockArea,
	Rect const& nodeRect,
	Rect const& visibleRect,
	Widget::Render_Params const& params) const
{
	auto test = impl::DA_BuildSplitNodeChildRects(nodeRect, split, dir);

	DENGINE_IMPL_GUI_ASSERT(a);
	DENGINE_IMPL_GUI_ASSERT(b);

	a->Render(
		dockArea,
		test[0],
		visibleRect,
		params);

	b->Render(
		dockArea,
		test[1],
		visibleRect,
		params);
}

namespace DEngine::Gui::impl {
	// Get const or non-const DA_Node type based on input const
	template<class T>
	using LayerT = Std::Trait::Cond<Std::Trait::isConst<T>, Layer const, Layer>;
	template<class T>
	using NodeT = Std::Trait::Cond<Std::Trait::isConst<T>, DA_Node const, DA_Node>;
	template<class T>
	using NodePtrT = Std::Trait::Cond<Std::Trait::isConst<T>, DA_Node const* const, DA_Node*>;
	template<class T>
	using SplitNodeT = Std::Trait::Cond<Std::Trait::isConst<T>, DA_SplitNode const, DA_SplitNode>;
	template<class T>
	using SplitNodePtrT = Std::Trait::Cond<Std::Trait::isConst<T>, DA_SplitNode const *const, DA_SplitNode const>;
	template<class T>
	using WindowNodeT = Std::Trait::Cond<Std::Trait::isConst<T>, DA_WindowNode const, DA_WindowNode>;
}

struct DA::Impl {
	// This should not be done immediately. Do it after iteration.
	static void DA_PushLayerToFront(DockArea& dockArea, uSize indexToPush)
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

	struct DA_LayerEndIt {};
	template<class DA_T>
	struct DA_LayerIt_Result_Inner;
	using DA_LayerConstIt_Result = DA_LayerIt_Result_Inner<DA const>;
	template<class DA_T>
	struct DA_LayerIt_Inner;
	template<class DA_T>
	struct DA_LayerItPair_Inner;
	using DA_LayerItPair = DA_LayerItPair_Inner<DA>;
	using DA_LayerConstItPair = DA_LayerItPair_Inner<DA const>;

	[[nodiscard]] static DA_LayerItPair_Inner<DA> DA_BuildLayerItPair(DA& dockArea) noexcept;

	[[nodiscard]] static DA_LayerItPair_Inner<DA const> DA_BuildLayerItPair(DA const& dockArea) noexcept;

	[[nodiscard]] static Rect DA_BuildLayerRect(
		Std::Span<DA::Layer const> layers,
		uSize layerIndex,
		Rect const& widgetRect);

	[[nodiscard]] static Rect DA_BuildLayerRect(
		DA_LayerIt_Result_Inner<DA const> const& itResult,
		Rect const& widgetRect) noexcept;

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
		Std::FrameAlloc& transientAlloc;
		Rect const& widgetRect;
		Rect const& visibleRect;
		DA_PointerMove_Pointer const& pointer;
		Widget::CursorMoveParams const& params;
	};

	[[nodiscard]] static bool DA_PointerMove(
		DA_PointerMove_Params2 const& params,
		bool pointerOccluded);

	struct DA_PointerMove_Return
	{
		bool pointerOccluded;
		Std::Opt<Math::Vec2Int> frontLayerFrontPos;
	};

	[[nodiscard]] static DA_PointerMove_Return DA_PointerMove_StateNormal(
		DA_PointerMove_Params2 const& params,
		bool pointerOccluded);
	[[nodiscard]] static DA_PointerMove_Return DA_PointerMove_StateMoving(
		DA_PointerMove_Params2 const& params,
		bool pointerOccluded);
	[[nodiscard]] static DA_PointerMove_Return DA_PointerMove_StateResizingSplit(
		DA_PointerMove_Params2 const& params,
		bool pointerOccluded);

	struct DA_PointerMove_StateMoving_UpdateHoveredWindow_Params
	{
		DA& dockArea;
		impl::DA_Node **nodePtr;
		Rect const& layerRect;
		Rect const& visibleRect;
		uSize layerIndex;
		DA_PointerMove_Pointer const& pointer;
		Std::FrameAlloc& transientAlloc;
	};

	static void DA_PointerMove_StateMoving_UpdateHoveredWindow(
		DA_PointerMove_StateMoving_UpdateHoveredWindow_Params const& params);

	struct DA_PointerMove_StateMoving_DispatchChild_Params
	{
		impl::DA_WindowNode& node;
		Rect const& nodeRect;
		Rect const& visibleRect;
		u32 totalTabHeight;
		DA_PointerMove_Pointer const& pointer;
		bool pointerOccluded;
		Widget::CursorMoveParams const& params;
	};

	[[nodiscard]] static void DA_PointerMove_StateMoving_DispatchChild(
		DA_PointerMove_StateMoving_DispatchChild_Params const& params);

	struct DA_PointerPress_Pointer
	{
		u8 id;
		Math::Vec2 pos;
		bool pressed;
	};
	struct DA_PointerPress_Params2
	{
		DockArea& dockArea;
		Std::FrameAlloc& transientAlloc;
		RectCollection const& rectCollection;
		TextManager& textManager;
		Rect const& widgetRect;
		Rect const& visibleRect;
		DA_PointerPress_Pointer const& pointer;
		Widget::CursorPressParams const& params;
	};
	// Returns true if event was consumed
	struct DA_PointerPress_Result
	{
		bool eventConsumed = false;
		struct DockingJob
		{
			uSize layerIndex;
			impl::DA_InnerDockingGizmo gizmo;
			impl::DA_WindowNode *targetNode;
		};
		Std::Opt<DockingJob> dockingJobOpt;
		Std::Opt<DockArea::StateDataT> newStateJob;
		Std::Opt<uSize> pushLayerToFrontJob;
	};


	[[nodiscard]] static bool DA_PointerPress(
		DA_PointerPress_Params2 const& params,
		bool eventConsumed);
	[[nodiscard]] static DA_PointerPress_Result DA_PointerPress_StateNormal(
		DA_PointerPress_Params2 const& params,
		bool eventConsumed);
	[[nodiscard]] static DA_PointerPress_Result DA_PointerPress_StateMoving(
		DA_PointerPress_Params2 const& params,
		bool eventConsumed);
	[[nodiscard]] static DA_PointerPress_Result DA_PointerPress_StateResizingSplit(
		DA_PointerPress_Params2 const& params,
		bool eventConsumed);

	static void DockNode(
		DockArea& dockArea,
		DA_PointerPress_Result::DockingJob const& dockingJob,
		Std::FrameAlloc& transientAlloc);

	static void Render_DrawWindowGizmos(
		DockArea const& dockArea,
		DA::State_Moving::HoveredWindow const& hoveredWindow,
		Rect const& widgetRect,
		Widget::Render_Params const& params);



	template<class T, bool includeRect>
	struct DA_NodeIt_Result_Inner;
	struct DA_Node_EndIt {};
	// DO NOT USE DIRECTLY
	template<class T, bool includeRect>
	struct DA_NodeIt_Inner;
	template<class T, bool includeRect>
	struct DA_NodeIt_Pair_Inner;
	template<bool includeRect>
	using DA_Node_ItPair = DA_NodeIt_Pair_Inner<impl::DA_Node, includeRect>;
	template<bool includeRect>
	using DA_Node_ConstItPair = DA_NodeIt_Pair_Inner<impl::DA_Node const, includeRect>;
};

template<class DA_T>
struct DA::Impl::DA_LayerIt_Result_Inner
{
	DA_T& dockArea;
	impl::NodePtrT<DA_T>* parentPtrToNode;
	impl::NodeT<DA_T>& node;
	int layerIndex = 0;
	// Needs to be here for checking if we are the back layer.
	int layerCount = 0;

	DA_LayerIt_Result_Inner(DA_T& dockArea, int layerIndex, int layerCount) noexcept :
		dockArea{ dockArea },
		parentPtrToNode { &dockArea.layers[layerIndex].root },
		node { **parentPtrToNode },
		layerIndex { layerIndex },
		layerCount { layerCount }
	{}

	DA_LayerIt_Result_Inner(DA_LayerIt_Result_Inner<Std::Trait::RemoveConst<DA_T>> const& in) noexcept
		requires(Std::Trait::isConst<DA_T>) :
		dockArea { in.dockArea},
		parentPtrToNode { in.parentPtrToNode},
		node{ in.node },
		layerIndex{ in.layerIndex},
		layerCount{ in.layerCount }
	{}
};

// DO NOT USE DIRECTLY
template<class DA_T>
struct DA::Impl::DA_LayerIt_Inner
{
	DA_T& dockArea;

	int layerCount = 0;
	int currIndex = 0;
	int endIndex = 0;
	bool reverse = false;

	~DA_LayerIt_Inner() = default;

	[[nodiscard]] DA_LayerIt_Result_Inner<DA_T> operator*() const noexcept
	{
		return DA_LayerIt_Result_Inner<DA_T>{ dockArea, currIndex, layerCount };
	}

	[[nodiscard]] bool operator!=(DA_LayerEndIt const& other) const noexcept
	{
		if (!reverse)
			return currIndex < endIndex;
		else
			return currIndex > endIndex;
	}

	auto& operator++() noexcept
	{
		if (!reverse)
			currIndex += 1;
		else
			currIndex -= 1;
		return *this;
	}
};

// DO NOT USE DIRECTLY
template<class DA_T>
struct DA::Impl::DA_LayerItPair_Inner
{
	DA_T& dockArea;
	uSize startIndex = 0;
	uSize endIndex = 0;
	bool reverse = false;

	[[nodiscard]] DA_LayerItPair_Inner Reverse() const noexcept
	{
		DA_LayerItPair_Inner returnVal = *this;
		returnVal.startIndex = endIndex - 1;
		returnVal.endIndex = startIndex - 1;
		returnVal.reverse = !reverse;
		return returnVal;
	}

	[[nodiscard]] auto begin() const noexcept
	{
		DA_LayerIt_Inner<DA_T> returnVal {
			.dockArea = dockArea,};
		returnVal.currIndex = startIndex;
		returnVal.endIndex = endIndex;
		returnVal.reverse = reverse;
		returnVal.layerCount = (int)dockArea.layers.size();
		return returnVal;
	}

	[[nodiscard]] auto end() const noexcept
	{
		return DA_LayerEndIt{};
	}
};

auto DA::Impl::DA_BuildLayerItPair(DA& dockArea) noexcept -> DA_LayerItPair
{
	DA_LayerItPair returnValue {
		.dockArea = dockArea,};
	returnValue.startIndex = 0;
	returnValue.endIndex = dockArea.layers.size();
	return returnValue;
}

auto DA::Impl::DA_BuildLayerItPair(DA const& dockArea) noexcept -> DA_LayerConstItPair
{
	DA_LayerConstItPair returnVal = {
		.dockArea = dockArea};
	returnVal.startIndex = 0;
	returnVal.endIndex = dockArea.layers.size();
	return returnVal;
}

Rect DA::Impl::DA_BuildLayerRect(
	Std::Span<DA::Layer const> layers,
	uSize layerIndex,
	Rect const& widgetRect)
{
	DENGINE_IMPL_GUI_ASSERT(layerIndex < layers.Size());

	// If we're on the rear-most layer, we want it to have
	// the full widget size.
	Rect returnValue = widgetRect;
	if (layers.Size() > 1 && layerIndex < layers.Size() - 1)
	{
		returnValue = layers[layerIndex].rect;
		returnValue.position += widgetRect.position;
	}

	// Clamp position to widgetRects size
	returnValue.position.x = Math::Min(
		returnValue.position.x,
		widgetRect.Right() - (i32)returnValue.extent.width);
	returnValue.position.x = Math::Max(
		returnValue.position.x,
		widgetRect.Left());

	returnValue.position.y = Math::Min(
		returnValue.position.y,
		widgetRect.Bottom() - (i32)returnValue.extent.height);
	returnValue.position.y = Math::Max(
		returnValue.position.y,
		widgetRect.Top());

	return returnValue;
}

Rect DA::Impl::DA_BuildLayerRect(
	DA_LayerConstIt_Result const& itResult,
	Rect const& widgetRect) noexcept
{
	return DA_BuildLayerRect(
		{ itResult.dockArea.layers.data(), itResult.dockArea.layers.size() },
		itResult.layerIndex,
		widgetRect);
}

template<class T, bool includeRect>
struct DA::Impl::DA_NodeIt_Result_Inner
{
	impl::NodePtrT<T>& parentPtrToNode;
	impl::NodeT<T>& node;
	Rect rect;

	explicit DA_NodeIt_Result_Inner(impl::NodePtrT<T>* parentPtr, Rect const& rect) :
		parentPtrToNode{ *parentPtr },
		node{ *parentPtrToNode },
		rect{ rect }
	{}
};

template<class T>
struct DA::Impl::DA_NodeIt_Result_Inner<T, false>
{
	impl::NodePtrT<T>& parentPtrToNode;
	impl::NodeT<T>& node;

	explicit DA_NodeIt_Result_Inner(impl::NodePtrT<T>* parentPtr) :
		parentPtrToNode{ *parentPtr },
		node{ *parentPtrToNode }
	{}
};

// DO NOT USE DIRECTLY
template<class T, bool includeRect>
struct DA::Impl::DA_NodeIt_Inner
{
	struct StackElement
	{
		// This always points to a split node.
		impl::NodePtrT<T>* nodePtr;
		Rect rect;
	};
	Std::Vec<StackElement, Std::FrameAlloc> stack;
	Rect layerRect = {};
	// This is always a window node.
	impl::NodePtrT<T>* nextNodePtr = nullptr;
	Rect nextRect = {};

	explicit DA_NodeIt_Inner(impl::NodePtrT<T>* topNode, Rect layerRectIn, Std::FrameAlloc& transientAlloc) :
		stack{ transientAlloc },
		layerRect{ layerRectIn }
	{
		// Just reserve some memory.
		stack.Reserve(5);

		impl::NodePtrT<T>* tempNodePtr = topNode;
		Rect tempRect = layerRect;

		nextNodePtr = tempNodePtr;
		nextRect = tempRect;
	}

	[[nodiscard]] auto operator*() const noexcept
	{
		if constexpr (includeRect)
			return DA_NodeIt_Result_Inner<T, true>{ nextNodePtr, nextRect };
		else
			return DA_NodeIt_Result_Inner<T, false>{ nextNodePtr };
	}

	[[nodiscard]] bool operator!=(DA_Node_EndIt const& other) const noexcept
	{
		// If our pointer is valid, keep iterating
		return nextNodePtr;
	}

	DA_NodeIt_Inner& operator++() noexcept
	{
		DENGINE_IMPL_GUI_ASSERT(nextNodePtr);
		impl::NodePtrT<T>* tempNodePtr = nextNodePtr;
		Rect tempRect = nextRect;

		// If we are a split-node, we want to move down its
		// A branch.
		if ((*tempNodePtr)->GetNodeType() == impl::NodeType::Split)
		{
			auto& currSplitNode = static_cast<impl::SplitNodeT<T>&>(**tempNodePtr);

			StackElement newElement = {};
			newElement.nodePtr = tempNodePtr;
			newElement.rect = tempRect;
			stack.PushBack(newElement);
			tempNodePtr = &currSplitNode.a;
			// Create the rect of this child
			auto childRects = impl::DA_BuildSplitNodeChildRects(tempRect, currSplitNode.split, currSplitNode.dir);
			tempRect = childRects[0];
		}
		else // We are a windownode. Jump upwards.
		{
			StackElement prevStackElement = {};
			impl::SplitNodeT<T>* prevSplitNode = nullptr;
			while (!stack.Empty())
			{
				prevStackElement = stack[stack.Size() - 1];
				prevSplitNode = static_cast<impl::SplitNodeT<T>*>(*prevStackElement.nodePtr);
				if (tempNodePtr == &prevSplitNode->a)
				{
					tempNodePtr = &prevSplitNode->b;
					// Create the rect of this child
					auto childRects = impl::DA_BuildSplitNodeChildRects(
						prevStackElement.rect,
						prevSplitNode->split,
						prevSplitNode->dir);
					tempRect = childRects[1];
					break;
				}
				else
				{
					DENGINE_IMPL_GUI_ASSERT(tempNodePtr == &prevSplitNode->b);
					tempNodePtr = prevStackElement.nodePtr;
					tempRect = prevStackElement.rect;
					stack.EraseBack();
				}
			}
			if (stack.Empty())
				tempNodePtr = nullptr;
		}

		nextNodePtr = tempNodePtr;
		nextRect = tempRect;

		return *this;
	}
};

template<class T, bool includeRect>
struct DA::Impl::DA_NodeIt_Pair_Inner
{
	impl::NodePtrT<T>* rootNode;
	Rect rootRect = {};
	Std::FrameAlloc& transientAlloc;

	[[nodiscard]] auto begin() const noexcept
	{
		return DA_NodeIt_Inner<T, includeRect>{ rootNode, rootRect, transientAlloc };
	}

	[[nodiscard]] auto end() const noexcept
	{
		return DA_Node_EndIt{};
	}
};

namespace DEngine::Gui::impl
{
	[[nodiscard]] static auto BuildNodeItPair(
		impl::DA_Node** rootNode,
		Rect rootRect,
		Std::FrameAlloc& transientAlloc)
	{
		return DA::Impl::DA_Node_ItPair<true>{
			.rootNode = rootNode,
			.rootRect = rootRect,
			.transientAlloc = transientAlloc };
	}

	[[nodiscard]] static auto BuildNodeItPair(
		impl::DA_Node **rootNode,
		Std::FrameAlloc& transientAlloc)
	{
		return DA::Impl::DA_Node_ItPair<false>{
			.rootNode = rootNode,
			.transientAlloc = transientAlloc };
	}

	[[nodiscard]] static auto BuildNodeItPair(
		impl::DA_Node const* const* rootNode,
		Rect rootRect,
		Std::FrameAlloc& transientAlloc)
	{
		return DA::Impl::DA_Node_ConstItPair<true>{
			.rootNode = rootNode,
			.rootRect = rootRect,
			.transientAlloc = transientAlloc };
	}

	[[nodiscard]] static auto BuildNodeItPair(
		impl::DA_Node const *const *rootNode,
		Std::FrameAlloc& transientAlloc)
	{
		return DA::Impl::DA_Node_ConstItPair<false>{
			.rootNode = rootNode,
			.transientAlloc = transientAlloc };
	}

	[[nodiscard]] static Rect DA_BuildSplitNodeResizeHandleRect(
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
}

auto DA::Impl::DA_PointerMove_StateNormal(
	DA_PointerMove_Params2 const& params,
	bool pointerOccluded)
		-> DA_PointerMove_Return
{
	auto& dockArea = params.dockArea;
 	auto& widgetRect = params.widgetRect;
	auto& visibleRect = params.visibleRect;
	auto& textManager = params.textManager;
	auto& transientAlloc = params.transientAlloc;
	auto& rectCollection = params.rectCollection;
	auto& pointer = params.pointer;

	DA_PointerMove_Return returnValue = {};
	returnValue.pointerOccluded = pointerOccluded;

	auto const totalTabHeight = textManager.GetLineheight() + dockArea.tabTextMargin * 2;

	auto temp = DA_BuildLayerItPair(dockArea);
	auto c = temp.end();
	for (auto b = temp.begin(); b != c; ++b)
	{
		auto const& layer = *b;

		auto const layerRect = DA_BuildLayerRect(
			layer,
			widgetRect);
		auto const pointerInsideLayer =
			layerRect.PointIsInside(pointer.pos) &&
			visibleRect.PointIsInside(pointer.pos);

		for (auto const& itResult : BuildNodeItPair(layer.parentPtrToNode, layerRect, transientAlloc))
		{
			if (itResult.node.GetNodeType() != impl::NodeType::Window)
				continue;

			auto& node = static_cast<impl::DA_WindowNode&>(itResult.node);
			auto& activeTab = node.tabs[node.activeTabIndex];
			auto& nodeRect = itResult.rect;

			auto const& mainWindowNodeRects = impl::BuildWindowNodePrimaryRects(
				nodeRect,
				totalTabHeight);
			auto const& titlebarRect = mainWindowNodeRects.titlebarRect;
			auto const& contentRect = mainWindowNodeRects.contentRect;

			// Check if we are inside the titlebar
			auto const pointerInsideTitlebar =
				titlebarRect.PointIsInside(pointer.pos) &&
				visibleRect.PointIsInside(pointer.pos);

			returnValue.pointerOccluded = returnValue.pointerOccluded || pointerInsideTitlebar;

			// Then dispatch to content
			if (activeTab.widget)
			{
				auto& widget = *activeTab.widget;
				auto const& childRectPair = rectCollection.GetRect(widget);
				auto const widgetReturn = widget.CursorMove(
					params.params,
					childRectPair.widgetRect,
					childRectPair.visibleRect,
					returnValue.pointerOccluded);
				returnValue.pointerOccluded = returnValue.pointerOccluded || widgetReturn;
			}
		}

		returnValue.pointerOccluded = returnValue.pointerOccluded || pointerInsideLayer;
	}

	auto const pointerInsideWidget =
		widgetRect.PointIsInside(pointer.pos) &&
		visibleRect.PointIsInside(pointer.pos);
	returnValue.pointerOccluded = returnValue.pointerOccluded || pointerInsideWidget;

	return returnValue;
}

void DA::Impl::DA_PointerMove_StateMoving_UpdateHoveredWindow(
	DA_PointerMove_StateMoving_UpdateHoveredWindow_Params const& params)
{
	auto& dockArea = params.dockArea;
	auto& nodePtr = params.nodePtr;
	auto& layerIndex = params.layerIndex;
	auto& layerRect = params.layerRect;
	auto& visibleRect = params.visibleRect;
	auto& pointer = params.pointer;
	auto& transientAlloc = params.transientAlloc;

	auto& stateData = dockArea.stateData.Get<DA::State_Moving>();

	// Check if we are hitting any window nodes on this layer
	for (auto const& itResult : BuildNodeItPair(nodePtr, layerRect, transientAlloc))
	{
		if (itResult.node.GetNodeType() != impl::NodeType::Window)
			continue;

		auto const pointerInsideNode =
			itResult.rect.PointIsInside(pointer.pos) &&
			visibleRect.PointIsInside(pointer.pos);
		if (pointerInsideNode)
		{
			DA::State_Moving::HoveredWindow newHoveredWindow = {};
			newHoveredWindow.layerIndex = layerIndex;
			newHoveredWindow.windowNode = &itResult.node;

			// Check if we are inside any docking gizmos
			using GizmoT = impl::DA_InnerDockingGizmo;
			Std::Opt<GizmoT> hitGizmo;
			for (GizmoT gizmo = {}; (int)gizmo < (int)GizmoT::COUNT; gizmo = GizmoT((int)gizmo + 1))
			{
				auto const rect = DA_BuildInnerDockingGizmoRect(itResult.rect, gizmo, dockArea.gizmoSize);
				auto const pointerInsideGizmo =
					rect.PointIsInside(pointer.pos) &&
					visibleRect.PointIsInside(pointer.pos);
				if (pointerInsideGizmo)
				{
					hitGizmo = gizmo;
					break;
				}
			}
			if (hitGizmo.HasValue())
				newHoveredWindow.gizmoHighlightOpt = (int)hitGizmo.Value();

			stateData.hoveredWindowOpt = newHoveredWindow;
			break;
		}
	}
}

void DA::Impl::DA_PointerMove_StateMoving_DispatchChild(
	DA_PointerMove_StateMoving_DispatchChild_Params const& params)
{
	auto& node = params.node;
	auto const& nodeRect = params.nodeRect;
	auto const& visibleRect = params.visibleRect;
	auto const& totalTabHeight = params.totalTabHeight;
	auto const& pointer = params.pointer;
	auto const& pointerOccluded = params.pointerOccluded;

	auto const& mainWindowNodeRects = impl::BuildWindowNodePrimaryRects(
		nodeRect,
		totalTabHeight);
	auto const& titlebarRect = mainWindowNodeRects.titlebarRect;
	auto const& contentRect = mainWindowNodeRects.contentRect;

	auto const pointerInsideTitlebar =
		titlebarRect.PointIsInside(pointer.pos) &&
		visibleRect.PointIsInside(pointer.pos);

	auto newPointerOccluded = pointerOccluded;
	newPointerOccluded = newPointerOccluded || pointerInsideTitlebar;

	auto& activeTab = node.tabs[node.activeTabIndex];
	auto& child = *activeTab.widget;

	using T = Widget::CursorMoveParams;
	if constexpr (Std::Trait::isSame<T, Widget::CursorMoveParams>)
	{
		child.CursorMove(
			params.params,
			contentRect,
			contentRect,
			newPointerOccluded);
	}
	else
	{

	}
}

auto DA::Impl::DA_PointerMove_StateMoving(
	DA_PointerMove_Params2 const& params,
	bool pointerOccluded)
		-> DA_PointerMove_Return
{
	auto& dockArea = params.dockArea;
	auto& transientAlloc = params.transientAlloc;
	auto& textManager = params.textManager;
	auto& widgetRect = params.widgetRect;
	auto& visibleRect = params.visibleRect;
	auto& pointer = params.pointer;

	// If we only have 0-1 layers, we shouldn't be in the moving state
	// to begin with.
	DENGINE_IMPL_GUI_ASSERT(dockArea.layers.size() >= 2);
	auto& stateData = dockArea.stateData.Get<DA::State_Moving>();

	DA_PointerMove_Return returnValue = {};

	// Reset stuff
	stateData.hoveredWindowOpt = Std::nullOpt;

	returnValue.pointerOccluded = pointerOccluded;
	for (auto const& layer : DA_BuildLayerItPair(dockArea))
	{
		auto const& layerIndex = layer.layerIndex;
		auto& root = layer.node;
		auto const layerRect = DA_BuildLayerRect(
			layer,
			widgetRect);

		auto const pointerInsideLayer =
			layerRect.PointIsInside(pointer.pos) &&
			visibleRect.PointIsInside(pointer.pos);

		// If we are the first layer, we just want to
		// move the layer and keep iterating to layers behind
		// us to see if we hovered over a window and display it's
		// layout gizmos
		if (pointer.id == stateData.pointerId && layerIndex == 0)
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

			returnValue.frontLayerFrontPos = a;

			returnValue.pointerOccluded = true;
		}
		else if (
			pointer.id == stateData.pointerId &&
			layerIndex != 0 &&
			pointerInsideLayer &&
			!stateData.hoveredWindowOpt.HasValue())
		{
			DA_PointerMove_StateMoving_UpdateHoveredWindow_Params tempParams = {
				.dockArea = dockArea,
				.nodePtr = layer.parentPtrToNode,
				.layerRect = layerRect,
				.visibleRect = visibleRect,
				.layerIndex = (uSize)layerIndex,
				.pointer = pointer,
				.transientAlloc = transientAlloc, };
			DA_PointerMove_StateMoving_UpdateHoveredWindow(tempParams);
		}

		// Dispatch to child
		/*if (node.GetNodeType() == impl::NodeType::Window)
		{
			auto& windowNode = static_cast<DA_WindowNode&>(node);
			auto const nodeRect = layerRect;
			auto const totalTabHeight = textManager.GetLineheight() + dockArea.tabTextMargin * 2;
			impl::DA_PointerMove_StateMoving_DispatchChild_Params temp = {
				.node = windowNode,
				.nodeRect = nodeRect,
				.visibleRect = visibleRect,
				.totalTabHeight = totalTabHeight,
				.pointer = pointer,
				.pointerOccluded = newPointerOccluded,
				.params = params.params,
			};
			impl::DA_PointerMove_StateMoving_DispatchChild(temp);
		}*/

		returnValue.pointerOccluded = returnValue.pointerOccluded || pointerInsideLayer;
	}

	auto const pointerInsideWidget =
		widgetRect.PointIsInside(pointer.pos) &&
		visibleRect.PointIsInside(pointer.pos);
	returnValue.pointerOccluded = returnValue.pointerOccluded || pointerInsideWidget;

	return returnValue;
}

auto DA::Impl::DA_PointerMove_StateResizingSplit(
	DA_PointerMove_Params2 const& params,
	bool pointerOccluded)
		-> DA_PointerMove_Return
{
	auto& dockArea = params.dockArea;
	auto& transientAlloc = params.transientAlloc;
	auto& textManager = params.textManager;
	auto& widgetRect = params.widgetRect;
	auto& visibleRect = params.visibleRect;
	auto& pointer = params.pointer;

	// If we only have 0-1 layers, we shouldn't be in the moving state
	// to begin with.
	auto& stateData = dockArea.stateData.Get<DA::State_ResizingSplit>();

	DA_PointerMove_Return returnValue = {};
	returnValue.pointerOccluded = pointerOccluded;





	{
		// Apply resize
		auto const layerIndex = stateData.resizingBack ? dockArea.layers.size() - 1 : 0;
		auto const layerRect = DA_BuildLayerRect(
			{ dockArea.layers.data(), dockArea.layers.size() },
			layerIndex,
			widgetRect);

		auto rootPtr = &dockArea.layers[layerIndex].root;
		for (auto const& nodeItResult : BuildNodeItPair(rootPtr, layerRect, transientAlloc))
		{
			if (nodeItResult.node.GetNodeType() != impl::NodeType::Split)
				continue;
			if (&nodeItResult.node != stateData.splitNode)
				continue;

			auto& node = static_cast<impl::DA_SplitNode&>(nodeItResult.node);
			auto const& nodeRect = nodeItResult.rect;

			// Apply new split
			auto const normalizedPointerPos = Math::Vec2{
				(pointer.pos.x - (f32)nodeRect.position.x) / (f32)nodeRect.extent.width,
				(pointer.pos.y - (f32)nodeRect.position.y) / (f32)nodeRect.extent.height };
			node.split =
				node.dir == impl::DA_SplitNode::Direction::Horizontal ?
					normalizedPointerPos.x :
					normalizedPointerPos.y;

			node.split = Math:: Clamp(node.split, 0.1f, 0.9f);

			break;
		}
	}




	return returnValue;
}

bool DA::Impl::DA_PointerMove(
	DA_PointerMove_Params2 const& params,
	bool pointerOccluded)
{
	auto& dockArea = params.dockArea;

	DA_PointerMove_Return temp = {};

	if (dockArea.stateData.IsA<DockArea::State_Normal>())
		temp = DA_PointerMove_StateNormal(params, pointerOccluded);
	else if (dockArea.stateData.IsA<DockArea::State_Moving>())
		temp = DA_PointerMove_StateMoving(params, pointerOccluded);
	else if (dockArea.stateData.IsA<DockArea::State_ResizingSplit>())
		temp = DA_PointerMove_StateResizingSplit(params, pointerOccluded);
	else
		DENGINE_IMPL_GUI_UNREACHABLE();

	if (temp.frontLayerFrontPos.HasValue())
	{
		auto const newFrontPos = temp.frontLayerFrontPos.Value();
		auto& frontLayerRect = dockArea.layers.front().rect;
		frontLayerRect.position = newFrontPos;
	}

	return temp.pointerOccluded;
}

auto DA::Impl::DA_PointerPress_StateNormal(
	DA_PointerPress_Params2 const& params,
	bool eventConsumed)
		-> DA_PointerPress_Result
{
	auto& dockArea = params.dockArea;
	auto& textManager = params.textManager;
	auto& rectCollection = params.rectCollection;
	auto& widgetRect = params.widgetRect;
	auto& visibleRect = params.visibleRect;
	auto& transientAlloc = params.transientAlloc;
	auto& pointer = params.pointer;

	auto const lineheight = textManager.GetLineheight();
	auto const totalTabHeight = lineheight + dockArea.tabTextMargin * 2;

	DA_PointerPress_Result returnValue = {};
	returnValue.eventConsumed = eventConsumed;

	for (auto const& layer : DA_BuildLayerItPair(dockArea))
	{
		auto const& layerIndex = layer.layerIndex;
		auto const& layerCount = layer.layerCount;
		auto const& layerRect = DA_BuildLayerRect(layer, widgetRect);
		auto const& pointerInsideLayer =
			layerRect.PointIsInside(pointer.pos) &&
			visibleRect.PointIsInside(pointer.pos);

		// If the event is not consumed and we down-pressed, on a layer that is not the front-most and not the rear-most
		// then we push this layer to front. AND we have not already found a layer to push to front
		auto const pushLayerToFront =
			!returnValue.pushLayerToFrontJob.HasValue() &&
			!returnValue.eventConsumed &&
			pointer.pressed &&
			pointerInsideLayer &&
			layerIndex != 0 &&
			layerIndex != layerCount - 1;
		if (pushLayerToFront)
			returnValue.pushLayerToFrontJob = layerIndex;

		// First we check if we hit a resize handle
		if (pointer.pressed && pointerInsideLayer)
		{
			for (auto const& itResult : impl::BuildNodeItPair(layer.parentPtrToNode, layerRect, transientAlloc))
			{
				if (itResult.node.GetNodeType() != impl::NodeType::Split)
					continue;

				auto& node = static_cast<impl::DA_SplitNode&>(itResult.node);
				auto const resizeHandle = impl::DA_BuildSplitNodeResizeHandleRect(
					itResult.rect,
					dockArea.resizeHandleThickness,
					dockArea.resizeHandleLength,
					node.split,
					node.dir);
				auto const pointerInsideHandle =
					resizeHandle.PointIsInside(pointer.pos) &&
					visibleRect.PointIsInside(pointer.pos);
				auto const startResizing =
					pointerInsideHandle &&
					!returnValue.eventConsumed;
				if (startResizing)
				{
					returnValue.eventConsumed = true;
					DA::State_ResizingSplit newStateJob = {};
					newStateJob.splitNode = &node;
					newStateJob.pointerId = pointer.id;
					newStateJob.resizingBack = layerIndex == (layerCount - 1);
					returnValue.newStateJob = newStateJob;

					// We started resizing, we don't need to see if we hit any other
					// resize handles.
					break;
				}
			}
		}

		for (auto const& itResult : impl::BuildNodeItPair(layer.parentPtrToNode, layerRect, transientAlloc))
		{
			if (itResult.node.GetNodeType() != impl::NodeType::Window)
				continue;

			auto& node = static_cast<impl::DA_WindowNode&>(itResult.node);
			auto& activeTab = node.tabs[node.activeTabIndex];
			auto const& nodeRect = itResult.rect;
			auto const& mainWindowNodeRects = impl::BuildWindowNodePrimaryRects(nodeRect, totalTabHeight);
			auto const& titlebarRect = mainWindowNodeRects.titlebarRect;
			auto const& contentRect = mainWindowNodeRects.contentRect;

			auto const pointerInsideTitlebar =
				titlebarRect.PointIsInside(pointer.pos) &&
				visibleRect.PointIsInside(pointer.pos);
			auto goToMovingState =
				!returnValue.eventConsumed &&
				pointerInsideTitlebar &&
				pointer.pressed &&
				layerIndex != layerCount - 1;
			if (goToMovingState)
			{
				returnValue.eventConsumed = true;

				DockArea::State_Moving newState = {};
				newState.pointerId = pointer.id;
				newState.movingSplitNode = false;
				newState.pointerOffset = pointer.pos - Math::Vec2{ (f32)layerRect.position.x, (f32)layerRect.position.y };
				returnValue.newStateJob = newState;
			}

			// Then dispatch to content
			if (activeTab.widget)
			{
				auto& widget = *activeTab.widget;
				auto const& rectPair = rectCollection.GetRect(widget);
				auto tempChildReturn = widget.CursorPress2(
					params.params,
					rectPair.widgetRect,
					contentRect,
					returnValue.eventConsumed);
				returnValue.eventConsumed = returnValue.eventConsumed || tempChildReturn;
			}
		}

		returnValue.eventConsumed = returnValue.eventConsumed || pointerInsideLayer;
	}

	return returnValue;
}

auto DA::Impl::DA_PointerPress_StateMoving(
	DA_PointerPress_Params2 const& params,
	bool eventConsumed)
		-> DA_PointerPress_Result
{
	auto& dockArea = params.dockArea;
	auto& widgetRect = params.widgetRect;
	auto& visibleRect = params.visibleRect;
	auto& transientAlloc = params.transientAlloc;
	auto& pointer = params.pointer;

	// If we only have 0-1 layers, we shouldn't be in the moving state
	// to begin with.
	DENGINE_IMPL_GUI_ASSERT(dockArea.layers.size() >= 2);
	auto& stateData = dockArea.stateData.Get<DA::State_Moving>();

	DA_PointerPress_Result returnValue = {};
	returnValue.eventConsumed = eventConsumed;

	for (auto const& layer : DA_BuildLayerItPair(dockArea))
	{
		auto layerIndex = layer.layerIndex;
		auto& root = layer.node;
		auto const& layerRect = DA_BuildLayerRect(layer, widgetRect);
		auto const pointerInsideLayer =
			layerRect.PointIsInside(pointer.pos) &&
			visibleRect.PointIsInside(pointer.pos);

		// If we unpressed, then we want to exit this moving state.
		if (layerIndex == 0 && pointer.id == stateData.pointerId && !pointer.pressed)
		{
			DockArea::State_Normal newState = {};
			returnValue.newStateJob = newState;
		}

		// Check if we hit any of the window docking gizmos
		if (layerIndex != 0 && !returnValue.dockingJobOpt.HasValue())
		{
			for (auto const& itResult : BuildNodeItPair(layer.parentPtrToNode, layerRect, transientAlloc))
			{
				if (itResult.node.GetNodeType() != impl::NodeType::Window)
					continue;

				auto& node = static_cast<impl::DA_WindowNode&>(itResult.node);
				auto const& nodeRect = itResult.rect;

				auto const pointerInsideNode =
					nodeRect.PointIsInside(pointer.pos) &&
					visibleRect.PointIsInside(pointer.pos);

				auto const hitInnerGizmo = impl::DA_CheckHitInnerDockingGizmo(
					nodeRect,
					dockArea.gizmoSize,
					pointer.pos);

				auto const dockNode =
					!returnValue.dockingJobOpt.HasValue() &&
					pointerInsideNode &&
					hitInnerGizmo.HasValue() &&
					!pointer.pressed &&
					stateData.pointerId == pointer.id;
				if (dockNode)
				{
					DA_PointerPress_Result::DockingJob dockingJob = {};
					dockingJob.layerIndex = layerIndex;
					dockingJob.gizmo = hitInnerGizmo.Value();
					dockingJob.targetNode = &node;
					returnValue.dockingJobOpt = dockingJob;
					break;
				}
			}
		}

		returnValue.eventConsumed = returnValue.eventConsumed || pointerInsideLayer;
	}

	return returnValue;
}

auto DA::Impl::DA_PointerPress_StateResizingSplit(
	DA_PointerPress_Params2 const& params,
	bool eventConsumed)
		-> DA_PointerPress_Result
{
	auto& dockArea = params.dockArea;
	auto& widgetRect = params.widgetRect;
	auto& visibleRect = params.visibleRect;
	auto& transientAlloc = params.transientAlloc;
	auto& pointer = params.pointer;

	auto& stateData = dockArea.stateData.Get<DA::State_ResizingSplit>();

	DA_PointerPress_Result returnValue = {};
	returnValue.eventConsumed = eventConsumed;

	if (!pointer.pressed && pointer.id == stateData.pointerId)
	{
		DA::State_Normal newStateData = {};
		returnValue.newStateJob = newStateData;
	}

	return returnValue;
}

void DA::Impl::DockNode(
	DockArea& dockArea,
	DA_PointerPress_Result::DockingJob const& dockingJob,
	Std::FrameAlloc& transientAlloc)
{
	impl::DA_Node** parentPtr = nullptr;
	{
		// Search for the target node that we wanted to dock in
		auto& targetLayer = dockArea.layers[dockingJob.layerIndex];
		for (auto const& itResult : BuildNodeItPair(&targetLayer.root, transientAlloc))
		{
			if (itResult.node.GetNodeType() != impl::NodeType::Window)
				continue;

			auto& node = static_cast<impl::DA_WindowNode&>(itResult.node);

			if (&node == dockingJob.targetNode)
			{
				// Grab the reference to the parents pointer
				parentPtr = &itResult.parentPtrToNode;
				break;
			}
		}
		DENGINE_IMPL_GUI_ASSERT(parentPtr);
	}

	auto prevFrontLayer = Std::Move(dockArea.layers[0]);

	auto const dockingIsHoriz =
		dockingJob.gizmo == impl::DA_InnerDockingGizmo::Left ||
		dockingJob.gizmo == impl::DA_InnerDockingGizmo::Right;
	auto const dockingIsVert =
		dockingJob.gizmo == impl::DA_InnerDockingGizmo::Top ||
		dockingJob.gizmo == impl::DA_InnerDockingGizmo::Bottom;
	if (dockingIsHoriz || dockingIsVert)
	{
		auto* newSplit = new impl::DA_SplitNode;
		(*parentPtr) = newSplit;

		newSplit->dir = dockingIsHoriz ? impl::DA_SplitNode::Direction::Horizontal : impl::DA_SplitNode::Direction::Vertical;
		auto const dockInA =
			dockingJob.gizmo == impl::DA_InnerDockingGizmo::Left ||
			dockingJob.gizmo == impl::DA_InnerDockingGizmo::Top;
		if (dockInA)
		{
			newSplit->a = prevFrontLayer.root;
			newSplit->b = dockingJob.targetNode;
		}
		else
		{
			newSplit->a = dockingJob.targetNode;
			newSplit->b = prevFrontLayer.root;
		}
	}
	else
	{
		DENGINE_IMPL_GUI_UNREACHABLE();
	}

	prevFrontLayer.root = nullptr;
	dockArea.layers.erase(dockArea.layers.begin());
}

bool DA::Impl::DA_PointerPress(
	DA_PointerPress_Params2 const& params,
	bool eventConsumed)
{
	auto& dockArea = params.dockArea;
	auto const& stateData = dockArea.stateData;
	auto& transientAlloc = params.transientAlloc;

	auto newConsumed = eventConsumed;

	DA_PointerPress_Result temp = {};
	if (stateData.IsA<DockArea::State_Normal>())
		temp = DA_PointerPress_StateNormal(params, newConsumed);
	else if (stateData.IsA<DockArea::State_Moving>())
		temp = DA_PointerPress_StateMoving(params, newConsumed);
	else if (stateData.IsA<DockArea::State_ResizingSplit>())
		temp = DA_PointerPress_StateResizingSplit(params, newConsumed);
	else
		DENGINE_IMPL_GUI_UNREACHABLE();
	newConsumed = temp.eventConsumed;

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
		dockArea.stateData = Std::Move(newState);
	}

	return newConsumed;
}

bool DockArea::CursorMove(
	Widget::CursorMoveParams const& params,
	Rect const& widgetRect,
	Rect const& visibleRect,
	bool occluded)
{
	Impl::DA_PointerMove_Pointer pointer = {};
	pointer.id = impl::cursorPointerId;
	pointer.pos = { (f32)params.event.position.x, (f32)params.event.position.y };

	Impl::DA_PointerMove_Params2 temp {
		.dockArea = *this,
		.rectCollection = params.rectCollection,
		.textManager = params.textManager,
		.transientAlloc = params.transientAlloc,
		.widgetRect = widgetRect,
		.visibleRect = visibleRect,
		.pointer = pointer,
		.params = params };

	return Impl::DA_PointerMove(
		temp,
		occluded);
}

bool DockArea::CursorPress2(
	Widget::CursorPressParams const& params,
	Rect const& widgetRect,
	Rect const& visibleRect,
	bool consumed)
{
	Impl::DA_PointerPress_Pointer pointer = {};
	pointer.id = impl::cursorPointerId;
	pointer.pos = { (f32)params.cursorPos.x, (f32)params.cursorPos.y };
	pointer.pressed = params.event.pressed;

	Impl::DA_PointerPress_Params2 temp {
		.dockArea = *this,
		.transientAlloc = params.transientAlloc,
		.rectCollection = params.rectCollection,
		.textManager = params.textManager,
		.widgetRect = widgetRect,
		.visibleRect = visibleRect,
		.pointer = pointer,
		.params = params, };
	return Impl::DA_PointerPress(temp, consumed);
}

SizeHint DockArea::GetSizeHint2(Widget::GetSizeHint2_Params const& params) const
{
	auto& dockArea = *this;
	auto& textManager = params.textManager;

	for (auto const layer : Impl::DA_BuildLayerItPair(dockArea))
	{
		for (auto const& itResult : impl::BuildNodeItPair(layer.parentPtrToNode, params.transientAlloc))
		{
			if (itResult.node.GetNodeType() != impl::NodeType::Window)
				continue;

			auto& node = static_cast<impl::DA_WindowNode const&>(itResult.node);
			auto const& activeTab = node.tabs[node.activeTabIndex];
			if (activeTab.widget)
			{
				auto const& widget = *activeTab.widget;
				widget.GetSizeHint2(params);
			}
		}
	}

	SizeHint sizeHint = {};
	sizeHint.expandX = true;
	sizeHint.expandY = true;
	sizeHint.minimum = { 400, 400 };

	params.pusher.Push(*this, sizeHint);

	return sizeHint;
}

void DockArea::BuildChildRects(
	BuildChildRects_Params const& params,
	Rect const& widgetRect,
	Rect const& visibleRect) const
{
	auto& dockArea = *this;
	auto& textManager = params.textManager;
	auto& transientAlloc = params.transientAlloc;

	auto const totalLineheight = textManager.GetLineheight() + (dockArea.tabTextMargin * 2);

	for (auto const& layer : Impl::DA_BuildLayerItPair(dockArea))
	{
		auto const& layerRect = Impl::DA_BuildLayerRect(
			layer,
			widgetRect);
		for (auto const& itResult : impl::BuildNodeItPair(layer.parentPtrToNode, layerRect, transientAlloc))
		{
			if (itResult.node.GetNodeType() != impl::NodeType::Window)
				continue;

			auto& node = static_cast<impl::DA_WindowNode const&>(itResult.node);
			auto const& nodeRect = itResult.rect;
			auto const windowRects = impl::BuildWindowNodePrimaryRects(nodeRect, totalLineheight);
			auto const& activeTab = node.tabs[node.activeTabIndex];
			if (activeTab.widget)
			{
				auto& widget = *activeTab.widget;
				params.pusher.Push(widget, { windowRects.contentRect, visibleRect });
				widget.BuildChildRects(
					params,
					windowRects.contentRect,
					visibleRect);
			}
		}
	}
}

void DA::Impl::Render_DrawWindowGizmos(
	DockArea const& dockArea,
	DA::State_Moving::HoveredWindow const& hoveredWindow,
	Rect const& widgetRect,
	Widget::Render_Params const& params)
{
	auto& transientAlloc = params.transientAlloc;
	auto& drawInfo = params.drawInfo;

	auto const& layer = dockArea.layers[hoveredWindow.layerIndex];
	auto layerRect = DA_BuildLayerRect(
		{ dockArea.layers.data(), dockArea.layers.size() },
		hoveredWindow.layerIndex,
		widgetRect);

	{
		// We now need to find the node we want to display docking gizmos on
		Rect nodeRect = {};
		bool nodeFound = false;
		for (auto const& itResult : impl::BuildNodeItPair(&layer.root, layerRect, transientAlloc))
		{
			if (itResult.node.GetNodeType() != impl::NodeType::Window)
				continue;
			auto& node = static_cast<impl::DA_WindowNode const&>(itResult.node);
			if (&node == hoveredWindow.windowNode)
			{
				nodeRect = itResult.rect;
				nodeFound = true;
				break;
			}
		}
		DENGINE_IMPL_GUI_ASSERT(nodeFound);

		using GizmoT = impl::DA_InnerDockingGizmo;
		if (hoveredWindow.gizmoHighlightOpt.HasValue())
		{
			auto const hoveredGizmo = (GizmoT)hoveredWindow.gizmoHighlightOpt.Value();
			auto const highlightRect = impl::DA_BuildDockingHighlightRect(nodeRect, hoveredGizmo);
			drawInfo.PushFilledQuad(highlightRect, dockArea.colors.dockingHighlight);
		}

		// Iterate over each inner gizmo.
		for (GizmoT gizmo = {}; (int)gizmo < (int)GizmoT::COUNT; gizmo = GizmoT((int)gizmo + 1))
		{
			auto const gizmoRect = impl::DA_BuildInnerDockingGizmoRect(nodeRect, gizmo, dockArea.gizmoSize);
			drawInfo.PushFilledQuad(gizmoRect, dockArea.colors.resizeHandle);
		}
	}
}

void DockArea::Render2(
	Render_Params const& params,
	Rect const& widgetRect,
	Rect const& visibleRect) const
{
	auto& dockArea = *this;
	auto& transientAlloc = params.transientAlloc;
	auto& drawInfo = params.drawInfo;

	auto scopedScissor = DrawInfo::ScopedScissor(
		drawInfo,
		Rect::Intersection(widgetRect, visibleRect));

	for (auto const& layer : Impl::DA_BuildLayerItPair(dockArea).Reverse())
	{
		auto const& layerRect = Impl::DA_BuildLayerRect(
			layer,
			widgetRect);

		for (auto const& itResult : impl::BuildNodeItPair(layer.parentPtrToNode, layerRect, transientAlloc))
		{
			if (itResult.node.GetNodeType() != impl::NodeType::Window)
				continue;

			auto const& node = static_cast<impl::DA_WindowNode const&>(itResult.node);
			auto const& nodeRect = itResult.rect;

			node.Render(
				*this,
				nodeRect,
				visibleRect,
				params);
		}

		for (auto const& itResult : impl::BuildNodeItPair(layer.parentPtrToNode, layerRect, transientAlloc))
		{
			if (itResult.node.GetNodeType() != impl::NodeType::Split)
				continue;

			auto const& node = static_cast<impl::DA_SplitNode const&>(itResult.node);
			auto const& nodeRect = itResult.rect;
			auto const resizeHandleRect = impl::DA_BuildSplitNodeResizeHandleRect(
				nodeRect,
				dockArea.resizeHandleThickness,
				dockArea.resizeHandleLength,
				node.split,
				node.dir);
			drawInfo.PushFilledQuad(resizeHandleRect, dockArea.colors.resizeHandle);
		}
	}


	// If we are in the moving-state, we want to draw gizmos on top of everything else.
	if (auto stateMoving = stateData.ToPtr<DockArea::State_Moving>())
	{
		if (auto hoveredWindow = stateMoving->hoveredWindowOpt.ToPtr())
		{
			Impl::Render_DrawWindowGizmos(
				dockArea,
				*hoveredWindow,
				widgetRect,
				params);

			/*
			if (auto highlight = hoveredWindow->gizmoHighlightOpt.ToPtr())
			{


				bool nodeFound = root.RenderDockingHighlight(
					impl::DA_GetLayerRect(layers, widgetRect, hoveredWindow->layerIndex),
					dockingHighlightColor,
					static_cast<impl::Node const*>(hoveredWindow->windowNode),
					(impl::DA_InnerLayoutGizmo)*highlight,
					drawInfo);
				DENGINE_IMPL_GUI_ASSERT(nodeFound);
			}*/
		}

		Rect const backLayerRect = widgetRect;

		// Iterate over each outer gizmo.
		using GizmoT = impl::DA_OuterLayoutGizmo;
		for (GizmoT gizmo = {}; (int)gizmo < (int)GizmoT::COUNT; gizmo = GizmoT((int)gizmo + 1))
		{
			Rect gizmoRect = impl::DA_GetOuterLayoutGizmoRect(
				backLayerRect,
				gizmo,
				gizmoSize);
			drawInfo.PushFilledQuad(gizmoRect, colors.resizeHandle);
		}

		// Draw the delete gizmo
		Rect deleteGizmoRect = impl::DA_GetDeleteGizmoRect(backLayerRect, gizmoSize);
		drawInfo.PushFilledQuad(deleteGizmoRect, colors.deleteLayerGizmo);

		// We can't highlight the outer gizmo AND the window-gizmo
		DENGINE_IMPL_GUI_ASSERT(!(
			stateMoving->backOuterGizmoHighlightOpt.HasValue() &&
			stateMoving->hoveredWindowOpt.HasValue() &&
			stateMoving->hoveredWindowOpt.Value().gizmoHighlightOpt.HasValue()));

		// If we are in the moving-state, we want to draw gizmos on top of everything else.
		if (auto backHighlight = stateMoving->backOuterGizmoHighlightOpt.ToPtr())
		{
			Rect highlightRect = impl::DA_BuildDockingHighlightRect(
				backLayerRect,
				impl::DA_ToInnerLayoutGizmo((impl::DA_OuterLayoutGizmo) *backHighlight));
			drawInfo.PushFilledQuad(highlightRect, colors.dockingHighlight);
		}
	}
}

void DockArea::CursorExit(Context& ctx)
{
	auto& dockArea = *this;
	for (auto const& layer : Impl::DA_BuildLayerItPair(dockArea).Reverse())
	{
		auto& rootNode = layer.node;


		/*if (rootNode.GetNodeType() == impl::NodeType::Window)
		{
			auto& windowNode = static_cast<impl::DA_WindowNode&>(rootNode);
			auto& activeTab = windowNode.tabs[windowNode.activeTabIndex];
			if (activeTab.widget)
			{
				auto& widget = *activeTab.widget;
				widget.CursorExit(ctx);
			}
		}*/
	}
}

void DockArea::CharRemoveEvent(
	Context& ctx,
	Std::FrameAlloc& transientAlloc)
{
	auto& dockArea = *this;
	for (auto const& layerIt : Impl::DA_BuildLayerItPair(dockArea))
	{
		for (auto const& nodeIt : impl::BuildNodeItPair(layerIt.parentPtrToNode, transientAlloc))
		{
			if (nodeIt.node.GetNodeType() != impl::NodeType::Window)
				continue;

			auto& node = static_cast<impl::DA_WindowNode&>(nodeIt.node);
			auto& activeTab = node.tabs[node.activeTabIndex];
			if (activeTab.widget)
			{
				activeTab.widget->CharRemoveEvent(ctx, transientAlloc);
			}
		}
	}
}

void DockArea::TextInput(
	Context& ctx,
	Std::FrameAlloc& transientAlloc,
	TextInputEvent const& event)
{
	auto& dockArea = *this;
	for (auto const& layerIt : Impl::DA_BuildLayerItPair(dockArea))
	{
		for (auto const& nodeIt : impl::BuildNodeItPair(layerIt.parentPtrToNode, transientAlloc))
		{
			if (nodeIt.node.GetNodeType() != impl::NodeType::Window)
				continue;

			auto& node = static_cast<impl::DA_WindowNode&>(nodeIt.node);
			auto& activeTab = node.tabs[node.activeTabIndex];
			if (activeTab.widget)
			{
				activeTab.widget->TextInput(ctx, transientAlloc, event);
			}
		}
	}
}

void DockArea::EndTextInputSession(
	Context& ctx,
	Std::FrameAlloc& transientAlloc,
	EndTextInputSessionEvent const& event)
{
	auto& dockArea = *this;
	for (auto const& layerIt : Impl::DA_BuildLayerItPair(dockArea))
	{
		for (auto const& nodeIt : impl::BuildNodeItPair(layerIt.parentPtrToNode, transientAlloc))
		{
			if (nodeIt.node.GetNodeType() != impl::NodeType::Window)
				continue;

			auto& node = static_cast<impl::DA_WindowNode&>(nodeIt.node);
			auto& activeTab = node.tabs[node.activeTabIndex];
			if (activeTab.widget)
			{
				activeTab.widget->EndTextInputSession(ctx, transientAlloc, event);
			}
		}
	}
}

void DockArea::Layer::Clear()
{
	if (root)
		delete root;
	root = nullptr;
}
