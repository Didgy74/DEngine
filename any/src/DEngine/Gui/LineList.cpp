#include <DEngine/Gui/LineList.hpp>

#include <DEngine/Gui/Context.hpp>

#include <DEngine/Math/Common.hpp>

#include "ImplData.hpp"

using namespace DEngine;
using namespace DEngine::Gui;

namespace DEngine::Gui::impl
{
	[[nodiscard]] uSize LineList_GetFirstVisibleLine(
		u32 lineHeight,
		i32 widgetPosY,
		i32 visiblePosY) noexcept
	{
		return static_cast<uSize>((visiblePosY - widgetPosY) / (f32)lineHeight);
	}

	[[nodiscard]] uSize LineList_GetVisibleLineCount(
		u32 lineHeight,
		u32 visibleHeight) noexcept
	{
		return static_cast<uSize>(visibleHeight / (f32)lineHeight) + 2;
	}
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
	SizeHint returnVal{};
	returnVal.preferred.height = static_cast<u32>(lineHeight * lines.size());
	for (auto const& line : lines)
	{
		SizeHint sizeHint = impl::TextManager::GetSizeHint(
			implData.textManager, 
			line);
		returnVal.preferred.width = Math::Max(returnVal.preferred.width, sizeHint.preferred.width);
	}
	return returnVal;
}

void LineList::CursorClick(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	Math::Vec2Int cursorPos,
	CursorClickEvent event)
{
	bool cursorIsInside = widgetRect.PointIsInside(cursorPos) && visibleRect.PointIsInside(cursorPos);
	if (cursorIsInside && event.button == CursorButton::Primary && event.clicked == false && canSelect)
	{
		impl::ImplData& implData = *static_cast<impl::ImplData*>(ctx.Internal_ImplData());

		u32 const lineHeight = implData.textManager.lineheight;
		uSize const visibleBegin = impl::LineList_GetFirstVisibleLine(lineHeight, widgetRect.position.y, visibleRect.position.y);
		uSize const visibleEnd = visibleBegin + impl::LineList_GetVisibleLineCount(lineHeight, visibleRect.extent.height);
		uSize const lineEnd = Math::Min(visibleEnd, lines.size());

		for (uSize i = visibleBegin; i < lineEnd; i++)
		{
			Rect lineRect = widgetRect;
			lineRect.extent.height = lineHeight;
			lineRect.position.y += static_cast<u32>(i * lineRect.extent.height);
			if (lineRect.PointIsInside(cursorPos))
			{
				selectedLine = Std::Opt<uSize>{ i };

				if (selectedLineChangedCallback)
					selectedLineChangedCallback(*this);

				break;
			}
		}
	}
}

void LineList::TouchEvent(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	Gui::TouchEvent event)
{
	if (event.type == TouchEventType::Up)
	{
		bool touchIsInside = widgetRect.PointIsInside(event.position) && visibleRect.PointIsInside(event.position);
		if (touchIsInside && canSelect)
		{
			impl::ImplData& implData = *static_cast<impl::ImplData*>(ctx.Internal_ImplData());

			u32 const lineHeight = implData.textManager.lineheight;
			uSize const visibleBegin = impl::LineList_GetFirstVisibleLine(lineHeight, widgetRect.position.y, visibleRect.position.y);
			uSize const visibleEnd = visibleBegin + impl::LineList_GetVisibleLineCount(lineHeight, visibleRect.extent.height);
			uSize const lineEnd = Math::Min(visibleEnd, lines.size());

			for (uSize i = visibleBegin; i < lineEnd; i++)
			{
				Rect lineRect = widgetRect;
				lineRect.extent.height = lineHeight;
				lineRect.position.y += static_cast<u32>(i * lineRect.extent.height);
				if (lineRect.PointIsInside(event.position))
				{
					selectedLine = Std::Opt<uSize>{ i };

					if (selectedLineChangedCallback)
						selectedLineChangedCallback(*this);

					break;
				}
			}
		}
	}
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
	uSize const visibleEnd = visibleBegin + impl::LineList_GetVisibleLineCount(lineHeight, visibleRect.extent.height);
	uSize const lineEnd = Math::Min(visibleEnd, lines.size());

	for (uSize i = visibleBegin; i < visibleEnd; i += 1)
	{
		Rect lineRect{};
		lineRect.position.x = widgetRect.position.x;
		lineRect.position.y = static_cast<u32>(widgetRect.position.y + i * lineHeight);
		lineRect.extent.width = widgetRect.extent.width;
		lineRect.extent.height = lineHeight;

		if (i % 2 == 0 && (!selectedLine.HasValue() ||  selectedLine.Value() != i))
		{
			drawInfo.PushFilledQuad(lineRect, alternatingLineColor);
		}
	}

	if (selectedLine.HasValue())
	{
		Rect selectedLineRect = widgetRect;
		selectedLineRect.extent.height = lineHeight;
		selectedLineRect.position.y += static_cast<u32>(selectedLine.Value() * lineHeight);
		if (!Rect::Intersection(selectedLineRect, visibleRect).IsNothing())
			drawInfo.PushFilledQuad(selectedLineRect, highlightColor);
	}

	for (uSize i = visibleBegin; i < lineEnd; i++)
	{
		Rect textRect{};
		textRect.position.x = widgetRect.position.x + (horizontalPadding / 2);
		textRect.position.y = static_cast<u32>(widgetRect.position.y + i * lineHeight);
		textRect.extent.width = widgetRect.extent.width;
		textRect.extent.height = lineHeight;

		impl::TextManager::RenderText(
			implData.textManager,
			lines[i],
			{ 1.f, 1.f, 1.f, 1.f },
			textRect,
			drawInfo);
	}
}