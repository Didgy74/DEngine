#include <DEngine/Gui/ScrollArea.hpp>

using namespace DEngine;
using namespace DEngine::Gui;

namespace DEngine::Gui::impl
{
  static Rect GetScrollbarRect(
    ScrollArea const& widget,
    Rect widgetRect,
    SizeHint childSizeHint)
  {
    Rect scrollbarRect{};
    scrollbarRect.extent.width = widget.scrollBarWidthPixels;
    scrollbarRect.extent.height = widgetRect.extent.height;
    scrollbarRect.position.x = widgetRect.position.x + widgetRect.extent.width - widget.scrollBarWidthPixels;
    scrollbarRect.position.y = widgetRect.position.y;
    if (widgetRect.extent.height < childSizeHint.preferred.height)
    {
      f32 factor = (f32)widgetRect.extent.height / childSizeHint.preferred.height;
      if (factor < 1.f)
        scrollbarRect.extent.height *= factor;

      f32 test = (f32)widget.scrollBarPos / (childSizeHint.preferred.height - widgetRect.extent.height);

      scrollbarRect.position.y += test * (widgetRect.extent.height - scrollbarRect.extent.height);
    }
    return scrollbarRect;
  }
}

SizeHint ScrollArea::SizeHint(
  Context const& ctx) const
{
  Gui::SizeHint returnVal{};
  returnVal.preferred = { 50, 50 };
  returnVal.expand = true;

  if (childType == ChildType::Layout)
  {
    Gui::SizeHint childSizeHint = layout->SizeHint(ctx);
    returnVal.preferred.width = childSizeHint.preferred.width;
  }
  else if (childType == ChildType::Widget)
  {
    Gui::SizeHint childSizeHint = widget->SizeHint(ctx);
    returnVal.preferred.width = childSizeHint.preferred.width;
  }

  returnVal.preferred.width += scrollBarWidthPixels;

  return returnVal;
}

SizeHint ScrollArea::SizeHint_Tick(
  Context const& ctx)
{
  return SizeHint(ctx);
}

void ScrollArea::Render(
  Context const& ctx,
  Extent framebufferExtent,
  Rect widgetRect,
  Rect visibleRect,
  DrawInfo& drawInfo) const
{
  Gui::SizeHint childSizeHint{};
  if (layout)
    childSizeHint = layout->SizeHint(ctx);
  else
    childSizeHint = widget->SizeHint(ctx);

  Rect childRect = widgetRect;
  childRect.position.y -= scrollBarPos;
  childRect.extent = childSizeHint.preferred;

  Rect childVisibleRect = Rect::Intersection(childRect, visibleRect);

  bool scissorAdded = false;
  if ((layout || widget) && !childVisibleRect.IsNothing())
  {
    scissorAdded = true;
    drawInfo.PushScissor(childVisibleRect);
  }

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
  if (scissorAdded)
    drawInfo.PopScissor();

  // Draw scrollbar
  {
    Gfx::GuiDrawCmd cmd{};
    cmd.type = Gfx::GuiDrawCmd::Type::FilledMesh;
    Rect scrollBarRect = impl::GetScrollbarRect(
      *this,
      widgetRect,
      childSizeHint);
    cmd.rectPosition.x = (f32)scrollBarRect.position.x / framebufferExtent.width;
    cmd.rectPosition.y = (f32)scrollBarRect.position.y / framebufferExtent.height;
    cmd.rectExtent.x = (f32)scrollBarRect.extent.width / framebufferExtent.width;
    cmd.rectExtent.y = (f32)scrollBarRect.extent.height / framebufferExtent.height;
    cmd.filledMesh.mesh = drawInfo.GetQuadMesh();
    if (scrollBarState == ScrollBarState::Normal)
      cmd.filledMesh.color = { 0.5f, 0.5f, 0.5f, 1.f };
    else if (scrollBarState == ScrollBarState::Hovered)
      cmd.filledMesh.color = { 0.75f, 0.75f, 0.75f, 1.f };
    else if (scrollBarState == ScrollBarState::Pressed)
      cmd.filledMesh.color = { 1.f, 1.f, 1.f, 1.f };
    drawInfo.drawCmds.push_back(cmd);
  }
}

#include <DEngine/Application.hpp>
void ScrollArea::Tick(
  Context& ctx, 
  Rect widgetRect,
  Rect visibleRect)
{
  if (widget || layout)
  {
    Gui::SizeHint childSizeHint{};
    if (childType == ChildType::Layout)
      childSizeHint = layout->SizeHint_Tick(ctx);
    else
      childSizeHint = widget->SizeHint_Tick(ctx);

    // Our child is larger than the scroll-area widget, so we now have a scrollbar.
    if (widgetRect.extent.height < childSizeHint.preferred.height)
    {
      if (App::ButtonEvent(App::Button::Up) == App::KeyEventType::Pressed && scrollBarPos > 0)
      {
        scrollBarPos -= Math::Min((u32)10, scrollBarPos);
      }
      if (App::ButtonEvent(App::Button::Down) == App::KeyEventType::Pressed)
      {
        u32 test = childSizeHint.preferred.height - widgetRect.extent.height - scrollBarPos;

        scrollBarPos += Math::Min((u32)10, test);
      }
    }


    Rect childRect = widgetRect;
    childRect.position.y -= scrollBarPos;
    childRect.extent = childSizeHint.preferred;
    if (childType == ChildType::Layout)
      layout->Tick(
        ctx,
        childRect,
        Rect::Intersection(childRect, visibleRect));
    else if (childType == ChildType::Widget)
      widget->Tick(
        ctx,
        childRect,
        Rect::Intersection(childRect, visibleRect));
  }
}

void ScrollArea::CursorMove(
  Test& test, 
  Rect widgetRect,
  Rect visibleRect,
  CursorMoveEvent event)
{
  if (childType == ChildType::Layout || childType == ChildType::Widget)
  {
    Gui::SizeHint childSizeHint{};
    if (childType == ChildType::Layout)
      childSizeHint = layout->SizeHint(test.GetContext());
    else
      childSizeHint = widget->SizeHint(test.GetContext());


    // Check if mouse is over
    if (scrollBarState == ScrollBarState::Pressed)
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
    else
    {
      Rect scrollBarRect = impl::GetScrollbarRect(
        *this,
        widgetRect,
        childSizeHint);

      bool cursorIsHovering = scrollBarRect.PointIsInside(event.position) && visibleRect.PointIsInside(event.position);
      if (cursorIsHovering)
          scrollBarState = ScrollBarState::Hovered;
      else
        scrollBarState = ScrollBarState::Normal;
    }

    Rect childRect = widgetRect;
    childRect.position.y -= scrollBarPos;
    childRect.extent = childSizeHint.preferred;
    if (childType == ChildType::Layout)
      layout->CursorMove(
        test,
        childRect,
        Rect::Intersection(childRect, visibleRect),
        event);
    else if (childType == ChildType::Widget)
      widget->CursorMove(
        test,
        childRect,
        Rect::Intersection(childRect, visibleRect),
        event);
  }
}

void ScrollArea::CursorClick(
  Context& ctx, 
  Rect widgetRect, 
  Rect visibleRect,
  Math::Vec2Int cursorPos, 
  CursorClickEvent event)
{
  if (childType == ChildType::Layout || childType == ChildType::Widget)
  {
    Gui::SizeHint childSizeHint{};
    if (childType == ChildType::Layout)
      childSizeHint = layout->SizeHint(ctx);
    else
      childSizeHint = widget->SizeHint(ctx);

    Rect scrollBarRect = impl::GetScrollbarRect(
      *this,
      widgetRect,
      childSizeHint);
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

    Rect childRect = widgetRect;
    childRect.position.y -= scrollBarPos;
    childRect.extent = childSizeHint.preferred;
    if (childType == ChildType::Layout)
      layout->CursorClick(
        ctx,
        childRect,
        Rect::Intersection(childRect, visibleRect),
        cursorPos,
        event);
    else if (childType == ChildType::Widget)
      widget->CursorClick(
        ctx,
        childRect,
        Rect::Intersection(childRect, visibleRect),
        cursorPos,
        event);
  }
}

void ScrollArea::TouchEvent(
  Context& ctx, 
  Rect widgetRect, 
  Rect visibleRect, 
  Gui::TouchEvent event)
{
  ParentType::TouchEvent(
    ctx,
    widgetRect,
    visibleRect,
    event);

  if (childType == ChildType::Layout || childType == ChildType::Widget)
  {
    Gui::SizeHint childSizeHint{};
    if (childType == ChildType::Layout)
      childSizeHint = layout->SizeHint(ctx);
    else
      childSizeHint = widget->SizeHint(ctx);

    Rect scrollBarRect = impl::GetScrollbarRect(
      *this,
      widgetRect,
      childSizeHint);
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

    Rect childRect = widgetRect;
    childRect.position.y -= scrollBarPos;
    childRect.extent = childSizeHint.preferred;
    if (childType == ChildType::Layout)
      layout->TouchEvent(
        ctx,
        childRect,
        Rect::Intersection(childRect, visibleRect),
        event);
    else if (childType == ChildType::Widget)
      widget->TouchEvent(
        ctx,
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
