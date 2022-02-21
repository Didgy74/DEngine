#include <DEngine/Gui/LineFloatEdit.hpp>

#include <DEngine/Gui/Context.hpp>
#include <DEngine/Gui/TextManager.hpp>

#include <sstream>
#include <cstdlib>

using namespace DEngine;
using namespace DEngine::Gui;

namespace DEngine::Gui::impl
{
	struct CustomData
	{
		explicit CustomData(RectCollection::AllocT& alloc) :
			glyphRects{ alloc }
		{}

		Extent textOuterExtent;
		Std::Vec<Rect, RectCollection::AllocT> glyphRects;
	};

	enum class PointerType : u8 { Primary, Secondary };
	[[nodiscard]] static PointerType ToPointerType(CursorButton in) noexcept
	{
		switch (in)
		{
			case CursorButton::Primary: return PointerType::Primary;
			case CursorButton::Secondary: return PointerType::Secondary;
			default: break;
		}
		DENGINE_IMPL_UNREACHABLE();
		return {};
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
		LineFloatEdit& widget;
		RectCollection const& rectCollection;
		Rect const& widgetRect;
		Rect const& visibleRect;
		PointerPress_Pointer const& pointer;
	};
}

struct LineFloatEdit::Impl
{
	static void StartInputConnection(LineFloatEdit& widget, Context& ctx)
	{
		SoftInputFilter filter = SoftInputFilter::SignedFloat;
		if (widget.min >= 0.0)
			filter = SoftInputFilter::UnsignedFloat;
		ctx.TakeInputConnection(
			widget,
			filter,
			{ widget.text.data(), widget.text.length() });
		widget.inputConnectionCtx = &ctx;
	}

	static void ClearInputConnection(LineFloatEdit& widget)
	{
		DENGINE_IMPL_GUI_ASSERT(widget.inputConnectionCtx);
		widget.inputConnectionCtx->ClearInputConnection(widget);
		widget.inputConnectionCtx = nullptr;
	}

	[[nodiscard]] static bool PointerPress(impl::PointerPress_Params const& in) noexcept
	{
		auto& widget = in.widget;
		auto& ctx = in.ctx;
		auto& pointer = in.pointer;
		auto& widgetRect = in.widgetRect;
		auto& visibleRect = in.visibleRect;

		bool eventConsumed = pointer.consumed;

		auto const pointerInside =
			widgetRect.PointIsInside(pointer.pos) &&
			visibleRect.PointIsInside(pointer.pos);

		// The field is currently being held.
		if (widget.pointerId.HasValue())
		{
			auto const currPointerId = widget.pointerId.Value();

			// It's a integration error if we received a click down on a pointer id
			// that's already holding this widget. I think?
			DENGINE_IMPL_GUI_ASSERT(!(currPointerId == pointer.id && pointer.pressed));

			if (currPointerId == pointer.id && !pointer.pressed)
			{
				widget.pointerId = Std::nullOpt;
			}

			bool beginInputSession =
				!eventConsumed &&
				pointerInside &&
				currPointerId == pointer.id &&
				!pointer.pressed &&
				pointer.type == impl::PointerType::Primary;
			if (beginInputSession)
			{
				StartInputConnection(widget, ctx);
				eventConsumed = true;
			}
		}
		else // The widget is not currently being held.
		{
			if (widget.HasInputSession())
			{
				// There are two scenarios in which we want to end the
				// input session.
				bool shouldEndInputSession = false;


				// First is if we pressed somewhere and
				// we hit something else before this widget is processed.
				shouldEndInputSession =
					shouldEndInputSession ||
					eventConsumed &&
					pointer.pressed;

				// Second is if we are processing this widget and we hit
				// outside it.
				shouldEndInputSession =
					shouldEndInputSession ||
					!eventConsumed &&
					pointer.pressed &&
					!pointerInside;

				if (shouldEndInputSession)
				{
					widget.inputConnectionCtx->ClearInputConnection(widget);
					widget.inputConnectionCtx = nullptr;
				}
			}
			else
			{
				// Remember pointerId that is holding our widget
				// if the event is not consumed yet.
				// And we are pressed down,
				// and we are inside
				// with a primary pointer-type.
				bool rememberPointerId =
					!eventConsumed &&
					pointer.pressed &&
					pointerInside &&
					pointer.type == impl::PointerType::Primary;
				if (rememberPointerId)
				{
					widget.pointerId = pointer.id;
					eventConsumed = true;
				}
			}
		}

		eventConsumed = eventConsumed || pointerInside;
		return eventConsumed;
	}
};

using namespace DEngine;
using namespace DEngine::Gui;

LineFloatEdit::~LineFloatEdit()
{
	if (inputConnectionCtx)
	{
		Impl::ClearInputConnection(*this);
	}
}

SizeHint LineFloatEdit::GetSizeHint(Context const& ctx) const
{
	/*
	impl::ImplData& implData = *static_cast<impl::ImplData*>(ctx.Internal_ImplData());
	return impl::TextManager::GetSizeHint(
		implData.textManager,
		{ text.data(), text.size() });
	 */
	return {};
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
		auto customData = impl::CustomData{ pusher.Alloc() };

		customData.glyphRects.Resize(text.size());

		customData.textOuterExtent = textManager.GetOuterExtent(
			{ text.data(), text.size() },
			{},
			customData.glyphRects.Data());
		returnValue.minimum = customData.textOuterExtent;

		pusher.AttachCustomData(pusherIt, Std::Move(customData));
	}
	else
	{
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
		auto* customDataPtr = pusher.GetCustomData2<impl::CustomData>(*this);
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
	auto const intersection = Rect::Intersection(widgetRect, visibleRect);
	if (intersection.IsNothing())
		return;

	auto& drawInfo = params.drawInfo;
	auto& rectCollection = params.rectCollection;

	// Draw the background rect
	auto color = backgroundColor;
	if (HasInputSession())
		color = { 0.5f, 0.0f, 0.0f, 1.0f };
	drawInfo.PushFilledQuad(widgetRect, color);

	// Grab our customData to push text
	auto* customDataPtr = rectCollection.GetCustomData2<impl::CustomData>(*this);
	DENGINE_IMPL_GUI_ASSERT(customDataPtr);
	auto& customData = *customDataPtr;

	DENGINE_IMPL_GUI_ASSERT(customData.glyphRects.Size() == text.size());

	// Apply rendering scissor only if needed
	auto const textIsBiggerThanExtent =
		(customData.textOuterExtent.width + margin * 2) > widgetRect.extent.width ||
		customData.textOuterExtent.height > widgetRect.extent.height;
	Std::Opt<DrawInfo::ScopedScissor> scissor;
	if (textIsBiggerThanExtent)
		scissor = DrawInfo::ScopedScissor(drawInfo, intersection);

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
	impl::PointerPress_Pointer pointer = {};
	pointer.id = impl::cursorPointerId;
	pointer.pressed = params.event.pressed;
	pointer.type = impl::ToPointerType(params.event.button);
	pointer.pos = { (f32)params.cursorPos.x, (f32)params.cursorPos.y };
	pointer.consumed = consumed;

	impl::PointerPress_Params temp = {
		.ctx = params.ctx,
		.widget = *this,
		.rectCollection = params.rectCollection,
		.widgetRect = widgetRect,
		.visibleRect = visibleRect,
		.pointer = pointer, };

	return Impl::PointerPress(temp);
}

void LineFloatEdit::CharRemoveEvent(
	Context& ctx,
	Std::FrameAlloc& transientAlloc)
{
	if (inputConnectionCtx && !text.empty())
	{
		text.pop_back();
		if (!text.empty() && text != "-" && text != "." && text != "-.")
		{
			char* endPtr = nullptr;
			f64 newValue = std::strtod(text.c_str(), &endPtr);
			if (*endPtr == 0) // No error, we met the end of the string.
			{
				value = newValue;
				if (valueChangedFn)
					valueChangedFn(*this, value);
			}
		}
	}
}


void LineFloatEdit::SetValue(f64 in)
{
	std::ostringstream out;
	out.precision(decimalPoints);
	out << std::fixed << in;
	text = out.str();
}

void LineFloatEdit::TextInput(
	Context& ctx,
	Std::FrameAlloc& transientAlloc,
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
			text[i + event.oldIndex] = event.newTextData[i];
	}
}
