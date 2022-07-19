#include <DEngine/Gui/LineFloatEdit.hpp>
#include <DEngine/Gui/TextManager.hpp>
#include <DEngine/Gui/DrawInfo.hpp>
#include <DEngine/Gui/Context.hpp>

#include <DEngine/Std/Containers/Vec.hpp>

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

	struct PointerPress_Params
	{
		Context& ctx;
		RectCollection const& rectCollection;
		LineFloatEdit& widget;
		Rect const& widgetRect;
		Rect const& visibleRect;
		PointerPress_Pointer const& pointer;
	};

	static void UpdateValue(LineFloatEdit& widget, bool updateText)
	{
		auto& text = widget.text;
		if (!text.empty() && text != "-" && text != "." && text != "-.")
		{
			char* index = nullptr;
			f64 newValue = std::strtof(text.c_str(), &index);
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
				stream.precision(widget.decimalPoints);
				stream << std::fixed << widget.value;
				widget.text = stream.str();
			}

			if (newValueIsDifferent && widget.valueChangedFn)
				widget.valueChangedFn(widget, widget.value);
		}
	}

	static void BeginInputSession(LineFloatEdit& widget, Context& ctx)
	{
		ctx.TakeInputConnection(
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
				BeginInputSession(widget, ctx);
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

LineFloatEdit::~LineFloatEdit()
{
	if (HasInputSession())
		ClearInputConnection();
}

void LineFloatEdit::ClearInputConnection()
{
	DENGINE_IMPL_GUI_ASSERT(this->inputConnectionCtx);
	this->inputConnectionCtx->ClearInputConnection(*this);
	this->inputConnectionCtx = nullptr;
}

void LineFloatEdit::SetValue(f64 in)
{
	std::ostringstream out;
	out.precision(decimalPoints);
	out << std::fixed << in;
	text = out.str();

	value = in;
}

SizeHint LineFloatEdit::GetSizeHint2(
	Widget::GetSizeHint2_Params const& params) const
{
	auto& textManager = params.textManager;
	auto& pusher = params.pusher;

	auto const pusherIt = pusher.AddEntry(*this);

	SizeHint returnValue = {};

	if (pusher.IncludeRendering())
	{
		auto customData = Impl::CustomData{ pusher.Alloc() };
		customData.glyphRects.Resize(text.size());

		customData.textOuterExtent = textManager.GetOuterExtent(
			{ text.data(), text.size() },
			{},
			customData.glyphRects.Data());
		returnValue.minimum = customData.textOuterExtent;

		pusher.AttachCustomData(pusherIt, Std::Move(customData));
	}
	else {
		returnValue.minimum = textManager.GetOuterExtent({ text.data(), text.size() });
	}

	returnValue.minimum.width += margin * 2;
	returnValue.minimum.height += margin * 2;
	returnValue.expandX = true;

	pusher.SetSizeHint(pusherIt, returnValue);
	return returnValue;
}

void LineFloatEdit::BuildChildRects(
	Widget::BuildChildRects_Params const& params,
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
			glyphRect.position.x += (i32)widgetRect.extent.width;
			glyphRect.position.x -= (i32)customData.textOuterExtent.width;
			glyphRect.position.x -= (i32)margin;
			glyphRect.position.y += (i32)margin;
		}
	}
}

void LineFloatEdit::Render2(
	Widget::Render_Params const& params,
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

	drawInfo.PushText(
		{ text.data(), text.size() },
		customData.glyphRects.Data(),
		{ 1.f, 1.f, 1.f, 1.f });
}

bool LineFloatEdit::CursorPress2(
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
		.rectCollection = params.rectCollection,
		.widget = *this,
		.widgetRect = widgetRect,
		.visibleRect = visibleRect,
		.pointer = pointer, };

	return Impl::PointerPress(temp);
}

bool LineFloatEdit::TouchPress2(
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
		.rectCollection = params.rectCollection,
		.widget = *this,
		.widgetRect = widgetRect,
		.visibleRect = visibleRect,
		.pointer = pointer, };

	return Impl::PointerPress(temp);
}

void LineFloatEdit::TextInput(
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

void LineFloatEdit::EndTextInputSession(
	Context& ctx,
	AllocRef const& transientAlloc,
	EndTextInputSessionEvent const& event)
{
	if (HasInputSession())
	{
		ClearInputConnection();
	}
}