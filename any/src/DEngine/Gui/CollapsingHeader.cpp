#include <DEngine/Gui/CollapsingHeader.hpp>
#include <DEngine/Gui/Context.hpp>
#include <DEngine/Gui/TextManager.hpp>
#include <DEngine/Gui/DrawInfo.hpp>

#include <DEngine/Gui/ButtonSizeBehavior.hpp>

#include <DEngine/Std/Containers/Vec.hpp>
#include <DEngine/Std/Containers/FnRef.hpp>

using namespace DEngine;
using namespace DEngine::Gui;

struct CollapsingHeader::Impl
{
public:
	static constexpr f32 textScale = 1.25f;

	struct CustomData
	{
		explicit CustomData(RectCollection::AllocRefT alloc) :
			glyphRects{ alloc } {}

		Extent titleTextOuterExtent = {};
		int marginAmount = {};
		FontFaceSizeId fontSizeId = FontFaceSizeId::Invalid;
		// Only included when rendering.
		Std::Vec<Rect, RectCollection::AllocRefT> glyphRects;
	};

	[[nodiscard]] static Rect BuildHeaderRect(
		Math::Vec2Int widgetPos,
		int widgetWidth,
		int textHeight,
		int marginAmount) noexcept
	{
		Rect returnVal = {};
		returnVal.position = widgetPos;
		returnVal.extent.width = widgetWidth;
		returnVal.extent.height = textHeight + 2 * marginAmount;
		return returnVal;
	}

	[[nodiscard]] static Rect BuildChildRect(
		Rect const& headerRect,
		u32 widgetHeight) noexcept
	{
		Rect returnVal = headerRect;
		returnVal.position.y += (i32)returnVal.extent.height;

		i32 temp = (i32)widgetHeight - (i32)headerRect.extent.height;
		temp = Math::Max(temp, 0);

		returnVal.extent.height = temp;
		return returnVal;
	}

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

	static constexpr u8 cursorPointerId = (u8)-1;

	struct PointerPress_Pointer
	{
		u8 id;
		PointerType type;
		Math::Vec2 pos;
		bool pressed;
		bool consumed;
	};
	using PointerPress_DispatchFnT = Std::FnRef<bool(Widget&, Rect const&, Rect const&, bool)>;
	struct PointerPress_Params
	{
		Context& ctx;
		RectCollection const& rectCollection;
		CollapsingHeader& widget;
		Rect const& widgetRect;
		Rect const& visibleRect;
		PointerPress_Pointer const& pointer;
		PointerPress_DispatchFnT const& dispatchFn;
	};

	[[nodiscard]] static bool PointerPress(PointerPress_Params const& params)
	{
		auto& ctx = params.ctx;
		auto& rectCollection = params.rectCollection;
		auto& widget = params.widget;
		auto& widgetRect = params.widgetRect;
		auto& visibleRect = params.visibleRect;
		auto& pointer = params.pointer;

		auto* customDataPtr = rectCollection.GetCustomData2<Impl::CustomData>(widget);
		DENGINE_IMPL_GUI_ASSERT(customDataPtr);
		auto& customData = *customDataPtr;
		auto marginAmount = customData.marginAmount;

		bool newEventConsumed = pointer.consumed;

		auto const textHeight = customData.titleTextOuterExtent.height;

		// Check if we are inside the header.
		auto const headerRect = BuildHeaderRect(
			widgetRect.position,
			widgetRect.extent.width,
			textHeight,
			marginAmount);

		auto const insideHeader =
			visibleRect.PointIsInside(pointer.pos) &&
			headerRect.PointIsInside(pointer.pos);

		auto const oldCollapsed = widget.collapsed;

		if (widget.headerPointerId.HasValue() && pointer.type == PointerType::Primary)
		{
			u8 heldPointerId = widget.headerPointerId.Value();
			if (heldPointerId == pointer.id && !pointer.pressed)
			{
				widget.headerPointerId = Std::nullOpt;
				if (insideHeader)
				{
					widget.collapsed = !widget.collapsed;
					if (widget.collapseFn)
						widget.collapseFn(widget);
				}
			}
		}
		else if (!widget.headerPointerId.HasValue() &&
				 insideHeader &&
				 pointer.pressed &&
				 pointer.type == PointerType::Primary &&
		         !newEventConsumed)
		{
			widget.headerPointerId = pointer.id;
			newEventConsumed = true;
		}


		// The "collapsed" variable might change depending on the
		// event handling above, so we use the old one.
		if (!oldCollapsed)
		{
			auto const childRect = BuildChildRect(
				headerRect,
				widgetRect.extent.height);
			auto const insideContent =
				childRect.PointIsInside(pointer.pos) &&
				visibleRect.PointIsInside(pointer.pos);
			auto const childVisibleRect = Rect::Intersection(childRect, visibleRect);

			// We only want to forward the event if it's currently not collapsed.
			if (!widget.collapsed && widget.child)
			{
				auto childEventConsumed = params.dispatchFn(
					*widget.child,
					childRect,
					childVisibleRect,
					newEventConsumed);
				newEventConsumed = newEventConsumed || childEventConsumed || insideContent;
			}
		}

		newEventConsumed = newEventConsumed || insideHeader;
		return newEventConsumed;
	}

	struct PointerMove_Pointer
	{
		u8 id;
		Math::Vec2 pos;
		bool occluded;
	};
	using PointerMove_DispatchFnT = Std::FnRef<void(Widget&, Rect const&, Rect const&, bool)>;
	struct PointerMove_Params
	{
		Context& ctx;
		CollapsingHeader& widget;
		RectCollection const& rectCollection;
		Rect const& widgetRect;
		Rect const& visibleRect;
		PointerMove_Pointer const& pointer;
		PointerMove_DispatchFnT const& dispatchFn;
	};

	[[nodiscard]] static bool PointerMove(PointerMove_Params const& params)
	{
		auto& ctx = params.ctx;
		auto& widget = params.widget;
		auto& rectCollection = params.rectCollection;
		auto& widgetRect = params.widgetRect;
		auto& visibleRect = params.visibleRect;
		auto& pointer = params.pointer;

		auto* customDataPtr = rectCollection.GetCustomData2<Impl::CustomData>(widget);
		DENGINE_IMPL_GUI_ASSERT(customDataPtr);
		auto& customData = *customDataPtr;
		auto marginAmount = customData.marginAmount;

		auto const textHeight = customData.titleTextOuterExtent.height;

		// Check if we are inside the header.
		auto const headerRect = BuildHeaderRect(
			widgetRect.position,
			widgetRect.extent.width,
			textHeight,
			marginAmount);

		auto const insideHeader =
			visibleRect.PointIsInside(pointer.pos) &&
			headerRect.PointIsInside(pointer.pos);

		if (pointer.id == cursorPointerId) {
			widget.hoveredByCursor = insideHeader && !pointer.occluded;
		}

		if (widget.child && !widget.collapsed) {
			auto const occludedForChild = pointer.occluded || insideHeader;
			auto const childRect = BuildChildRect(headerRect, widgetRect.extent.height);

			params.dispatchFn(
				*widget.child,
				childRect,
				visibleRect,
				occludedForChild);
		}

		auto const pointerOccluded = PointIsInAll(pointer.pos, { widgetRect, visibleRect});
		return pointerOccluded;
	}
};

CollapsingHeader::CollapsingHeader()
{

}

SizeHint CollapsingHeader::GetSizeHint2(
	Widget::GetSizeHint2_Params const& params) const
{
	auto const& ctx = params.ctx;
	auto const& window = params.window;
	auto& pusher = params.pusher;
	auto& textMgr = params.textManager;

	auto const& pusherIt = pusher.AddEntry(*this);

	auto& customData = pusher.AttachCustomData(pusherIt,Impl::CustomData{ pusher.Alloc() });
	if (pusher.IncludeRendering()) {
		customData.glyphRects.Resize(title.size());
	}

	auto normalTextScale = ctx.fontScale * window.contentScale;
	auto fontSizeId = FontFaceSizeId::Invalid;
	auto marginAmount = 0;
	{
		auto normalHeight = textMgr.GetLineheight(normalTextScale, window.dpiX, window.dpiY);
		auto normalHeightMargin = (u32)Math::Round((f32)normalHeight * ctx.defaultMarginFactor);
		normalHeight += 2 * normalHeightMargin;

		auto minHeight = CmToPixels(ctx.minimumHeightCm, window.dpiY);

		if (normalHeight > minHeight) {
			fontSizeId = textMgr.GetFontFaceSizeId(normalTextScale, window.dpiX, window.dpiY);
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
	customData.marginAmount = marginAmount;

	auto textOuterExtent = textMgr.GetOuterExtent(
		{ title.data(), title.size() },
		fontSizeId,
		TextHeightType::Alphas,
		customData.glyphRects.ToSpan());
	customData.titleTextOuterExtent = textOuterExtent;

	SizeHint returnVal = {};
	returnVal.minimum = customData.titleTextOuterExtent;
	returnVal.minimum.AddPadding(marginAmount);
	returnVal.minimum.width = Math::Max(
		returnVal.minimum.width,
		(u32)CmToPixels(ctx.minimumHeightCm, window.dpiX));
	returnVal.minimum.height = Math::Max(
		returnVal.minimum.height,
		(u32)CmToPixels(ctx.minimumHeightCm, window.dpiY));

	// Add the child if there is one
	if (!collapsed && child) {
		auto const childHint = child->GetSizeHint2(params);
		returnVal.minimum.width = Math::Max(
			returnVal.minimum.width,
			childHint.minimum.width);

		returnVal.minimum.height += childHint.minimum.height;
	}

	pusher.SetSizeHint(pusherIt, returnVal);

	return returnVal;
}

void CollapsingHeader::BuildChildRects(
	Widget::BuildChildRects_Params const& params,
	Rect const& widgetRect,
	Rect const& visibleRect) const
{
	auto& pusher = params.pusher;
	auto& textManager = params.textManager;

	auto* customDataPtr = pusher.GetCustomData2<Impl::CustomData>(*this);
	DENGINE_IMPL_GUI_ASSERT(customDataPtr);
	auto& customData = *customDataPtr;
	auto marginAmount = customData.marginAmount;

	auto textHeight = customData.titleTextOuterExtent.height;
	auto const headerRect = Impl::BuildHeaderRect(
		widgetRect.position,
		widgetRect.extent.width,
		textHeight,
		marginAmount);

	if (!collapsed && child) {
		auto& childWidget = *child;

		auto const childRect = Impl::BuildChildRect(
			headerRect,
			widgetRect.extent.height);
		auto childVisibleRect = Rect::Intersection(visibleRect, childRect);

		auto childEntry = pusher.GetEntry(childWidget);
		pusher.SetRectPair(childEntry, { childRect, childVisibleRect });
		childWidget.BuildChildRects(
			params,
			childRect,
			childVisibleRect);
	}
}

bool CollapsingHeader::CursorMove(
	Widget::CursorMoveParams const& params,
	Rect const& widgetRect,
	Rect const& visibleRect,
	bool occluded)
{
	Impl::PointerMove_Pointer pointer = {};
	pointer.pos = { (f32)params.event.position.x, (f32)params.event.position.y };
	pointer.occluded = occluded;
	pointer.id = Impl::cursorPointerId;
	auto dispatch = [&params](
		Widget& childIn,
		Rect const& childRect,
		Rect const& visibleRect,
		bool occluded)
	{
		childIn.CursorMove(
			params,
			childRect,
			visibleRect,
			occluded);
	};

	Impl::PointerMove_Params temp = {
		.ctx = params.ctx,
		.widget = *this,
		.rectCollection = params.rectCollection,
		.widgetRect = widgetRect,
		.visibleRect = visibleRect,
		.pointer = pointer,
		.dispatchFn = dispatch,
	};
	return Impl::PointerMove(temp);
}

bool CollapsingHeader::CursorPress2(
	Widget::CursorPressParams const& params,
	Rect const& widgetRect,
	Rect const& visibleRect,
	bool consumed)
{
	Impl::PointerPress_Pointer pointer = {};
	pointer.pos = { (f32)params.cursorPos.x, (f32)params.cursorPos.y };
	pointer.consumed = consumed;
	pointer.id = Impl::cursorPointerId;
	pointer.type = Impl::ToPointerType(params.event.button);
	pointer.pressed = params.event.pressed;
	auto dispatch = [&params](
		Widget& childIn,
		Rect const& childRect,
		Rect const& visibleRect,
		bool occluded)
	{
		return childIn.CursorPress2(
			params,
			childRect,
			visibleRect,
			occluded);
	};

	Impl::PointerPress_Params temp = {
		.ctx = params.ctx,
		.rectCollection = params.rectCollection,
		.widget = *this,
		.widgetRect = widgetRect,
		.visibleRect = visibleRect,
		.pointer = pointer,
		.dispatchFn = dispatch,
	};
	return Impl::PointerPress(temp);
}

bool CollapsingHeader::TouchMove2(
	TouchMoveParams const& params,
	Rect const& widgetRect,
	Rect const& visibleRect,
	bool occluded)
{
	Impl::PointerMove_Pointer pointer = {};
	pointer.pos = params.event.position;
	pointer.occluded = occluded;
	pointer.id = params.event.id;
	auto dispatch = [&params](
		Widget& childIn,
		Rect const& childRect,
		Rect const& visibleRect,
		bool occluded)
	{
		childIn.TouchMove2(
			params,
			childRect,
			visibleRect,
			occluded);
	};

	Impl::PointerMove_Params temp = {
		.ctx = params.ctx,
		.widget = *this,
		.rectCollection = params.rectCollection,
		.widgetRect = widgetRect,
		.visibleRect = visibleRect,
		.pointer = pointer,
		.dispatchFn = dispatch,
	};
	return Impl::PointerMove(temp);
}

bool CollapsingHeader::TouchPress2(
	TouchPressParams const& params,
	Rect const& widgetRect,
	Rect const& visibleRect,
	bool consumed)
{
	Impl::PointerPress_Pointer pointer = {};
	pointer.pos = params.event.position;
	pointer.consumed = consumed;
	pointer.id = params.event.id;
	pointer.type = Impl::PointerType::Primary;
	pointer.pressed = params.event.pressed;
	auto dispatch = [&params](
		Widget& childIn,
		Rect const& childRect,
		Rect const& visibleRect,
		bool occluded)
	{
		return childIn.TouchPress2(
			params,
			childRect,
			visibleRect,
			occluded);
	};

	Impl::PointerPress_Params temp = {
		.ctx = params.ctx,
		.rectCollection = params.rectCollection,
		.widget = *this,
		.widgetRect = widgetRect,
		.visibleRect = visibleRect,
		.pointer = pointer,
		.dispatchFn = dispatch,
	};
	return Impl::PointerPress(temp);
}

void CollapsingHeader::Render2(
	Widget::Render_Params const& params,
	Rect const& widgetRect,
	Rect const& visibleRect) const
{
	auto& rectCollection = params.rectCollection;
	auto& drawInfo = params.drawInfo;

	if (Rect::Intersection(widgetRect, visibleRect).IsNothing())
		return;

	auto* customDataPtr = rectCollection.GetCustomData2<Impl::CustomData>(*this);
	DENGINE_IMPL_GUI_ASSERT(customDataPtr);
	auto& customData = *customDataPtr;
	DENGINE_IMPL_GUI_ASSERT(customData.glyphRects.Size() == title.size());
	auto fontSizeId = customData.fontSizeId;
	auto textOuterExtent = customData.titleTextOuterExtent;
	auto marginAmount = customData.marginAmount;

	auto const textHeight = customData.titleTextOuterExtent.height;

	auto const headerRect = Impl::BuildHeaderRect(
		widgetRect.position,
		(int)widgetRect.extent.width,
		(int)textHeight,
		marginAmount);

	// Draw the child and its background.
	auto childRect = Impl::BuildChildRect(
		headerRect,
		widgetRect.extent.height);
	if (!collapsed && child && !childRect.GetIntersect(visibleRect).IsNothing()) {
		auto scissorGuard = DrawInfo::ScopedScissor(drawInfo, childRect);
		drawInfo.PushFilledQuad(childRect, { 1.f, 1.f, 1.f, 0.1f });
		child->Render2(
			params,
			childRect,
			childRect.GetIntersect(visibleRect));
	}

	Math::Vec4 headerBgColor;
	Math::Vec4 textColor = { 1.f, 1.f, 1.f, 1.f };
	if (headerPointerId.HasValue()) {
		headerBgColor = { 1.f, 1.f, 1.f, 1.f };
		textColor = { 0.f, 0.f, 0.f, 1.f };
	}
	else {
		if (collapsed) {
			if (!hoveredByCursor)
				headerBgColor = collapsedColor;
			else
				headerBgColor = { 0.4f, 0.4f, 0.4f, 1.f };
		} else {
			if (!hoveredByCursor)
				headerBgColor = expandedColor;
			else
				headerBgColor = { 0.7f, 0.7f, 0.7f, 1.f };
		}
	}

	auto minDimension = Math::Min(headerRect.extent.width, headerRect.extent.height);
	auto radius = (int)Math::Floor((f32)minDimension * 0.25f);
	drawInfo.PushFilledQuad(headerRect, headerBgColor, { radius, 0, 0, radius });

	auto drawScissor = DrawInfo::ScopedScissor(drawInfo, headerRect);

	auto textRect = Rect{ headerRect.position, textOuterExtent };
	textRect.position.x += marginAmount;
	textRect.position.y += CenterRangeOffset((int)headerRect.extent.height, (int)textOuterExtent.height);

	drawInfo.PushText(
		(u64)fontSizeId,
		{ title.data(), title.size() },
		customData.glyphRects.Data(),
		textRect.position,
		textColor);
}

void CollapsingHeader::TextInput(
	Context& ctx,
	AllocRef const& transientAlloc,
	TextInputEvent const& event)
{
	if (!collapsed && child)
		child->TextInput(ctx, transientAlloc, event);
}

void CollapsingHeader::EndTextInputSession(
	Context& ctx,
	AllocRef const& transientAlloc,
	EndTextInputSessionEvent const& event)
{
	if (!collapsed && child)
		child->EndTextInputSession(ctx, transientAlloc, event);
}