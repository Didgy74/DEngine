#include <DEngine/Gui/LineFloatEdit.hpp>

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
		WindowID windowId = {};
		LineFloatEdit* widget = {};
		Rect widgetRect = {};
		Rect visibleRect = {};

		u8 pointerId = {};
		Math::Vec2 pointerPos = {};
		bool pointerPressed = {};
		PointerType pointerType = {};
	};
}

struct DEngine::Gui::impl::LineFloatEditImpl
{
public:
	[[nodiscard]] static bool PointerPressed(PointerPress_Params const& in) noexcept
	{
		auto pointerInside = in.widgetRect.PointIsInside(in.pointerPos) &&
			in.visibleRect.PointIsInside(in.pointerPos);

		// The field is currently being held.
		if (in.widget->pointerId.HasValue())
		{
			auto const& pointerId = in.widget->pointerId.Value();

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

LineFloatEdit::~LineFloatEdit()
{
	if (inputConnectionCtx)
	{
		ClearInputConnection();
	}
}

SizeHint LineFloatEdit::GetSizeHint(Context const& ctx) const
{
	impl::ImplData& implData = *static_cast<impl::ImplData*>(ctx.Internal_ImplData());
	return impl::TextManager::GetSizeHint(
		implData.textManager,
		{ text.data(), text.size() });
}

void LineFloatEdit::Render(
	Context const& ctx,
	Extent framebufferExtent,
	Rect widgetRect,
	Rect visibleRect,
	DrawInfo& drawInfo) const
{
	drawInfo.PushFilledQuad(widgetRect, backgroundColor);

	impl::ImplData& implData = *static_cast<impl::ImplData*>(ctx.Internal_ImplData());
	impl::TextManager::RenderText(
		implData.textManager,
		{ text.data(), text.size() },
		{ 1.f, 1.f, 1.f, 1.f },
		widgetRect,
		drawInfo);
}

void LineFloatEdit::CharEnterEvent(Context& ctx)
{
	if (inputConnectionCtx)
	{
		DENGINE_IMPL_GUI_ASSERT(inputConnectionCtx == &ctx);
		EndEditingSession();
	}
}

void LineFloatEdit::CharEvent(
	Context& ctx, 
	u32 charEvent)
{
	DENGINE_IMPL_GUI_ASSERT(min <= max);

	if (inputConnectionCtx)
	{
		bool validChar = false;
		if ('0' <= charEvent && charEvent <= '9')
		{
			validChar = true;
		}
		else if (min < 0.0 && charEvent == '-' && text.empty())
		{
			validChar = true;
		}
		else if (charEvent == '.') // Check if we already have dot
		{
			bool alreadyHasDot = text.find('.') != std::string::npos;
			if (!alreadyHasDot)
			{
				validChar = true;
			}
		}
		if (validChar)
		{
			text.push_back((u8)charEvent);
			if (!text.empty() && text != "-" && text != "." && text != "-.")
			{
				f64 newValue = std::stod(text);
				if (newValue >= min && newValue <= max && newValue != currentValue)
				{
					currentValue = newValue;
					if (valueChangedFn)
						valueChangedFn(*this, newValue);
				}
			}
		}
	}
}

void LineFloatEdit::CharRemoveEvent(Context& ctx)
{
	if (inputConnectionCtx && !text.empty())
	{
		text.pop_back();
		if (!text.empty() && text != "-" && text != "." && text != "-.")
		{
			f64 newValue = std::stod(text);
			if (newValue >= min && newValue <= max && newValue != currentValue)
			{
				currentValue = newValue;
				if (valueChangedFn)
					valueChangedFn(*this, newValue);
			}
		}
	}
}

void LineFloatEdit::InputConnectionLost()
{
	if (this->inputConnectionCtx)
	{
		ClearInputConnection();
	}
}

bool LineFloatEdit::CursorPress(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	Math::Vec2Int cursorPos,
	CursorClickEvent event)
{
	impl::PointerPress_Params params = {};
	params.ctx = &ctx;
	params.windowId = windowId;
	params.widget = this;
	params.widgetRect = widgetRect;
	params.visibleRect = visibleRect;
	params.pointerId = impl::cursorPointerId;
	params.pointerPos = { (f32)cursorPos.x, (f32)cursorPos.y };
	params.pointerPressed = event.clicked;
	params.pointerType = impl::ToPointerType(event.button);

	return impl::LineFloatEditImpl::PointerPressed(params);
}

bool LineFloatEdit::TouchPressEvent(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	Gui::TouchPressEvent event)
{
	impl::PointerPress_Params params = {};
	params.ctx = &ctx;
	params.pointerId = event.id;
	params.pointerPos = event.position;
	params.pointerPressed = event.pressed;
	params.pointerType = impl::PointerType::Primary;

	return impl::LineFloatEditImpl::PointerPressed(params);
}

void LineFloatEdit::EndEditingSession()
{
	f64 newValue = 0;

	if (text.empty() || text == "-" || text == "." || text == "-.")
	{
		newValue = 0;

		// Add 2 to acommodate "0."
		text.resize((uSize)decimalPoints + 2);
		text[0] = '0';
		text[1] = '.';
		for (uSize i = 2; i < text.size(); i += 1)
			text[i] = '0';
	}
	else
	{
		newValue = std::stod(text);
		if (max < newValue)
			newValue = max;
		else if (min > newValue)
			newValue = min;
		text = std::to_string(newValue);

		// Fix decimals
		Std::Opt<uSize> dotIndex;
		for (uSize i = 0; i < text.size(); i += 1)
		{
			if (text[i] == '.')
			{
				dotIndex = i;
				break;
			}
		}
		if (dotIndex.HasValue())
		{
			// Check if we have too many decimals or too little
			uSize decimalCount = text.size() - dotIndex.Value();
			if (decimalCount > decimalPoints)
			{
				text.resize(dotIndex.Value() + decimalPoints + 1);
			}
		}
		else
		{
			// We have no decimals. Add dot and N zeroes.
			text.reserve(text.size() + 1 + decimalPoints);
			text.push_back('.');
			for (uSize i = 0; i < decimalPoints; i += 1)
				text.push_back('0');
		}
	}

	if (currentValue != newValue)
	{
		currentValue = newValue;
		if (valueChangedFn)
			valueChangedFn(*this, newValue);
	}

	ClearInputConnection();
}

void DEngine::Gui::LineFloatEdit::StartInputConnection(Context& ctx)
{
	SoftInputFilter filter = SoftInputFilter::SignedFloat;
	if (min >= 0.0)
		filter = SoftInputFilter::UnsignedFloat;
	ctx.TakeInputConnection(
		*this,
		filter,
		{ text.data(), text.length() });
	inputConnectionCtx = &ctx;
}

void LineFloatEdit::ClearInputConnection()
{
	DENGINE_IMPL_GUI_ASSERT(this->inputConnectionCtx);
	inputConnectionCtx->ClearInputConnection(*this);
	inputConnectionCtx = nullptr;
}