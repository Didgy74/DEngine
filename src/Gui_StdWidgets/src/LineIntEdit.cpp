#include <DEngine/Gui/StdWidgets/LineIntEdit.hpp>
#include <DEngine/Gui/StdWidgets/Utilities/LineEditUtils.hpp>


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
		u32 textMarginPx = 0;
		u32 cornerRadiusPx = 0;
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

				widget.value = Std::Clamp(newValue, widget.min, widget.max);
			}

			if (updateText) {
				std::stringstream stream;
				stream << widget.value;
				widget.text = stream.str();
			}

			if (newValueIsDifferent && widget.valueChangedFn)
				widget.valueChangedFn(widget, widget.value);
		}
	}

	static void BeginInputSession(
		LineIntEdit& widget,
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

	static void EndInputSession(LineIntEdit& widget) {
		DENGINE_IMPL_GUI_ASSERT(widget.textEditingSessionOpt.Has());
		widget.textEditingSessionOpt = Std::nullOpt;
	}
};

using namespace DEngine;
using namespace DEngine::Gui;

LineIntEdit::~LineIntEdit()
{
}

Widget::GetSizeHint2_ReturnT LineIntEdit::GetSizeHint2(
	Widget::GetSizeHint2_Params const& params) const
{
	auto& textEngine = params.textEngine;
	auto& pusher = params.pusher;
	auto const& window = params.window;
	auto const& fontScale = params.FontScale();
	auto const& minimumSizeCm = params.MinimumHeightCm();

	auto const pusherIt = pusher.AddEntry(*this);
	auto& customData = pusher.AttachCustomData(pusherIt, Impl::CustomData{ pusher.Alloc() });

	if (pusher.IncludeRendering()) {
		customData.glyphRects.Resize(this->text.size());
	}

	LineIntEdit::Theme theme;
	if (this->getThemeFn) {
		theme = this->getThemeFn({ .widget = *this, .appData = params.appData });
	}
	auto textMarginPx = theme.margin.ResolvePx(window.dpi, window.contentScale);

	LineEditUtils::GetSizeHintParams sizeHintParams = {
		.textEngine = textEngine,
		.windowContentScale = window.contentScale,
		.fontScale = fontScale,
		.windowDpi = window.dpi,
		.text = { this->text.data(), this->text.size() },
		.outGlyphRects = customData.glyphRects.ToSpan(),
		.minimumSizeCm = minimumSizeCm,
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

void LineIntEdit::BuildChildRects(
	Widget::BuildChildRects_Params const& params,
	Rect const& widgetRect,
	Rect const& visibleRect,
	Std::Opt<RectCollection::Iter> const& rectCollIter) const
{
}

void LineIntEdit::Render2(
	Widget::Render_Params const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter) const
{
	auto const& rectColl = params.rectCollection;

	auto const* customDataPtr = rectColl.GetCustomData<Impl::CustomData>(*this);
	DENGINE_IMPL_GUI_ASSERT(customDataPtr);
	auto const& customData = *customDataPtr;

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

bool LineIntEdit::CursorPress2(
	Widget::CursorPressParams const& params,
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

bool LineIntEdit::CursorMove(
	CursorMoveParams const &params,
	Std::Opt<RectCollection::Iter> const& rectCollIter,
	bool occluded)
{
	auto& rectColl = params.rectCollection;

	LineEditUtils::PointerMove_Pointer pointer = {
		.id = LineEditUtils::cursorPointerId,
		.pos = { (f32)params.CursorPos().x, (f32)params.CursorPos().y },
		.consumed = occluded };

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

TouchEventConsumption LineIntEdit::WidgetEvent_TouchPress(
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
	auto startnputConnectionFn = [&] { Impl::BeginInputSession(*this, params.windowHandler); };
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
		.startInputConnectionFn = startnputConnectionFn,
		.endInputConnectionFn = endInputConnectionFn, };
	return LineEditUtils::PointerPress(tempParams);
}

TouchEventConsumption LineIntEdit::WidgetEvent_TouchMove(
	WidgetEvent_TouchMoveParams const &params,
	Std::Opt<RectCollection::Iter> const& rectCollIter,
	bool occluded)
{
	LineEditUtils::PointerMove_Pointer pointer = {
		.id = params.event.id,
		.pos = params.event.position,
		.consumed = occluded, };

	auto setHeldPointerIdFn = [&](Std::Opt<u8> val) { this->pointerId = Std::Move(val); };

	LineEditUtils::PointerMove_Params tempParams = {
		.rectColl = params.rectCollection,
		.widget = *this,
		.rectCollIter = rectCollIter,
		.pointer = pointer,
		.currentlyHeldPointerIdOpt = this->pointerId,
		.setHeldPointerIdFn = setHeldPointerIdFn, };
	return LineEditUtils::PointerMove(tempParams);
}

void LineIntEdit::SetValue(i64 in) {
	std::ostringstream out;
	out << in;
	text = out.str();

	value = in;
}

void LineIntEdit::WidgetEvent_TextInput(
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
	/*
	if (this->textEditingSessionOpt.Has()) {
		DENGINE_IMPL_GUI_ASSERT(event.start + event.count <= text.size());

		auto sizeDifference = (int)event.newText.Size() - (int)event.count;

		// First check if we need to expand our storage.
		if (sizeDifference > 0)
		{
			// We need to move all content behind the old substring
			// To the right.
			auto oldSize = (int)text.size();
			text.resize(text.size() + sizeDifference);
			int end = (int)event.start + (int)event.count - 1;
			for (int i = oldSize - 1; i > end; i -= 1)
				text[i + sizeDifference] = text[i];
		}
		else if (sizeDifference < 0)
		{
			// We need to move all content behind the old substring
			// To the left.
			auto oldSize = (int)text.size();
			int begin = (int)event.start + (int)event.count;
			for (int i = begin; i < oldSize; i += 1)
				text[i + sizeDifference] = text[i];
			text.resize(text.size() + sizeDifference);
		}

		for (int i = 0; i < event.newText.Size(); i += 1)
			text[i + event.start] = (char)event.newText[i];

		Impl::UpdateValue(*this, false);
	}
	*/
}

void LineIntEdit::WidgetEvent_TextSelection(
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

void LineIntEdit::WidgetEvent_EndTextInputSession(
	AllocRef const& transientAlloc,
	WidgetEvent_EndTextInputSessionParams const& event)
{
	if (this->textEditingSessionOpt.Has()) {
		this->textEditingSessionOpt = Std::nullOpt;
	}
}

void LineIntEdit::AccessibilityTest(
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
