#include <DEngine/Gui/StdWidgets/LineEdit.hpp>


using namespace DEngine;
using namespace DEngine::Gui;

struct LineEdit::Impl
{
	struct CustomData
	{
		explicit CustomData(RectCollection::AllocRefT const& alloc) :
			glyphRects{ alloc } {}

		Extent textOuterExtent = {};
		FontFaceSizeId fontSizeId = FontFaceSizeId::Invalid;
		Std::Vec<Rect, RectCollection::AllocRefT> glyphRects;
		u32 textMarginPx = 0;
		u32 cornerRadiusPx = 0;
	};

	static void BeginInputSession(
		LineEdit& widget,
		Widget::WidgetEvent_WindowHandler& windowHandler)
	{
		DENGINE_IMPL_GUI_ASSERT(!widget.textEditingSessionOpt.Has());
		TextEditingSession textEditingSession = {};
		// TODO: This is hardcoded for some testing!
		u32 selStart = widget.text.length();
		u32 selCount = 0;
		//selStart = 1;
		//selCount = 1;
		textEditingSession.selectionRange.start = selStart;
		textEditingSession.selectionRange.count = selCount;
		textEditingSession.textInputSession = windowHandler.TakeTextInputConnection(
			widget,
			Gui::TextInputType::NoFilter,
			{ widget.text.data(), widget.text.length() },
			selStart,
			selCount);

		widget.textEditingSessionOpt = Std::Move(textEditingSession);
	}
};

LineEdit::~LineEdit()
{
}

bool LineEdit::CursorPress2(
	CursorPressParams const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter,
	bool consumed)
{
	LineEditUtils::PointerPress_Pointer pointer = {
		.id = LineEditUtils::cursorPointerId,
		.pos = { (f32)params.cursorPos.x, (f32)params.cursorPos.y },
		.type = LineEditUtils::ToPointerType(params.CursorButton()),
		.pressed = params.CursorPressed(),
		.consumed = consumed, };

	auto setHeldPointerIdFn = [&](Std::Opt<u8> val) {
		this->pointerId = Std::Move(val);
	};
	auto startInputConnectionFn = [&] {
		Impl::BeginInputSession(*this, params.windowHandler);
	};
	auto endInputConnectionFn = [&] {
		this->textEditingSessionOpt = Std::nullOpt;
	};

	LineEditUtils::PointerPress_Params tempParams = {
		.rectColl = params.rectCollection,
		.widget = *this,
		.rectCollIter = rectCollIter,
		.pointer = pointer,
		.currentlyHeldPointerIdOpt = this->pointerId,
		.hasTextEditingSession = this->textEditingSessionOpt.Has(),
		.setHeldPointerIdFn = setHeldPointerIdFn,
		.startInputConnectionFn = startInputConnectionFn,
		.endInputConnectionFn = endInputConnectionFn,
	};
	auto temp = LineEditUtils::PointerPress(tempParams);
	return temp.consumed;
}

bool LineEdit::CursorMove(
	CursorMoveParams const &params,
	Std::Opt<RectCollection::Iter> const& rectCollIter,
	bool occluded)
{
	auto cursorPos = params.CursorPos();
	auto const& rectColl = params.rectCollection;

	LineEditUtils::PointerMove_Pointer pointer = {
		.id = LineEditUtils::cursorPointerId,
		.pos = { (f32)cursorPos.x, (f32)cursorPos.y },
		.consumed = occluded, };

	auto setHeldPointerIdFn = [&](Std::Opt<u8> val) { this->pointerId = Std::Move(val); };

	LineEditUtils::PointerMove_Params tempParams = {
		.rectColl = rectColl,
		.widget = *this,
		.rectCollIter = rectCollIter,
		.pointer = pointer,
		.currentlyHeldPointerIdOpt = this->pointerId,
		.setHeldPointerIdFn = setHeldPointerIdFn,
	};
	auto temp = LineEditUtils::PointerMove(tempParams);
	return temp.consumed;
}

TouchEventConsumption LineEdit::WidgetEvent_TouchPress(
	WidgetEvent_TouchPressParams const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter,
	bool consumed)
{
	LineEditUtils::PointerPress_Pointer pointer = {
		.id = params.event.id,
		.pos = params.event.position,
		.type = LineEditUtils::PointerType::Primary,
		.pressed = params.event.pressed,
		.consumed = consumed };

	auto setHeldPointerIdFn = [&](Std::Opt<u8> val) {
		this->pointerId = Std::Move(val);
	};
	auto startInputConnectionFn = [&] {
		Impl::BeginInputSession(*this, params.windowHandler);
	};
	auto endInputConnectionFn = [&] {
		this->textEditingSessionOpt = Std::nullOpt;
	};

	LineEditUtils::PointerPress_Params temp = {
		.rectColl = params.rectCollection,
		.widget = *this,
		.rectCollIter = rectCollIter,
		.pointer = pointer,
		.currentlyHeldPointerIdOpt = this->pointerId,
		.hasTextEditingSession = this->textEditingSessionOpt.Has(),
		.setHeldPointerIdFn = setHeldPointerIdFn,
		.startInputConnectionFn = startInputConnectionFn,
		.endInputConnectionFn = endInputConnectionFn,
	};
	return LineEditUtils::PointerPress(temp);
}

TouchEventConsumption LineEdit::WidgetEvent_TouchMove(
	WidgetEvent_TouchMoveParams const &params,
	Std::Opt<RectCollection::Iter> const& rectCollIter,
	bool occluded)
{
	LineEditUtils::PointerMove_Pointer pointer = {
		.id = params.event.id,
		.pos = params.event.position,
		.consumed = occluded, };

	auto setHeldPointerIdFn = [&](Std::Opt<u8> val) {
		this->pointerId = Std::Move(val);
	};

	LineEditUtils::PointerMove_Params temp = {
		.rectColl = params.rectCollection,
		.widget = *this,
		.rectCollIter = rectCollIter,
		.pointer = pointer,
		.currentlyHeldPointerIdOpt = this->pointerId,
		.setHeldPointerIdFn = setHeldPointerIdFn,
	};
	return LineEditUtils::PointerMove(temp);
}

void LineEdit::WidgetEvent_TextInput(
	AllocRef const& transientAlloc,
	WidgetEvent_TextInputParams const& event)
{
	if (this->textEditingSessionOpt.Has()) {
		auto& textEditingSession = this->textEditingSessionOpt.Get();
		LineEditUtils::TextInput_Params temp = {
			.sourceText = this->text,
			.event = event,
			.setSelectionRangeFn = [&](LineEditUtils::SelectionRange newRange) {
				textEditingSession.selectionRange = newRange;
			}
		};
		LineEditUtils::TextInput(temp);
	}
}

void LineEdit::WidgetEvent_TextSelection(
	AllocRef const& transientAlloc,
	WidgetEvent_TextSelectionParams const& event)
{
	if (this->textEditingSessionOpt.Has()) {
		auto& textEditingSession = this->textEditingSessionOpt.Get();
		LineEditUtils::TextSelection_Params temp = {
			.sourceText = this->text,
			.event = event,
			.setSelectionRangeFn = [&](LineEditUtils::SelectionRange newRange) {
				textEditingSession.selectionRange = newRange;
			}
		};
		LineEditUtils::TextSelection(temp);
	}
}

void LineEdit::WidgetEvent_EndTextInputSession(
	AllocRef const& transientAlloc,
	WidgetEvent_EndTextInputSessionParams const& event)
{
	if (this->textEditingSessionOpt.Has()) {
		this->textEditingSessionOpt = Std::nullOpt;
	}
}

Widget::GetSizeHint2_ReturnT LineEdit::GetSizeHint2(
	GetSizeHint2_Params const& params) const
{
	auto& textEngine = params.textEngine;
	auto& pusher = params.pusher;
	auto& window = params.window;
	auto const& fontScale = params.FontScale();
	auto const& minimumHeightCm = params.MinimumHeightCm();

	auto const pusherIt = pusher.AddEntry(*this);
	auto& customData = pusher.AttachCustomData(pusherIt, Impl::CustomData{ pusher.Alloc() });

	if (pusher.IncludeRendering()) {
		customData.glyphRects.Resize(this->text.size());
	}

	LineEdit::Theme theme;
	if (this->getThemeFn) {
		theme = this->getThemeFn({ .widget = *this, .appData = params.appData });
	}
	auto textMarginPx = theme.margin.ResolvePx(window.dpi, window.dpi);

	LineEditUtils::GetSizeHintParams sizeHintParams = {
		.textEngine = textEngine,
		.windowContentScale = window.contentScale,
		.fontScale = fontScale,
		.windowDpi = window.dpi,
		.text = { this->text.data(), this->text.size() },
		.outGlyphRects = customData.glyphRects.ToSpan(),
		.minimumSizeCm = minimumHeightCm,
		.textMarginPx = textMarginPx,
	};
	auto result = LineEditUtils::GetSizeHint(sizeHintParams);

	customData.fontSizeId = result.fontId;
	customData.textOuterExtent = result.textOuterExtent;
	customData.textMarginPx = textMarginPx;
	customData.cornerRadiusPx = theme.cornerRadius.ResolvePx(window.dpi, window.contentScale);

	pusher.SetSizeHint(pusherIt, result.sizeHint, true);
	return {
		.iter = pusherIt,
		.sizeHint = result.sizeHint };
}

void LineEdit::Render2(
	Render_Params const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter) const
{
	auto& rectColl = params.rectCollection;

	auto* customDataPtr = rectColl.GetCustomData<Impl::CustomData>(*this);
	DENGINE_IMPL_GUI_ASSERT(customDataPtr);
	auto& customData = *customDataPtr;

	Std::Opt<LineEditUtils::SelectionRange> selectionRangeOpt;
	if (this->textEditingSessionOpt.Has()) {
		auto const& textEditingSession = this->textEditingSessionOpt.Get();
		selectionRangeOpt = { textEditingSession.selectionRange.start, textEditingSession.selectionRange.count };
	}

	LineEditUtils::Render_Params temp = {
		.renderParams = params,
		.widget = *this,
		.rectColl = rectColl,
		.rectCollIter = rectCollIter,
		.backgroundColor = this->backgroundColor,
		.textColor = params.TextColor(),
		.textOuterExtent = customData.textOuterExtent,
		.text = { this->text.data(), this->text.size() },
		.glyphRects = customData.glyphRects.ToSpan(),
		.fontId = customData.fontSizeId,
		.selectionRangeOpt = selectionRangeOpt,
		.cornerRadiusPx = customData.cornerRadiusPx,
	};
	LineEditUtils::Render(temp);
}

void LineEdit::AccessibilityTest(
	AccessibilityTest_Params const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter) const
{
	LineEditUtils::AccessibilityStuff_Params temp = {
		.eventParams = params,
		.widget = *this,
		.rectCollIter = rectCollIter,
		.text = { this->text.data(), this->text.size() },
		.pusher = params.pusher, };
	LineEditUtils::AccessibilityStuff(temp);
}

bool LineEdit::WidgetEvent_Activate(
	WidgetEvent_ActivateParams const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter)
{
	Impl::BeginInputSession(
		*this,
		params.windowHandler);

	return true;
}
