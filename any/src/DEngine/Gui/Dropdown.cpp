#include <DEngine/Gui/Dropdown.hpp>
#include <DEngine/Gui/TextManager.hpp>
#include <DEngine/Gui/Layer.hpp>

#include <DEngine/Gui/Context.hpp>
#include <DEngine/Gui/StackLayout.hpp>

#include <DEngine/Std/Utility.hpp>

using namespace DEngine;
using namespace DEngine::Gui;

namespace DEngine::Gui::impl
{
	enum class PointerType : u8 { Primary, Secondary };
	[[nodiscard]] static PointerType ToPointerType(CursorButton in) noexcept
	{
		switch (in)
		{
			case CursorButton::Primary: return PointerType::Primary;
			case CursorButton::Secondary: return PointerType::Secondary;
			default:
				DENGINE_IMPL_UNREACHABLE();
				break;
		}
		return {};
	}

	constexpr u8 cursorPointerId = (u8)-1;

	class DropdownLayer : public Layer
	{
	public:
		Dropdown* dropdownWidget = nullptr;

		Math::Vec2Int pos = {};
		u32 dropdownWidgetWidth = 0;
		struct PressedLine
		{
			u8 pointerId;
			uSize index;
		};
		Std::Opt<PressedLine> pressedLine;
		Std::Opt<uSize> hoveredByCursorIndex;

		virtual void BuildSizeHints(BuildSizeHints_Params const& params) const override;

		virtual void BuildRects(BuildRects_Params const& params) const override;

		virtual void Render(Render_Params const& params) const override;

		virtual void Render(
			Context const& ctx,
			Rect const& windowRect,
			Rect const& usableRect,
			DrawInfo& drawInfo) const override;

		virtual bool CursorMove(
			CursorMoveParams const& params,
			bool occluded) override;

		virtual Press_Return CursorPress(
			CursorPressParams const& params,
			bool eventConsumed) override;

		virtual bool CursorMove(
			Context& ctx,
			Rect const& windowRect,
			Rect const& usableRect,
			CursorMoveEvent const& event,
			bool occluded) override;

		virtual Press_Return CursorPress(
			Context& ctx,
			Rect const& windowRect,
			Rect const& usableRect,
			Math::Vec2Int cursorPos,
			CursorPressEvent const& event) override;

		virtual Press_Return TouchPress(
			Context& ctx,
			Rect const& windowRect,
			Rect const& usableRect,
			TouchPressEvent const& event) override;
	};

	static void CreateDropdownLayer(
		Dropdown& widget,
		Context& ctx,
		WindowID windowId,
		Rect const& widgetRect)
	{
		auto layer = Std::Box{ new DropdownLayer };
		layer->dropdownWidget = &widget;
		layer->pos = widgetRect.position;
		layer->pos.y += widgetRect.extent.height;
		layer->dropdownWidgetWidth = widgetRect.extent.width;

		ctx.SetFrontmostLayer(
			windowId,
			Std::Move(layer));
	}

	[[nodiscard]] static Extent DropdownLayer_BuildOuterExtent(
		Std::Span<decltype(Dropdown::items)::value_type const> widgetItems,
		u32 textMargin,
		TextManager& textManager,
		Rect const& safeAreaRect)
	{
		auto const textHeight = textManager.GetLineheight();
		auto const lineHeight = textHeight + (textMargin * 2);

		Extent returnValue = {};
		returnValue.height = lineHeight * widgetItems.Size();

		for (auto const& line : widgetItems)
		{
			auto const textExtent = textManager.GetOuterExtent({ line.data(), line.size() });

			returnValue.width = Math::Max(
				returnValue.width,
				textExtent.width);
		}
		// Add margin to extent.
		returnValue.width += textMargin * 2;

		return returnValue;
	}

	[[nodiscard]] static Rect DropdownLayer_BuildOuterRect(
		DropdownLayer const& layer,
		TextManager& textManager,
		Rect const& usableRect)
	{
		auto const& dropdownWidget = *layer.dropdownWidget;

		Rect widgetRect = {};
		widgetRect.position = layer.pos;

		widgetRect.extent = DropdownLayer_BuildOuterExtent(
			{ dropdownWidget.items.data(), dropdownWidget.items.size() },
			dropdownWidget.textMargin,
			textManager,
			usableRect);

		// Adjust the position of the widget.
		widgetRect.position.y = Math::Max(
			widgetRect.Top(),
			usableRect.Top());
		widgetRect.position.y = Math::Min(
			widgetRect.Top(),
			usableRect.Bottom() - (i32)widgetRect.extent.height);

		return widgetRect;
	}

	[[nodiscard]] static Std::Opt<uSize> DropdownLayer_CheckLineHit(
		Math::Vec2Int rectPos,
		u32 rectWidth,
		uSize lineCount,
		u32 lineHeight,
		Math::Vec2 pointerPos)
	{
		Std::Opt<uSize> lineHit;
		for (uSize i = 0; i < lineCount; i++)
		{
			Rect lineRect = {};
			lineRect.position = rectPos;
			lineRect.position.y += lineHeight * i;
			lineRect.extent.width = rectWidth;
			lineRect.extent.height = lineHeight;

			if (lineRect.PointIsInside(pointerPos))
			{
				lineHit = i;
				break;
			}
		}
		return lineHit;
	}

	struct PointerPress_Pointer
	{
		u8 id;
		PointerType type;
		Math::Vec2 pos;
		bool pressed;
	};

	[[nodiscard]] static Layer::Press_Return DropdownLayer_PointerPress(
		DropdownLayer& widget,
		Context& ctx,
		TextManager& textManager,
		Rect const& usableRect,
		Rect const& listRectOuter,
		PointerPress_Pointer const& pointer,
		bool eventConsumed)
	{
		auto& dropdownWidget = *widget.dropdownWidget;
		auto const textHeight = textManager.GetLineheight();

		auto const pointerInsideOuter = listRectOuter.PointIsInside(pointer.pos);

		Layer::Press_Return returnVal = {};
		returnVal.eventConsumed = pointerInsideOuter;
		returnVal.destroyLayer = !pointerInsideOuter;

		if (pointer.type != PointerType::Primary)
		{
			return returnVal;
		}
		if (!pointerInsideOuter)
		{
			return returnVal;
		}
		if (eventConsumed)
		{
			return returnVal;
		}

		auto const lineHeight = textHeight + (dropdownWidget.textMargin * 2);

		// Figure out if we hit a line
		auto lineHit = DropdownLayer_CheckLineHit(
			listRectOuter.position,
			listRectOuter.extent.width,
			dropdownWidget.items.size(),
			lineHeight,
			pointer.pos);

		if (widget.pressedLine.HasValue())
		{
			auto const pressedLine = widget.pressedLine.Value();
			if (pressedLine.pointerId == pointer.id)
			{
				if (!pointer.pressed)
					widget.pressedLine = Std::nullOpt;

				if (lineHit.HasValue() &&
					lineHit.Value() == pressedLine.index &&
					!pointer.pressed)
				{
					dropdownWidget.selectedItem = pressedLine.index;

					if (dropdownWidget.selectionChangedCallback)
						dropdownWidget.selectionChangedCallback(dropdownWidget);

					returnVal.destroyLayer = true;
				}
			}
		}
		else
		{
			if (pointerInsideOuter && pointer.pressed && lineHit.HasValue())
			{
				DropdownLayer::PressedLine newPressedLine = {};
				newPressedLine.pointerId = pointer.id;
				newPressedLine.index = lineHit.Value();
				widget.pressedLine = newPressedLine;
			}
		}

		return returnVal;
	}

	struct PointerMove_Pointer
	{
		u8 id;
		Math::Vec2 pos;
		bool occluded;
	};

	[[nodiscard]] bool DropdownLayer_PointerMove(
		DropdownLayer& layer,
		Context& ctx,
		TextManager& textManager,
		Rect const& windowRect,
		Rect const& usableRect,
		Rect const& listRectOuter,
		PointerMove_Pointer const& pointer)
	{
		auto& dropdownWidget = *layer.dropdownWidget;

		auto const textHeight = textManager.GetLineheight();

		auto const pointerInsideOuter = listRectOuter.PointIsInside(pointer.pos);

		if (pointer.id == cursorPointerId)
		{
			u32 const lineHeight = textHeight + (dropdownWidget.textMargin * 2);

			// Figure out if we hit a line
			auto lineHit = DropdownLayer_CheckLineHit(
				listRectOuter.position,
				listRectOuter.extent.width,
				dropdownWidget.items.size(),
				lineHeight,
				pointer.pos);

			layer.hoveredByCursorIndex = lineHit;
		}

		return pointerInsideOuter;
	}

	[[nodiscard]] static Rect Dropdown_BuildTextRect(
		Rect const& widgetRect,
		u32 textMargin) noexcept
	{
		Rect returnVal = widgetRect;
		returnVal.position.x += textMargin;
		returnVal.position.y += textMargin;
		returnVal.extent.width -= textMargin * 2;
		returnVal.extent.height -= textMargin * 2;
		return returnVal;
	}
}

void Gui::impl::DropdownLayer::BuildSizeHints(
	BuildSizeHints_Params const& params) const
{
	SizeHint returnValue = {};
	returnValue.minimum = DropdownLayer_BuildOuterExtent(
		{ dropdownWidget->items.data(), dropdownWidget->items.size() },
		dropdownWidget->textMargin,
		params.textManager,
		params.safeAreaRect);

	returnValue.minimum.width = dropdownWidgetWidth;

	params.pusher.Push(*this, returnValue);
}

void Gui::impl::DropdownLayer::BuildRects(
	Layer::BuildRects_Params const& params) const
{
	auto const childSizeHint = params.pusher.GetSizeHint(*this);

	Rect visibleRect = params.visibleRect;

	Rect widgetRect = {};
	widgetRect.extent = childSizeHint.minimum;

	widgetRect.position = pos;

	// Adjust the position of the widget.
	widgetRect.position.y = Math::Max(
		widgetRect.Top(),
		visibleRect.Top());
	widgetRect.position.y = Math::Min(
		widgetRect.Top(),
		visibleRect.Bottom() - (i32)widgetRect.extent.height);

	params.pusher.Push(*this, { widgetRect, visibleRect });
}

void Gui::impl::DropdownLayer::Render(
	Render_Params const& params) const
{
	auto const& ctx = params.ctx;
	auto& textManager = params.textManager;
	auto const& windowRect = params.windowRect;
	auto const& safeAreaRect = params.safeAreaRect;
	auto& drawInfo = params.drawInfo;

	auto const textHeight = textManager.GetLineheight();

	auto const intersectionRect = Rect::Intersection(windowRect, safeAreaRect);
	if (intersectionRect.IsNothing())
		return;

	auto const& rectCombo = params.rectCollection.GetRect(*this);
	auto const& listOuterRect = rectCombo.widgetRect;

	drawInfo.PushFilledQuad(listOuterRect, { 0.25f, 0.25f, 0.25f, 1.f });

	auto const itemsCount = (int)dropdownWidget->items.size();
	for (int i = 0; i < itemsCount; i += 1)
	{
		auto const& line = dropdownWidget->items[i];

		// The height of one line is the textheight + the spacing below it.
		auto const lineHeight = textHeight + (dropdownWidget->textMargin * 2);

		auto lineRect = listOuterRect;
		lineRect.position.y += lineHeight * i;
		lineRect.extent.height = lineHeight;
		if (hoveredByCursorIndex.HasValue() && hoveredByCursorIndex.Value() == i)
		{
			drawInfo.PushFilledQuad(lineRect, { 0.4f, 0.4f, 0.4f, 1.f });
		}

		auto textRect = lineRect;
		textRect.position.x += dropdownWidget->textMargin;
		textRect.position.y += dropdownWidget->textMargin;
		textRect.extent.width -= dropdownWidget->textMargin * 2;
		textRect.extent.height -= dropdownWidget->textMargin * 2;

		Math::Vec4 textColor = { 1.f, 1.f, 1.f, 1.f };
		textManager.RenderText(
			{ line.data(), line.size() },
			textColor,
			textRect,
			drawInfo);
	}
}

bool Gui::impl::DropdownLayer::CursorMove(
	CursorMoveParams const& params,
	bool occluded)
{
	auto const& rectPair = params.rectCollection.GetRect(*this);

	PointerMove_Pointer pointer = {};
	pointer.id = cursorPointerId;
	pointer.pos = { (f32)params.event.position.x, (f32)params.event.position.y };
	pointer.occluded = occluded;

	return DropdownLayer_PointerMove(
		*this,
		params.ctx,
		params.textManager,
		params.windowRect,
		params.safeAreaRect,
		rectPair.widgetRect,
		pointer);
}

Layer::Press_Return Gui::impl::DropdownLayer::CursorPress(
	Layer::CursorPressParams const& params,
	bool eventConsumed)
{
	auto const& rectPair = params.rectCollection.GetRect(*this);

	auto const intersection = Rect::Intersection(params.windowRect, params.safeAreaRect);

	PointerPress_Pointer pointer = {};
	pointer.id = cursorPointerId;
	pointer.pos = { (f32)params.cursorPos.x, (f32)params.cursorPos.y };
	pointer.pressed = params.event.pressed;
	pointer.type = ToPointerType(params.event.button);

	return DropdownLayer_PointerPress(
		*this,
		params.ctx,
		params.textManager,
		intersection,
		rectPair.widgetRect,
		pointer,
		eventConsumed);
}

class [[maybe_unused]] Gui::impl::DropdownImpl
{
public:
	[[nodiscard]] static bool Dropdown_PointerPress(
		Dropdown& widget,
		Context& ctx,
		WindowID windowId,
		Rect const& widgetRect,
		Rect const& visibleRect,
		PointerPress_Pointer const& pointer)
	{
		auto const pointerInside =
			widgetRect.PointIsInside(pointer.pos) &&
			visibleRect.PointIsInside(pointer.pos);
		if (pointer.pressed && !pointerInside)
			return false;

		// If our pointer-type is not primary, we don't want to do anything,
		// Just consume the event if it's inside the widget and move on.
		if (pointer.type != PointerType::Primary)
			return pointerInside;

		// We can now trust pointer.type == Primary
		if (widget.heldPointerId.HasValue())
		{
			auto const heldPointerId = widget.heldPointerId.Value();
			if (!pointer.pressed)
				widget.heldPointerId = Std::nullOpt;

			if (heldPointerId == pointer.id && pointerInside && !pointer.pressed)
			{
				// Now we open the dropdown menu
				CreateDropdownLayer(widget, ctx, windowId, widgetRect);
			}
		}
		else
		{
			if (pointerInside && pointer.pressed)
			{
				widget.heldPointerId = pointer.id;
			}
		}

		// We are inside the button, so we always want to consume the event.
		return pointerInside;
	}

	[[nodiscard]] static bool Dropdown_PointerMove(
		Dropdown& widget,
		Context& ctx,
		WindowID windowId,
		Rect const& widgetRect,
		Rect const& visibleRect,
		PointerMove_Pointer const& pointer)
	{
		auto pointerInside =
			widgetRect.PointIsInside(pointer.pos) &&
			visibleRect.PointIsInside(pointer.pos);

		return pointerInside;
	}
};

void Gui::impl::DropdownLayer::Render(
	Context const& ctx,
	Rect const& windowRect,
	Rect const& usableRect,
	DrawInfo& drawInfo) const
{
}

bool Gui::impl::DropdownLayer::CursorMove(
	Context& ctx,
	Rect const& windowRect,
	Rect const& usableRect,
	CursorMoveEvent const& event,
	bool occluded)
{
	return false;
}

Layer::Press_Return Gui::impl::DropdownLayer::CursorPress(
	Context& ctx,
	Rect const& windowRect,
	Rect const& usableRect,
	Math::Vec2Int cursorPos,
	CursorPressEvent const& event)
{
	return {};
}

Layer::Press_Return Gui::impl::DropdownLayer::TouchPress(
	Context& ctx,
	Rect const& windowRect,
	Rect const& usableRect,
	TouchPressEvent const& event)
{
	return {};
}

Dropdown::Dropdown()
{
}

Dropdown::~Dropdown()
{
}

SizeHint Dropdown::GetSizeHint(Context const& ctx) const
{
/*
	auto const textHeight = implData.textManager.lineheight;

	SizeHint returnVal = {};
	returnVal.minimum.height = textHeight + (textMargin * 2);
	for (auto& item : items)
	{
		auto const childSizeHint = impl::TextManager::GetSizeHint(
			implData.textManager,
			{ item.data(), item.size() });
		returnVal.minimum.width = Math::Max(
			returnVal.minimum.width,
			childSizeHint.minimum.width);
	}
	returnVal.minimum.width += textMargin * 2;

	return returnVal;
*/

	return {};
}

void Dropdown::Render(
	Context const& ctx, 
	Extent framebufferExtent, 
	Rect widgetRect,
	Rect visibleRect, 
	DrawInfo& drawInfo) const
{
}

bool Dropdown::CursorPress(
	Context& ctx, 
	WindowID windowId, 
	Rect widgetRect, 
	Rect visibleRect, 
	Math::Vec2Int cursorPos,
	CursorPressEvent event)
{
	impl::PointerPress_Pointer pointer = {};
	pointer.id = impl::cursorPointerId;
	pointer.pos = { (f32)cursorPos.x, (f32)cursorPos.y };
	pointer.pressed = event.pressed;
	pointer.type = impl::ToPointerType(event.button);

	return impl::DropdownImpl::Dropdown_PointerPress(
		*this,
		ctx,
		windowId,
		widgetRect,
		visibleRect,
		pointer);
}

bool Dropdown::CursorMove(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	CursorMoveEvent event,
	bool cursorOccluded)
{
	impl::PointerMove_Pointer pointer = {};
	pointer.id = impl::cursorPointerId;
	pointer.pos = { (f32)event.position.x, (f32)event.position.y };
	pointer.occluded = cursorOccluded;

	return impl::DropdownImpl::Dropdown_PointerMove(
		*this,
		ctx,
		windowId,
		widgetRect,
		visibleRect,
		pointer);
}

bool Dropdown::TouchPressEvent(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	Gui::TouchPressEvent event)
{
	impl::PointerPress_Pointer pointer = {};
	pointer.id = impl::cursorPointerId;
	pointer.pos = event.position;
	pointer.pressed = event.pressed;
	pointer.type = impl::PointerType::Primary;

	return impl::DropdownImpl::Dropdown_PointerPress(
		*this,
		ctx,
		windowId,
		widgetRect,
		visibleRect,
		pointer);
}

bool Dropdown::TouchMoveEvent(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	Gui::TouchMoveEvent event,
	bool occluded)
{
	impl::PointerMove_Pointer pointer = {};
	pointer.id = event.id;
	pointer.pos = event.position;
	pointer.occluded = occluded;

	return impl::DropdownImpl::Dropdown_PointerMove(
		*this,
		ctx,
		windowId,
		widgetRect,
		visibleRect,
		pointer);
}

SizeHint Dropdown::GetSizeHint2(
	Widget::GetSizeHint2_Params const& params) const
{
	auto& textManager = params.textManager;

	auto const textHeight = textManager.GetLineheight();

	SizeHint returnVal = {};
	returnVal.minimum.width = 20;
	returnVal.minimum.height = textHeight + (textMargin * 2);
	for (auto& item : items)
	{
		auto const textExtent = textManager.GetOuterExtent({ item.data(), item.size() });
		returnVal.minimum.width = Math::Max(
			returnVal.minimum.width,
			textExtent.width);
	}
	returnVal.minimum.width += textMargin * 2;

	params.pusher.Push(*this, returnVal);

	return returnVal;
}

void Dropdown::Render2(
	Widget::Render_Params const& params,
	Rect const& widgetRect,
	Rect const& visibleRect) const
{
	auto& textManager = params.textManager;
	auto& drawInfo = params.drawInfo;

	drawInfo.PushFilledQuad(
		widgetRect,
		boxColor);

	auto const& string = items[selectedItem];

	auto const textRect = impl::Dropdown_BuildTextRect(widgetRect, textMargin);

	textManager.RenderText(
		{ string.data(), string.size() },
		{ 1.f, 1.f, 1.f, 1.f },
		textRect,
		drawInfo);
}

bool Dropdown::CursorPress2(
	Widget::CursorPressParams const& params,
	Rect const& widgetRect,
	Rect const& visibleRect,
	bool consumed)
{
	auto& ctx = params.ctx;
	auto windowId = params.windowId;
	auto const& cursorPos = params.cursorPos;
	auto const& event = params.event;

	impl::PointerPress_Pointer pointer = {};
	pointer.id = impl::cursorPointerId;
	pointer.pos = { (f32)cursorPos.x, (f32)cursorPos.y };
	pointer.pressed = event.pressed;
	pointer.type = impl::ToPointerType(event.button);

	return impl::DropdownImpl::Dropdown_PointerPress(
		*this,
		ctx,
		windowId,
		widgetRect,
		visibleRect,
		pointer);
}
