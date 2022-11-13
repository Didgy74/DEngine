#include <DEngine/Gui/Text.hpp>
#include <DEngine/Gui/Context.hpp>
#include <DEngine/Gui/TextManager.hpp>

using namespace DEngine;
using namespace DEngine::Gui;

struct Text::Impl
{
	// This data is only available when rendering.
	struct CustomData
	{
		explicit CustomData(RectCollection::AllocRefT const& alloc) :
			glyphRects{ alloc } {}

		Extent textOuterExtent = {};
		FontFaceId fontFaceId;
		Std::Vec<Rect, RectCollection::AllocRefT> glyphRects;
	};
};

namespace DEngine::Gui::impl {
	[[nodiscard]] static Math::Vec2Int BuildTextOffset(Extent const& widgetExtent, Extent const& textExtent) noexcept {
		Math::Vec2Int out = {};
		for (int i = 0; i < 2; i++) {
			auto temp = (i32)Math::Round((f32)widgetExtent[i] * 0.5f - (f32)textExtent[i] * 0.5f);
			temp = Math::Max(0, temp);
			out[i] = temp;
		}

		out.x = 0.f;
		return out;
	}
}

SizeHint Text::GetSizeHint2(
	Widget::GetSizeHint2_Params const& params) const
{
	auto const& ctx = params.ctx;
	auto& textManager = params.textManager;
	auto& pusher = params.pusher;
	auto const& window = params.window;

	SizeHint returnValue = {};

	auto pusherIt = pusher.AddEntry(*this);

	auto textScale = ctx.fontScale * window.contentScale * relativeScale;

	if (pusher.IncludeRendering())
	{
		auto customData = Impl::CustomData{ pusher.Alloc() };
		customData.glyphRects.Resize(text.size());

		customData.fontFaceId = textManager.GetFontFaceId(
			textScale,
			window.dpiX,
			window.dpiY);

		auto const textOuterExtent = textManager.GetOuterExtent(
			{ text.data(), text.size() },
			textScale,
			window.dpiX,
			window.dpiY,
			customData.glyphRects.Data());

		customData.textOuterExtent = textOuterExtent;

		returnValue.minimum = textOuterExtent;
		pusher.AttachCustomData(pusherIt, Std::Move(customData));
	}
	else
	{
		returnValue.minimum = textManager.GetOuterExtent(
			{ text.data(), text.size() },
			textScale,
			window.dpiX,
			window.dpiY);
	}

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
	auto& pusher = params.pusher;

	if (!pusher.IncludeRendering())
		return;

	auto* customDataPtr = pusher.GetCustomData2<Impl::CustomData>(*this);
	DENGINE_IMPL_GUI_ASSERT(customDataPtr);
	auto& customData = *customDataPtr;

	auto const textPosOffset = impl::BuildTextOffset(widgetRect.extent, customData.textOuterExtent);

	int const textLength = (int)text.size();
	DENGINE_IMPL_GUI_ASSERT(textLength == customData.glyphRects.Size());
	for (int i = 0; i < textLength; i += 1)
	{
		auto& rect = customData.glyphRects[i];
		rect.position += widgetRect.position;
		rect.position += textPosOffset;
	}
}

void Text::Render2(
	Widget::Render_Params const& params,
	Rect const& widgetRect,
	Rect const& visibleRect) const
{
	auto& drawInfo = params.drawInfo;
	auto& rectCollection = params.rectCollection;

	auto const intersection = Rect::Intersection(widgetRect, visibleRect);
	if (intersection.IsNothing())
		return;

	DENGINE_IMPL_GUI_ASSERT(rectCollection.BuiltForRendering());

	auto* customDataPtr = rectCollection.GetCustomData2<Impl::CustomData>(*this);
	DENGINE_IMPL_GUI_ASSERT(customDataPtr);
	auto& customData = *customDataPtr;

	Rect textRect = {};
	textRect.position = widgetRect.position + impl::BuildTextOffset(widgetRect.extent, customData.textOuterExtent);
	textRect.extent = customData.textOuterExtent;
	auto scissor = DrawInfo::ScopedScissor(drawInfo, textRect, widgetRect);

	drawInfo.PushText(
		(u64)customData.fontFaceId,
		{ text.data(), text.size() },
		customData.glyphRects.Data(),
		{},
		color);
}