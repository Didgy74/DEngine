#include <DEngine/Gui/Dropdown.hpp>

#include <DEngine/Gui/Layer.hpp>

#include <DEngine/Gui/Context.hpp>
#include <DEngine/Gui/StackLayout.hpp>

#include <DEngine/Std/Utility.hpp>

#include "ImplData.hpp"

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
			default: break;
		}
		DENGINE_IMPL_UNREACHABLE();
		return {};
	}

	constexpr u8 cursorPointerId = (u8)-1;

	class DropdownLayer : public Layer
	{
	public:
		Dropdown* dropdownWidget = nullptr;

		Math::Vec2Int pos = {};
		struct PressedLine
		{
			u8 pointerId;
			uSize index;
		};
		Std::Opt<PressedLine> pressedLine;
		Std::Opt<uSize> hoveredByCursorIndex;

		virtual void Render(
			Context const& ctx,
			Rect const& windowRect,
			Rect const& usableRect,
			DrawInfo& drawInfo) const override;

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
			CursorClickEvent const& event) override;

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

		ctx.SetFrontmostLayer(
			windowId,
			Std::Move(layer));
	}

	[[nodiscard]] static Rect DropdownLayer_BuildOuterRect(
		DropdownLayer const& widget,
		Context const& ctx,
		Rect const& usableRect)
	{
		auto const& dropdownWidget = *widget.dropdownWidget;

		auto& implData = *static_cast<impl::ImplData*>(ctx.Internal_ImplData());

		auto const textHeight = implData.textManager.lineheight;
		auto const lineHeight = textHeight + (dropdownWidget.textMargin * 2);

		Rect widgetRect = {};
		widgetRect.position = widget.pos;
		widgetRect.extent.height = lineHeight * dropdownWidget.items.size();
		// Calculate width of widget.
		widgetRect.extent.width = 0;
		for (auto const& line : dropdownWidget.items)
		{
			auto const sizeHint = impl::TextManager::GetSizeHint(
				implData.textManager,
				{ line.data(), line.size() });

			widgetRect.extent.width = Math::Max(
				widgetRect.extent.width,
				sizeHint.preferred.width);
		}
		// Add margin to widget.
		widgetRect.extent.width += widget.dropdownWidget->textMargin * 2;

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
		Rect const& usableRect,
		PointerPress_Pointer const& pointer)
	{
		auto& dropdownWidget = *widget.dropdownWidget;
		auto& implData = *static_cast<impl::ImplData*>(ctx.Internal_ImplData());
		auto const textHeight = implData.textManager.lineheight;

		auto const widgetRectOuter = DropdownLayer_BuildOuterRect(
			widget,
			ctx,
			usableRect);
		auto const pointerInsideOuter = widgetRectOuter.PointIsInside(pointer.pos);

		Layer::Press_Return returnVal = {};
		returnVal.eventConsumed = pointerInsideOuter;
		returnVal.destroy = !pointerInsideOuter;

		if (pointer.type != PointerType::Primary)
		{
			return returnVal;
		}

		// Figure out if we hit a line
		auto lineHit = DropdownLayer_CheckLineHit(
			widgetRectOuter.position,
			widgetRectOuter.extent.width,
			dropdownWidget.items.size(),
			textHeight + (dropdownWidget.textMargin * 2),
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

					returnVal.destroy = true;
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
		DropdownLayer& widget,
		Context& ctx,
		Rect const& windowRect,
		Rect const& usableRect,
		PointerMove_Pointer const& pointer)
	{
		auto& dropdownWidget = *widget.dropdownWidget;
		auto& implData = *static_cast<impl::ImplData*>(ctx.Internal_ImplData());
		auto const textHeight = implData.textManager.lineheight;

		auto const intersection = Rect::Intersection(windowRect, usableRect);

		auto const widgetRectOuter = DropdownLayer_BuildOuterRect(
			widget,
			ctx,
			intersection);

		auto const pointerInsideOuter = widgetRectOuter.PointIsInside(pointer.pos);

		if (pointer.id == cursorPointerId)
		{
			// Figure out if we hit a line
			auto lineHit = DropdownLayer_CheckLineHit(
				widgetRectOuter.position,
				widgetRectOuter.extent.width,
				dropdownWidget.items.size(),
				textHeight + (dropdownWidget.textMargin * 2),
				pointer.pos);

			widget.hoveredByCursorIndex = lineHit;
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

class [[maybe_unused]] impl::DropdownImpl
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

void impl::DropdownLayer::Render(
	Context const& ctx,
	Rect const& windowRect,
	Rect const& usableRect,
	DrawInfo& drawInfo) const
{
	auto& implData = *static_cast<impl::ImplData*>(ctx.Internal_ImplData());
	auto const textHeight = implData.textManager.lineheight;

	auto const intersectionRect = Rect::Intersection(windowRect, usableRect);

	auto const widgetRectOuter = DropdownLayer_BuildOuterRect(
		*this,
		ctx,
		intersectionRect);

	drawInfo.PushFilledQuad(widgetRectOuter, { 0.3f, 0.3f, 0.3f, 1.f });

	for (uSize i = 0; i < dropdownWidget->items.size(); i += 1)
	{
		auto const& line = dropdownWidget->items[i];

		// The height of one line is the textheight + the spacing below it.
		auto const lineHeight = textHeight + (dropdownWidget->textMargin * 2);

		auto lineRect = widgetRectOuter;
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
		impl::TextManager::RenderText(
			implData.textManager,
			{ line.data(), line.size() },
			textColor,
			textRect,
			drawInfo);
	}
}

bool impl::DropdownLayer::CursorMove(
	Context& ctx,
	Rect const& windowRect,
	Rect const& usableRect,
	CursorMoveEvent const& event,
	bool occluded)
{
	PointerMove_Pointer pointer = {};
	pointer.id = impl::cursorPointerId;
	pointer.occluded = occluded;
	pointer.pos = { (f32)event.position.x, (f32)event.position.y };
	
	return DropdownLayer_PointerMove(
		*this,
		ctx,
		windowRect,
		usableRect,
		pointer);
}

Layer::Press_Return impl::DropdownLayer::CursorPress(
	Context& ctx,
	Rect const& windowRect,
	Rect const& usableRect,
	Math::Vec2Int cursorPos,
	CursorClickEvent const& event)
{
	impl::PointerPress_Pointer pointer = {};
	pointer.id = impl::cursorPointerId;
	pointer.pos = { (f32)cursorPos.x, (f32)cursorPos.y };
	pointer.pressed = event.clicked;
	pointer.type = impl::ToPointerType(event.button);

	auto const intersection = Rect::Intersection(windowRect, usableRect);

	return DropdownLayer_PointerPress(
		*this,
		ctx,
		intersection,
		pointer);
}

Layer::Press_Return impl::DropdownLayer::TouchPress(
	Context& ctx,
	Rect const& windowRect,
	Rect const& usableRect,
	TouchPressEvent const& event)
{
	impl::PointerPress_Pointer pointer = {};
	pointer.id = event.id;
	pointer.pos = event.position;
	pointer.pressed = event.pressed;
	pointer.type = impl::PointerType::Primary;

	auto const intersection = Rect::Intersection(windowRect, usableRect);

	return DropdownLayer_PointerPress(
		*this,
		ctx,
		intersection,
		pointer);
}

Dropdown::Dropdown()
{
}

Dropdown::~Dropdown()
{
}

SizeHint Dropdown::GetSizeHint(Context const& ctx) const
{
	auto& implData = *static_cast<impl::ImplData*>(ctx.Internal_ImplData());

	auto const textHeight = implData.textManager.lineheight;

	SizeHint returnVal = {};
	returnVal.preferred.height = textHeight + (textMargin * 2);
	for (auto& item : items)
	{
		auto const childSizeHint = impl::TextManager::GetSizeHint(
			implData.textManager,
			{ item.data(), item.size() });
		returnVal.preferred.width = Math::Max(
			returnVal.preferred.width,
			childSizeHint.preferred.width);
	}
	returnVal.preferred.width += textMargin * 2;

	return returnVal;
}

void Dropdown::Render(
	Context const& ctx, 
	Extent framebufferExtent, 
	Rect widgetRect,
	Rect visibleRect, 
	DrawInfo& drawInfo) const
{
	impl::ImplData& implData = *static_cast<impl::ImplData*>(ctx.Internal_ImplData());

	drawInfo.PushFilledQuad(
		widgetRect,
		{ 0.f, 0.f, 0.f, 0.25f });

	auto const& string = items[selectedItem];

	auto const textRect = impl::Dropdown_BuildTextRect(widgetRect, textMargin);

	impl::TextManager::RenderText(
		implData.textManager,
		{ string.data(), string.size() },
		{ 1.f, 1.f, 1.f, 1.f },
		textRect,
		drawInfo);
}

bool Dropdown::CursorPress(
	Context& ctx, 
	WindowID windowId, 
	Rect widgetRect, 
	Rect visibleRect, 
	Math::Vec2Int cursorPos,
	CursorClickEvent event)
{
	impl::PointerPress_Pointer pointer = {};
	pointer.id = impl::cursorPointerId;
	pointer.pos = { (f32)cursorPos.x, (f32)cursorPos.y };
	pointer.pressed = event.clicked;
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