#include <DEngine/Gui/LineList.hpp>

#include <DEngine/Gui/Context.hpp>

#include <DEngine/Math/Common.hpp>

#include "ImplData.hpp"

using namespace DEngine;
using namespace DEngine::Gui;

void LineList::AddLine(std::string_view const& text)
{
	lines.push_back(std::string(text));
}

std::string_view LineList::GetLine(uSize index) const
{
	DENGINE_IMPL_GUI_ASSERT(index < lines.size());
	return lines[index];
}

Std::Opt<uSize> LineList::GetSelectedLine() const
{
	return selectedLine;
}

uSize LineList::LineCount() const
{
	return lines.size();
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

void LineList::SetCanSelect(bool in)
{
	if (!in)
		selectedLine = Std::nullOpt;
	canSelect = in;
}

SizeHint LineList::GetSizeHint(Context const& ctx) const
{
	impl::ImplData& implData = *static_cast<impl::ImplData*>(ctx.Internal_ImplData());
	SizeHint returnVal{};
	returnVal.preferred.height = static_cast<u32>(implData.textManager.lineheight * lines.size());
	for (auto const& line : lines)
	{
		SizeHint sizeHint = impl::TextManager::GetSizeHint(
			implData.textManager, 
			line);
		returnVal.preferred.width = Math::Max(returnVal.preferred.width, sizeHint.preferred.width);
	}
	returnVal.expandX = true;
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
	if (cursorIsInside && event.button == CursorButton::Left && event.clicked == false && canSelect)
	{
		impl::ImplData& implData = *static_cast<impl::ImplData*>(ctx.Internal_ImplData());

		uSize begin = static_cast<uSize>((visibleRect.position.y - widgetRect.position.y) / (f32)implData.textManager.lineheight);
		uSize canHoldLineNum = static_cast<uSize>(visibleRect.extent.height / (f32)implData.textManager.lineheight) + 1;
		uSize end = begin + canHoldLineNum + 1;
		end = Math::Min(end, lines.size());

		for (uSize i = begin; i < end; i++)
		{
			Rect lineRect = widgetRect;
			lineRect.extent.height = implData.textManager.lineheight;
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

			uSize begin = static_cast<uSize>((visibleRect.position.y - widgetRect.position.y) / (f32)implData.textManager.lineheight);
			uSize canHoldLineNum = static_cast<uSize>(visibleRect.extent.height / (f32)implData.textManager.lineheight) + 1;
			uSize end = begin + canHoldLineNum + 1;
			end = Math::Min(end, lines.size());

			for (uSize i = begin; i < end; i++)
			{
				Rect lineRect = widgetRect;
				lineRect.extent.height = implData.textManager.lineheight;
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
	impl::ImplData& implData = *static_cast<impl::ImplData*>(ctx.Internal_ImplData());

	visibleRect = Rect::Intersection(widgetRect, visibleRect);
	bool anythingVisible = !visibleRect.IsNothing();
	if (!anythingVisible)
		return;

	uSize begin = static_cast<uSize>((visibleRect.position.y - widgetRect.position.y) / (f32)implData.textManager.lineheight);
	uSize canHoldLineNum = uSize(visibleRect.extent.height / (f32)implData.textManager.lineheight) + 1;
	uSize end = begin + canHoldLineNum + 1;
	end = Math::Min(end, lines.size());

	if (selectedLine.HasValue())
	{
		Rect selectedLineRect = widgetRect;
		selectedLineRect.extent.height = implData.textManager.lineheight;
		selectedLineRect.position.y += static_cast<u32>(selectedLine.Value() * selectedLineRect.extent.height);
		if (!Rect::Intersection(selectedLineRect, visibleRect).IsNothing())
			drawInfo.PushFilledQuad(selectedLineRect, { 1.f, 1.f, 1.f, 0.5f });
	}

	for (uSize i = begin; i < end; i++)
	{
		Rect textRect{};
		textRect.position.x = widgetRect.position.x;
		textRect.position.y = static_cast<u32>(widgetRect.position.y + i * implData.textManager.lineheight);
		textRect.extent.width = widgetRect.extent.width;
		textRect.extent.height = implData.textManager.lineheight;

		impl::TextManager::RenderText(
			implData.textManager,
			lines[i],
			{ 1.f, 1.f, 1.f, 1.f },
			textRect,
			drawInfo);
	}
}