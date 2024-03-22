#include <DEngine/Gui/LineIntEdit.hpp>

#include <DEngine/Gui/Context.hpp>
#include <DEngine/Gui/TextManager.hpp>
#include <DEngine/Gui/ButtonSizeBehavior.hpp>

#include <sstream>
#include <cstdlib>

using namespace DEngine;
using namespace DEngine::Gui;

struct LineIntEdit::Impl
{
	struct CustomData
	{
		explicit CustomData(RectCollection::AllocRefT const& alloc) :
			glyphRects{ alloc } {}

		Extent textOuterExtent = {};
		FontFaceSizeId fontSizeId = FontFaceSizeId::Invalid;
		Std::Vec<Rect, RectCollection::AllocRefT> glyphRects;
	};

	enum class PointerType : u8 { Primary, Secondary };
	[[nodiscard]] static PointerType ToPointerType(CursorButton in) noexcept
	{
		switch (in) {
			case CursorButton::Primary: return PointerType::Primary;
			case CursorButton::Secondary: return PointerType::Secondary;
			default:
				DENGINE_IMPL_UNREACHABLE();
				return {};
		}
	}

	static constexpr u8 cursorPointerId = ~static_cast<u8>(0);

	static void UpdateValue(LineIntEdit& widget, bool updateText)
	{
		auto& text = widget.text;
		if (!text.empty() && text != "-" && text != "." && text != "-.")
		{
			char* index = nullptr;
			i64 newValue;
			if (widget.min >= 0)
				newValue = (i64)std::strtoull(text.c_str(), &index, 10);
			else
				newValue = std::strtoll(text.c_str(), &index, 10);
			bool newValueIsDifferent = false;
			if (*index != 0) // error
			{
			}
			else
			{
				newValueIsDifferent = newValue != widget.value;

				widget.value = Math::Clamp(newValue, widget.min, widget.max);
			}

			if (updateText)
			{
				std::stringstream stream;
				stream << widget.value;
				widget.text = stream.str();
			}

			if (newValueIsDifferent && widget.valueChangedFn)
				widget.valueChangedFn(widget, widget.value);
		}
	}

	static void BeginInputSession(WindowID windowId, LineIntEdit& widget, Context& ctx)
	{
		ctx.TakeInputConnection(
			windowId,
			widget,
			Gui::SoftInputFilter::SignedInteger,
			{ widget.text.data(), widget.text.length() });
		widget.inputConnectionCtx = &ctx;
	}

	struct PointerPress_Pointer
	{
		u8 id;
		Math::Vec2 pos;
		PointerType type;
		bool pressed;
		bool consumed;
	};

	struct PointerPress_Params
	{
		Context& ctx;
		WindowID windowId = WindowID::Invalid;
		RectCollection const& rectCollection;
		LineIntEdit& widget;
		Rect const& widgetRect;
		Rect const& visibleRect;
		PointerPress_Pointer const& pointer;
	};
	[[nodiscard]] static bool PointerPress(PointerPress_Params const& in) noexcept
	{
		auto& widget = in.widget;
		auto& ctx = in.ctx;
		auto& pointer = in.pointer;
		auto& widgetRect = in.widgetRect;
		auto& visibleRect = in.visibleRect;

		bool eventConsumed = pointer.consumed;

		auto const pointerInside = PointIsInAll(pointer.pos, { widgetRect, visibleRect });

		// The field is currently being held.
		if (widget.pointerId.Has())
		{
			auto const currPointerId = widget.pointerId.Get();

			// It's a integration error if we received a click down on a pointer id
			// that's already holding this widget. I think?
			DENGINE_IMPL_GUI_ASSERT(!(currPointerId == pointer.id && pointer.pressed));

			if (currPointerId == pointer.id && !pointer.pressed) {
				widget.pointerId = Std::nullOpt;
			}

			bool beginInputSession =
				!eventConsumed &&
				pointerInside &&
				currPointerId == pointer.id &&
				!pointer.pressed &&
				pointer.type == PointerType::Primary;
			if (beginInputSession) {
				BeginInputSession(in.windowId, widget, ctx);
				eventConsumed = true;
			}
		}
		else {
			// The field is not currently being held.

			if (widget.HasInputSession())
			{
				bool shouldEndInputSession = false;

				shouldEndInputSession = shouldEndInputSession ||
				                        eventConsumed &&
				                        pointer.pressed;
				shouldEndInputSession = shouldEndInputSession ||
				                        !eventConsumed &&
				                        pointer.pressed &&
				                        !pointerInside;

				if (shouldEndInputSession)
				{
					widget.ClearInputConnection();
					eventConsumed = true;
				}
			}
			else
			{
				bool startHoldingOnWidget =
					!eventConsumed &&
					pointer.pressed &&
					pointerInside &&
					pointer.type == PointerType::Primary;
				if (startHoldingOnWidget) {
					widget.pointerId = pointer.id;
				}
			}
		}

		eventConsumed = eventConsumed || pointerInside;
		return eventConsumed;
	}
};

using namespace DEngine;
using namespace DEngine::Gui;

LineIntEdit::~LineIntEdit()
{
	if (inputConnectionCtx)
	{
		ClearInputConnection();
	}
}

void LineIntEdit::ClearInputConnection()
{
	DENGINE_IMPL_GUI_ASSERT(this->inputConnectionCtx);
	this->inputConnectionCtx->ClearInputConnection(*this);
	this->inputConnectionCtx = nullptr;
}

SizeHint LineIntEdit::GetSizeHint2(
	Widget::GetSizeHint2_Params const& params) const
{
	auto const& ctx = params.ctx;
	auto const& window = params.window;
	auto& textMgr = params.textManager;
	auto& pusher = params.pusher;

	auto const pusherIt = pusher.AddEntry(*this);
	auto& customData = pusher.AttachCustomData(pusherIt, Impl::CustomData{ pusher.Alloc() });
	if (pusher.IncludeRendering()) {
		customData.glyphRects.Resize(text.size());
	}

	auto normalTextScale = ctx.fontScale * window.contentScale;
	auto fontSizeId = FontFaceSizeId::Invalid;
	auto marginAmount = 0;
	{
		auto normalHeight = textMgr.GetLineheight(normalTextScale, window.dpiX, window.dpiY);
		auto normalHeightMargin = (u32)Math::Round((f32)normalHeight * ctx.defaultMarginFactor);
		normalHeight += 2 * normalHeightMargin;

		auto minHeight = CmToPixels(ctx.minimumHeightCm, window.dpiY);

		if (normalHeight > minHeight) {
			fontSizeId = textMgr.GetFontFaceSizeId(normalTextScale, window.dpiX, window.dpiY);
			marginAmount = (i32)normalHeightMargin;
		} else {
			// We can't just do minHeight * defaultMarginFactor for this one, because defaultMarginFactor applies to
			// content, not the outer size. So we set up an equation `height = 2 * marginFactor * content + content`
			// and we solve for content.
			auto contentSizeCm = ctx.minimumHeightCm / (2 * ctx.defaultMarginFactor + 1.f);
			auto contentSize = CmToPixels(contentSizeCm, window.dpiY);
			fontSizeId = textMgr.FontFaceSizeIdForLinePixelHeight(
					contentSize,
					TextHeightType::Alphas);
			marginAmount = (i32)Math::Round((f32)contentSize * ctx.defaultMarginFactor);
		}
	}
	customData.fontSizeId = fontSizeId;

	auto textOuterExtent = textMgr.GetOuterExtent(
		{ this->text.data(), this->text.size() },
		fontSizeId,
		TextHeightType::Numerals,
		customData.glyphRects.ToSpan());
	customData.textOuterExtent = textOuterExtent;

	SizeHint returnVal = {};
	returnVal.minimum = textOuterExtent;
	returnVal.minimum.AddPadding(marginAmount);
	returnVal.minimum.width = Math::Max(
		returnVal.minimum.width,
		(u32)CmToPixels(ctx.minimumHeightCm, window.dpiX));
	returnVal.minimum.height = Math::Max(
		returnVal.minimum.height,
		(u32)CmToPixels(ctx.minimumHeightCm, window.dpiY));
	pusher.SetSizeHint(pusherIt, returnVal);
	return returnVal;
}

void LineIntEdit::BuildChildRects(
	Widget::BuildChildRects_Params const& params,
	Rect const& widgetRect,
	Rect const& visibleRect) const
{
}

void LineIntEdit::Render2(
	Widget::Render_Params const& params,
	Rect const& widgetRect,
	Rect const& visibleRect) const
{
	auto const intersection = Intersection(widgetRect, visibleRect);
	if (intersection.IsNothing())
		return;

	auto& drawInfo = params.drawInfo;
	auto& rectCollection = params.rectCollection;
	auto* customDataPtr = rectCollection.GetCustomData2<Impl::CustomData>(*this);
	DENGINE_IMPL_GUI_ASSERT(customDataPtr);
	auto& customData = *customDataPtr;
	DENGINE_IMPL_GUI_ASSERT(customData.glyphRects.Size() == text.size());

	auto fontSizeId = customData.fontSizeId;
	auto textOuterExtent = customData.textOuterExtent;

	Math::Vec4 tempBackground = {};
	if (HasInputSession())
		tempBackground = { 0.5f, 0.f, 0.f, 1.f };
	else
		tempBackground = backgroundColor;

	// Calculate radius
	auto minDimension = Math::Min(widgetRect.extent.width, widgetRect.extent.height);
	auto radius = (int)Math::Floor((f32)minDimension * 0.25f);
	drawInfo.PushFilledQuad(widgetRect, tempBackground, radius);

	auto drawScissor = DrawInfo::ScopedScissor(drawInfo, intersection);

	auto centeringOffset = Extent::CenteringOffset(widgetRect.extent, textOuterExtent);
	centeringOffset.x = Math::Max(centeringOffset.x, 0);
	centeringOffset.y = Math::Max(centeringOffset.y, 0);

	auto textRect = Rect{ widgetRect.position + centeringOffset, customData.textOuterExtent };
	drawInfo.PushText(
		(u64)fontSizeId,
		{ text.data(), text.size() },
		customData.glyphRects.Data(),
		textRect.position,
		{ 1.f, 1.f, 1.f, 1.f });
}

bool LineIntEdit::CursorPress2(
	Widget::CursorPressParams const& params,
	Rect const& widgetRect,
	Rect const& visibleRect,
	bool consumed)
{
	Impl::PointerPress_Pointer pointer = {};
	pointer.id = Impl::cursorPointerId;
	pointer.pressed = params.event.pressed;
	pointer.type = Impl::ToPointerType(params.event.button);
	pointer.pos = { (f32)params.cursorPos.x, (f32)params.cursorPos.y };
	pointer.consumed = consumed;

	Impl::PointerPress_Params temp = {
		.ctx = params.ctx,
		.windowId = params.windowId,
		.rectCollection = params.rectCollection,
		.widget = *this,
		.widgetRect = widgetRect,
		.visibleRect = visibleRect,
		.pointer = pointer, };

	return Impl::PointerPress(temp);
}

bool LineIntEdit::TouchPress2(
	TouchPressParams const& params,
	Rect const& widgetRect,
	Rect const& visibleRect,
	bool consumed)
{
	Impl::PointerPress_Pointer pointer = {};
	pointer.id = params.event.id;
	pointer.pressed = params.event.pressed;
	pointer.type = Impl::PointerType::Primary;
	pointer.pos = params.event.position;
	pointer.consumed = consumed;

	Impl::PointerPress_Params temp = {
		.ctx = params.ctx,
		.windowId = params.windowId,
		.rectCollection = params.rectCollection,
		.widget = *this,
		.widgetRect = widgetRect,
		.visibleRect = visibleRect,
		.pointer = pointer, };

	return Impl::PointerPress(temp);
}

void LineIntEdit::SetValue(i64 in)
{
	std::ostringstream out;
	out << in;
	text = out.str();

	value = in;
}

void LineIntEdit::TextInput(
	Context& ctx,
	AllocRef const& transientAlloc,
	TextInputEvent const& event)
{
	if (HasInputSession())
	{
		DENGINE_IMPL_GUI_ASSERT(event.oldIndex + event.oldCount <= text.size());

		auto sizeDifference = (int)event.newTextSize - (int)event.oldCount;

		// First check if we need to expand our storage.
		if (sizeDifference > 0)
		{
			// We need to move all content behind the old substring
			// To the right.
			auto oldSize = (int)text.size();
			text.resize(text.size() + sizeDifference);
			int end = (int)event.oldIndex + (int)event.oldCount - 1;
			for (int i = oldSize - 1; i > end; i -= 1)
				text[i + sizeDifference] = text[i];
		}
		else if (sizeDifference < 0)
		{
			// We need to move all content behind the old substring
			// To the left.
			auto oldSize = (int)text.size();
			int begin = (int)event.oldIndex + (int)event.oldCount;
			for (int i = begin; i < oldSize; i += 1)
				text[i + sizeDifference] = text[i];

			text.resize(text.size() + sizeDifference);
		}

		for (int i = 0; i < event.newTextSize; i += 1)
			text[i + event.oldIndex] = (char)event.newTextData[i];

		Impl::UpdateValue(*this, false);
	}
}

void LineIntEdit::EndTextInputSession(
	Context& ctx,
	AllocRef const& transientAlloc,
	EndTextInputSessionEvent const& event)
{
	if (HasInputSession())
	{
		ClearInputConnection();
	}
}
