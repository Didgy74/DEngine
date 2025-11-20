#include <DEngine/Gui/StdWidgets/LineList.hpp>



import DEngine.Gui.DrawEngine;
import DEngine.Gui.FontFaceSizeId;
import DEngine.Gui.TextEngine;

using namespace DEngine;
using namespace DEngine::Gui;

namespace DEngine::Gui::impl
{
	[[nodiscard]] static int LineList_GetFirstVisibleLine(
		u32 lineHeight,
		i32 widgetPosY,
		i32 visiblePosY,
		uSize linecount) noexcept
	{
		auto temp = visiblePosY - widgetPosY;
		temp = Std::Max(temp, 0);
		auto b = (uSize)Std::Floor((f32)temp / (f32)lineHeight);
		DENGINE_IMPL_ASSERT(b >= 0);
		b = Std::Min(linecount, b);
		return b;
	}

	[[nodiscard]] static uSize LineList_GetVisibleLineCount(
		u32 lineHeight,
		u32 visibleHeight) noexcept
	{
		return (uSize)Std::Ceil((f32)visibleHeight / (f32)lineHeight) + 1;
	}

	[[nodiscard]] static int LineList_GetLastVisibleLine(
		u32 lineHeight,
		i32 widgetPosY,
		i32 visiblePosY,
		u32 visibleHeight,
		uSize linecount) noexcept
	{
		auto const visibleBegin = impl::LineList_GetFirstVisibleLine(
			lineHeight,
			widgetPosY,
			visiblePosY,
			linecount);
		auto temp = visibleBegin + impl::LineList_GetVisibleLineCount(lineHeight, visibleHeight);
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

void LineList::RemoveLine(uSize index) {
	DENGINE_IMPL_GUI_ASSERT(index < lines.size());

	lines.erase(lines.begin() + static_cast<long long>(index));
	if (selectedLine.HasValue()) {
		if (selectedLine.Value() == index) {
			selectedLine = Std::nullOpt;
		}
		else if (selectedLine.Value() < index) {
			selectedLine.Value() -= 1;
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

		u32 totalLineHeight = 0;
		u32 marginAmount = 0;
		FontFaceSizeId fontSizeId = FontFaceSizeId::Invalid;

		// Only included when we are rendering
		Std::Vec<Rect, RectCollection::AllocRefT> lineGlyphRects;
		Std::Vec<uSize, RectCollection::AllocRefT> lineGlyphRectOffsets;
	};


	static constexpr u8 cursorPointerId = (u8)-1;

	enum class PointerType : u8 { Primary, Secondary };
	[[nodiscard]] static constexpr PointerType ToPointerType(Gui::CursorButton in) noexcept {
		switch (in)
		{
			case Gui::CursorButton::Primary: return PointerType::Primary;
			case Gui::CursorButton::Secondary: return PointerType::Secondary;
			default: break;
		}
		DENGINE_IMPL_UNREACHABLE();
		return {};
	}

	struct PointerPress_Pointer {
		u8 id;
		PointerType type;
		Math::Vec2 pos;
		bool pressed;
	};

	struct PointerPress_Params {
		WidgetEvent_DeferredJobQueue& jobQueue;
		LineList& widget;
		RectCollection const& rectColl;
		Std::Opt<RectCollection::Iter> const& rectCollIter;
		TextEngine& textManager;
		PointerPress_Pointer const& pointer;
		bool eventConsumed;
	};

	struct PointerMove_Pointer {
		u8 id;
		Math::Vec2 pos;
	};
	struct PointerMove_Params {
		LineList& widget;
		RectCollection const& rectColl;
		Std::Opt<RectCollection::Iter> const& rectCollIter;
		TextEngine& textManager;
		PointerMove_Pointer const& pointer;
		bool pointerOccluded;
	};

	[[nodiscard]] static TouchEventConsumption PointerPress(
		PointerPress_Params const& params);

	[[nodiscard]] static TouchEventConsumption PointerMove(
		PointerMove_Params const& params);


	static void RenderBackgroundLines(
		LineList const& lineList,
		Rect const& widgetRect,
		Rect const& visibleRect,
		int totalLineHeight,
		DrawEngine& drawInfo);
};

Widget::GetSizeHint2_ReturnT LineList::GetSizeHint2(
	Widget::GetSizeHint2_Params const& params) const
{
	auto const& window = params.window;
	auto const& fontScale = params.FontScale();
	auto const& minimumSizeCm = params.MinimumHeightCm();
	auto& pusher = params.pusher;
	auto& textEngine = params.textEngine;
	auto const& appData = params.appData;

	Theme theme;
	if (getThemeFn)
		theme = getThemeFn({ .widget = *this, .appData = appData });

	auto normalTextScale = fontScale * window.contentScale;
	auto normalFontSizeId = textEngine.GetFontFaceSizeId(normalTextScale, window.dpi, window.dpi);
	auto normalHeight = textEngine.GetFontFaceSizeMetrics(normalFontSizeId).lineHeight;
	auto marginAmount = theme.textMargin.ResolvePx(window.dpi, window.contentScale);
	auto totalLineHeight = normalHeight + 2*marginAmount;

	auto const& pusherIt = pusher.AddEntry(*this);
	auto& customData = pusher.AttachCustomData(pusherIt, Impl::CustomData{ pusher.Alloc() });
	customData.totalLineHeight = totalLineHeight;
	customData.marginAmount = marginAmount;
	customData.fontSizeId = normalFontSizeId;

	SizeHint returnVal = {};
	returnVal.minimum.height = totalLineHeight * lines.size();

	returnVal.minimum.width = Std::Max(
		returnVal.minimum.width,
		(u32)CmToPixels(minimumSizeCm, window.dpi));
	returnVal.minimum.height = Std::Max(
		returnVal.minimum.height,
		(u32)lines.size() * (u32)CmToPixels(minimumSizeCm, window.dpi));

	pusher.SetSizeHint(pusherIt, returnVal, false);

	return {
		.iter = pusherIt,
		.sizeHint = returnVal };
}

void LineList::BuildChildRects(
	Widget::BuildChildRects_Params const& params,
	Rect const& widgetRect,
	Rect const& visibleRect,
	Std::Opt<RectCollection::Iter> const& rectCollIter) const
{
	auto& pusher = params.pusher;
	auto& textManager = params.textEngine;

	if (pusher.IncludeRendering()) {
		auto* customDataPtr = pusher.GetCustomData<Impl::CustomData>(*this);
		DENGINE_IMPL_ASSERT(customDataPtr);
		auto& customData = *customDataPtr;

		auto const totalLineHeight = customData.totalLineHeight;
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
		visibleEnd = Std::Min(linecount, visibleEnd);

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
		for (int i = 0; i < visibleLineCount; i += 1) {
			auto const lineIndex = i + visibleBegin;

			auto const& line = lines[lineIndex];
			auto const lineGlyphRectOffset = customData.lineGlyphRectOffsets[i];
			textManager.GetOuterExtent(
				{ line.data(), line.size() },
				customData.fontSizeId,
				customData.lineGlyphRects.ToSpan().Subspan(lineGlyphRectOffset, line.size()));
		}
	}
}

void LineList::Impl::RenderBackgroundLines(
	LineList const& lineList,
	Rect const& widgetRect,
	Rect const& visibleRect,
	int totalLineHeight,
	DrawEngine& drawInfo)
{
	auto linecount = lineList.lines.size();
	auto const& selectedLine = lineList.selectedLine;
	auto const& lineCursorHover = lineList.lineCursorHover;

	auto visibleBegin = impl::LineList_GetFirstVisibleLine(
		totalLineHeight,
		widgetRect.position.y,
		visibleRect.position.y,
		linecount);
	auto visibleEnd = impl::LineList_GetLastVisibleLine(
		totalLineHeight,
		widgetRect.position.y,
		visibleRect.position.y,
		visibleRect.extent.height,
		linecount);

	auto const visibleLineCount = visibleEnd - visibleBegin;
	for (uSize i = visibleBegin; i < visibleEnd; i += 1) {
		auto const lineRect = impl::GetLineRect(
			widgetRect.position,
			widgetRect.extent.width,
			totalLineHeight,
			i);

		if (selectedLine.HasValue() && selectedLine.Value() == i) {
			drawInfo.PushFilledQuad(lineRect, highlightOverlayColor);
		} else if (lineCursorHover.HasValue() && lineCursorHover.Value() == i) {
			drawInfo.PushFilledQuad(lineRect, hoverOverlayColor);
		} else if (i % 2 == 0) {
			// TODO: There is a off-by-one bug here where we will start issuing draw commands
			// that are entirely out of bounds.
			if (Intersection(lineRect, visibleRect).IsNothing())
				break;
			drawInfo.PushFilledQuad(lineRect, alternatingLineOverlayColor);
		}
	}
}

void LineList::Render2(
	Render_Params const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter) const
{
	auto& rectColl = params.rectCollection;

	auto rectPairOpt = rectColl.GetRect(*this, rectCollIter);
	DENGINE_IMPL_GUI_ASSERT(rectPairOpt.Has());
	auto const& absWidgetRect = rectPairOpt.Value().widgetRect;
	auto const& absVisibleRect = rectPairOpt.Value().visibleRect;

	if (Intersection(absWidgetRect, absVisibleRect).IsNothing())
		return;

	auto& drawInfo = params.drawEngine;
	auto& textMgr = params.textEngine;

	auto* customDataPtr = rectColl.GetCustomData<Impl::CustomData>(*this);
	DENGINE_IMPL_ASSERT(customDataPtr);
	auto const& customData = *customDataPtr;
	auto totalLineHeight = customData.totalLineHeight;
	auto fontSizeId = customData.fontSizeId;
	auto textMargin = customData.marginAmount;
	auto linecount = (int)this->lines.size();

	Impl::RenderBackgroundLines(
		*this,
		absWidgetRect,
		absVisibleRect,
		totalLineHeight,
		drawInfo);

	auto visibleBegin = impl::LineList_GetFirstVisibleLine(
		totalLineHeight,
		absWidgetRect.position.y,
		absVisibleRect.position.y,
		linecount);
	// The end is inclusive
	auto visibleEnd = impl::LineList_GetLastVisibleLine(
		totalLineHeight,
		absWidgetRect.position.y,
		absVisibleRect.position.y,
		absVisibleRect.extent.height,
		linecount);
	visibleEnd = Std::Min(linecount, visibleEnd);
	auto visibleLineCount = visibleEnd - visibleBegin;

	auto textHeight = textMgr.GetFontFaceSizeMetrics(fontSizeId).lineHeight;;

	for (int i = 0; i < visibleLineCount; i++) {
		auto const lineIndex = i + visibleBegin;
		auto& line = lines[lineIndex];
		auto const& lineRectOffset = customData.lineGlyphRectOffsets[i];
		auto const* lineRects = &customData.lineGlyphRects[lineRectOffset];

		auto lineRect = impl::GetLineRect(
			absWidgetRect.position,
			absWidgetRect.extent.width,
			totalLineHeight,
			lineIndex);
		// Offset to center
		auto textRect = lineRect;
		textRect.extent.height = textHeight;
		textRect.position.x += textMargin;
		textRect.position.y += CenterRangeOffset((int)lineRect.extent.height, (int)textHeight);
		drawInfo.PushText(
			fontSizeId,
			{ line.data(), line.size() },
			lineRects,
			textRect.position,
			{ 1.f, 1.f, 1.f, 1.f });
	}
}

TouchEventConsumption LineList::Impl::PointerPress(
	PointerPress_Params const& params)
{
	auto& widget = params.widget;
	auto const& rectColl = params.rectColl;
	auto const& rectCollIter = params.rectCollIter;
	auto const& pointer = params.pointer;

	auto rectPairOpt = rectColl.GetRect(widget, rectCollIter);
	DENGINE_IMPL_GUI_ASSERT(rectPairOpt.Has());
	auto const& absWidgetRect = rectPairOpt.Value().widgetRect;
	auto const& absVisibleRect = rectPairOpt.Value().visibleRect;

	TouchEventConsumption result = {};
	result.consumed = params.eventConsumed;

	if (pointer.type != PointerType::Primary) {
		return result;
	}

	auto const pointerInside =
		absWidgetRect.PointIsInside(pointer.pos) &&
		absVisibleRect.PointIsInside(pointer.pos);
	result.consumed = result.consumed || pointerInside;

	auto const* customDataPtr = rectColl.GetCustomData<Impl::CustomData>(widget);
	DENGINE_IMPL_GUI_ASSERT(customDataPtr);
	auto const& customData = *customDataPtr;

	auto const totalLineHeight = customData.totalLineHeight;

	auto const hoveredIndex = impl::GetHoveredIndex(
		(i32)pointer.pos.y,
		absWidgetRect.position.y,
		totalLineHeight);

	if (widget.currPressedLine.HasValue()) {
		auto const pressedLine = widget.currPressedLine.Value();

		if (!pointer.pressed && pressedLine.pointerId == pointer.id) {
		 	widget.currPressedLine = Std::nullOpt;

			bool selectedLineChanged = false;
			if (pressedLine.lineIndex.HasValue()) {
				// We are currently holding an existing line.
				// Check if we unpressed that same line
				auto const lineIndex = pressedLine.lineIndex.Value();
				if (lineIndex == hoveredIndex) {
					widget.selectedLine = lineIndex;
					selectedLineChanged = true;
				}
			} else
			{
				// We were currently not holding a line
				// Check if we unpressed outside any lines
				if (hoveredIndex >= widget.lines.size()) {
					widget.selectedLine = Std::nullOpt;
					selectedLineChanged = true;
				}
			}

			if (selectedLineChanged && widget.selectedLineChangedFn) {
				widget.selectedLineChangedFn(widget, params.jobQueue);
			}
		}
	}
	else
	{
		// We are not currently pressing a line, check if we hit a new line
		// on the event if it was not already consumed
		if (pointer.pressed && pointerInside && !params.eventConsumed) {
			// We don't want to go into pressed state if we
			// pressed a line that is already selected.
			auto const hoveringSelectingLine =
				widget.selectedLine.HasValue() &&
				widget.selectedLine.Value() == hoveredIndex;
			if (!hoveringSelectingLine) {
				// We did not press a line that is already selected.
				LineList::PressedLine_T newPressedLine = {};
				newPressedLine.pointerId = pointer.id;
				if (hoveredIndex < widget.lines.size()) {
					newPressedLine.lineIndex = hoveredIndex;
				}
				else {
					newPressedLine.lineIndex = Std::nullOpt;
				}
				widget.currPressedLine = newPressedLine;
			}
		}
	}

	return result;
}

TouchEventConsumption LineList::Impl::PointerMove(
	PointerMove_Params const& params)
{
	auto& widget = params.widget;
	auto const& rectColl = params.rectColl;
	auto const& rectCollIter = params.rectCollIter;
	auto const& pointer = params.pointer;

	auto rectPairOpt = rectColl.GetRect(widget, rectCollIter);
	DENGINE_IMPL_GUI_ASSERT(rectPairOpt.Has());
	auto const& absWidgetRect = rectPairOpt.Value().widgetRect;
	auto const& absVisibleRect = rectPairOpt.Value().visibleRect;

	TouchEventConsumption result = {};
	result.consumed = params.pointerOccluded;

	auto const pointerInside =
		absWidgetRect.PointIsInside(pointer.pos) &&
		absVisibleRect.PointIsInside(pointer.pos);

	auto* customDataPtr = rectColl.GetCustomData<Impl::CustomData>(widget);
	DENGINE_IMPL_GUI_ASSERT(customDataPtr);
	auto const& customData = *customDataPtr;

	auto const totalLineHeight = customData.totalLineHeight;

	if (pointer.id == cursorPointerId) {
		if (!pointerInside || params.pointerOccluded) {
			widget.lineCursorHover = Std::nullOpt;
		}
		else {
			auto const hoveredIndex = impl::GetHoveredIndex(
				(i32)pointer.pos.y,
				absWidgetRect.position.y,
				totalLineHeight);
			if (hoveredIndex < widget.lines.size())
				widget.lineCursorHover = hoveredIndex;
			else
				widget.lineCursorHover = Std::nullOpt;
		}
	}
	// If the parent Widget consumed this event, we cancel ongoing gesture tracking
	// for this pointer-id.
	if (widget.currPressedLine.Has()) {
		auto const currPressedLine = widget.currPressedLine.Get();
		if (currPressedLine.pointerId == pointer.id && result.consumed) {
			widget.currPressedLine = Std::nullOpt;
		}
	}

	result.consumed = result.consumed || pointerInside;

	return result;
}

bool LineList::CursorPress2(
	Widget::CursorPressParams const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter,
	bool consumed)
{
	Impl::PointerPress_Pointer pointer = {
		.id = Impl::cursorPointerId,
		.type = Impl::ToPointerType(params.CursorButton()),
		.pos = { (f32)params.cursorPos.x, (f32)params.cursorPos.y },
		.pressed = params.CursorPressed(), };

	Impl::PointerPress_Params tempParams = {
		.jobQueue = params.jobQueue,
		.widget = *this,
		.rectColl = params.rectCollection,
		.rectCollIter = rectCollIter,
		.textManager = params.textEngine,
		.pointer = pointer,
		.eventConsumed = consumed };
	auto temp = Impl::PointerPress(tempParams);
	return temp.consumed;
}

bool LineList::CursorMove(
	Widget::CursorMoveParams const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter,
	bool occluded)
{
	auto cursorPos = params.CursorPos();

	Impl::PointerMove_Pointer pointer = {
		.id = Impl::cursorPointerId,
		.pos = { (f32)cursorPos.x, (f32)cursorPos.y } };

	Impl::PointerMove_Params tempParams = {
		.widget = *this,
		.rectColl = params.rectCollection,
		.rectCollIter = rectCollIter,
		.textManager = params.textEngine,
		.pointer = pointer,
		.pointerOccluded = occluded };
	auto temp = Impl::PointerMove(tempParams);
	return temp.consumed;
}

TouchEventConsumption LineList::WidgetEvent_TouchMove(
	WidgetEvent_TouchMoveParams const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter,
	bool occluded)
{
	Impl::PointerMove_Pointer pointer = {
		.id = params.event.id,
		.pos = params.event.position, };

	Impl::PointerMove_Params temp = {
		.widget = *this,
		.rectColl = params.rectCollection,
		.rectCollIter = rectCollIter,
		.textManager = params.textEngine,
		.pointer = pointer,
		.pointerOccluded = occluded };
	return Impl::PointerMove(temp);
}

TouchEventConsumption LineList::WidgetEvent_TouchPress(
	WidgetEvent_TouchPressParams const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter,
	bool consumed)
{
	Impl::PointerPress_Pointer pointer = {
		.id = params.event.id,
		.type = Impl::PointerType::Primary,
		.pos = params.event.position,
		.pressed = params.event.pressed };

	Impl::PointerPress_Params temp = {
		.jobQueue = params.jobQueue,
		.widget = *this,
		.rectColl = params.rectCollection,
		.rectCollIter = rectCollIter,
		.textManager = params.textEngine,
		.pointer = pointer,
		.eventConsumed = consumed };
	return Impl::PointerPress(temp);
}

void LineList::CursorExit() {
	lineCursorHover = Std::nullOpt;
}
