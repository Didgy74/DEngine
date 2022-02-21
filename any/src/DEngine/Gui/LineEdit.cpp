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
		explicit CustomData(RectCollection::AllocT& alloc) :
			glyphRects{ alloc }
		{

		}


		Extent textOuterExtent = {};
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
		RectCollection const& rectCollection;
		LineEdit& widget;
		Rect const& widgetRect;
		Rect const& visibleRect;
		PointerPress_Pointer const& pointer;
	};

	static void BeginInputSession(LineEdit& widget, Context& ctx)
	{
		SoftInputFilter filter = {};
		if (widget.type == LineEdit::Type::Float)
			filter = SoftInputFilter::SignedFloat;
		else if (widget.type == LineEdit::Type::Integer)
			filter = SoftInputFilter::SignedInteger;
		else if (widget.type == LineEdit::Type::UnsignedInteger)
			filter = SoftInputFilter::UnsignedInteger;
		else
			DENGINE_IMPL_UNREACHABLE();

		ctx.TakeInputConnection(
			widget,
			filter,
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

			if (currPointerId == pointer.id &&
				!pointer.pressed)
			{
				widget.pointerId = Std::nullOpt;
			}

			if (!eventConsumed &&
				pointerInside &&
				currPointerId == pointer.id &&
				!pointer.pressed &&
				pointer.type == PointerType::Primary)
			{
				BeginInputSession(widget, ctx);
				eventConsumed = true;
			}
		}
		else {
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
					widget.inputConnectionCtx->ClearInputConnection(widget);
					widget.inputConnectionCtx = nullptr;
					eventConsumed = true;
				}
			}
			else
			{
				if (!eventConsumed &&
					pointer.pressed &&
					pointerInside &&
					pointer.type == PointerType::Primary)
				{
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

SizeHint LineEdit::GetSizeHint(Context const& ctx) const
{
/*
	auto& implData = *static_cast<impl::ImplData const*>(ctx.Internal_ImplData());
	auto& textManager = implData.GetTextManager();

	auto returnVal = impl::TextManager::GetSizeHint(
		textManager,
		{ text.data(), text.size() });
	returnVal.minimum.width += margin * 2;
	returnVal.minimum.height += margin * 2;
	return returnVal;
*/
	return {};
}

void LineEdit::Render(
	Context const& ctx,
	Extent framebufferExtent,
	Rect widgetRect,
	Rect visibleRect,
	DrawInfo& drawInfo) const
{

}

/*
void LineEdit::CharEnterEvent(Context& ctx)
{
	if (inputConnectionCtx)
	{
		if (text.empty())
		{
			text = "0";
			if (textChangedFn)
				textChangedFn(*this);
		}
		ClearInputConnection();
	}
}

void LineEdit::CharEvent(Context& ctx, u32 charEvent)
{
	if (inputConnectionCtx)
	{
		bool validChar = false;
		switch (this->type)
		{
		case Type::Float:
			if ('0' <= charEvent && charEvent <= '9')
			{
				validChar = true;
				break;
			}
			if (charEvent == '-' && text.length() == 0)
			{
				validChar = true;
				break;
			}
			if (charEvent == '.') // Check if we already have dot
			{
				bool alreadyHasDot = text.find('.') != std::string::npos;
				if (!alreadyHasDot)
				{
					validChar = true;
					break;
				}
			}
			break;
		case Type::Integer:
			if ('0' <= charEvent && charEvent <= '9')
			{
				validChar = true;
				break;
			}
			if (charEvent == '-' && text.length() == 0)
			{
				validChar = true;
				break;
			}
			break;
		case Type::UnsignedInteger:
			if ('0' <= charEvent && charEvent <= '9')
			{
				validChar = true;
				break;
			}
			break;
		}
		if (validChar)
		{
			text.push_back((u8)charEvent);
			auto const& string = text;
			if (string != "" && string != "-" && string != "." && string != "-.")
			{
				if (textChangedFn)
					textChangedFn(*this);
			}
		}
	}
}
 */

void LineEdit::CharRemoveEvent(
	Context& ctx,
	Std::FrameAlloc& transientAlloc)
{
	if (inputConnectionCtx && !text.empty())
	{
		text.pop_back();
		auto const& string = text;
		if (!string.empty() && string != "-" && string != "." && string != "-.")
		{
			if (textChangedFn)
				textChangedFn(*this);
		}
	}
}

void LineEdit::InputConnectionLost()
{
	if (this->inputConnectionCtx)
	{
		ClearInputConnection();
	}
}

bool LineEdit::TouchPressEvent(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	Gui::TouchPressEvent event)
{
	/*
	impl::PointerPress_Params temp = {};
	temp.ctx = &ctx;
	temp.pointerId = impl::cursorPointerId;
	temp.pointerPos = event.position;
	temp.pointerPressed = event.pressed;
	temp.pointerType = impl::PointerType::Primary;
	temp.visibleRect = visibleRect;
	temp.widgetRect = widgetRect;
	temp.widget = this;

	return impl::LineEditImpl::PointerPressed(temp);
	*/
	return false;
}

void LineEdit::ClearInputConnection()
{
	DENGINE_IMPL_GUI_ASSERT(this->inputConnectionCtx);
	this->inputConnectionCtx->ClearInputConnection(*this);
	this->inputConnectionCtx = nullptr;
}


SizeHint LineEdit::GetSizeHint2(
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

void LineEdit::BuildChildRects(
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

void LineEdit::Render2(
	Widget::Render_Params const& params,
	Rect const& widgetRect,
	Rect const& visibleRect) const
{
	auto const intersection = Rect::Intersection(widgetRect, visibleRect);
	if (intersection.IsNothing())
		return;

	auto& drawInfo = params.drawInfo;
	auto& rectCollection = params.rectCollection;

	drawInfo.PushFilledQuad(widgetRect, backgroundColor);

	auto* customDataPtr = rectCollection.GetCustomData2<Impl::CustomData>(*this);
	DENGINE_IMPL_GUI_ASSERT(customDataPtr);
	auto& customData = *customDataPtr;

	DENGINE_IMPL_GUI_ASSERT(customData.glyphRects.Size() == text.size());

	auto const textIsBiggerThanExtent =
		(customData.textOuterExtent.width + margin * 2)  > widgetRect.extent.width ||
		customData.textOuterExtent.height > widgetRect.extent.height;

	Std::Opt<DrawInfo::ScopedScissor> scissor;
	if (textIsBiggerThanExtent)
		scissor = DrawInfo::ScopedScissor(drawInfo, intersection);

	drawInfo.PushText(
		{ text.data(), text.size() },
		customData.glyphRects.Data(),
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
		.rectCollection = params.rectCollection,
		.widget = *this,
		.widgetRect = widgetRect,
		.visibleRect = visibleRect,
		.pointer = pointer, };

	return Impl::PointerPress(temp);
}