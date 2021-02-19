#include <DEngine/Gui/DockArea.hpp>

#include <DEngine/Gui/Context.hpp>

#include "ImplData.hpp"

#include <DEngine/Std/Containers/StackVec.hpp>

using namespace DEngine;
using namespace DEngine::Gui;

struct DockArea::Node
{
	virtual void Render(
		Context const& ctx,
		Extent framebufferExtent,
		Rect nodeRect,
		Rect visibleRect,
		DrawInfo& drawInfo) const = 0;

	struct CursorClick_Params
	{
		Context* ctx = nullptr;
		WindowID windowId;
		Rect nodeRect;
		Rect visibleRect;
		Math::Vec2Int cursorPos;
		CursorClickEvent event;
		DockArea* dockArea = nullptr;
		uSize layerIndex;
	};
	[[nodiscad]] virtual bool CursorClick(CursorClick_Params const&)
	{
		return false;
	}

	struct CursorMove_Params
	{
		Context* ctx = nullptr;
		WindowID windowId;
		Rect nodeRect;
		Rect visibleRect;
		CursorMoveEvent event;
		DockArea* dockArea = nullptr;
		Rect widgetRect;
		uSize layerIndex;
	};
	[[nodiscad]] virtual bool CursorMove(CursorMove_Params const&)
	{
		return false;
	}

	struct TouchEvent_Params
	{
		Context* ctx = nullptr;
		WindowID windowId;
		Rect nodeRect;
		Rect visibleRect;
		Gui::TouchEvent event;
		DockArea* dockArea = nullptr;
		Rect widgetRect;
		uSize layerIndex;
	};
	[[nodiscad]] virtual bool TouchEvent(TouchEvent_Params const&)
	{
		return false;
	}

	virtual ~Node() {}
};

namespace DEngine::Gui::impl
{
	void DockArea_PushLayerToFront(
		DockArea& dockArea,
		uSize indexToPush)
	{
		DENGINE_IMPL_GUI_ASSERT(indexToPush < dockArea.layers.size());
		if (indexToPush == 0)
			return;

		// First remove the element and store it in a temporary
		DockArea::Layer temp = static_cast<DockArea::Layer&&>(dockArea.layers[indexToPush]);
		dockArea.layers.erase(dockArea.layers.begin() + indexToPush);

		// Insert it at front
		dockArea.layers.insert(dockArea.layers.begin(), static_cast<DockArea::Layer&&>(temp));
	}

	struct DockArea_WindowTab
	{
		std::string title;
		Math::Vec4 color{};
		Std::Box<Widget> widget;
	};

	struct DockArea_WindowNode : public DockArea::Node
	{
		uSize selectedTab = 0;
		std::vector<DockArea_WindowTab> tabs;

		virtual void Render(
			Context const& ctx,
			Extent framebufferExtent,
			Rect nodeRect,
			Rect visibleRect,
			DrawInfo& drawInfo) const override
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

		[[nodiscad]] virtual bool CursorClick(CursorClick_Params const& in) override
		{
			bool returnVal{};

			if (in.event.button == CursorButton::Primary)
			{
				PointerPress_Params params{};
				params.ctx = in.ctx;
				params.dockArea = in.dockArea;
				params.layerIndex = in.layerIndex;
				params.nodeRect = in.nodeRect;
				params.pointerId = DockArea::cursorPointerID;
				params.pointerPos = { (f32)in.cursorPos.x, (f32)in.cursorPos.y };
				params.pressed = in.event.clicked;
				params.visibleRect = in.visibleRect;
				returnVal = PointerPress(params);
			}

			return returnVal;
		}

		[[nodiscad]] virtual bool CursorMove(CursorMove_Params const& in) override
		{
			PointerMove_Params params{};
			params.dockArea = in.dockArea;
			params.layerIndex = in.layerIndex;
			params.pointerId = DockArea::cursorPointerID;
			params.pointerPos = { (f32)in.event.position.x, (f32)in.event.position.y };
			params.widgetPos = in.widgetRect.position;
			return PointerMove(params);
		}

		struct PointerPress_Params
		{
			Context* ctx = nullptr;
			Rect nodeRect;
			Rect visibleRect;

			DockArea* dockArea = nullptr;
			uSize layerIndex;
			u8 pointerId;
			Math::Vec2 pointerPos;
			bool pressed;
		};
		// Returns true if event was consumed
		[[nodiscard]] bool PointerPress(PointerPress_Params const& in)
		{
			bool eventConsumed = false;
			if (in.dockArea->behaviorData.IsA<DockArea::BehaviorData_Normal>())
			{
				bool cursorInside = in.nodeRect.PointIsInside(in.pointerPos) && in.visibleRect.PointIsInside(in.pointerPos);
				if (!cursorInside)
				{
					return false;
				}

				if (in.layerIndex == in.dockArea->layers.size() - 1)
					return false;

				// and push this layer to the front, if it's not the rear-most layer.
				if (in.layerIndex != 0 && in.layerIndex != in.dockArea->layers.size() - 1)
					DockArea_PushLayerToFront(*in.dockArea, in.layerIndex);

				auto& implData = *static_cast<impl::ImplData*>(in.ctx->Internal_ImplData());

				// If we are within the titlebar, we want to transition the
				// dockarea widget into "moving" mode.
				Rect titlebarRect = in.nodeRect;
				titlebarRect.extent.height = implData.textManager.lineheight;
				if (in.pressed && titlebarRect.PointIsInside(in.pointerPos))
				{
					DockArea::BehaviorData_Moving newBehavior{};
					newBehavior.pointerID = in.pointerId;
					Math::Vec2 temp = in.pointerPos - Math::Vec2{ (f32)in.nodeRect.position.x, (f32)in.nodeRect.position.y };
					newBehavior.pointerOffset = temp;
					in.dockArea->behaviorData.Set(newBehavior);

					eventConsumed = true;
				}
			}
			else if (in.dockArea->behaviorData.IsA<DockArea::BehaviorData_Moving>())
			{
				auto& behaviorData = in.dockArea->behaviorData.Get<DockArea::BehaviorData_Moving>();
				if (!in.pressed && in.pointerId == behaviorData.pointerID)
				{
					in.dockArea->behaviorData.Set(DockArea::BehaviorData_Normal{});
					return true;
				}
			}

			return eventConsumed;
		}

		struct PointerMove_Params
		{
			DockArea* dockArea = nullptr;
			Math::Vec2Int widgetPos;
			uSize layerIndex;
			u8 pointerId;
			Math::Vec2 pointerPos;
		};
		[[nodiscard]] bool PointerMove(PointerMove_Params const& in)
		{
			bool eventConsumed{};

			if (in.dockArea->behaviorData.IsA<DockArea::BehaviorData_Moving>())
			{
				if (in.layerIndex == 0)
					eventConsumed = true;

				auto& behaviorMoving = in.dockArea->behaviorData.Get<DockArea::BehaviorData_Moving>();
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


		[[nodiscard]] virtual bool TouchEvent(TouchEvent_Params const& in) override
		{
			bool eventConsumed = false;

			if (in.event.type == TouchEventType::Down || in.event.type == TouchEventType::Up)
			{
				PointerPress_Params params{};
				params.ctx = in.ctx;
				params.dockArea = in.dockArea;
				params.layerIndex = in.layerIndex;
				params.nodeRect = in.nodeRect;
				params.pointerId = in.event.id;
				params.pointerPos = in.event.position;
				params.pressed = in.event.type == TouchEventType::Down;
				params.visibleRect = in.visibleRect;
				eventConsumed = PointerPress(params);
			}
			else if (in.event.type == TouchEventType::Moved)
			{
				PointerMove_Params params{};
				params.dockArea = in.dockArea;
				params.layerIndex = in.layerIndex;
				params.pointerId = in.event.id;
				params.pointerPos = in.event.position;
				params.widgetPos = in.widgetRect.position;
				eventConsumed = PointerMove(params);
			}
			else
				DENGINE_DETAIL_UNREACHABLE();

			return eventConsumed;
		}
	};

	// DO NOT USE DIRECTLY
	template<typename T>
	struct DockArea_LayerIt
	{
		T* dockArea = nullptr;
		Rect widgetRect{};
		uSize layerIndex = 0;
		bool reverse = false;

		// For lack of a better name...
		[[nodiscard]] uSize GetActualIndex() const noexcept 
		{
			return !reverse ? layerIndex : layerIndex - 1;
		}

		// DO NOT USE DIRECTLY
		struct Result
		{
			using NodeT = Std::Trait::Cond<Std::Trait::isConst<T>, DockArea::Node const, DockArea::Node>;
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

			Result returnVal{
				*layer.root.Get(),
				layerRect,
				actualIndex };

			return returnVal;
		}

		[[nodiscard]] bool operator!=(DockArea_LayerIt const& other) const noexcept
		{
			DENGINE_IMPL_GUI_ASSERT(dockArea == other.dockArea);
			DENGINE_IMPL_GUI_ASSERT(reverse == other.reverse);
			return layerIndex != other.layerIndex;
		}

		DockArea_LayerIt& operator++() noexcept
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
	struct DockArea_LayerItPair
	{
		T* dockArea = nullptr;
		Rect widgetRect{};
		uSize startIndex = 0;
		uSize endIndex = 0;
		bool reverse = false;

		[[nodiscard]] DockArea_LayerItPair<T> Reverse() const noexcept
		{
			DockArea_LayerItPair returnVal = *this;
			returnVal.reverse = !reverse;
			return returnVal;
		}

		[[nodiscard]] DockArea_LayerIt<T> begin() const noexcept
		{
			DockArea_LayerIt<T> returnVal{};
			returnVal.dockArea = dockArea;
			returnVal.widgetRect = widgetRect;
			returnVal.reverse = reverse;
			if (!reverse)
				returnVal.layerIndex = startIndex;
			else
				returnVal.layerIndex = endIndex;
			return returnVal;
		}

		[[nodiscard]] DockArea_LayerIt<T> end() const noexcept
		{
			DockArea_LayerIt<T> returnVal{};
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

	[[nodiscard]] static DockArea_LayerItPair<DockArea> DockArea_GetLayerItPair(
		DockArea& dockArea,
		Rect widgetRect) noexcept
	{
		DockArea_LayerItPair<DockArea> returnVal{};
		returnVal.dockArea = &dockArea;
		returnVal.widgetRect = widgetRect;
		returnVal.startIndex = 0;
		returnVal.endIndex = dockArea.layers.size();
		return returnVal;
	}

	[[nodiscard]] static DockArea_LayerItPair<DockArea const> DockArea_GetLayerItPair(
		DockArea const& dockArea, 
		Rect widgetRect) noexcept
	{
		DockArea_LayerItPair<DockArea const> returnVal{};
		returnVal.dockArea = &dockArea;
		returnVal.widgetRect = widgetRect;
		returnVal.startIndex = 0;
		returnVal.endIndex = dockArea.layers.size();
		return returnVal;
	}

	// Start-index is inclusive.
	// End-index is exclusive.
	[[nodiscard]] static DockArea_LayerItPair<DockArea const> DockArea_GetLayerItPair(
		DockArea const& dockArea,
		Rect widgetRect,
		uSize startIndex,
		uSize endIndex) noexcept
	{
		DockArea_LayerItPair<DockArea const> returnVal{};
		returnVal.dockArea = &dockArea;
		returnVal.widgetRect = widgetRect;
		returnVal.startIndex = startIndex;
		returnVal.endIndex = endIndex;
		return returnVal;
	}

	/*
	// DO NOT USE THIS DIRECTLY
	struct DockArea_NodeItResult
	{
		DockArea::Node const& node;
		Rect rect{};
	};

	// DO NOT USE THIS DIRECTLY
	struct DockArea_NodeIt
	{
		struct Inner
		{
			DockArea::Node const* ptr = nullptr;
			Rect rect{};
		};
		Std::StackVec<Inner, 8> stack;

		[[nodiscard]] DockArea_NodeItResult operator*() const noexcept
		{
			Inner const& inner = stack[stack.Size() - 1];
			DockArea_NodeItResult returnVal{
				*inner.ptr,
				inner.rect };
			return returnVal;
		}

		[[nodiscard]] bool operator!=(DockArea_NodeIt other) const noexcept
		{
			return true;
		}

		DockArea_NodeIt& operator++() noexcept
		{
			return *this;
		}
	};

	// DO NOT USE THIS DIRECTLY
	struct DockArea_NodeItPair
	{
		DockArea::Node const* rootNode = nullptr;
		Rect nodeRect{};

		[[nodiscard]] DockArea_NodeIt begin() const noexcept
		{
			DockArea_NodeIt returnVal{};
			DockArea_NodeIt::Inner inner{};
			inner.ptr = rootNode;
			inner.rect = nodeRect;
			returnVal.stack.PushBack(inner);
			return returnVal;
		}

		[[nodiscard]] DockArea_NodeIt end() const noexcept
		{
			return {};
		}
	};

	[[nodiscard]] static DockArea_NodeItPair DockArea_GetNodeItPair(
		DockArea::Node const& node,
		Rect nodeRect)
	{
		DockArea_NodeItPair returnVal{};
		returnVal.rootNode = &node;
		returnVal.nodeRect = nodeRect;
		return returnVal;
	}
	*/

	enum class DockArea_ResizeSide : char { Top, Bottom, Left, Right, COUNT };
	[[nodiscard]] Rect DockArea_GetResizeHandleRect(
		Rect layerRect,
		DockArea_ResizeSide in,
		u32 handleThickness,
		u32 handleLength) noexcept
	{
		Rect returnVal = layerRect;
		switch (in)
		{
			case DockArea_ResizeSide::Top:
			{
				returnVal.position.x += layerRect.extent.width / 2 - handleLength / 2;
				returnVal.position.y -= handleThickness / 2;
				returnVal.extent = { handleLength, handleThickness };
			}
			break;
			case DockArea_ResizeSide::Bottom:
			{
				returnVal.position.x += layerRect.extent.width / 2 - handleLength / 2;
				returnVal.position.y += layerRect.extent.height - handleThickness / 2;
				returnVal.extent = { handleLength, handleThickness };
			}
			break;
			case DockArea_ResizeSide::Left:
			{
				returnVal.position.x -= handleThickness / 2;
				returnVal.position.y += layerRect.extent.height / 2 - handleLength / 2;
				returnVal.extent = { handleThickness, handleLength };
			}
			break;
			case DockArea_ResizeSide::Right:
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
	struct DockArea_LayerResizeIt
	{
		DockArea_ResizeSide current = {};
		Rect layerRect = {};
		u32 handleThickness = 0;
		u32 handleLength = 0;

		struct Result
		{
			DockArea_ResizeSide side = {};
			Rect rect = {};
		};
		[[nodiscard]] Result operator*() const noexcept
		{
			Result returnVal{};
			returnVal.side = current;
			returnVal.rect = impl::DockArea_GetResizeHandleRect(
				layerRect, 
				current, 
				handleThickness, 
				handleLength);
			return returnVal;
		}

		[[nodiscard]] bool operator!=(DockArea_LayerResizeIt const& other) const noexcept
		{
			return current != other.current;
		}

		DockArea_LayerResizeIt& operator++() noexcept
		{
			current = DockArea_ResizeSide((int)current + 1);
			return *this;
		}
	};
	struct DockArea_LayerResizeItPair
	{
		Rect layerRect = {};
		u32 handleThickness = 0;
		u32 handleLength = 0;

		[[nodiscard]] DockArea_LayerResizeIt begin() const noexcept
		{
			DockArea_LayerResizeIt returnVal{};
			returnVal.layerRect = layerRect;
			returnVal.handleThickness = handleThickness;
			returnVal.handleLength = handleLength;
			returnVal.current = (impl::DockArea_ResizeSide)0;
			return returnVal;
		}

		[[nodiscard]] DockArea_LayerResizeIt end() const noexcept
		{
			DockArea_LayerResizeIt returnVal{};
			returnVal.layerRect = layerRect;
			returnVal.handleThickness = handleThickness;
			returnVal.handleLength = handleLength;
			returnVal.current = impl::DockArea_ResizeSide::COUNT;
			return returnVal;
		}
	};

	[[nodiscard]] static DockArea_LayerResizeItPair DockArea_GetLayerResizeItPair(
		Rect layerRect,
		u32 handleThickness,
		u32 handleLength)
	{
		return DockArea_LayerResizeItPair{ layerRect, handleThickness, handleLength };
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
	impl::DockArea_WindowNode* node = new impl::DockArea_WindowNode;
	newLayer.root = Std::Box<DockArea::Node>{ node };
	node->tabs.push_back(impl::DockArea_WindowTab());
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
	auto const layerItPair = impl::DockArea_GetLayerItPair(*this, widgetRect).Reverse();
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
}

void DockArea::CursorClick(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	Math::Vec2Int cursorPos,
	CursorClickEvent event)
{
	bool cursorInside = widgetRect.PointIsInside(cursorPos) && visibleRect.PointIsInside(cursorPos);
	if (!cursorInside)
		return;

	auto const layerItPair = impl::DockArea_GetLayerItPair(*this, widgetRect);
	for (auto const& layerItem : layerItPair)
	{
		Node::CursorClick_Params params{};
		params.ctx = &ctx;
		params.cursorPos = cursorPos;
		params.dockArea = this;
		params.event = event;
		params.layerIndex = layerItem.layerIndex;
		params.nodeRect = layerItem.layerRect;
		params.visibleRect = visibleRect;
		params.windowId = windowId;
		bool eventConsumed = layerItem.rootNode.CursorClick(params);
		if (eventConsumed)
			break;
	}
}

void DockArea::CursorMove(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	CursorMoveEvent event)
{
	auto const layerItPair = impl::DockArea_GetLayerItPair(*this, widgetRect);
	for (auto const& layerItem : layerItPair)
	{
		Node::CursorMove_Params params{};
		params.ctx = &ctx;
		params.dockArea = this;
		params.event = event;
		params.layerIndex = layerItem.layerIndex;
		params.nodeRect = layerItem.layerRect;
		params.visibleRect = visibleRect;
		params.widgetRect = widgetRect;
		params.windowId = windowId;
		bool eventConsumed = layerItem.rootNode.CursorMove(params);
		if (eventConsumed)
			break;
	}
}

void DockArea::TouchEvent(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	Gui::TouchEvent event) 
{
	auto const layerItPair = impl::DockArea_GetLayerItPair(*this, widgetRect);
	for (auto const& layerItem : layerItPair)
	{
		Node::TouchEvent_Params params{};
		params.ctx = &ctx;
		params.dockArea = this;
		params.event = event;
		params.layerIndex = layerItem.layerIndex;
		params.nodeRect = layerItem.layerRect;
		params.visibleRect = visibleRect;
		params.widgetRect = widgetRect;
		params.windowId = windowId;
		bool eventConsumed = layerItem.rootNode.TouchEvent(params);
		if (eventConsumed)
			break;
	}
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