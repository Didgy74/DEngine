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
		Math::Vec2Int widgetPosition,
		u32 widgetWidth,
		u32 lineHeight,
		uSize index) noexcept
	{
		Rect returnVal = {};
		returnVal.position = widgetPosition;
		returnVal.position.y += lineHeight * index;
		returnVal.extent.width = widgetWidth;
		returnVal.extent.height = lineHeight;
		return returnVal;
	}

	[[nodiscard]] static constexpr uSize GetHoveredIndex(
		i32 pointerPosY,
		i32 widgetPosY,
		u32 lineHeight) noexcept
	{
		uSize hoveredIndex = (pointerPosY - widgetPosY) / lineHeight;
		// Hitting an index below 0 is a bug.
		DENGINE_IMPL_GUI_ASSERT(hoveredIndex >= 0);
		return hoveredIndex;
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

	struct PointerMove_Params
	{
		LineList* widget = nullptr;
		Context* ctx = nullptr;
		Rect const* widgetRect = nullptr;
		Rect const* visibleRect = nullptr;
		u8 pointerId = 0;
		Math::Vec2 pointerPos = {};
		bool pointerOccluded = false;
	};
}

class [[maybe_unused]] Gui::impl::LineListImpl
{
public:
	[[nodiscard]] static bool PointerPress(
		LineList& widget,
		Context& ctx,
		Rect const& widgetRect,
		Rect const& visibleRect,
		PointerPressEvent const& pointer)
	{
		bool pressConsumed = false;

		auto const pointerInside =
			widgetRect.PointIsInside(pointer.pos) &&
			visibleRect.PointIsInside(pointer.pos);

		if (pointerInside)
			pressConsumed = true;

		if (pointer.type != PointerType::Primary)
			return pointerInside;

		auto& implData = *static_cast<impl::ImplData*>(ctx.Internal_ImplData());
		auto const textHeight = implData.textManager.lineheight;
		auto const lineHeight = textHeight + (widget.textMargin * 2);

		// We are not currently pressing a line, check if we hit a new line
		auto const hoveredIndex = impl::GetHoveredIndex(
			(i32)pointer.pos.y,
			widgetRect.position.y,
			lineHeight);

		if (widget.currPressedLine.HasValue())
		{
			auto const pressedLine = widget.currPressedLine.Value();
			if (!pointer.pressed && pressedLine.pointerId == pointer.id)
			{
				widget.currPressedLine = Std::nullOpt;

				bool selectedLineChanged = false;
				if (pressedLine.lineIndex.HasValue())
				{
					// We are currently holding an existing line.
					// Check if we unpressed that same line
					auto const lineIndex = pressedLine.lineIndex.Value();
					if (lineIndex == hoveredIndex)
					{
						widget.selectedLine = lineIndex;
						selectedLineChanged = true;
					}
				}
				else
				{
					// We were currently not holding a line
					// Check if we unpressed outside any lines
					if (hoveredIndex >= widget.lines.size())
					{
						widget.selectedLine = Std::nullOpt;
						selectedLineChanged = true;
					}
				}

				if (selectedLineChanged && widget.selectedLineChangedFn)
					widget.selectedLineChangedFn(widget);
			}
		}
		else
		{
			if (pointer.pressed && pointerInside)
			{
				// We don't want to go into pressed state if we
				// pressed a line that is already selected.
				auto const hoveringSelectingLine =
					widget.selectedLine.HasValue() &&
					widget.selectedLine.Value() == hoveredIndex;
				if (!hoveringSelectingLine)
				{
					// We did not press a line that is already selected.
					LineList::PressedLine_T newPressedLine = {};
					newPressedLine.pointerId = pointer.id;
					if (hoveredIndex < widget.lines.size())
						newPressedLine.lineIndex = hoveredIndex;
					else
						newPressedLine.lineIndex = Std::nullOpt;
					widget.currPressedLine = newPressedLine;
				}
			}
		}

		return pressConsumed;
	}

	[[nodiscard]] static bool PointerMove(
		PointerMove_Params const& params) noexcept
	{
		auto& widget = *params.widget;
		auto& ctx = *params.ctx;
		auto& widgetRect = *params.widgetRect;
		auto& visibleRect = *params.visibleRect;

		auto const pointerInside =
			widgetRect.PointIsInside(params.pointerPos) &&
			visibleRect.PointIsInside(params.pointerPos);

		auto& implData = *static_cast<impl::ImplData*>(ctx.Internal_ImplData());
		auto const textHeight = implData.textManager.lineheight;
		auto const lineHeight = textHeight + (widget.textMargin * 2);

		if (params.pointerId == cursorPointerId)
		{
			if (!pointerInside || params.pointerOccluded)
				widget.lineCursorHover = Std::nullOpt;
			else
			{
				auto const hoveredIndex = impl::GetHoveredIndex(
					(i32)params.pointerPos.y,
					widgetRect.position.y,
					lineHeight);
				if (hoveredIndex < widget.lines.size())
					widget.lineCursorHover = hoveredIndex;
				else
					widget.lineCursorHover = Std::nullOpt;
			}
		}

		return pointerInside;
	}
};

void LineList::RemoveLine(uSize index)
{
	DENGINE_IMPL_GUI_ASSERT(index < lines.size());

	lines.erase(lines.begin() + index);
	if (selectedLine.HasValue())
	{
		if (selectedLine.Value() == index)
		{
			selectedLine = Std::nullOpt;
			if (selectedLineChangedFn)
				selectedLineChangedFn(*this);
		}
		else if (selectedLine.Value() < index)
		{
			selectedLine.Value() -= 1;
			if (selectedLineChangedFn)
				selectedLineChangedFn(*this);
		}
	}
}

SizeHint LineList::GetSizeHint(Context const& ctx) const
{
	auto& implData = *static_cast<impl::ImplData*>(ctx.Internal_ImplData());
	auto const textHeight = implData.textManager.lineheight;
	auto const lineHeight = textHeight + (textMargin * 2);
	SizeHint returnVal = {};
	returnVal.preferred.height = lineHeight * lines.size();
	for (auto const& line : lines)
	{
		auto const sizeHint = impl::TextManager::GetSizeHint(
			implData.textManager, 
			{ line.data(), line.size() });
		returnVal.preferred.width = Math::Max(
			returnVal.preferred.width,
			sizeHint.preferred.width);
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
	impl::PointerMove_Params params = {};
	params.widget = this;
	params.ctx = &ctx;
	params.widgetRect = &widgetRect;
	params.visibleRect = &visibleRect;
	params.pointerId = impl::cursorPointerId;
	params.pointerPos = { (f32)event.position.x, (f32)event.position.y };
	params.pointerOccluded = occluded;

	return impl::LineListImpl::PointerMove(params);
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
	return impl::LineListImpl::PointerPress(
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
	impl::PointerMove_Params params = {};
	params.widget = this;
	params.ctx = &ctx;
	params.widgetRect = &widgetRect;
	params.visibleRect = &visibleRect;
	params.pointerId = event.id;
	params.pointerPos = event.position;
	params.pointerOccluded = occluded;

	return impl::LineListImpl::PointerMove(params);
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
	return impl::LineListImpl::PointerPress(
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

	auto const textHeight = implData.textManager.lineheight;
	auto const lineHeight = textHeight + (textMargin * 2);
	uSize const visibleBegin = impl::LineList_GetFirstVisibleLine(lineHeight, widgetRect.position.y, visibleRect.position.y);
	uSize const visibleEnd = impl::LineList_GetLastVisibleLine(
		lineHeight,
		widgetRect.position.y,
		visibleRect.position.y,
		visibleRect.extent.height);

	for (uSize i = visibleBegin; i < visibleEnd; i += 1)
	{
		auto const lineRect = impl::GetLineRect(
			widgetRect.position,
			widgetRect.extent.width,
			lineHeight,
			i);

		if (selectedLine.HasValue() && selectedLine.Value() == i)
		{
			drawInfo.PushFilledQuad(lineRect, highlightOverlayColor);
		}
		else if (lineCursorHover.HasValue() && lineCursorHover.Value() == i)
		{
			drawInfo.PushFilledQuad(lineRect, hoverOverlayColor);
		}
		else if (i % 2 == 0)
		{
			drawInfo.PushFilledQuad(lineRect, alternatingLineOverlayColor);
		}
	}

	for (uSize i = visibleBegin; i < Math::Min(visibleEnd, lines.size()); i++)
	{
		auto textRect = impl::GetLineRect(
			widgetRect.position,
			widgetRect.extent.height,
			lineHeight,
			i);
		textRect.position.x += textMargin;
		textRect.position.y += textMargin;

		auto& line = lines[i];
		impl::TextManager::RenderText(
			implData.textManager,
			{ line.data(), line.size() },
			{ 1.f, 1.f, 1.f, 1.f },
			textRect,
			drawInfo);
	}
}