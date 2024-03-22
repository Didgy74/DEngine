#include <DEngine/Gui/Text.hpp>
#include <DEngine/Gui/Context.hpp>
#include <DEngine/Gui/TextManager.hpp>

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

SizeHint Text::GetSizeHint2(
	Widget::GetSizeHint2_Params const& params) const
{
	auto const& ctx = params.ctx;
	auto& textManager = params.textManager;
	auto& pusher = params.pusher;
	auto const& window = params.window;

	auto pusherIt = pusher.AddEntry(*this);

	auto textScale = ctx.fontScale * window.contentScale * this->relativeScale;

	Impl::CustomData& customData = pusher.AttachCustomData(pusherIt, Impl::CustomData{ pusher.Alloc() });
	auto& finalText = customData.text;
	finalText.Reserve(32);
	if (this->getText) {
		Text::TextPusher textPusher {
			&finalText,
			[](void* data, Std::Span<char const> in) {
				auto* temp = (decltype(customData.text)*)data;
				temp->PushBack(in);
			}
		};
		this->getText(ctx, params.appData, textPusher);
	} else {
		finalText.PushBack({ this->text.data(), this->text.size() });
	}
	if (pusher.IncludeRendering()) {
		customData.glyphRects.Resize(finalText.Size());
	}

	auto fontFaceId = textManager.GetFontFaceSizeId(textScale, window.dpiX, window.dpiY);

	auto textOuterExtent = textManager.GetOuterExtent(
		finalText.ToSpan(),
		fontFaceId,
		TextHeightType::Alphas,
		customData.glyphRects.ToSpan());

	customData.textOuterExtent = textOuterExtent;
	customData.fontFaceId = fontFaceId;

	SizeHint returnValue = {};
	returnValue.minimum = textOuterExtent;
	auto margin = (i32)Math::Round(
		(f32)Math::Min(textOuterExtent.width, textOuterExtent.height) *
		ctx.defaultMarginFactor);
	returnValue.minimum.AddPadding(margin);
	returnValue.expandX = expandX;

	pusher.SetSizeHint(pusherIt, returnValue);
	return returnValue;
}

void Text::BuildChildRects(
	Widget::BuildChildRects_Params const& params,
	Rect const& widgetRect,
	Rect const& visibleRect) const
{
}

void Text::Render2(
	Widget::Render_Params const& params,
	Rect const& widgetRect,
	Rect const& visibleRect) const
{
	auto& drawInfo = params.drawInfo;
	auto& rectColl = params.rectCollection;

	auto const intersection = Rect::Intersection(widgetRect, visibleRect);
	if (intersection.IsNothing())
		return;

	DENGINE_IMPL_GUI_ASSERT(rectColl.BuiltForRendering());

	auto* customDataPtr = rectColl.GetCustomData2<Impl::CustomData>(*this);
	DENGINE_IMPL_GUI_ASSERT(customDataPtr);
	auto& customData = *customDataPtr;
	auto const& finalText = customData.text;

	Rect textRect = {};
	textRect.extent = customData.textOuterExtent;
	textRect.position = widgetRect.position + Extent::CenteringOffset(widgetRect.extent, textRect.extent);
	auto scissor = DrawInfo::ScopedScissor(drawInfo, textRect, widgetRect);

	drawInfo.PushText(
		(u64)customData.fontFaceId,
		finalText.ToSpan(),
		customData.glyphRects.Data(),
		textRect.position,
		color);
}

void Text::AccessibilityTest(
	AccessibilityTest_Params const& params,
	Rect const& widgetRect,
	Rect const& visibleRect) const
{
	auto const& rectColl = params.rectColl;
	auto& pusher = params.pusher;

	auto* customDataPtr = rectColl.GetCustomData2<Impl::CustomData>(*this);
	DENGINE_IMPL_GUI_ASSERT(customDataPtr);
	auto& customData = *customDataPtr;
	auto const& finalText = customData.text;

	AccessibilityInfoElement element = {};
	element.textStart = pusher.PushText(finalText.ToSpan());
	element.textCount = (int)finalText.Size();
	element.rect = Intersection(widgetRect, visibleRect);
	element.isClickable = false;
	pusher.PushElement(element);
}