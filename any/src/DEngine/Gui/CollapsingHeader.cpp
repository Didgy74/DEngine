#include <DEngine/Gui/CollapsingHeader.hpp>
#include <DEngine/Gui/TextManager.hpp>
#include <DEngine/Gui/DrawInfo.hpp>

#include <DEngine/Std/Containers/Vec.hpp>

using namespace DEngine;
using namespace DEngine::Gui;

struct CollapsingHeader::Impl
{
public:
	struct CustomData
	{
		explicit CustomData(RectCollection::AllocT& alloc) :
			glyphRects{ alloc }
		{
		}

		Extent titleTextOuterExtent = {};
		// Only included when rendering.
		Std::Vec<Rect, RectCollection::AllocT> glyphRects;
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
	struct PointerPress_Params
	{
		Context& ctx;
		RectCollection const& rectCollection;
		CollapsingHeader& widget;
		Rect const& widgetRect;
		Rect const& visibleRect;
		PointerPress_Pointer const& pointer;
		Widget::CursorPressParams const* eventParams_cursorPress;
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

		bool newEventConsumed = pointer.consumed;

		auto const textHeight = customData.titleTextOuterExtent.height;

		// Check if we are inside the header.
		auto const headerRect = BuildHeaderRect(
			widgetRect.position,
			widgetRect.extent.width,
			textHeight,
			widget.titleMargin);

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
				auto childEventConsumed = false;

				if (pointer.id == cursorPointerId)
				{
					childEventConsumed = widget.child->CursorPress2(
						*params.eventParams_cursorPress,
						childRect,
						childVisibleRect,
						newEventConsumed);
				}
				else
				{
					DENGINE_IMPL_GUI_UNREACHABLE();

				}

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
	struct PointerMove_Params
	{
		Context& ctx;
		CollapsingHeader& widget;
		RectCollection const& rectCollection;
		Rect const& widgetRect;
		Rect const& visibleRect;
		PointerMove_Pointer const& pointer;
		Widget::CursorMoveParams const* eventParams_cursorMove;
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

		auto const textHeight = customData.titleTextOuterExtent.height;

		// Check if we are inside the header.
		auto const headerRect = BuildHeaderRect(
			widgetRect.position,
			widgetRect.extent.width,
			textHeight,
			widget.titleMargin);

		auto const insideHeader =
			visibleRect.PointIsInside(pointer.pos) &&
			headerRect.PointIsInside(pointer.pos);

		if (pointer.id == cursorPointerId)
		{
			widget.hoveredByCursor = insideHeader && !pointer.occluded;
		}

		if (widget.child && !widget.collapsed)
		{
			auto const occludedForChild = pointer.occluded || insideHeader;
			auto const childRect = BuildChildRect(headerRect, widgetRect.extent.height);
			if (pointer.id == cursorPointerId)
			{
				widget.child->CursorMove(
					*params.eventParams_cursorMove,
					childRect,
					visibleRect,
					occludedForChild);
			}
			else
			{
				DENGINE_IMPL_GUI_UNREACHABLE();
				/*
				widget.child->TouchMoveEvent(
					ctx,
					params.windowId,
					childRect,
					visibleRect,
					*params.touchMove,
					occludedForChild);
				 */
			}
		}

		auto const pointerOccluded =
			widgetRect.PointIsInside(pointer.pos) &&
			visibleRect.PointIsInside(pointer.pos);
		return pointerOccluded;
	}
};

CollapsingHeader::CollapsingHeader()
{

}

SizeHint CollapsingHeader::GetSizeHint(
	Context const& ctx) const
{
/*
	SizeHint returnVal = {};

	auto& implData = *static_cast<impl::ImplData*>(ctx.Internal_ImplData());

	auto textRectInner = impl::TextManager::GetSizeHint(
		implData.textManager,
		{ title.data(), title.size() });

	returnVal.minimum.width += textRectInner.minimum.width;
	returnVal.minimum.height += textRectInner.minimum.height;

	returnVal.minimum.width += titleMargin * 2;
	returnVal.minimum.height += titleMargin * 2;

	// Add the child if there is one
	if (!collapsed && child)
	{
		auto const childHint = child->GetSizeHint(ctx);
		returnVal.minimum.width = Math::Max(
			returnVal.minimum.width,
			childHint.minimum.width);
		returnVal.minimum.height += childHint.minimum.height;
	}

	return returnVal;
*/

	return {};
}

void CollapsingHeader::Render(
	Context const& ctx, 
	Extent framebufferExtent, 
	Rect widgetRect, 
	Rect visibleRect,
	DrawInfo& drawInfo) const
{

}

void CollapsingHeader::CharRemoveEvent(
	Context& ctx,
	Std::FrameAlloc& transientAlloc)
{
	if (!collapsed && child)
		child->CharRemoveEvent(ctx, transientAlloc);
}

void CollapsingHeader::InputConnectionLost()
{
	if (!collapsed && child)
		child->InputConnectionLost();
}



SizeHint CollapsingHeader::GetSizeHint2(
	Widget::GetSizeHint2_Params const& params) const
{
	auto& pusher = params.pusher;
	auto& textManager = params.textManager;

	SizeHint returnValue = {};

	auto const& pusherIt = pusher.AddEntry(*this);

	auto customData = Impl::CustomData{ pusher.Alloc() };

	if (pusher.IncludeRendering())
	{
		customData.glyphRects.Resize(title.size());

		customData.titleTextOuterExtent = textManager.GetOuterExtent(
			{ title.data(), title.size() },
			{},
			customData.glyphRects.Data());
	}
	else
	{
		customData.titleTextOuterExtent = textManager.GetOuterExtent({ title.data(), title.size() });
	}

	returnValue.minimum = customData.titleTextOuterExtent;
	returnValue.minimum.width += titleMargin * 2;
	returnValue.minimum.height += titleMargin * 2;

	// Add the child if there is one
	if (!collapsed && child)
	{
		auto const childHint = child->GetSizeHint2(params);
		returnValue.minimum.width = Math::Max(
			returnValue.minimum.width,
			childHint.minimum.width);

		returnValue.minimum.height += childHint.minimum.height;
	}

	pusher.AttachCustomData(pusherIt, Std::Move(customData));
	pusher.SetSizeHint(pusherIt, returnValue);

	return returnValue;
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

	auto textHeight = customData.titleTextOuterExtent.height;

	if (pusher.IncludeRendering())
	{
		for (auto& item : customData.glyphRects)
		{
			item.position += widgetRect.position;
			item.position.x += titleMargin;
			item.position.y += titleMargin;
		}
	}

	auto const headerRect = Impl::BuildHeaderRect(
		widgetRect.position,
		widgetRect.extent.width,
		textHeight,
		titleMargin);

	if (!collapsed && child)
	{
		auto& childWidget = *child;

		auto const childRect = Impl::BuildChildRect(
			headerRect,
			widgetRect.extent.height);
		auto childVisibleRect = Rect::Intersection(visibleRect, childRect);

		pusher.Push(childWidget, { childRect, childVisibleRect });
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

	Impl::PointerMove_Params temp = {
		.ctx = params.ctx,
		.widget = *this,
		.rectCollection = params.rectCollection,
		.widgetRect = widgetRect,
		.visibleRect = visibleRect,
		.pointer = pointer,
		.eventParams_cursorMove = &params,
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

	Impl::PointerPress_Params temp = {
		.ctx = params.ctx,
		.rectCollection = params.rectCollection,
		.widget = *this,
		.widgetRect = widgetRect,
		.visibleRect = visibleRect,
		.pointer = pointer,
		.eventParams_cursorPress = &params,
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
	auto& framebufferExtent = params.framebufferExtent;

	if (Rect::Intersection(widgetRect, visibleRect).IsNothing())
		return;

	auto* customDataPtr = rectCollection.GetCustomData2<Impl::CustomData>(*this);
	DENGINE_IMPL_GUI_ASSERT(customDataPtr);
	auto& customData = *customDataPtr;

	DENGINE_IMPL_GUI_ASSERT(customData.glyphRects.Size() == title.size());

	auto const textHeight = customData.titleTextOuterExtent.height;

	auto const headerRect = Impl::BuildHeaderRect(
		widgetRect.position,
		widgetRect.extent.width,
		textHeight,
		titleMargin);

	auto const childRect = Impl::BuildChildRect(
		headerRect,
		widgetRect.extent.height);
	if (!collapsed && child && !childRect.IsNothing())
	{
		auto scissorGuard = DrawInfo::ScopedScissor(drawInfo, childRect);
		drawInfo.PushFilledQuad(childRect, { 1.f, 1.f, 1.f, 0.3f });
		child->Render2(
			params,
			childRect,
			visibleRect);
	}

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

	drawInfo.PushText(
		{ title.data(), title.size() },
		customData.glyphRects.Data(),
		textColor);
}

void CollapsingHeader::TextInput(
	Context& ctx,
	Std::FrameAlloc& transientAlloc,
	TextInputEvent const& event)
{
	if (!collapsed && child)
		child->TextInput(ctx, transientAlloc, event);
}
