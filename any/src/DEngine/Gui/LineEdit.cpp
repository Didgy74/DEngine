#include <DEngine/Gui/LineEdit.hpp>
#include <DEngine/Gui/TextManager.hpp>
#include <DEngine/Gui/DrawInfo.hpp>
#include <DEngine/Gui/Context.hpp>

#include <DEngine/Std/Containers/Vec.hpp>

using namespace DEngine;
using namespace DEngine::Gui;

struct LineEdit::Impl
{
	struct CustomData
	{
		explicit CustomData(RectCollection::AllocRefT const& alloc) :
			glyphRects{ alloc } {}

		FontFaceSizeId fontFaceId;
		Extent textOuterExtent = {};
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

	struct PointerPress_Pointer
	{
		u8 id;
		Math::Vec2 pos;
		PointerType type;
		bool pressed;
		bool consumed;
	};

	struct PointerPress_Params {
		Context& ctx;
		WindowID windowId = WindowID::Invalid;
		RectCollection const& rectCollection;
		LineEdit& widget;
		Rect const& widgetRect;
		Rect const& visibleRect;
		PointerPress_Pointer const& pointer;
	};



	static void BeginInputSession(WindowID windowId, LineEdit& widget, Context& ctx)
	{
		ctx.TakeInputConnection(
			windowId,
			widget,
			Gui::SoftInputFilter::NoFilter,
			{ widget.text.data(), widget.text.length() });
		widget.inputConnectionCtx = &ctx;
	}

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

LineEdit::~LineEdit()
{
	if (HasInputSession())
		ClearInputConnection();
}

void LineEdit::ClearInputConnection()
{
	DENGINE_IMPL_GUI_ASSERT(this->inputConnectionCtx);
	this->inputConnectionCtx->ClearInputConnection(*this);
	this->inputConnectionCtx = nullptr;
}

SizeHint LineEdit::GetSizeHint2(
	GetSizeHint2_Params const& params) const
{
	auto const& ctx = params.ctx;
	auto const& window = params.window;
	auto& textManager = params.textManager;
	auto& pusher = params.pusher;

	auto const pusherIt = pusher.AddEntry(*this);

	auto textScale = ctx.fontScale * window.contentScale;

	SizeHint returnValue = {};

	if (pusher.IncludeRendering()) {
		auto customData = Impl::CustomData{ pusher.Alloc() };
		customData.glyphRects.Resize(text.size());

		customData.fontFaceId = textManager.GetFontFaceSizeId(textScale, window.dpiX, window.dpiY);

		customData.textOuterExtent = textManager.GetOuterExtent(
			{ text.data(), text.size() },
			textScale,
			window.dpiX,
			window.dpiY,
			customData.glyphRects.ToSpan());
		returnValue.minimum = customData.textOuterExtent;

		pusher.AttachCustomData(pusherIt, Std::Move(customData));
	}
	else {
		returnValue.minimum = textManager.GetOuterExtent(
			{ text.data(), text.size() },
			textScale,
			window.dpiX,
			window.dpiY);
	}

	//auto actualMargin = (u32)Math::Round((f32)returnValue.minimum.height * 0.25f);
	auto actualMargin = margin;
	returnValue.minimum.width += actualMargin * 2;
	returnValue.minimum.height += actualMargin * 2;
	returnValue.expandX = true;

	pusher.SetSizeHint(pusherIt, returnValue);
	return returnValue;
}

void LineEdit::BuildChildRects(
	BuildChildRects_Params const& params,
	Rect const& widgetRect,
	Rect const& visibleRect) const
{
	auto& pusher = params.pusher;

	if (pusher.IncludeRendering())
	{
		auto* customDataPtr = pusher.GetCustomData2<Impl::CustomData>(*this);
		DENGINE_IMPL_GUI_ASSERT(customDataPtr);
		auto& customData = *customDataPtr;

		for (auto& glyphRect : customData.glyphRects)
		{
			glyphRect.position += widgetRect.position;
			glyphRect.position.x += (i32)margin;
			glyphRect.position.y += (i32)margin;
		}
	}
}

void LineEdit::Render2(
	Render_Params const& params,
	Rect const& widgetRect,
	Rect const& visibleRect) const
{
	auto const intersection = Intersection(widgetRect, visibleRect);
	if (intersection.IsNothing())
		return;

	auto& drawInfo = params.drawInfo;
	auto& rectCollection = params.rectCollection;

	Math::Vec4 tempBackground = {};
	if (HasInputSession())
		tempBackground = { 0.5f, 0.f, 0.f, 1.f };
	else
		tempBackground = backgroundColor;
	drawInfo.PushFilledQuad(widgetRect, tempBackground);

	auto* customDataPtr = rectCollection.GetCustomData2<Impl::CustomData>(*this);
	DENGINE_IMPL_GUI_ASSERT(customDataPtr);
	auto& customData = *customDataPtr;

	DENGINE_IMPL_GUI_ASSERT(customData.glyphRects.Size() == text.size());

	auto drawScissor = DrawInfo::ScopedScissor(drawInfo, intersection);

	auto glyphPosOffset = widgetRect.position;
	glyphPosOffset.x += (i32)this->margin;
	glyphPosOffset.x += (i32)this->margin;

	drawInfo.PushText(
		(u64)customData.fontFaceId,
		{ text.data(), text.size() },
		customData.glyphRects.Data(),
		glyphPosOffset,
		{ 1.f, 1.f, 1.f, 1.f });
}

bool LineEdit::CursorPress2(
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

bool LineEdit::TouchPress2(
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

void LineEdit::TextInput(
	Context& ctx,
	AllocRef const& transientAlloc,
	TextInputEvent const& event)
{
	if (this->HasInputSession()) {

		ReplaceText(
			(int)this->text.size(),
			[&]() { return this->text.data(); },
			[&](auto newSize) { this->text.resize(newSize); },
			(i64)event.start,
			(i64)event.count,
			{ (int)event.newText.Size(), [&](int i) { return (char)event.newText[i]; } });

		ctx.GetWindowHandler().UpdateTextInputConnection(
			event.start,
			event.count,
			event.newText);


		// TODO: Might need to handle the case where the insertion was not successful for whatever reason.
	}
}

void LineEdit::TextSelection(
	Context& ctx,
	AllocRef const& transientAlloc,
	TextSelectionEvent const& event)
{
	if (this->HasInputSession()) {
		if (event.start != this->text.size() || event.count != 0) {
			DENGINE_IMPL_UNREACHABLE();
		}

		ctx.GetWindowHandler().UpdateTextInputConnectionSelection(event.start, event.count);

		// TODO: do something with the selection.
	}
}

void LineEdit::TextDelete(
	Context& ctx,
	AllocRef const& transientAlloc,
	WindowID windowId)
{
	if (HasInputSession())
	{
		u64 selIndex = text.size();
		u64 selCount = 0;
		if (selIndex > 0) {
			selIndex -= 1;
			selCount = 1;
		}

		ReplaceText(
			(int)text.size(),
			[&]() { return text.data(); },
			[&](auto newSize) { text.resize(newSize); },
			(i64)selIndex,
			(i64)selCount,
			{});

		ctx.UpdateInputConnection(
			(int)selIndex,
			selCount,
			{});
	}
}

void LineEdit::EndTextInputSession(
	Context& ctx,
	AllocRef const& transientAlloc,
	EndTextInputSessionEvent const& event)
{
	if (HasInputSession())
	{
		inputConnectionCtx->ClearInputConnection(*this);
		inputConnectionCtx = nullptr;
	}
}


