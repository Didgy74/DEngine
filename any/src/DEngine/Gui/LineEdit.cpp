#include <DEngine/Gui/LineEdit.hpp>

#include <DEngine/Gui/Context.hpp>
#include "ImplData.hpp"

namespace DEngine::Gui::impl
{
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

	constexpr u8 cursorPointerId = ~static_cast<u8>(0);

	struct PointerPress_Params
	{
		Context* ctx = {};
		LineEdit* widget = {};
		Rect widgetRect = {};
		Rect visibleRect = {};

		u8 pointerId = {};
		Math::Vec2 pointerPos = {};
		bool pointerPressed = {};
		PointerType pointerType = {};
	};
}

struct DEngine::Gui::impl::LineEditImpl
{
public:
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

	[[nodiscard]] static bool PointerPressed(PointerPress_Params const& in) noexcept
	{
		auto pointerInside =
			in.widgetRect.PointIsInside(in.pointerPos) &&
			in.visibleRect.PointIsInside(in.pointerPos);

		// The field is currently being held.
		if (in.widget->pointerId.HasValue())
		{
			auto const pointerId = in.widget->pointerId.Value();

			// It's a developer bug if we received a click down on a pointer id
			// that's already holding this widget.
			DENGINE_IMPL_GUI_ASSERT(!(pointerId == in.pointerId && in.pointerPressed));

			if (pointerInside &&
				pointerId == in.pointerId &&
				!in.pointerPressed &&
				in.pointerType == PointerType::Primary)
			{
				in.widget->pointerId = Std::nullOpt;

				BeginInputSession(*in.widget, *in.ctx);
			}
			else if (!pointerInside && pointerId == in.pointerId && !in.pointerPressed)
			{
				in.widget->pointerId = Std::nullOpt;
			}
		}
		else
		{
			if (pointerInside && in.pointerType == PointerType::Primary)
			{
				in.widget->pointerId = in.pointerId;
			}
		}

		return pointerInside;
	}
};

using namespace DEngine;
using namespace DEngine::Gui;

LineEdit::~LineEdit()
{
	if (CurrentlyBeingEdited())
		ClearInputConnection();
}

SizeHint LineEdit::GetSizeHint(Context const& ctx) const
{
	impl::ImplData& implData = *static_cast<impl::ImplData*>(ctx.Internal_ImplData());
	auto returnVal = impl::TextManager::GetSizeHint(
		implData.textManager,
		{ text.data(), text.size() });
	returnVal.preferred.width += margin * 2;
	returnVal.preferred.height += margin * 2;
	return returnVal;
}

void LineEdit::Render(
	Context const& ctx,
	Extent framebufferExtent,
	Rect widgetRect,
	Rect visibleRect,
	DrawInfo& drawInfo) const
{
	drawInfo.PushFilledQuad(widgetRect, backgroundColor);

	impl::ImplData& implData = *static_cast<impl::ImplData*>(ctx.Internal_ImplData());

	auto textRect = widgetRect;
	textRect.position.x += margin;
	textRect.position.y += margin;
	textRect.extent.width -= margin * 2;
	textRect.extent.height -= margin * 2;

	impl::TextManager::RenderText(
		implData.textManager,
		{ text.data(), text.size() },
		{ 1.f, 1.f, 1.f, 1.f },
		textRect,
		drawInfo);
}

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

void LineEdit::CharRemoveEvent(Context& ctx)
{
	if (inputConnectionCtx && !text.empty())
	{
		text.pop_back();
		auto const& string = text;
		if (string != "" && string != "-" && string != "." && string != "-.")
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

bool LineEdit::CursorPress(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	Math::Vec2Int cursorPos,
	CursorClickEvent event)
{
	impl::PointerPress_Params temp = {};
	temp.ctx = &ctx;
	temp.pointerId = impl::cursorPointerId;
	temp.pointerPos = { (f32)cursorPos.x, (f32)cursorPos.y };
	temp.pointerPressed = event.clicked;
	temp.pointerType = impl::ToPointerType(event.button);
	temp.visibleRect = visibleRect;
	temp.widgetRect = widgetRect;
	temp.widget = this;

	return impl::LineEditImpl::PointerPressed(temp);
}

bool LineEdit::TouchPressEvent(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	Gui::TouchPressEvent event)
{
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
}

void LineEdit::ClearInputConnection()
{
	DENGINE_IMPL_GUI_ASSERT(this->inputConnectionCtx);
	this->inputConnectionCtx->ClearInputConnection(*this);
	this->inputConnectionCtx = nullptr;
}
