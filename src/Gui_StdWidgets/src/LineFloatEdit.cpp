#include <DEngine/Gui/StdWidgets/LineFloatEdit.hpp>


#include <sstream>

using namespace DEngine;
using namespace DEngine::Gui;

struct LineFloatEdit::Impl
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

	static void BeginInputSession(LineFloatEdit& widget, WidgetEvent_WindowHandler& windowHandler) {
		DENGINE_IMPL_GUI_ASSERT(!widget.textEditingSessionOpt.Has());
		TextEditingSession textEditingSession;
		// TODO: This is hardcoded for some testing!
		u32 selStart = widget.text.length();
		u32 selCount = 0;
		//selStart = 1;
		//selCount = 1;
		textEditingSession.selectionRange.start = selStart;
		textEditingSession.selectionRange.count = selCount;
		textEditingSession.textInputSession = windowHandler.TakeTextInputConnection(
			widget,
			Gui::TextInputType::SignedFloat,
			{widget.text.data(), widget.text.length()},
			selStart,
			selCount);
		widget.textEditingSessionOpt = Std::Move(textEditingSession);
	}

	static void EndInputSession(WindowID windowId, LineFloatEdit& widget) {
		DENGINE_IMPL_GUI_ASSERT(widget.textEditingSessionOpt.Has());
		widget.textEditingSessionOpt = Std::nullOpt;
	}

	static void UpdateValue(LineFloatEdit& widget, bool updateText)
	{
		auto& text = widget.text;
		if (!text.empty() && text != "-" && text != "." && text != "-.") {
			char* index = nullptr;
			f64 newValue = std::strtof(text.c_str(), &index);
			bool newValueIsDifferent = false;
			if (*index != 0) {
				// Error
			} else {
				newValueIsDifferent = newValue != widget.value;
				widget.value = Std::Clamp(newValue, widget.min, widget.max);
			}

			if (updateText) {
				std::stringstream stream;
				stream.precision(widget.decimalPoints);
				stream << std::fixed << widget.value;
				widget.text = stream.str();
			}

			if (newValueIsDifferent && widget.valueChangedFn)
				widget.valueChangedFn(widget, widget.value);
		}
	}
};

LineFloatEdit::~LineFloatEdit()
{
}

void LineFloatEdit::SetValue(f64 in) {
	// TODO: This should likely be done using std::from_chars to avoid allocations
	// but at the time of writing, std::from_chars is not implemented for floats on Android NDK.
	std::ostringstream out;
	out.precision(decimalPoints);
	out << std::fixed << in;
	text = out.str();

	value = in;
}

bool LineFloatEdit::CursorPress2(
	Widget::CursorPressParams const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter,
	bool consumed)
{
	auto const& rectColl = params.rectCollection;

	LineEditUtils::PointerPress_Pointer pointer = {
		.id = LineEditUtils::cursorPointerId,
		.pos = { (f32)params.cursorPos.x, (f32)params.cursorPos.y },
		.type = LineEditUtils::ToPointerType(params.CursorButton()),
		.pressed = params.CursorPressed(),
		.consumed = consumed, };

	auto setHeldPointerIdFn = [&](Std::Opt<u8> val) { this->pointerId = Std::Move(val); };
	auto startInputConnectionFn = [&] { Impl::BeginInputSession( *this, params.windowHandler); };
	auto endInputConnectionFn = [&] {
		this->textEditingSessionOpt = Std::nullOpt;
	};

	LineEditUtils::PointerPress_Params tempParams = {
		.rectColl  = rectColl,
		.widget = *this,
		.rectCollIter = rectCollIter,
		.pointer = pointer,
		.currentlyHeldPointerIdOpt = this->pointerId,
		.hasTextEditingSession = this->textEditingSessionOpt.Has(),
		.setHeldPointerIdFn = setHeldPointerIdFn,
		.startInputConnectionFn = startInputConnectionFn,
		.endInputConnectionFn = endInputConnectionFn, };
	auto temp = LineEditUtils::PointerPress(tempParams);
	return temp.consumed;
}

bool LineFloatEdit::CursorMove(
	CursorMoveParams const &params,
	Std::Opt<RectCollection::Iter> const& rectCollIter,
	bool occluded)
{
	auto& rectColl = params.rectCollection;

	LineEditUtils::PointerMove_Pointer pointer = {
		.id = LineEditUtils::cursorPointerId,
		.pos = { (f32)params.CursorPos().x, (f32)params.CursorPos().y },
		.consumed = occluded, };

	auto setHeldPointerFn = [&](Std::Opt<u8> val) { this->pointerId = Std::Move(val); };

	LineEditUtils::PointerMove_Params tempParams = {
		.rectColl = rectColl,
		.widget = *this,
		.rectCollIter = rectCollIter,
		.pointer = pointer,
		.currentlyHeldPointerIdOpt = this->pointerId,
		.setHeldPointerIdFn = setHeldPointerFn, };
	auto temp = LineEditUtils::PointerMove(tempParams);
	return temp.consumed;
}

TouchEventConsumption LineFloatEdit::WidgetEvent_TouchPress(
	WidgetEvent_TouchPressParams const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter,
	bool consumed)
{
	LineEditUtils::PointerPress_Pointer pointer = {
		.id = params.event.id,
		.pos = params.event.position,
		.type = LineEditUtils::PointerType::Primary,
		.pressed = params.event.pressed,
		.consumed = consumed, };

	auto setHeldPointerIdFn = [&](Std::Opt<u8> val) { this->pointerId = Std::Move(val); };
	auto startInputConnectionFn = [&] { Impl::BeginInputSession(*this, params.windowHandler); };
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
		.endInputConnectionFn = endInputConnectionFn, };
	return LineEditUtils::PointerPress(temp);
}

TouchEventConsumption LineFloatEdit::WidgetEvent_TouchMove(
	WidgetEvent_TouchMoveParams const &params,
	Std::Opt<RectCollection::Iter> const& rectCollIter,
	bool occluded)
{
	LineEditUtils::PointerMove_Pointer pointer = {
		.id = params.event.id,
		.pos = params.event.position,
		.consumed = occluded, };

	auto setHeldPointerFn = [&](Std::Opt<u8> val) { this->pointerId = Std::Move(val); };

	LineEditUtils::PointerMove_Params tempParams = {
		.rectColl = params.rectCollection,
		.widget = *this,
		.rectCollIter = rectCollIter,
		.pointer = pointer,
		.currentlyHeldPointerIdOpt = this->pointerId,
		.setHeldPointerIdFn = setHeldPointerFn,
	};
	return LineEditUtils::PointerMove(tempParams);
}

void LineFloatEdit::WidgetEvent_TextInput(
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

		Impl::UpdateValue(*this, false);
	}
}

void LineFloatEdit::WidgetEvent_EndTextInputSession(
	AllocRef const&,
	WidgetEvent_EndTextInputSessionParams const&)
{
	if (this->textEditingSessionOpt.Has()) {
		this->textEditingSessionOpt = Std::nullOpt;
	}
}

Widget::GetSizeHint2_ReturnT LineFloatEdit::GetSizeHint2(
	Widget::GetSizeHint2_Params const& params) const
{
	auto const& window = params.window;
	auto const& fontScale = params.FontScale();
	auto const& minimumHeightCm = params.MinimumHeightCm();
	auto& textEngine = params.textEngine;
	auto& pusher = params.pusher;

	auto const pusherIt = pusher.AddEntry(*this);
	auto& customData = pusher.AttachCustomData(pusherIt, Impl::CustomData{ pusher.Alloc() });

	if (pusher.IncludeRendering()) {
		customData.glyphRects.Resize(this->text.size());
	}

	LineFloatEdit::Theme theme;
	if (this->getThemeFn) {
		theme = this->getThemeFn({ .widget = *this, .appData = params.appData });
	}

	auto textMarginPx = theme.margin.ResolvePx(window.dpi, window.contentScale);

	LineEditUtils::GetSizeHintParams sizeHintParams = {
		.textEngine = textEngine,
		.windowContentScale = params.window.contentScale,
		.fontScale = fontScale,
		.windowDpi = params.window.dpi,
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

	pusher.SetSizeHint(pusherIt, result.sizeHint, false);
	return {
		.iter = pusherIt,
		.sizeHint = result.sizeHint };
}

void LineFloatEdit::BuildChildRects(
	Widget::BuildChildRects_Params const& params,
	Rect const& widgetRect,
	Rect const& visibleRect,
	Std::Opt<RectCollection::Iter> const& rectCollIter) const
{
}

void LineFloatEdit::Render2(
	Widget::Render_Params const& renderParams,
	Std::Opt<RectCollection::Iter> const& rectCollIter) const
{
	auto& rectColl = renderParams.rectCollection;

	auto* customDataPtr = rectColl.GetCustomData<Impl::CustomData>(*this);
	DENGINE_IMPL_GUI_ASSERT(customDataPtr);
	auto& customData = *customDataPtr;

	Std::Opt<LineEditUtils::SelectionRange> selectionRangeOpt;
	if (this->textEditingSessionOpt.Has()) {
		auto const& textEditingSession = this->textEditingSessionOpt.Get();
		selectionRangeOpt = { textEditingSession.selectionRange.start, textEditingSession.selectionRange.count };
	}

	LineEditUtils::Render_Params temp = {
		.renderParams = renderParams,
		.widget = *this,
		.rectColl = rectColl,
		.rectCollIter = rectCollIter,
		.backgroundColor = this->backgroundColor,
		.textColor = renderParams.TextColor(),
		.textOuterExtent = customData.textOuterExtent,
		.text = { this->text.data(), this->text.size() },
		.glyphRects = customData.glyphRects.ToSpan(),
		.fontId = customData.fontSizeId,
		.selectionRangeOpt = selectionRangeOpt,
		.cornerRadiusPx = customData.cornerRadiusPx,
	};
	LineEditUtils::Render(temp);
}

void LineFloatEdit::AccessibilityTest(
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
