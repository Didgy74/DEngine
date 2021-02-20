#include <DEngine/Gui/DockArea.hpp>

#include <DEngine/Gui/Context.hpp>

#include "ImplData.hpp"

#include <DEngine/Std/Containers/StackVec.hpp>
#include <DEngine/Std/Utility.hpp>

// We have a shorthand for DockArea called DA in this file.

using namespace DEngine;
using namespace DEngine::Gui;
using DA = DockArea;

struct DA::Node
{
	virtual void Render(
		Context const& ctx,
		Extent framebufferExtent,
		Rect nodeRect,
		Rect visibleRect,
		DrawInfo& drawInfo) const = 0;

	struct PointerPress_Params
	{
		Context* ctx = nullptr;
		DA* dockArea = nullptr;
		Rect nodeRect;
		Rect visibleRect;
		uSize layerIndex;

		u8 pointerId;
		Math::Vec2 pointerPos;
		bool pointerPressed;
	};
	// Returns true if event was consumed
	[[nodiscard]] virtual bool PointerPress(PointerPress_Params const& in)
	{
		return false;
	}

	struct PointerMove_Params
	{
		DA* dockArea = nullptr;
		Math::Vec2Int widgetPos;
		uSize layerIndex;

		u8 pointerId;
		Math::Vec2 pointerPos;
	};
	[[nodiscard]] virtual bool PointerMove(PointerMove_Params const& in)
	{
		return false;
	}

	virtual ~Node() {}
};

namespace DEngine::Gui::impl
{
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

		// Returns true if event was consumed
		[[nodiscard]] virtual bool PointerPress(PointerPress_Params const& in) override;

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
	};

	// DO NOT USE DIRECTLY
	template<typename T>
	struct DA_LayerIt
	{
		T* dockArea = nullptr;
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
			auto& layer = dockArea->layers[actualIndex];

			Rect layerRect = layer.rect;
			layerRect.position += widgetRect.position;
			// If we're on the rear-most layer, we want it to have
			// the full widget size.
			if (actualIndex == dockArea->layers.size() - 1)
				layerRect = widgetRect;

			DENGINE_IMPL_GUI_ASSERT(layer.root);
			Result returnVal{
				*layer.root.Get(),
				layerRect,
				actualIndex };

			return returnVal;
		}

		[[nodiscard]] bool operator!=(DA_LayerIt const& other) const noexcept
		{
			DENGINE_IMPL_GUI_ASSERT(dockArea == other.dockArea);
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
		T* dockArea = nullptr;
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
			returnVal.dockArea = dockArea;
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
			returnVal.dockArea = dockArea;
			returnVal.widgetRect = widgetRect;
			returnVal.reverse = reverse;
			if (!reverse)
				returnVal.layerIndex = endIndex;
			else
				returnVal.layerIndex = startIndex;
			return returnVal;
		}
	};

	[[nodiscard]] static DA_LayerItPair<DA> DA_GetLayerItPair(
		DA& dockArea,
		Rect widgetRect) noexcept
	{
		DA_LayerItPair<DA> returnVal{};
		returnVal.dockArea = &dockArea;
		returnVal.widgetRect = widgetRect;
		returnVal.startIndex = 0;
		returnVal.endIndex = dockArea.layers.size();
		return returnVal;
	}

	[[nodiscard]] static DA_LayerItPair<DA const> DA_GetLayerItPair(
		DA const& dockArea, 
		Rect widgetRect) noexcept
	{
		DA_LayerItPair<DA const> returnVal{};
		returnVal.dockArea = &dockArea;
		returnVal.widgetRect = widgetRect;
		returnVal.startIndex = 0;
		returnVal.endIndex = dockArea.layers.size();
		return returnVal;
	}

	// Start-index is inclusive.
	// End-index is exclusive.
	[[nodiscard]] static DA_LayerItPair<DA const> DA_GetLayerItPair(
		DA const& dockArea,
		Rect widgetRect,
		uSize startIndex,
		uSize endIndex) noexcept
	{
		DA_LayerItPair<DA const> returnVal{};
		returnVal.dockArea = &dockArea;
		returnVal.widgetRect = widgetRect;
		returnVal.startIndex = startIndex;
		returnVal.endIndex = endIndex;
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

	// Do not use directly
	struct DA_LayerResizeIt
	{
		DA_ResizeSide current = {};
		Rect layerRect = {};
		u32 handleThickness = 0;
		u32 handleLength = 0;

		struct Result
		{
			DA_ResizeSide side = {};
			Rect rect = {};
		};
		[[nodiscard]] Result operator*() const noexcept
		{
			Result returnVal{};
			returnVal.side = current;
			returnVal.rect = impl::DA_GetResizeHandleRect(
				layerRect, 
				current, 
				handleThickness, 
				handleLength);
			return returnVal;
		}

		[[nodiscard]] bool operator!=(DA_LayerResizeIt const& other) const noexcept
		{
			return current != other.current;
		}

		DA_LayerResizeIt& operator++() noexcept
		{
			current = DA_ResizeSide((int)current + 1);
			return *this;
		}
	};
	struct DA_LayerResizeItPair
	{
		Rect layerRect = {};
		u32 handleThickness = 0;
		u32 handleLength = 0;

		[[nodiscard]] DA_LayerResizeIt begin() const noexcept
		{
			DA_LayerResizeIt returnVal{};
			returnVal.layerRect = layerRect;
			returnVal.handleThickness = handleThickness;
			returnVal.handleLength = handleLength;
			returnVal.current = (impl::DA_ResizeSide)0;
			return returnVal;
		}

		[[nodiscard]] DA_LayerResizeIt end() const noexcept
		{
			DA_LayerResizeIt returnVal{};
			returnVal.layerRect = layerRect;
			returnVal.handleThickness = handleThickness;
			returnVal.handleLength = handleLength;
			returnVal.current = impl::DA_ResizeSide::COUNT;
			return returnVal;
		}
	};

	[[nodiscard]] static DA_LayerResizeItPair DA_GetLayerResizeItPair(
		Rect layerRect,
		u32 handleThickness,
		u32 handleLength) noexcept
	{
		return DA_LayerResizeItPair{ layerRect, handleThickness, handleLength };
	}

	enum class DA_LayoutGizmo : char { Top, Bottom, Left, Right, COUNT };
	[[nodiscard]] Rect DA_GetLayoutGizmoRect(
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
				returnVal.position.y += gizmoSize;
				break;
			case DA_LayoutGizmo::Bottom:
				returnVal.position.x += layerRect.extent.width / 2 - gizmoSize / 2;
				returnVal.position.y += layerRect.extent.height - gizmoSize * 2;
				break;
			case DA_LayoutGizmo::Left:
				returnVal.position.x += gizmoSize;
				returnVal.position.y += layerRect.extent.height / 2 - gizmoSize / 2;
				break;
			case DA_LayoutGizmo::Right:
				returnVal.position.x += layerRect.extent.width - gizmoSize * 2;;
				returnVal.position.y += layerRect.extent.height / 2 - gizmoSize / 2;
				break;
			default:
				DENGINE_DETAIL_UNREACHABLE();
				break;
		}
		return returnVal;
	}

	struct DA_PointerPress_Params
	{
		Context* ctx = nullptr;
		DA* dockArea = nullptr;
		Rect widgetRect;
		Rect visibleRect;

		u8 pointerId;
		Math::Vec2 pointerPos;
		bool pointerPressed;
	};
	static void DA_PointerPress(DA_PointerPress_Params const& in);

	struct DA_PointerMove_Params
	{
		DA* dockArea = nullptr;
		Rect widgetRect;
		Rect visibleRect;
		
		u8 pointerId;
		Math::Vec2 pointerPos;
	};
	static void DA_PointerMove(DA_PointerMove_Params const& in);
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
		Rect backLayerRect = in.widgetRect;
		// Iterate over each gizmo.
		using GizmoT = impl::DA_LayoutGizmo;
		Std::Opt<GizmoT> gizmoHit;
		for (GizmoT gizmo = {}; (int)gizmo < (int)GizmoT::COUNT; gizmo = GizmoT((int)gizmo + 1))
		{
			Rect gizmoRect = impl::DA_GetLayoutGizmoRect(
				backLayerRect,
				gizmo,
				in.dockArea->gizmoSize);
			if (gizmoRect.PointIsInside(in.pointerPos))
			{
				gizmoHit = Std::Opt{ gizmo };
				break;
			}
		}
		if (gizmoHit.HasValue())
		{
			in.dockArea->behaviorData.Set(DA::BehaviorData_Normal{});
			eventConsumed = true;
			GizmoT gizmo = gizmoHit.Value();

			Std::Box<DA::Node> origFrontNode = Std::Move(in.dockArea->layers.front().root);
			in.dockArea->layers.erase(in.dockArea->layers.begin());
			auto& backLayer = in.dockArea->layers.back();
			Std::Box<DA::Node> origBackNode = static_cast<Std::Box<DA::Node>&&>(backLayer.root);

			DA_SplitNode* newNode = new DA_SplitNode;
			backLayer.root = Std::Box<DA::Node>{ newNode };

			newNode->dir = gizmo == GizmoT::Top || gizmo == GizmoT::Bottom ?
				DA_SplitNode::Direction::Vertical :
				DA_SplitNode::Direction::Horizontal;
			newNode->a = Std::Move(gizmo == GizmoT::Top || gizmo == GizmoT::Left ? origFrontNode : origBackNode);
			newNode->b = Std::Move(gizmo == GizmoT::Top || gizmo == GizmoT::Left ? origBackNode : origFrontNode);
		}
	}

	if (!eventConsumed)
	{
		auto const layerItPair = impl::DA_GetLayerItPair(*in.dockArea, in.widgetRect);
		for (auto const& layerItem : layerItPair)
		{
			DA::Node::PointerPress_Params params = {};
			params.ctx = in.ctx;
			params.dockArea = in.dockArea;
			params.layerIndex = layerItem.layerIndex;
			params.nodeRect = layerItem.layerRect;
			params.pointerId = in.pointerId;
			params.pointerPos = in.pointerPos;
			params.pointerPressed = in.pointerPressed;
			params.visibleRect = in.visibleRect;
			eventConsumed = layerItem.rootNode.PointerPress(params);
			if (eventConsumed)
				break;
		}
	}
}

bool impl::DA_WindowNode::PointerPress(PointerPress_Params const& in)
{
	bool cursorInside = in.nodeRect.PointIsInside(in.pointerPos) && in.visibleRect.PointIsInside(in.pointerPos);
	if (!cursorInside)
	{
		return false;
	}

	bool eventConsumed = false;
	if (in.dockArea->behaviorData.IsA<DA::BehaviorData_Normal>())
	{
		if (in.layerIndex == in.dockArea->layers.size() - 1)
			return false;

		// and push this layer to the front, if it's not the rear-most layer.
		if (in.layerIndex != 0 && in.layerIndex != in.dockArea->layers.size() - 1)
			DA_PushLayerToFront(*in.dockArea, in.layerIndex);

		auto& implData = *static_cast<impl::ImplData*>(in.ctx->Internal_ImplData());

		// If we are within the titlebar, we want to transition the
		// dockarea widget into "moving" mode.
		Rect titlebarRect = in.nodeRect;
		titlebarRect.extent.height = implData.textManager.lineheight;
		if (in.pointerPressed && titlebarRect.PointIsInside(in.pointerPos))
		{
			DA::BehaviorData_Moving newBehavior{};
			newBehavior.pointerID = in.pointerId;
			Math::Vec2 temp = in.pointerPos - Math::Vec2{ (f32)in.nodeRect.position.x, (f32)in.nodeRect.position.y };
			newBehavior.pointerOffset = temp;
			in.dockArea->behaviorData.Set(newBehavior);

			eventConsumed = true;
		}
	}
	else if (in.dockArea->behaviorData.IsA<DA::BehaviorData_Moving>())
	{
		auto& behaviorData = in.dockArea->behaviorData.Get<DA::BehaviorData_Moving>();
		if (!in.pointerPressed && in.pointerId == behaviorData.pointerID)
		{
			in.dockArea->behaviorData.Set(DA::BehaviorData_Normal{});
			return true;
		}
	}

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
	}
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
	drawInfo.PushFilledQuad(titlebarRect, activeTab.color);

	{
		uSize tabHorizontalOffset = 0;
		for (auto const& tab : tabs)
		{
			SizeHint tabTitleSizeHint = TextManager::GetSizeHint(implData.textManager, tab.title);
			// Render tabs and their text.
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

bool impl::DA_WindowNode::PointerMove(PointerMove_Params const& in)
{
	bool eventConsumed{};

	if (in.dockArea->behaviorData.IsA<DA::BehaviorData_Moving>())
	{
		if (in.layerIndex == 0)
			eventConsumed = true;

		auto& behaviorMoving = in.dockArea->behaviorData.Get<DA::BehaviorData_Moving>();
		// If we only have 0-1 layer, we shouldn't be in the moving state
		// to begin with.
		DENGINE_IMPL_GUI_ASSERT(in.dockArea->layers.size() > 1);
		if (in.pointerId == behaviorMoving.pointerID)
		{
			Math::Vec2 temp = in.pointerPos;
			temp -= { (f32)in.widgetPos.x, (f32)in.widgetPos.y };
			temp -= behaviorMoving.pointerOffset;
			in.dockArea->layers[in.layerIndex].rect.position = { (i32)temp.x, (i32)temp.y };
		}
	}

	return eventConsumed;
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
	params.dockArea = this;
	params.pointerId = DockArea::cursorPointerID;
	params.pointerPos = { (f32)cursorPos.x, (f32)cursorPos.y };
	params.pointerPressed = event.clicked;
	params.visibleRect = visibleRect;
	params.widgetRect = widgetRect;
	impl::DA_PointerPress(params);
}

void DockArea::CursorMove(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	CursorMoveEvent event)
{
	
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
		params.visibleRect = visibleRect;
		impl::DA_PointerPress(params);
	}
	else if (event.type == TouchEventType::Moved)
	{
		
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