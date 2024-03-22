#include <DEngine/Gui/Button.hpp>
#include <DEngine/Gui/Context.hpp>
#include <DEngine/Gui/TextManager.hpp>
#include <DEngine/Gui/DrawInfo.hpp>

#include <DEngine/Std/Containers/Vec.hpp>

#include <DEngine/Math/Common.hpp>

#include <DEngine/Gui/ButtonSizeBehavior.hpp>

namespace DEngine::Gui::impl
{
	enum class PointerType : u8 { Primary, Secondary };
	[[nodiscard]] static PointerType ToPointerType(CursorButton in) noexcept
	{
		switch (in)
		{
			case CursorButton::Primary: return PointerType::Primary;
			case CursorButton::Secondary: return PointerType::Secondary;
			default:
				DENGINE_IMPL_UNREACHABLE();
				return {};
		}
	}

	static constexpr u8 cursorPointerId = (u8)-1;

	struct PointerMove_Pointer
	{
		u8 id;
		Math::Vec2 pos;
		bool occluded;
	};

	struct PointerMove_Params
	{
		Button& btn;
		Rect const& widgetRect;
		Rect const& visibleRect;
		PointerMove_Pointer const& pointer;
	};

	struct PointerPress_Pointer
	{
		u8 id;
		PointerType type;
		Math::Vec2 pos;
		bool pressed;
		bool consumed;
	};

	struct PointerPress_Params
	{
		Button& btn;
		Rect const& widgetRect;
		Rect const& visibleRect;
		PointerPress_Pointer const& pointer;
        Std::AnyRef customData;
	};
}

using namespace DEngine;
using namespace DEngine::Gui;

class [[maybe_unused]] Button::Impl
{
public:
	// This data is only available when rendering.
	struct [[maybe_unused]] CustomData
	{
		explicit CustomData(RectCollection::AllocRefT const& alloc) : glyphRects{ alloc } {}

		FontFaceSizeId fontSizeId = FontFaceSizeId::Invalid;
		Extent textOuterExtent = {};
		Std::Vec<Rect, RectCollection::AllocRefT> glyphRects;
	};

	// Return true if the event is consumed.
	[[nodiscard]] static bool PointerPress(impl::PointerPress_Params const& params);

	[[nodiscard]] static bool PointerMove(impl::PointerMove_Params const& params);
};

bool Gui::Button::Impl::PointerPress(impl::PointerPress_Params const& params)
{
	auto& widget = params.btn;
	auto const& widgetRect = params.widgetRect;
	auto const& visibleRect = params.visibleRect;
	auto const& pointer = params.pointer;

	bool returnVal = pointer.consumed;

	auto const pointerInside =
		widgetRect.PointIsInside(pointer.pos) &&
		visibleRect.PointIsInside(pointer.pos);

	returnVal = returnVal || pointerInside;

	if (pointer.type != impl::PointerType::Primary)
		return returnVal;

	if (widget.heldPointerId.HasValue()) {
		auto const currPointerId = widget.heldPointerId.Value();
		if (params.pointer.id == currPointerId && !params.pointer.pressed && !params.pointer.consumed)
		{
			widget.heldPointerId = Std::nullOpt;

			if (pointerInside)
				widget.Activate(params.customData);
		}
	} else {
		if (pointerInside && params.pointer.pressed && !params.pointer.consumed)
			widget.heldPointerId = params.pointer.id;
	}

	return returnVal;
}

bool Gui::Button::Impl::PointerMove(impl::PointerMove_Params const& params)
{
	auto& widget = params.btn;
	auto const& widgetRect = params.widgetRect;
	auto const& visibleRect = params.visibleRect;
	auto const& pointer	= params.pointer;

	auto const pointerInside =
		widgetRect.PointIsInside(pointer.pos) &&
		visibleRect.PointIsInside(pointer.pos);

	if (params.pointer.id == impl::cursorPointerId)
		widget.hoveredByCursor = pointerInside && !params.pointer.occluded;

	return pointerInside;
}

Button::Button()
{
	text = "Button";
}

void Button::Activate(Std::AnyRef customData)
{
	if (type == Type::Toggle) {
		toggled = !toggled;
	}

	if (activateFn)
		activateFn(*this, customData);
}

void Button::SetToggled(bool toggled)
{
	toggled = toggled;
}

bool Button::GetToggled() const
{
	return toggled;
}

void Button::CursorExit(Context& ctx)
{
	hoveredByCursor = false;
}

bool Button::CursorMove(
	Widget::CursorMoveParams const& params,
	Rect const& widgetRect,
	Rect const& visibleRect,
	bool occluded)
{
	impl::PointerMove_Pointer pointer = {};
	pointer.id = impl::cursorPointerId;
	pointer.pos = { (f32)params.event.position.x, (f32)params.event.position.y };
	pointer.occluded = occluded;

	impl::PointerMove_Params temp = {
		*this,
		widgetRect,
		visibleRect,
		pointer };

	return Impl::PointerMove(temp);
}

bool Button::CursorPress2(
	Widget::CursorPressParams const& params,
	Rect const& widgetRect,
	Rect const& visibleRect,
	bool consumed)
{
	impl::PointerPress_Pointer pointer = {};
	pointer.id = impl::cursorPointerId;
	pointer.type = impl::ToPointerType(params.event.button);
	pointer.pos = { (f32)params.cursorPos.x, (f32)params.cursorPos.y };
	pointer.pressed = params.event.pressed;
	pointer.consumed = consumed;

	impl::PointerPress_Params temp {
		*this,
		widgetRect,
		visibleRect,
		pointer,
        params.customData };

	return Impl::PointerPress(temp);
}

bool Button::TouchMove2(
	TouchMoveParams const& params,
	Rect const& widgetRect,
	Rect const& visibleRect,
	bool occluded)
{
	impl::PointerMove_Pointer pointer = {};
	pointer.id = params.event.id;
	pointer.pos = params.event.position;
	pointer.occluded = occluded;

	impl::PointerMove_Params temp = {
		*this,
		widgetRect,
		visibleRect,
		pointer };

	return Impl::PointerMove(temp);
}

bool Button::TouchPress2(
	TouchPressParams const& params,
	Rect const& widgetRect,
	Rect const& visibleRect,
	bool consumed)
{
	impl::PointerPress_Pointer pointer = {};
	pointer.id = params.event.id;
	pointer.type = impl::PointerType::Primary;
	pointer.pos = params.event.position;
	pointer.pressed = params.event.pressed;
	pointer.consumed = consumed;

	impl::PointerPress_Params temp {
		.btn = *this,
		.widgetRect = widgetRect,
		.visibleRect = visibleRect,
		.pointer = pointer,
		.customData = params.customData };

	return Impl::PointerPress(temp);
}

SizeHint Button::GetSizeHint2(
	GetSizeHint2_Params const& params) const
{
	auto const& ctx = params.ctx;
	auto& textMgr = params.textManager;
	auto& pusher = params.pusher;
	auto const& window = params.window;

	auto pusherIt = pusher.AddEntry(*this);

	auto& customData = pusher.AttachCustomData(pusherIt, Impl::CustomData{ pusher.Alloc() });
	if (pusher.IncludeRendering()) {
		customData.glyphRects.Resize(text.size());
	}

	auto normalTextScale = ctx.fontScale * window.contentScale;
	auto fontSizeId = FontFaceSizeId::Invalid;
	auto marginAmount = 0;
	{
		auto normalFontSizeId = textMgr.GetFontFaceSizeId(normalTextScale, window.dpiX, window.dpiY);
		auto normalHeight = textMgr.GetLineheight(normalFontSizeId, TextHeightType::Alphas);
		auto normalHeightMargin = (u32)Math::Round((f32)normalHeight * ctx.defaultMarginFactor);
		auto totalNormalHeight = normalHeight + 2 * normalHeightMargin;

		auto minHeight = CmToPixels(ctx.minimumHeightCm, window.dpiY);

		if (totalNormalHeight > minHeight) {
			fontSizeId = normalFontSizeId;
			marginAmount = (i32)normalHeightMargin;
		} else {
			// We can't just do minHeight * defaultMarginFactor for this one, because defaultMarginFactor applies to
			// content, not the outer size. So we set up an equation `height = 2 * marginFactor * content + content`
			// and we solve for content.
			auto contentSizeCm = ctx.minimumHeightCm / ((2 * ctx.defaultMarginFactor) + 1.f);
			auto contentSize = CmToPixels(contentSizeCm, window.dpiY);
			fontSizeId = textMgr.FontFaceSizeIdForLinePixelHeight(
				contentSize,
				TextHeightType::Alphas);
			marginAmount = CmToPixels((f32)contentSizeCm * ctx.defaultMarginFactor, window.dpiY);
		}
	}
	customData.fontSizeId = fontSizeId;

	Std::Span<Rect> glyphRects = {};
	if (pusher.IncludeRendering())
		glyphRects = customData.glyphRects.ToSpan();

	auto textOuterExtent = textMgr.GetOuterExtent(
		{ text.data(), text.size() },
		fontSizeId,
		TextHeightType::Alphas,
		glyphRects);
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

void Button::BuildChildRects(
	Widget::BuildChildRects_Params const& params,
	Rect const& widgetRect,
	Rect const& visibleRect) const
{
}

void Button::Render2(
	Render_Params const& params,
	Rect const& widgetRect,
	Rect const& visibleRect) const
{
	auto& rectCollection = params.rectCollection;
	auto& drawInfo = params.drawInfo;

	auto const intersection = Rect::Intersection(widgetRect, visibleRect);
	if (intersection.IsNothing())
		return;

	Math::Vec4 currColor = {};
	Math::Vec4 currTextColor = {};

	if (heldPointerId.HasValue()) {
		// We have a finger pressing on the button atm
		currColor = colors.pressed;
		currTextColor = colors.pressedText;
	}
	else {
		if (type == Type::Toggle) {
			if (toggled) {
				currColor = colors.toggled;
				currTextColor = colors.toggledText;
			} else {
				currColor = colors.normal;
				currTextColor = colors.normalText;
			}
		} else if (type == Type::Push) {
			currColor = colors.normal;
			currTextColor = colors.normalText;
		}
		else
			DENGINE_IMPL_UNREACHABLE();

		if (hoveredByCursor) {
			currColor += hoverOverlayColor.AsVec4(0.f);
		}
	}

	currColor = currColor.ClampAll(0.f, 1.f);
	currTextColor = currTextColor.ClampAll(0.f, 1.f);

	// Calculate radius
	auto minDimension = Math::Min(widgetRect.extent.width, widgetRect.extent.height);
	auto radius = (int)Math::Floor((f32)minDimension * 0.25f);

	drawInfo.PushFilledQuad(
		widgetRect,
		currColor,
		radius);

	auto* customDataPtr = rectCollection.GetCustomData2<Impl::CustomData>(*this);
	DENGINE_IMPL_GUI_ASSERT(customDataPtr);
	auto& customData = *customDataPtr;

	auto centeringOffset = Extent::CenteringOffset(widgetRect.extent, customData.textOuterExtent);
	centeringOffset.x = Math::Max(centeringOffset.x, 0);
	centeringOffset.y = Math::Max(centeringOffset.y, 0);

	auto textRect = Rect{ widgetRect.position + centeringOffset, customData.textOuterExtent };

	// Used for debugging to display the textRect, to see if the bounding box is correct.
	//drawInfo.PushFilledQuad(textRect, { 1, 0, 0, 0.5f });

	auto scissor = DrawInfo::ScopedScissor(drawInfo, textRect, widgetRect);
	drawInfo.PushText(
		(u64)customData.fontSizeId,
		{ text.data(), text.size() },
		customData.glyphRects.Data(),
		textRect.position,
		currTextColor);
}

void Button::AccessibilityTest(
	AccessibilityTest_Params const& params,
	Rect const& widgetRect,
	Rect const& visibleRect) const
{
	auto& pusher = params.pusher;

	AccessibilityInfoElement element = {};
	element.textStart = pusher.PushText({ this->text.data(), this->text.size() });
	element.textCount = (int)this->text.size();
	element.rect = Intersection(widgetRect, visibleRect);
	element.isClickable = true;
	pusher.PushElement(element);
}