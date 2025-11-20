#include <DEngine/Gui/StdWidgets/Text.hpp>


import DEngine.Gui.FontFaceSizeId;
import DEngine.Gui.DrawEngine;
import DEngine.Gui.TextEngine;

using namespace DEngine;
using namespace DEngine::Gui;

struct Text::Impl
{
	// This data is only available when rendering.
	struct CustomData {
		explicit CustomData(RectCollection::AllocRefT const& alloc) :
			text{ alloc}, glyphRects{ alloc } {}

		Extent textOuterExtent = {};
		FontFaceSizeId fontFaceId = FontFaceSizeId::Invalid;
		Std::Vec<char, RectCollection::AllocRefT> text;
		Std::Vec<Rect, RectCollection::AllocRefT> glyphRects;
	};
};

Widget::GetSizeHint2_ReturnT Text::GetSizeHint2(
	Widget::GetSizeHint2_Params const& params) const
{
	auto& textManager = params.textEngine;
	auto& pusher = params.pusher;
	auto const& window = params.window;
	auto const& fontScale = params.FontScale();
	auto const& minimumSizeCm = params.MinimumHeightCm();

	auto pusherIt = pusher.AddEntry(*this);

	auto textScale = fontScale * window.contentScale * this->relativeScale;

	Impl::CustomData& customData = pusher.AttachCustomData(pusherIt, Impl::CustomData{ pusher.Alloc() });
	auto& finalText = customData.text;
	finalText.Reserve(32);
	if (this->getTextFn) {
		Text::TextPusher textPusher {
			&finalText,
			[](void* data, Std::Span<char const> in) {
				auto* temp = (decltype(customData.text)*)data;
				temp->PushBack(in);
			}
		};
		Text::GetTextParams getTextParams {
			.fontScale = fontScale,
			.minimumHeightCm = minimumSizeCm,
			.windowInfo = window,
			.appData = params.appData,
			.pusher = textPusher, };
		this->getTextFn(getTextParams);
	} else {
		finalText.PushBack({ this->text.data(), this->text.size() });
	}
	if (pusher.IncludeRendering()) {
		customData.glyphRects.Resize(finalText.Size());
	}

	auto fontFaceId = textManager.GetFontFaceSizeId(textScale, window.dpi, window.dpi);

	auto textOuterExtent = textManager.GetOuterExtent(
		finalText.ToSpan(),
		fontFaceId,
		customData.glyphRects.ToSpan());

	customData.textOuterExtent = textOuterExtent;
	customData.fontFaceId = fontFaceId;

	Text::Theme theme = {};
	if (this->getThemeFn)
		theme = this->getThemeFn({ .widget = *this, .appData = params.appData });
	auto marginPx = theme.textMargin.ResolvePx(window.dpi, window.contentScale);

	SizeHint returnValue = {};
	returnValue.minimum = textOuterExtent;
	returnValue.minimum.AddPadding(marginPx);
	returnValue.expandX = expandX;

	pusher.SetSizeHint(pusherIt, returnValue, false);
	return {
		.iter = pusherIt,
		.sizeHint = returnValue };
}

void Text::BuildChildRects(
	Widget::BuildChildRects_Params const& params,
	Rect const& widgetRect,
	Rect const& visibleRect,
	Std::Opt<RectCollection::Iter> const& rectCollIter) const
{
}

void Text::Render2(
	Render_Params const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter) const
{
	auto& drawInfo = params.drawEngine;
	auto& rectColl = params.rectCollection;

	auto rectPairOpt = rectColl.GetRect(*this, rectCollIter);
	DENGINE_IMPL_GUI_ASSERT(rectPairOpt.Has());
	auto const& absWidgetRect = rectPairOpt.Value().widgetRect;
	auto const& absVisibleRect = rectPairOpt.Value().visibleRect;

	auto const intersection = Rect::Intersection(absWidgetRect, absVisibleRect);
	if (intersection.IsNothing())
		return;

	DENGINE_IMPL_GUI_ASSERT(rectColl.BuiltForRendering());

	auto* customDataPtr = rectColl.GetCustomData<Impl::CustomData>(*this);
	DENGINE_IMPL_GUI_ASSERT(customDataPtr);
	auto& customData = *customDataPtr;
	auto const& finalText = customData.text;

	Rect textRect = {};
	textRect.extent = customData.textOuterExtent;
	textRect.position =
		absWidgetRect.position
		+ Extent::CenteringOffset(absWidgetRect.extent, textRect.extent);
	textRect.position.x = Std::Max(textRect.position.x, absWidgetRect.position.x);
	auto scissor = DrawEngine::ScopedScissor(drawInfo, textRect, absWidgetRect);

	drawInfo.PushText(
		customData.fontFaceId,
		finalText.ToSpan(),
		customData.glyphRects.Data(),
		textRect.position,
		color);
}

void Text::AccessibilityTest(
	AccessibilityTest_Params const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter) const
{
	auto const& rectColl = params.rectColl;
	auto& pusher = params.pusher;

	auto rectPairOpt = rectColl.GetRect(*this, rectCollIter);
	DENGINE_IMPL_GUI_ASSERT(rectPairOpt.Has());
	auto const& absWidgetRect = rectPairOpt.Value().widgetRect;
	auto const& absVisibleRect = rectPairOpt.Value().visibleRect;

	auto const accessRect = absWidgetRect.GetIntersect(absVisibleRect);
	if (accessRect.IsNothing())
		return;

	auto* customDataPtr = rectColl.GetCustomData<Impl::CustomData>(*this);
	DENGINE_IMPL_GUI_ASSERT(customDataPtr);
	auto& customData = *customDataPtr;
	auto const& finalText = customData.text;

	AccessibilityInfoElement element = {};
	element.textStart = pusher.PushText(finalText.ToSpan());
	element.textCount = (int)finalText.Size();
	element.rect = accessRect;
	element.isClickable = false;
	// TODO: For now we just cast this item to u64
	pusher.PushElement(reinterpret_cast<uSize>(this), element);
}
