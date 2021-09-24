#include <DEngine/Gui/CollapsingHeader.hpp>

#include <DEngine/Gui/DrawInfo.hpp>

#include <DEngine/Gui/Context.hpp>
#include "ImplData.hpp"

using namespace DEngine;
using namespace DEngine::Gui;

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

	static constexpr u8 cursorPointerId = (u8)-1;

	struct PointerPress_Params
	{
		Context* ctx = nullptr;
		WindowID windowId = {};
		CollapsingHeader* widget = nullptr;
		Rect const* widgetRect = nullptr;
		Rect const* visibleRect = nullptr;
		u8 pointerId = {};
		PointerType pointerType = {};
		Math::Vec2 pointerPos = {};
		bool pointerPressed = {};
		CursorClickEvent const* cursorClick = nullptr;
		TouchPressEvent const* touchPress = nullptr;
	};

	struct PointerMove_Params
	{
		Context* ctx = nullptr;
		WindowID windowId = {};
		CollapsingHeader* widget = nullptr;
		Rect const* widgetRect = nullptr;
		Rect const* visibleRect = nullptr;
		u8 pointerId = {};
		Math::Vec2 pointerPos = {};
		bool pointerOccluded = {};
		CursorMoveEvent const* cursorMove = nullptr;
		TouchMoveEvent const* touchMove = nullptr;
	};

	[[nodiscard]] static Rect BuildHeaderRect(
		Math::Vec2Int widgetPos,
		u32 widgetWidth,
		u32 textHeight,
		u32 textMargin) noexcept
	{
		Rect returnVal = {};
		returnVal.position = widgetPos;
		returnVal.extent.width = widgetWidth;
		returnVal.extent.height = textHeight + (textMargin * 2);
		return returnVal;
	}

	[[nodiscard]] static Rect BuildChildRect(
		Rect const& headerRect,
		u32 widgetHeight) noexcept
	{
		Rect returnVal = headerRect;
		returnVal.position.y += returnVal.extent.height;
		returnVal.extent.height = widgetHeight - headerRect.extent.height;
		return returnVal;
	}
}

struct [[maybe_unused]] Gui::impl::CH_Impl
{
public:
	[[nodiscard]] static bool PointerPress(PointerPress_Params const& params)
	{
		auto& ctx = *params.ctx;
		auto& widget = *params.widget;
		auto& widgetRect = *params.widgetRect;
		auto& visibleRect = *params.visibleRect;

		bool pressEventConsumed = false;

		auto& implData = *static_cast<impl::ImplData*>(ctx.Internal_ImplData());
		auto const textHeight = implData.textManager.lineheight;

		// Check if we are inside the header.
		auto const headerRect = BuildHeaderRect(
			widgetRect.position,
			widgetRect.extent.width,
			textHeight,
			widget.titleMargin);

		auto const insideHeader =
			visibleRect.PointIsInside(params.pointerPos) &&
			headerRect.PointIsInside(params.pointerPos);

		auto const oldCollapsed = widget.collapsed;

		if (widget.headerPointerId.HasValue() &&
			params.pointerType == PointerType::Primary)
		{
			u8 heldPointerId = widget.headerPointerId.Value();
			if (heldPointerId == params.pointerId && !params.pointerPressed)
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
				 params.pointerPressed &&
				 params.pointerType == PointerType::Primary)
		{
			widget.headerPointerId = params.pointerId;
			pressEventConsumed = true;
		}


		// The "collapsed" variable might change depending on the
		// event handling above, so we use the old one.
		if (!oldCollapsed)
		{
			auto const childRect = BuildChildRect(
				headerRect,
				widgetRect.extent.height);
			auto const insideContent =
				childRect.PointIsInside(params.pointerPos) &&
				visibleRect.PointIsInside(params.pointerPos);
			if (insideContent)
				pressEventConsumed = true;

			// We only want to forward the event if it's currently not collapsed.
			if (!widget.collapsed && widget.child)
			{
				auto childEventConsumed = false;
				if (params.pointerId == cursorPointerId)
				{
					childEventConsumed = widget.child->CursorPress(
						*params.ctx,
						params.windowId,
						childRect,
						visibleRect,
						{ (i32)params.pointerPos.x, (i32)params.pointerPos.y },
						*params.cursorClick);
				}
				else
				{
					childEventConsumed = widget.child->TouchPressEvent(
						*params.ctx,
						params.windowId,
						childRect,
						visibleRect,
						*params.touchPress);
				}

				pressEventConsumed = pressEventConsumed || childEventConsumed;
			}
		}

		return pressEventConsumed;
	}

	[[nodiscard]] static bool PointerMove(PointerMove_Params const& params)
	{
		auto& ctx = *params.ctx;
		auto& widget = *params.widget;
		auto& widgetRect = *params.widgetRect;
		auto& visibleRect = *params.visibleRect;

		auto& implData = *static_cast<impl::ImplData*>(ctx.Internal_ImplData());
		auto const textHeight = implData.textManager.lineheight;

		// Check if we are inside the header.
		auto const headerRect = BuildHeaderRect(
			widgetRect.position,
			widgetRect.extent.width,
			textHeight,
			widget.titleMargin);

		auto const insideHeader =
			visibleRect.PointIsInside(params.pointerPos) &&
			headerRect.PointIsInside(params.pointerPos);

		if (params.pointerId == cursorPointerId)
		{
			widget.hoveredByCursor = insideHeader && !params.pointerOccluded;
		}

		if (widget.child && !widget.collapsed)
		{
			auto const occludedForChild = params.pointerOccluded || insideHeader;
			auto const childRect = BuildChildRect(headerRect, widgetRect.extent.height);
			if (params.pointerId == cursorPointerId)
			{
				widget.child->CursorMove(
					ctx,
					params.windowId,
					childRect,
					visibleRect,
					*params.cursorMove,
					occludedForChild);
			}
			else
			{
				widget.child->TouchMoveEvent(
					ctx,
					params.windowId,
					childRect,
					visibleRect,
					*params.touchMove,
					occludedForChild);
			}
		}

		auto const pointerOccluded =
			widgetRect.PointIsInside(params.pointerPos) &&
			visibleRect.PointIsInside(params.pointerPos);
		return pointerOccluded;
	}
};

CollapsingHeader::CollapsingHeader()
{

}

SizeHint CollapsingHeader::GetSizeHint(
	Context const& ctx) const
{
	SizeHint returnVal = {};

	auto& implData = *static_cast<impl::ImplData*>(ctx.Internal_ImplData());

	auto textRectInner = impl::TextManager::GetSizeHint(
		implData.textManager,
		{ title.data(), title.size() });

	returnVal.preferred.width += textRectInner.preferred.width;
	returnVal.preferred.height += textRectInner.preferred.height;

	returnVal.preferred.width += titleMargin * 2;
	returnVal.preferred.height += titleMargin * 2;

	// Add the child if there is one
	if (!collapsed && child)
	{
		auto const childHint = child->GetSizeHint(ctx);
		returnVal.preferred.width = Math::Max(
			returnVal.preferred.width,
			childHint.preferred.width);
		returnVal.preferred.height += childHint.preferred.height;
	}

	return returnVal;
}

void CollapsingHeader::Render(
	Context const& ctx, 
	Extent framebufferExtent, 
	Rect widgetRect, 
	Rect visibleRect,
	DrawInfo& drawInfo) const
{
	if (Rect::Intersection(widgetRect, visibleRect).IsNothing())
		return;

	auto& implData = *static_cast<impl::ImplData*>(ctx.Internal_ImplData());
	auto const textHeight = implData.textManager.lineheight;

	auto const headerRect = impl::BuildHeaderRect(
		widgetRect.position,
		widgetRect.extent.width,
		textHeight,
		titleMargin);

	Math::Vec4 headerBgColor;
	Math::Vec4 textColor = { 1.f, 1.f, 1.f, 1.f };
	if (headerPointerId.HasValue())
	{
		headerBgColor = { 1.f, 1.f, 1.f, 1.f };
		textColor = { 0.f, 0.f, 0.f, 1.f };
	}
	else
	{
		if (collapsed)
		{
			if (!hoveredByCursor)
				headerBgColor = collapsedColor;
			else
				headerBgColor = { 0.4f, 0.4f, 0.4f, 1.f };
		}
		else
		{
			if (!hoveredByCursor)
				headerBgColor = expandedColor;
			else
				headerBgColor = { 0.7f, 0.7f, 0.7f, 1.f };
		}
	}
	drawInfo.PushFilledQuad(headerRect, headerBgColor);

	auto textRectInner = headerRect;
	textRectInner.position.x += titleMargin;
	textRectInner.position.y += titleMargin;
	textRectInner.extent.height = textHeight;

	impl::TextManager::RenderText(
		implData.textManager,
		{ title.data(), title.size() },
		textColor,
		textRectInner,
		drawInfo);

	if (!collapsed && child)
	{
		auto const childRect = impl::BuildChildRect(
			headerRect,
			widgetRect.extent.height);

		auto scissorGuard = DrawInfo::ScopedScissor(drawInfo, childRect);

		drawInfo.PushFilledQuad(childRect, { 1.f, 1.f, 1.f, 0.3f });

		child->Render(
			ctx,
			framebufferExtent,
			childRect,
			visibleRect,
			drawInfo);
	}
}

void CollapsingHeader::CharEnterEvent(
	Context& ctx)
{
	if (!collapsed && child)
		child->CharEnterEvent(ctx);
}

void CollapsingHeader::CharEvent(
	Context& ctx,
	u32 utfValue)
{
	if (!collapsed && child)
		child->CharEvent(ctx, utfValue);
}

void CollapsingHeader::CharRemoveEvent(
	Context& ctx)
{
	if (!collapsed && child)
		child->CharRemoveEvent(ctx);
}

void CollapsingHeader::InputConnectionLost()
{
	if (!collapsed && child)
		child->InputConnectionLost();
}

bool CollapsingHeader::CursorMove(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	CursorMoveEvent event,
	bool occluded)
{
	impl::PointerMove_Params params = {};
	params.ctx = &ctx;
	params.cursorMove = &event;
	params.widget = this;
	params.widgetRect = &widgetRect;
	params.visibleRect = &visibleRect;
	params.windowId = windowId;
	params.pointerId = impl::cursorPointerId;
	params.pointerPos = { (f32)event.position.x, (f32)event.position.y };
	params.pointerOccluded = occluded;

	return impl::CH_Impl::PointerMove(params);
}

bool CollapsingHeader::CursorPress(
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
	params.widgetRect = &widgetRect;
	params.visibleRect = &visibleRect;
	params.pointerId = impl::cursorPointerId;
	params.pointerPos = { (f32)cursorPos.x, (f32)cursorPos.y };
	params.pointerType = impl::ToPointerType(event.button);
	params.pointerPressed = event.clicked;
	params.widget = this;
	params.cursorClick = &event;

	return impl::CH_Impl::PointerPress(params);
}

bool CollapsingHeader::TouchPressEvent(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	Gui::TouchPressEvent event)
{
	impl::PointerPress_Params params = {};
	params.ctx = &ctx;
	params.windowId = windowId;
	params.widgetRect = &widgetRect;
	params.visibleRect = &visibleRect;
	params.pointerId = event.id;
	params.pointerPos = event.position;
	params.pointerType = impl::PointerType::Primary;
	params.pointerPressed = event.pressed;
	params.widget = this;
	params.touchPress = &event;

	return impl::CH_Impl::PointerPress(params);
}

bool CollapsingHeader::TouchMoveEvent(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	Gui::TouchMoveEvent event,
	bool occluded)
{
	return false;
}