#include <DEngine/Gui/StdWidgets/Utilities/LineEditUtils.hpp>

#include <DEngine/Gui/DebugLog.hpp>

import DEngine.Gui.DrawEngine;
import DEngine.Gui.TextEngine;
import DEngine.Std.RangeFnRef;

using namespace DEngine;
using namespace DEngine::Gui;

namespace impl {
    // TODO: This could probably be turned into some TextUtil function?
    static void DrawTextSelection(
        i32 selectionStartIndex,
        i32 selectionCount,
        u32 caretWidth,
        DrawEngine& drawInfo,
        Math::Vec2Int textOffset,
        Std::Span<char const> text,
        TextEngine& textEngine,
        FontFaceSizeId fontId,
        TextEngine::FontFaceSizeMetrics const& fontFaceSizeMetrics)
    {
        u32 xLeft = textEngine.GetCaretPosX(
            text,
            fontId,
            selectionStartIndex);
        if (selectionCount == 0) {
            drawInfo.PushFilledQuad(
                Rect{
                    .position = { textOffset.x + (i32)xLeft, textOffset.y },
                    .extent = {
                        caretWidth,
                        (u32)fontFaceSizeMetrics.ascender - fontFaceSizeMetrics.descender } },
            { 1, 1, 1, 1 });
        } else {
            auto xRight = textEngine.GetCaretPosX(
                text,
                fontId,
                selectionStartIndex + selectionCount);
            drawInfo.PushFilledQuad(
                Rect{
                    .position = { textOffset.x + (i32)xLeft, textOffset.y },
                    .extent = {
                        xRight - xLeft,
                        (u32)fontFaceSizeMetrics.ascender - fontFaceSizeMetrics.descender } },
            { 0.25, 0.25, 1, 1 });
        }
    }

    template<class GetTextPtrFnT, class ResizeStringFnT>
    void ReplaceText(
        i64 currStringSize,
        GetTextPtrFnT const& getTextPtrFn,
        ResizeStringFnT const& resizeStringFn,
        i64 selIndex,
        i64 selCount,
        Std::RangeFnRef<Std::Trait::RemoveCVRef<decltype(*getTextPtrFn())>> const& newTextRange)
    {
        if (selIndex > currStringSize)
            DENGINE_IMPL_UNREACHABLE();

        auto oldStringSize = currStringSize;
        auto sizeDiff = newTextRange.Size() - selCount;

        // First check if we need to expand our storage.
        if (sizeDiff > 0) {
            // We need to move all content behind the old substring
            // To the right.
            resizeStringFn(oldStringSize + sizeDiff);
            auto* textPtr = getTextPtrFn();
            for (i64 i = oldStringSize - 1; i >= selIndex + selCount; i--)
                textPtr[i + sizeDiff] = textPtr[i];
        } else if (sizeDiff < 0) {
            // We need to move all content behind the old substring
            // To the left.
            auto* textPtr = getTextPtrFn();
            for (auto i = selIndex + selCount; i < oldStringSize; i += 1)
                textPtr[i + sizeDiff] = textPtr[i];
            resizeStringFn(currStringSize + sizeDiff);
        }

        auto* textPtr = getTextPtrFn();
        for (auto i = 0; i < newTextRange.Size(); i += 1)
            textPtr[i + selIndex] = newTextRange.Invoke(i);
    }
}

LineEditUtils::GetSizeHint_Return LineEditUtils::GetSizeHint(
    GetSizeHintParams const& params)
{
    auto& textEngine = params.textEngine;
    auto windowContentScale = params.windowContentScale;
    auto fontScale = params.fontScale;
    auto windowDpi = params.windowDpi;
    auto text = params.text;
    auto const& glyphRects = params.outGlyphRects;
    auto minimumSizeCm = params.minimumSizeCm;
	auto textMarginPx = params.textMarginPx;

    auto fontSizeId = textEngine.GetFontFaceSizeId(
        fontScale * windowContentScale,
        windowDpi,
        windowDpi);

    auto textOuterExtent = textEngine.GetOuterExtent(
        text,
        fontSizeId,
        glyphRects);

    // Calculate the extent we would need to fit the text + some margin
    auto wantedWidgetExtent = textOuterExtent;
    wantedWidgetExtent.height += textMarginPx * 2;
    wantedWidgetExtent.width += textMarginPx * 2;

    SizeHint sizeHint = {};
    sizeHint.minimum = wantedWidgetExtent;
    sizeHint.minimum.width = Std::Max(
        sizeHint.minimum.width,
        (u32)CmToPixels(minimumSizeCm, windowDpi));
    sizeHint.minimum.height = Std::Max(
        sizeHint.minimum.height,
        (u32)CmToPixels(minimumSizeCm, windowDpi));

	sizeHint.minimum.width = Std::Max(sizeHint.minimum.width, sizeHint.minimum.height);

    GetSizeHint_Return returnVal = {};
    returnVal.sizeHint = sizeHint;
    returnVal.fontId = fontSizeId;
    returnVal.textOuterExtent = textOuterExtent;

    return returnVal;
}

void LineEditUtils::Render(Render_Params const& params)
{
    auto& drawInfo = params.renderParams.drawEngine;
	auto const& widget = params.widget;
	auto const& rectColl = params.rectColl;
	auto const& rectCollIter = params.rectCollIter;
    auto& textEngine = params.renderParams.textEngine;
    auto const& backgroundColorIn = params.backgroundColor;
	auto const& textColor = params.textColor;
    auto const& fontId = params.fontId;
    auto const& textOuterExtent = params.textOuterExtent;
    auto const& text = params.text;
    auto const& glyphRects = params.glyphRects;
    auto const& selectionRangeOpt = params.selectionRangeOpt;
    auto const& caretWidth = params.renderParams.CaretWidth();
	auto const& cornerRadiusPx = params.cornerRadiusPx;

	auto rectPairOpt = rectColl.GetRect(widget, rectCollIter);
	DENGINE_IMPL_GUI_ASSERT(rectPairOpt.Has());
	auto const& absWidgetRect = rectPairOpt.Value().widgetRect;
	auto const& absVisibleRect = rectPairOpt.Value().visibleRect;

    auto const intersection = Intersection(absWidgetRect, absVisibleRect);
    if (intersection.IsNothing()) {
    	return;
    }

    // Draw the background
	auto backgroundColor = backgroundColorIn;
	auto widgetIsFocused =
		params.renderParams.focusedWidget.Has()
		&& params.renderParams.focusedWidget.Value().widget == &widget;
	if (widgetIsFocused) {
		DENGINE_IMPL_GUI_ASSERT(!params.renderParams.focusedWidget.Value().secondaryIndex.Has());
		backgroundColor += Math::Vec4{ 0.2, 0.2, 0.2, 0 };
	}
    drawInfo.PushFilledQuad(absWidgetRect, backgroundColor, cornerRadiusPx);

    auto drawScissor = DrawEngine::ScopedScissor(drawInfo, intersection);

    auto fontFaceSizeMetrics = textEngine.GetFontFaceSizeMetrics(fontId);

    // Center based on lowercase 'x'
    // Move the text so the top of of the line matches the centerY of the outer box.
    f64 offset = (f64)absWidgetRect.extent.height / 2;
    // Then move it back up so that the baseline of the line is what matches centerY of the box.
    offset -= fontFaceSizeMetrics.ascender;
    // Then move it back down by half of the x-height
    offset += (f64)fontFaceSizeMetrics.xHeight / 2;
    i32 textOffsetY = (i32)Std::Round(offset);

    // Center it horizontally. Maybe clamp the offset to 0 to not make it go out to the left.
    auto textOffsetX = (i32)Std::Round((f32)absWidgetRect.extent.width / 2.f - (f32)textOuterExtent.width / 2.f);
    textOffsetX = Std::Max(textOffsetX, 0);

    if (selectionRangeOpt.Has()) {
        ::impl::DrawTextSelection(
            (i32)selectionRangeOpt.Get().start,
            (i32)selectionRangeOpt.Get().count,
            caretWidth,
            drawInfo,
            absWidgetRect.position + Math::Vec2Int{ textOffsetX, textOffsetY },
            text,
            textEngine,
            fontId,
            fontFaceSizeMetrics);
    }

	if (text.Empty())
		return;

    drawInfo.PushText(
        fontId,
        text,
        glyphRects.Data(),
        absWidgetRect.position + Math::Vec2Int{ textOffsetX, textOffsetY },
        textColor);
}

TouchEventConsumption LineEditUtils::PointerPress(PointerPress_Params const& params)
{
	auto& rectColl = params.rectColl;
	auto& widget = params.widget;
	auto& rectCollIter = params.rectCollIter;
    auto& pointer = params.pointer;
    auto& currentlyHeldPointerIdOpt = params.currentlyHeldPointerIdOpt;
    auto& hasTextEditingSession = params.hasTextEditingSession;
    auto& setHeldPointerIdFn = params.setHeldPointerIdFn;
    auto& startInputSessionFn = params.startInputConnectionFn;
    auto& endInputSessionFn = params.endInputConnectionFn;

	auto rectPairOpt = rectColl.GetRect(widget, rectCollIter);
	DENGINE_IMPL_GUI_ASSERT(rectPairOpt.Has());
	auto const& absWidgetRect = rectPairOpt.Value().widgetRect;
	auto const& absVisibleRect = rectPairOpt.Value().visibleRect;

	TouchEventConsumption result = {};
	result.consumed = pointer.consumed;

    auto const pointerInside = PointIsInAll(pointer.pos, { absWidgetRect, absVisibleRect });

    // The field is currently being held.
    if (currentlyHeldPointerIdOpt.Has()) {
        auto const currPointerId = currentlyHeldPointerIdOpt.Get();

        // It's a integration error if we received a click down on a pointer id
        // that's already holding this widget. I think?
        DENGINE_IMPL_GUI_ASSERT(!(currPointerId == pointer.id && pointer.pressed));

        if (currPointerId == pointer.id && !pointer.pressed) {
            setHeldPointerIdFn(Std::nullOpt);
        }

        bool beginInputSession =
            !result.consumed &&
            pointerInside &&
            currPointerId == pointer.id &&
            !pointer.pressed &&
            pointer.type == PointerType::Primary;
        if (beginInputSession) {
            startInputSessionFn();
            result.consumed = true;
        }
    }
    else {
        // The field is not currently being held.
        if (hasTextEditingSession) {
            bool shouldEndInputSession = false;
            shouldEndInputSession = shouldEndInputSession ||
                result.consumed &&
                pointer.pressed;
            shouldEndInputSession = shouldEndInputSession ||
                !result.consumed &&
                pointer.pressed &&
                !pointerInside;

            if (shouldEndInputSession) {
                endInputSessionFn();
                result.consumed = true;
            }
        }
        else {
            bool startHoldingOnWidget =
                !result.consumed &&
                pointer.pressed &&
                pointerInside &&
                pointer.type == PointerType::Primary;
            if (startHoldingOnWidget) {
                setHeldPointerIdFn(pointer.id);
            }
        }
    }

    result.consumed = result.consumed || pointerInside;
    return result;
}

TouchEventConsumption LineEditUtils::PointerMove(PointerMove_Params const& params)
{
	auto& rectColl = params.rectColl;
	auto& widget = params.widget;
	auto& rectCollIter = params.rectCollIter;
    auto& pointerIdOpt = params.currentlyHeldPointerIdOpt;
    auto& pointer = params.pointer;
    auto& setPointerIdFn = params.setHeldPointerIdFn;

	auto rectPairOpt = rectColl.GetRect(widget, rectCollIter);
	DENGINE_IMPL_GUI_ASSERT(rectPairOpt.Has());
	auto const& absWidgetRect = rectPairOpt.Value().widgetRect;
	auto const& absVisibleRect = rectPairOpt.Value().visibleRect;

	// If we are tracking a pointer, and we get a move-event that is consumed before it hits our
	// Widget consumed, then we want to stop tracking that pointer. I.e with ScrollArea
    if (pointerIdOpt.Has()) {
        auto const heldPointerId = pointerIdOpt.Get();
        if (heldPointerId == pointer.id && pointer.consumed) {
        	setPointerIdFn(Std::nullOpt);
        }
    }

	TouchEventConsumption result = {};
    auto pointerInside = absWidgetRect.PointIsInside(pointer.pos)
        && absVisibleRect.PointIsInside(pointer.pos);
	result.consumed = pointerInside || pointer.consumed;
    return result;
}

void LineEditUtils::TextInput(TextInput_Params const& params)
{
    auto& sourceText = params.sourceText;
    auto& event = params.event;
    auto& setSelectionRangeFn = params.setSelectionRangeFn;

    DENGINE_IMPL_GUI_ASSERT(event.start <= sourceText.size());
    ::impl::ReplaceText(
        (int)sourceText.size(),
        [&]() { return sourceText.data(); },
        [&](auto newSize) { sourceText.resize(newSize); },
        (i64)event.start,
        (i64)event.count,
        { (int)event.newText.Size(), [&](int i) { return (char)event.newText[i]; } });

    DENGINE_IMPL_GUI_ASSERT(event.start + event.newText.Size() <= sourceText.size());
    setSelectionRangeFn({
        .start = event.start + event.newText.Size(),
        .count = 0 });

    // TODO: This has got to contain the new selection stuff!!!
    /*
    auto tempText = Std::NewVec_Reserve<u32>(transientAlloc, (int)this->text.size());
    for (auto const& item : this->text)
        tempText.PushBack(item);
    ctx.GetWindowHandler().UpdateTextInputConnection(
        event.windowId,
        selRange.start,
        selRange.count,
        tempText.ToSpan());
     */

    // TODO: Might need to handle the case where the insertion was not successful for whatever reason.
}

void LineEditUtils::TextSelection(TextSelection_Params const& params)
{
    auto& sourceText = params.sourceText;
    auto& event = params.event;
    auto& setSelectionRangeFn = params.setSelectionRangeFn;

    // TODO: This should be an if-test and only if it's out-of-bounds, we clamp it and
    // tell the system the new state.
    DENGINE_IMPL_GUI_ASSERT(event.start + event.count <= sourceText.size());
    SelectionRange range = {
        .start = event.start,
        .count = event.count };
    setSelectionRangeFn(range);

    // TODO: This should be an if-test and only if it's out-of-bounds, we clamp it and
    // tell the system the new state.
    DENGINE_IMPL_GUI_ASSERT(range.start + range.count <= sourceText.size());
}

void LineEditUtils::AccessibilityStuff(AccessibilityStuff_Params const& params)
{
    auto& widget = params.widget;
    auto& rectColl = params.eventParams.rectColl;
	auto& rectCollIter = params.rectCollIter;
	auto& treeOffset = params.eventParams.treeOffset;
	auto& windowExtent = params.eventParams.windowExtent;
    auto& text = params.text;
    auto& accessPusher = params.pusher;

	auto rectPairOpt = rectColl.GetRect(widget, rectCollIter);
	DENGINE_IMPL_GUI_ASSERT(rectPairOpt.Has());
	auto const& absWidgetRect = rectPairOpt.Value().widgetRect;
	auto const& absVisibleRect = rectPairOpt.Value().visibleRect;

    auto const intersection = Intersection(absWidgetRect, absVisibleRect);
    if (intersection.IsNothing())
        return;

    Widget::AccessibilityInfoElement accessItem = {};
    accessItem.rect = intersection;
    accessItem.textStart = accessPusher.PushText(text);
    accessItem.textCount = (int)text.Size();
    accessPusher.PushElement(
        reinterpret_cast<uSize>(&widget),
        accessItem);
}
