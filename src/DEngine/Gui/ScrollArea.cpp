#include <DEngine/Gui/ScrollArea.hpp>

using namespace DEngine;
using namespace DEngine::Gui;

namespace DEngine::Gui::impl
{
  static Rect GetChildRect(
    Rect widgetRect,
    SizeHint childSizeHint,
    u32 scrollBarPos,
    u32 scrollBarThickness)
  {
    Rect childRect{};
    childRect.position = widgetRect.position;
    if (childSizeHint.expandX)
    {
      childRect.extent.width = widgetRect.extent.width - scrollBarThickness;
    }
    else
    {
      childRect.extent.width = childSizeHint.preferred.width;
    }
    if (childSizeHint.expandY)
    {
      childRect.extent.height = widgetRect.extent.height;
    }
    else
    {
      childRect.position.y -= scrollBarPos;
      childRect.extent.height = childSizeHint.preferred.height;
    }
    return childRect;
  }

  static Rect GetScrollbarRect(
    ScrollArea const& widget,
    Rect widgetRect,
    u32 widgetHeight)
  {
    Rect scrollbarRect{};
    scrollbarRect.extent.width = widget.scrollBarThickness;
    scrollbarRect.extent.height = widgetRect.extent.height;
    scrollbarRect.position.x = widgetRect.position.x + widgetRect.extent.width - widget.scrollBarThickness;
    scrollbarRect.position.y = widgetRect.position.y;
    if (widgetRect.extent.height < widgetHeight)
    {
      f32 factor = (f32)widgetRect.extent.height / widgetHeight;
      if (factor < 1.f)
        scrollbarRect.extent.height *= factor;

      f32 test = (f32)widget.scrollBarPos / (widgetHeight - widgetRect.extent.height);

      scrollbarRect.position.y += test * (widgetRect.extent.height - scrollbarRect.extent.height);
    }
    return scrollbarRect;
  }
}

SizeHint ScrollArea::GetSizeHint(
  Context const& ctx) const
{
  SizeHint returnVal{};
  returnVal.preferred = { 50, 50 };
  returnVal.expandX = true;
  returnVal.expandY = true;

  if (childType == ChildType::Layout)
  {
    Gui::SizeHint childSizeHint = layout->GetSizeHint(ctx);
    returnVal.preferred.width = childSizeHint.preferred.width;
  }
  else if (childType == ChildType::Widget)
  {
    Gui::SizeHint childSizeHint = widget->GetSizeHint(ctx);
    returnVal.preferred.width = childSizeHint.preferred.width;
  }

  returnVal.preferred.width += scrollBarThickness;

  return returnVal;
}

void ScrollArea::Render(
  Context const& ctx,
  Extent framebufferExtent,
  Rect widgetRect,
  Rect visibleRect,
  DrawInfo& drawInfo) const
{
  SizeHint childSizeHint{};
  if (layout)
    childSizeHint = layout->GetSizeHint(ctx);
  else
    childSizeHint = widget->GetSizeHint(ctx);

  Rect childRect = impl::GetChildRect(
    widgetRect,
    childSizeHint,
    scrollBarPos,
    scrollBarThickness);
  if (childSizeHint.expandY)
  {
    scrollBarPos = 0;
  }

  Rect childVisibleRect = Rect::Intersection(childRect, visibleRect);

  if ((layout || widget) && !childVisibleRect.IsNothing())
  {
    drawInfo.PushScissor(childVisibleRect);

    if (childType == ChildType::Layout)
    {
      layout->Render(
        ctx,
        framebufferExtent,
        childRect,
        childVisibleRect,
        drawInfo);
    }
    else if (childType == ChildType::Widget)
    {
      widget->Render(
        ctx,
        framebufferExtent,
        childRect,
        childVisibleRect,
        drawInfo);
    }

    // Add scissor remove drawcmd
    drawInfo.PopScissor();
  }

  // Draw scrollbar
  {
    Rect scrollBarRect = impl::GetScrollbarRect(
      *this,
      widgetRect,
      childRect.extent.height);

    Math::Vec4 color{};
    if (scrollBarState == ScrollBarState::Normal)
      color = { 0.5f, 0.5f, 0.5f, 1.f };
    else if (scrollBarState == ScrollBarState::Hovered)
      color = { 0.75f, 0.75f, 0.75f, 1.f };
    else if (scrollBarState == ScrollBarState::Pressed)
      color = { 1.f, 1.f, 1.f, 1.f };
    drawInfo.PushFilledQuad(scrollBarRect, color);
  }
}

void ScrollArea::CursorMove(
  Context& ctx,
  WindowID windowId,
  Rect widgetRect,
  Rect visibleRect,
  CursorMoveEvent event)
{
  if (childType == ChildType::Layout || childType == ChildType::Widget)
  {
    SizeHint childSizeHint{};
    if (childType == ChildType::Layout)
      childSizeHint = layout->GetSizeHint(ctx);
    else
      childSizeHint = widget->GetSizeHint(ctx);

    Rect childRect = impl::GetChildRect(
      widgetRect,
      childSizeHint,
      scrollBarPos,
      scrollBarThickness);

    // Check if mouse is over
    if (scrollBarState == ScrollBarState::Pressed)
    {
      if (widgetRect.extent.height < childRect.extent.height)
      {
        // The scroll-bar is currently pressed so we need to move it with the cursor.
        // Find out where mouse is relative to the scroll bar position
        i32 test = event.position.y - widgetRect.position.y - scrollBarCursorRelativePosY;
        u32 scrollBarHeight = (f32)widgetRect.extent.height * (f32)widgetRect.extent.height / (f32)childRect.extent.height;
        f32 factor = (f32)test / (widgetRect.extent.height - scrollBarHeight);
        factor = Math::Clamp(factor, 0.f, 1.f);
        scrollBarPos = u32(factor * (childRect.extent.height - widgetRect.extent.height));
      }
    }
    else
    {
      Rect scrollBarRect = impl::GetScrollbarRect(
        *this,
        widgetRect,
        childRect.extent.height);

      bool cursorIsHovering = scrollBarRect.PointIsInside(event.position) && visibleRect.PointIsInside(event.position);
      if (cursorIsHovering)
          scrollBarState = ScrollBarState::Hovered;
      else
        scrollBarState = ScrollBarState::Normal;
    }

    if (childType == ChildType::Layout)
      layout->CursorMove(
        ctx,
        windowId,
        childRect,
        Rect::Intersection(childRect, visibleRect),
        event);
    else if (childType == ChildType::Widget)
      widget->CursorMove(
        ctx,
        windowId,
        childRect,
        Rect::Intersection(childRect, visibleRect),
        event);
  }
}

void ScrollArea::CursorClick(
  Context& ctx,
  WindowID windowId,
  Rect widgetRect, 
  Rect visibleRect,
  Math::Vec2Int cursorPos, 
  CursorClickEvent event)
{
  if (childType == ChildType::Layout || childType == ChildType::Widget)
  {
    SizeHint childSizeHint{};
    if (childType == ChildType::Layout)
      childSizeHint = layout->GetSizeHint(ctx);
    else
      childSizeHint = widget->GetSizeHint(ctx);

    Rect childRect = impl::GetChildRect(
      widgetRect,
      childSizeHint,
      scrollBarPos,
      scrollBarThickness);

    Rect scrollBarRect = impl::GetScrollbarRect(
      *this,
      widgetRect,
      childRect.extent.height);
    bool cursorIsHovered = scrollBarRect.PointIsInside(cursorPos) && visibleRect.PointIsInside(cursorPos);
    if (cursorIsHovered)
    {
      if (event.clicked)
      {
        scrollBarState = ScrollBarState::Pressed;
        scrollBarCursorRelativePosY = cursorPos.y - scrollBarRect.position.y;
      }
      else
        scrollBarState = ScrollBarState::Hovered;
    }
    else
    {
      if (!event.clicked)
        scrollBarState = ScrollBarState::Normal;
    }

    if (childType == ChildType::Layout)
      layout->CursorClick(
        ctx,
        windowId,
        childRect,
        Rect::Intersection(childRect, visibleRect),
        cursorPos,
        event);
    else if (childType == ChildType::Widget)
      widget->CursorClick(
        ctx,
        windowId,
        childRect,
        Rect::Intersection(childRect, visibleRect),
        cursorPos,
        event);
  }
}

void ScrollArea::TouchEvent(
  Context& ctx, 
  WindowID windowId,
  Rect widgetRect, 
  Rect visibleRect, 
  Gui::TouchEvent event)
{
  ParentType::TouchEvent(
    ctx,
    windowId,
    widgetRect,
    visibleRect,
    event);

  if (childType == ChildType::Layout || childType == ChildType::Widget)
  {
    SizeHint childSizeHint{};
    if (childType == ChildType::Layout)
      childSizeHint = layout->GetSizeHint(ctx);
    else
      childSizeHint = widget->GetSizeHint(ctx);

    Rect childRect = impl::GetChildRect(
      widgetRect,
      childSizeHint,
      scrollBarPos,
      scrollBarThickness);

    Rect scrollBarRect = impl::GetScrollbarRect(
      *this,
      widgetRect,
      childRect.extent.height);

    bool cursorIsHovered = scrollBarRect.PointIsInside(event.position) && visibleRect.PointIsInside(event.position);
    if (cursorIsHovered && event.type == TouchEventType::Down && !scrollBarTouchIndex.HasValue())
    {
        scrollBarState = ScrollBarState::Pressed;
        scrollBarCursorRelativePosY = (i32)event.position.y - scrollBarRect.position.y;
        scrollBarTouchIndex = event.id;
    }

    if (event.type == TouchEventType::Up && scrollBarTouchIndex.HasValue() && scrollBarTouchIndex.Value() == event.id)
    {
      scrollBarState = ScrollBarState::Normal;
      scrollBarTouchIndex = Std::nullOpt;
    }

    if (event.type == TouchEventType::Moved && scrollBarTouchIndex.HasValue() && scrollBarTouchIndex.Value() == event.id)
    {
      if (widgetRect.extent.height < childSizeHint.preferred.height)
      {
        // The scroll-bar is currently pressed so we need to move it with the cursor.
        // Find out where mouse is relative to the scroll bar position
        i32 test = event.position.y - widgetRect.position.y - scrollBarCursorRelativePosY;
        u32 scrollBarHeight = widgetRect.extent.height * widgetRect.extent.height / childSizeHint.preferred.height;
        f32 factor = (f32)test / (widgetRect.extent.height - scrollBarHeight);
        factor = Math::Clamp(factor, 0.f, 1.f);
        scrollBarPos = u32(factor * (childSizeHint.preferred.height - widgetRect.extent.height));
      }
    }

    if (childType == ChildType::Layout)
      layout->TouchEvent(
        ctx,
        windowId,
        childRect,
        Rect::Intersection(childRect, visibleRect),
        event);
    else if (childType == ChildType::Widget)
      widget->TouchEvent(
        ctx,
        windowId,
        childRect,
        Rect::Intersection(childRect, visibleRect),
        event);
  }
}

void ScrollArea::CharEnterEvent(Context& ctx)
{
  if (childType == ChildType::Layout)
    layout->CharEnterEvent(ctx);
  else if (childType == ChildType::Widget)
    widget->CharEnterEvent(ctx);
}

void ScrollArea::CharEvent(Context& ctx, u32 utfValue)
{
  if (childType == ChildType::Layout)
    layout->CharEvent(ctx, utfValue);
  else if (childType == ChildType::Widget)
    widget->CharEvent(ctx, utfValue);
}

void ScrollArea::CharRemoveEvent(Context& ctx)
{
  if (childType == ChildType::Layout)
    layout->CharRemoveEvent(ctx);
  else if (childType == ChildType::Widget)
    widget->CharRemoveEvent(ctx);
}
