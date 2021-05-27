#include <DEngine/Gui/LineList.hpp>

#include <DEngine/Gui/Context.hpp>

#include <DEngine/Math/Common.hpp>

#include "ImplData.hpp"

using namespace DEngine;
using namespace DEngine::Gui;

namespace DEngine::Gui::impl
{
	[[nodiscard]] static constexpr uSize LineList_GetFirstVisibleLine(
		u32 lineHeight,
		i32 widgetPosY,
		i32 visiblePosY) noexcept
	{
		return static_cast<uSize>((visiblePosY - widgetPosY) / (f32)lineHeight);
	}

	[[nodiscard]] static constexpr uSize LineList_GetVisibleLineCount(
		u32 lineHeight,
		u32 visibleHeight) noexcept
	{
		return static_cast<uSize>(visibleHeight / (f32)lineHeight) + 2;
	}

	[[nodiscard]] static constexpr uSize LineList_GetLastVisibleLine(
		u32 lineHeight,
		i32 widgetPosY,
		i32 visiblePosY,
		u32 visibleHeight) noexcept
	{
		uSize const visibleBegin = impl::LineList_GetFirstVisibleLine(lineHeight, widgetPosY, visiblePosY);
		return visibleBegin + impl::LineList_GetVisibleLineCount(lineHeight, visibleHeight);
	}

	[[nodiscard]] static constexpr Rect GetLineRect(
		Rect widgetRect,
		u32 lineHeight,
		uSize index) noexcept
	{
		Rect returnVal = widgetRect;
		returnVal.position.y += lineHeight * index;
		returnVal.extent.height = lineHeight;
		return returnVal;
	}

	[[nodiscard]] static constexpr uSize GetHoverdIndex(
		i32 pointerPosY,
		i32 widgetPosY,
		u32 lineHeight) noexcept
	{
		uSize hoveredIndex = (pointerPosY - widgetPosY) / lineHeight;
		// Hitting an index below 0 is a bug.
		DENGINE_IMPL_GUI_ASSERT(hoveredIndex >= 0);
		return hoveredIndex;
	}

	[[nodiscard]] static bool PointerMove(
		Rect const& widgetRect,
		Rect const& visibleRect,
		Math::Vec2 pointerPos) noexcept
	{
		bool pointerInside = widgetRect.PointIsInside(pointerPos) && visibleRect.PointIsInside(pointerPos);
		return pointerInside;
	}

	static constexpr u8 cursorPointerId = (u8)-1;

	enum class PointerType : u8 { Primary, Secondary };
	[[nodiscard]] static constexpr PointerType ToPointerType(Gui::CursorButton in) noexcept
	{
		switch (in)
		{
			case Gui::CursorButton::Primary: return PointerType::Primary;
			case Gui::CursorButton::Secondary: return PointerType::Secondary;
			default: break;
		}
		DENGINE_IMPL_UNREACHABLE();
		return {};
	}

	struct PointerPressEvent
	{
		u8 id;
		PointerType type;
		Math::Vec2 pos;
		bool pressed;
	};
	[[nodiscard]] static bool PointerPress(
		LineList& widget,
		Context& ctx,
		Rect const& widgetRect,
		Rect const& visibleRect,
		PointerPressEvent const& pointer) noexcept;
}

static bool impl::PointerPress(
	LineList& widget,
	Context& ctx,
	Rect const& widgetRect,
	Rect const& visibleRect,
	PointerPressEvent const& pointer) noexcept
{
	// We have currently selected a line that no longer exists.
	// Unselect it.
	if (auto selected = widget.selectedLine.ToPtr();
		selected && *selected >= widget.lines.size())
	{
		widget.selectedLine = Std::nullOpt;
	}

	// The line we have currently pressed is on a line that no longer exists.
	// Don't press it anymore.
	if (auto pressedLine = widget.currPressedLine.ToPtr())
	{
		if (auto lineIndex = pressedLine->lineIndex.ToPtr();
			lineIndex && *lineIndex >= widget.lines.size())
		{
			widget.currPressedLine = Std::nullOpt;
		}

		// If we are currently pressing a line that is now been selected through other means
		// we want to exit pressed state.
		if (auto lineIndex = pressedLine->lineIndex.ToPtr())
		{
			if (widget.selectedLine.HasValue() && widget.selectedLine.Value() == *lineIndex)
			{
				widget.currPressedLine = Std::nullOpt;
			}
		}
	}



	bool pointerInside = widgetRect.PointIsInside(pointer.pos) && visibleRect.PointIsInside(pointer.pos);
	if (!pointerInside && pointer.pressed)
		return false;

	if (pointer.type != PointerType::Primary)
		return pointerInside;

	impl::ImplData& implData = *static_cast<impl::ImplData*>(ctx.Internal_ImplData());

	u32 const lineHeight = implData.textManager.lineheight;

	// We are not currently pressing a line, check if we hit a new line
	i32 hoveredIndex = impl::GetHoverdIndex((i32)pointer.pos.y, widgetRect.position.y, lineHeight);

	if (widget.currPressedLine.HasValue())
	{
		auto& pressedLine = widget.currPressedLine.Value();

		if (!pointer.pressed && pressedLine.pointerId == pointer.id && pointer.type == PointerType::Primary)
		{
			if (pressedLine.lineIndex.HasValue())
			{
				uSize lineIndex = pressedLine.lineIndex.Value();
				Rect const selectedLineRect = GetLineRect(widgetRect, lineHeight, lineIndex);
				bool pointerInsideLine = selectedLineRect.PointIsInside(pointer.pos) && visibleRect.PointIsInside(pointer.pos);
				if (pointerInsideLine)
				{
					// if we unpressed, inside the line, with the same pointer-id, with the primary button
					// Then we want to change the selected line this one.
					widget.currPressedLine = Std::nullOpt;
					widget.selectedLine = lineIndex;
					if (widget.selectedLineChangedCallback)
						widget.selectedLineChangedCallback(widget);

					return pointerInside;
				}
				else
				{
					// If we unpressed, with the same pointer-id, with the primary button, but OUTSIDE the line
					// Then we want to go out of pressed state, without changing the selected line.
					widget.currPressedLine = Std::nullOpt;
					return pointerInside;
				}
			}
			else
			{
				// We currently were pressing outside the lines, to deselect.
				// Check that we unpressed outside any lines and if so, 
				// deselect currently selected line.
				if (hoveredIndex >= widget.lines.size())
				{
					widget.currPressedLine = Std::nullOpt;
					widget.selectedLine = Std::nullOpt;
					if (widget.selectedLineChangedCallback)
						widget.selectedLineChangedCallback(widget);
					return pointerInside;
				}
				else
				{
					widget.currPressedLine = Std::nullOpt;
					return pointerInside;
				}
			}
		}
	}

	if (!widget.currPressedLine.HasValue() && pointer.pressed && pointer.type == PointerType::Primary)
	{
		// We don't want to go into pressed state if we
		// pressed a line that is already selected.
		if (widget.selectedLine.HasValue() && widget.selectedLine.Value() == hoveredIndex)
		{
			return pointerInside;
		}
		else
		{
			LineList::PressedLine_T newPressedLine = {};
			newPressedLine.pointerId = pointer.id;
			if (hoveredIndex >= widget.lines.size())
				newPressedLine.lineIndex = Std::nullOpt;
			else
				newPressedLine.lineIndex = hoveredIndex;
			widget.currPressedLine = newPressedLine;
			return pointerInside;
		}
	}

	return pointerInside;
}

void LineList::RemoveLine(uSize index)
{
	DENGINE_IMPL_GUI_ASSERT(index < lines.size());

	lines.erase(lines.begin() + index);
	if (selectedLine.HasValue())
	{
		if (selectedLine.Value() == index)
		{
			selectedLine = Std::nullOpt;
			if (selectedLineChangedCallback)
				selectedLineChangedCallback(*this);
		}
		else if (selectedLine.Value() < index)
		{
			selectedLine.Value() -= 1;
			if (selectedLineChangedCallback)
				selectedLineChangedCallback(*this);
		}
	}
}

SizeHint LineList::GetSizeHint(Context const& ctx) const
{
	impl::ImplData& implData = *static_cast<impl::ImplData*>(ctx.Internal_ImplData());
	uSize lineHeight = implData.textManager.lineheight;
	SizeHint returnVal = {};
	returnVal.preferred.height = static_cast<u32>(lineHeight * lines.size());
	for (auto const& line : lines)
	{
		SizeHint sizeHint = impl::TextManager::GetSizeHint(
			implData.textManager, 
			{ line.data(), line.size() });
		returnVal.preferred.width = Math::Max(returnVal.preferred.width, sizeHint.preferred.width);
	}
	return returnVal;
}

bool LineList::CursorMove(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	CursorMoveEvent event,
	bool occluded)
{
	return impl::PointerMove(
		widgetRect,
		visibleRect,
		{ (f32)event.position.x, (f32)event.position.y });
}

bool LineList::CursorPress(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	Math::Vec2Int cursorPos,
	CursorClickEvent event)
{
	impl::PointerPressEvent pointer = {};
	pointer.id = impl::cursorPointerId;
	pointer.pos = { (f32)cursorPos.x, (f32)cursorPos.y };
	pointer.pressed = event.clicked;
	pointer.type = impl::ToPointerType(event.button);
	return impl::PointerPress(
		*this,
		ctx,
		widgetRect,
		visibleRect,
		pointer);
}

bool LineList::TouchMoveEvent(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	Gui::TouchMoveEvent event,
	bool occluded)
{
	return impl::PointerMove(
		widgetRect,
		visibleRect,
		event.position);
}

bool LineList::TouchPressEvent(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	Gui::TouchPressEvent event)
{
	impl::PointerPressEvent pointer = {};
	pointer.id = event.id;
	pointer.pos = event.position;
	pointer.pressed = event.pressed;
	pointer.type = impl::PointerType::Primary;
	return impl::PointerPress(
		*this,
		ctx,
		widgetRect,
		visibleRect,
		pointer);
}

void LineList::Render(
	Context const& ctx,
	Extent framebufferExtent,
	Rect widgetRect,
	Rect visibleRect,
	DrawInfo& drawInfo) const
{
	if (Rect::Intersection(widgetRect, visibleRect).IsNothing())
		return;

	impl::ImplData& implData = *static_cast<impl::ImplData*>(ctx.Internal_ImplData());

	u32 const lineHeight = implData.textManager.lineheight;
	uSize const visibleBegin = impl::LineList_GetFirstVisibleLine(lineHeight, widgetRect.position.y, visibleRect.position.y);
	uSize const visibleEnd = impl::LineList_GetLastVisibleLine(
		lineHeight,
		widgetRect.position.y,
		visibleRect.position.y,
		visibleRect.extent.height);

	

	for (uSize i = visibleBegin; i < visibleEnd; i += 1)
	{
		Rect lineRect = impl::GetLineRect(widgetRect, lineHeight, i);

		if (i % 2 == 0 && (!selectedLine.HasValue() ||  selectedLine.Value() != i))
		{
			drawInfo.PushFilledQuad(lineRect, alternatingLineColor);
		}
	}

	if (selectedLine.HasValue())
	{
		Rect const selectedLineRect = impl::GetLineRect(widgetRect, lineHeight, selectedLine.Value());
		if (!Rect::Intersection(selectedLineRect, visibleRect).IsNothing())
			drawInfo.PushFilledQuad(selectedLineRect, highlightColor);
	}

	for (uSize i = visibleBegin; i < Math::Min(visibleEnd, lines.size()); i++)
	{
		Rect textRect = impl::GetLineRect(widgetRect, lineHeight, i);
		textRect.position.x += (horizontalPadding / 2);

		auto& line = lines[i];
		impl::TextManager::RenderText(
			implData.textManager,
			{ line.data(), line.size() },
			{ 1.f, 1.f, 1.f, 1.f },
			textRect,
			drawInfo);
	}
}