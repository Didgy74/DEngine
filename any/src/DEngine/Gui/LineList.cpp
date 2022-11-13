#include <DEngine/Gui/LineList.hpp>
#include <DEngine/Gui/Context.hpp>
#include <DEngine/Gui/TextManager.hpp>

#include <DEngine/Std/Containers/Vec.hpp>

using namespace DEngine;
using namespace DEngine::Gui;

namespace DEngine::Gui::impl
{
	[[nodiscard]] static uSize LineList_GetFirstVisibleLine(
		u32 lineHeight,
		i32 widgetPosY,
		i32 visiblePosY,
		uSize linecount) noexcept
	{
		auto temp = visiblePosY - widgetPosY;
		auto b = (uSize)Math::Floor((f32)temp / (f32)lineHeight);
		DENGINE_IMPL_ASSERT(b >= 0);
		b = Math::Min(linecount, b);
		return b;
	}

	[[nodiscard]] static uSize LineList_GetVisibleLineCount(
		u32 lineHeight,
		u32 visibleHeight) noexcept
	{
		return (uSize)Math::Ceil((f32)visibleHeight / (f32)lineHeight);
	}

	[[nodiscard]] static uSize LineList_GetLastVisibleLine(
		u32 lineHeight,
		i32 widgetPosY,
		i32 visiblePosY,
		u32 visibleHeight,
		uSize linecount) noexcept
	{
		uSize const visibleBegin = impl::LineList_GetFirstVisibleLine(
			lineHeight,
			widgetPosY,
			visiblePosY,
			linecount);
		auto temp = visibleBegin + impl::LineList_GetVisibleLineCount(lineHeight, visibleHeight);
		temp = Math::Min(linecount, temp);
		return temp;
	}

	[[nodiscard]] static Rect GetLineRect(
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

	[[nodiscard]] static uSize GetHoveredIndex(
		i32 pointerPosY,
		i32 widgetPosY,
		u32 lineHeight) noexcept
	{
		uSize hoveredIndex = (pointerPosY - widgetPosY) / lineHeight;
		// Hitting an index below 0 is a bug.
		DENGINE_IMPL_GUI_ASSERT(hoveredIndex >= 0);
		return hoveredIndex;
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
			if (selectedLineChangedFn)
				selectedLineChangedFn(*this, nullptr);
		}
		else if (selectedLine.Value() < index)
		{
			selectedLine.Value() -= 1;
			if (selectedLineChangedFn)
				selectedLineChangedFn(*this, nullptr);
		}
	}
}

struct LineList::Impl
{
	// A reference for the serialized struct we use.
	struct CustomData
	{
		explicit CustomData(RectCollection::AllocRefT const& alloc) noexcept :
			lineGlyphRects{ alloc },
			lineGlyphRectOffsets{ alloc}
		{
		}

		// Does NOT include any margins.
		u32 lineHeight = {};

		// Only included when we are rendering
		FontFaceId fontFaceId;
		Std::Vec<Rect, RectCollection::AllocRefT> lineGlyphRects;
		Std::Vec<uSize, RectCollection::AllocRefT> lineGlyphRectOffsets;
	};


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

	struct PointerPress_Pointer
	{
		u8 id;
		PointerType type;
		Math::Vec2 pos;
		bool pressed;
	};

	struct PointerPress_Params
	{
		Context& ctx;
		LineList& widget;
		RectCollection const& rectCollection;
		TextManager& textManager;
		Rect const& widgetRect;
		Rect const& visibleRect;
		PointerPress_Pointer const& pointer;
		bool eventConsumed;
	};

	struct PointerMove_Pointer
	{
		u8 id;
		Math::Vec2 pos;
	};
	struct PointerMove_Params
	{
		LineList& widget;
		RectCollection const& rectCollection;
		TextManager& textManager;
		Rect const& widgetRect;
		Rect const& visibleRect;
		PointerMove_Pointer const& pointer;
		bool pointerOccluded;
	};

	[[nodiscard]] static bool PointerPress(
		PointerPress_Params const& params);

	[[nodiscard]] static bool PointerMove(
		PointerMove_Params const& params);
};

SizeHint LineList::GetSizeHint2(
	Widget::GetSizeHint2_Params const& params) const
{
	auto const& ctx = params.ctx;
	auto const& window = params.window;
	auto& pusher = params.pusher;
	auto& transientAlloc = params.transientAlloc;
	auto& textManager = params.textManager;

	auto textSize = ctx.fontScale * window.contentScale;

	auto const& pusherIt = pusher.AddEntry(*this);

	auto customData = Impl::CustomData{ pusher.Alloc() };
	customData.lineHeight = textManager.GetLineheight(textSize, window.dpiX, window.dpiY);
	if (pusher.IncludeRendering()) {
		customData.fontFaceId = textManager.GetFontFaceId(textSize, window.dpiX, window.dpiY);
	}

	SizeHint returnValue = {};
	returnValue.minimum.width = 100;
	returnValue.minimum.height = customData.lineHeight * lines.size();
	returnValue.minimum.height += textMargin * 2 * lines.size();

	pusher.AttachCustomData(pusherIt, Std::Move(customData));
	pusher.SetSizeHint(pusherIt, returnValue);

	return returnValue;
}

void LineList::BuildChildRects(
	Widget::BuildChildRects_Params const& params,
	Rect const& widgetRect,
	Rect const& visibleRect) const
{
	auto const& ctx = params.ctx;
	auto const& window = params.window;
	auto& pusher = params.pusher;
	auto& textManager = params.textManager;

	auto textSize = ctx.fontScale * window.contentScale;

	if (pusher.IncludeRendering())
	{
		auto* customDataPtr = pusher.GetCustomData2<Impl::CustomData>(*this);
		DENGINE_IMPL_ASSERT(customDataPtr);
		auto& customData = *customDataPtr;

		auto const totalLineHeight = customData.lineHeight + textMargin * 2;
		auto const linecount = (int)lines.size();

		auto visibleBegin = (int)impl::LineList_GetFirstVisibleLine(
			totalLineHeight,
			widgetRect.position.y,
			visibleRect.position.y,
			linecount);
		auto visibleEnd = (int)impl::LineList_GetLastVisibleLine(
			totalLineHeight,
			widgetRect.position.y,
			visibleRect.position.y,
			visibleRect.extent.height,
			linecount);

		// First count up the total amount of Rects we need
		auto const visibleLineCount = visibleEnd - visibleBegin;

		customData.lineGlyphRectOffsets.Resize(visibleLineCount);
		uSize rectCount = 0;
		for (int i = 0; i < visibleLineCount; i += 1) {
			customData.lineGlyphRectOffsets[i] = rectCount;
			auto const lineIndex = i + visibleBegin;
			rectCount += lines[lineIndex].size();
		}

		// Then allocate the space we need for these rects, and load into them.
		customData.lineGlyphRects.Resize(rectCount);
		for (int i = 0; i < visibleLineCount; i += 1)
		{
			auto const lineIndex = i + visibleBegin;

			auto posOffset = widgetRect.position;
			posOffset.y += (i32)totalLineHeight * lineIndex;
			posOffset.x += (i32)textMargin;
			posOffset.y += (i32)textMargin;

			auto const& line = lines[lineIndex];
			auto const lineGlyphRectOffset = customData.lineGlyphRectOffsets[i];
			Std::Span lineGlyphRects {
				&customData.lineGlyphRects[lineGlyphRectOffset],
				line.size() };

			textManager.GetOuterExtent(
				{ line.data(), line.size() },
				textSize,
				window.dpiX,
				window.dpiY,
				lineGlyphRects.Data());
			// Then apply the pos offset to the rects
			for (auto& item : lineGlyphRects)
				item.position += posOffset;
		}
	}
}

void LineList::Render2(
	Widget::Render_Params const& params,
	Rect const& widgetRect,
	Rect const& visibleRect) const
{
	if (Rect::Intersection(widgetRect, visibleRect).IsNothing())
		return;

	auto& rectCollection = params.rectCollection;
	auto& drawInfo = params.drawInfo;

	auto* customDataPtr = rectCollection.GetCustomData2<Impl::CustomData>(*this);
	DENGINE_IMPL_ASSERT(customDataPtr);
	auto const& customData = *customDataPtr;

	auto const totalLineHeight = customData.lineHeight + textMargin * 2;

	auto const linecount = lines.size();

	auto const visibleBegin = impl::LineList_GetFirstVisibleLine(
		totalLineHeight,
		widgetRect.position.y,
		visibleRect.position.y,
		linecount);
	auto const visibleEnd = impl::LineList_GetLastVisibleLine(
		totalLineHeight,
		widgetRect.position.y,
		visibleRect.position.y,
		visibleRect.extent.height,
		linecount);

	auto const visibleLineCount = visibleEnd - visibleBegin;

	for (uSize i = visibleBegin; i < visibleEnd; i += 1)
	{
		auto const lineRect = impl::GetLineRect(
			widgetRect.position,
			widgetRect.extent.width,
			totalLineHeight,
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

	for (uSize i = 0; i < visibleLineCount; i++)
	{
		auto const lineIndex = i + visibleBegin;
		auto& line = lines[lineIndex];
		auto const lineRectOffset = customData.lineGlyphRectOffsets[i];
		auto const* lineRects = &customData.lineGlyphRects[lineRectOffset];

		drawInfo.PushText(
			(u64)customData.fontFaceId,
			{ line.data(), line.size() },
			lineRects,
			{},
			{ 1.f, 1.f, 1.f, 1.f });
	}
}

bool LineList::Impl::PointerPress(
	PointerPress_Params const& params)
{
	auto& ctx = params.ctx;
	auto& widget = params.widget;
	auto& rectCollection = params.rectCollection;
	auto& widgetRect = params.widgetRect;
	auto& visibleRect = params.visibleRect;
	auto& pointer = params.pointer;

	auto newConsumed = params.eventConsumed;

	if (pointer.type != PointerType::Primary)
		return newConsumed;

	auto const pointerInside =
		widgetRect.PointIsInside(pointer.pos) &&
		visibleRect.PointIsInside(pointer.pos);
	newConsumed = newConsumed || pointerInside;

	auto* customDataPtr = rectCollection.GetCustomData2<Impl::CustomData>(widget);
	DENGINE_IMPL_GUI_ASSERT(customDataPtr);
	auto const& customData = *customDataPtr;

	auto const totalLineHeight = customData.lineHeight + widget.textMargin * 2;

	auto const hoveredIndex = impl::GetHoveredIndex(
		(i32)pointer.pos.y,
		widgetRect.position.y,
		totalLineHeight);

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
				widget.selectedLineChangedFn(widget, &ctx);
		}
	}
	else
	{
		// We are not currently pressing a line, check if we hit a new line
		// on the event if it was not already consumed
		if (pointer.pressed && pointerInside && !params.eventConsumed)
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

	return newConsumed;
}

bool LineList::Impl::PointerMove(
	PointerMove_Params const& params)
{
	auto& widget = params.widget;
	auto const& rectCollection = params.rectCollection;
	auto const& widgetRect = params.widgetRect;
	auto const& visibleRect = params.visibleRect;
	auto const& pointer = params.pointer;

	auto newOccluded = params.pointerOccluded;

	auto const pointerInside =
		widgetRect.PointIsInside(pointer.pos) &&
		visibleRect.PointIsInside(pointer.pos);

	auto* customDataPtr = rectCollection.GetCustomData2<Impl::CustomData>(widget);
	DENGINE_IMPL_GUI_ASSERT(customDataPtr);
	auto const& customData = *customDataPtr;

	auto const totalLineHeight = customData.lineHeight + widget.textMargin * 2;

	if (pointer.id == cursorPointerId)
	{
		if (!pointerInside || params.pointerOccluded)
			widget.lineCursorHover = Std::nullOpt;
		else
		{
			auto const hoveredIndex = impl::GetHoveredIndex(
				(i32)pointer.pos.y,
				widgetRect.position.y,
				totalLineHeight);
			if (hoveredIndex < widget.lines.size())
				widget.lineCursorHover = hoveredIndex;
			else
				widget.lineCursorHover = Std::nullOpt;
		}
	}

	newOccluded = newOccluded || pointerInside;

	return newOccluded;
}

bool LineList::CursorPress2(
	Widget::CursorPressParams const& params,
	Rect const& widgetRect,
	Rect const& visibleRect,
	bool consumed)
{
	Impl::PointerPress_Pointer pointer = {};
	pointer.id = Impl::cursorPointerId;
	pointer.pos = { (f32)params.cursorPos.x, (f32)params.cursorPos.y };
	pointer.type = Impl::ToPointerType(params.event.button);
	pointer.pressed = params.event.pressed;

	Impl::PointerPress_Params temp = {
		.ctx = params.ctx,
		.widget = *this,
		.rectCollection = params.rectCollection,
		.textManager = params.textManager,
		.widgetRect = widgetRect,
		.visibleRect = visibleRect,
		.pointer = pointer,
		.eventConsumed = consumed };
	return Impl::PointerPress(temp);
}

bool LineList::CursorMove(
	Widget::CursorMoveParams const& params,
	Rect const& widgetRect,
	Rect const& visibleRect,
	bool occluded)
{
	Impl::PointerMove_Pointer pointer = {};
	pointer.id = Impl::cursorPointerId;
	pointer.pos = { (f32)params.event.position.x, (f32)params.event.position.y };

	Impl::PointerMove_Params temp = {
		.widget = *this,
		.rectCollection = params.rectCollection,
		.textManager = params.textManager,
		.widgetRect = widgetRect,
		.visibleRect = visibleRect,
		.pointer = pointer,
		.pointerOccluded = occluded };
	return Impl::PointerMove(temp);
}

bool LineList::TouchMove2(
	TouchMoveParams const& params,
	Rect const& widgetRect,
	Rect const& visibleRect,
	bool occluded)
{
	Impl::PointerMove_Pointer pointer = {};
	pointer.id = params.event.id;
	pointer.pos = params.event.position;

	Impl::PointerMove_Params temp = {
		.widget = *this,
		.rectCollection = params.rectCollection,
		.textManager = params.textManager,
		.widgetRect = widgetRect,
		.visibleRect = visibleRect,
		.pointer = pointer,
		.pointerOccluded = occluded };
	return Impl::PointerMove(temp);
}

bool LineList::TouchPress2(
	TouchPressParams const& params,
	Rect const& widgetRect,
	Rect const& visibleRect,
	bool consumed)
{
	Impl::PointerPress_Pointer pointer = {};
	pointer.id = params.event.id;
	pointer.pos = params.event.position;
	pointer.type = Impl::PointerType::Primary;
	pointer.pressed = params.event.pressed;

	Impl::PointerPress_Params temp = {
		.ctx = params.ctx,
		.widget = *this,
		.rectCollection = params.rectCollection,
		.textManager = params.textManager,
		.widgetRect = widgetRect,
		.visibleRect = visibleRect,
		.pointer = pointer,
		.eventConsumed = consumed };
	return Impl::PointerPress(temp);
}

void LineList::CursorExit(Context& ctx)
{
	lineCursorHover = Std::nullOpt;
}
