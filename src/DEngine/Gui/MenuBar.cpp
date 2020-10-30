#include <DEngine/Gui/MenuBar.hpp>

#include <DEngine/Gui/Context.hpp>

#include "ImplData.hpp"

#include <DEngine/Std/Utility.hpp>

namespace DEngine::Gui::impl
{
	static StackLayout::Direction toStackLayoutDir(MenuBar::Direction dir)
	{
		if (dir == MenuBar::Direction::Horizontal)
			return StackLayout::Direction::Horizontal;
		else
			return StackLayout::Direction::Vertical;
	}
}

using namespace DEngine;
using namespace DEngine::Gui;

MenuBar::MenuBar(MenuBar* parentMenuBar, Direction dir) :
	parentMenuBar{ parentMenuBar },
	stackLayout{ impl::toStackLayoutDir(dir) }
{
}

void MenuBar::ClearActiveButton(
	Context& ctx,
	WindowID windowId)
{
	DENGINE_IMPL_GUI_ASSERT(activeButton);

	activeButton->Clear(ctx, windowId);

	activeButton = nullptr;
}

SizeHint MenuBar::GetSizeHint(Context const& ctx) const
{
	return stackLayout.GetSizeHint(ctx);
}

void MenuBar::Render(
	Context const& ctx,
	Extent framebufferExtent, 
	Rect widgetRect, 
	Rect visibleRect,
	DrawInfo& drawInfo) const
{
	stackLayout.Render(
		ctx,
		framebufferExtent,
		widgetRect,
		visibleRect,
		drawInfo);
}

void MenuBar::CharEnterEvent(Context& ctx)
{
	stackLayout.CharEnterEvent(ctx);
}

void MenuBar::CharEvent(Context& ctx, u32 utfValue)
{
	stackLayout.CharEvent(ctx, utfValue);
}

void MenuBar::CharRemoveEvent(Context& ctx)
{
	stackLayout.CharRemoveEvent(ctx);
}

void MenuBar::CursorMove(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	CursorMoveEvent event)
{
	stackLayout.CursorMove(
		ctx,
		windowId,
		widgetRect,
		visibleRect,
		event);
}

void MenuBar::CursorClick(
	Context& ctx, 
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	Math::Vec2Int cursorPos, 
	CursorClickEvent event)
{
	impl::ImplData& implData = *(impl::ImplData*)ctx.Internal_ImplData();

	bool cursorIsInside = widgetRect.PointIsInside(cursorPos) && visibleRect.PointIsInside(cursorPos);

	// If cursor is outside and we have an active button, we destroy any open menus.
	// And make button unactive
	if (!cursorIsInside && event.clicked && event.button == CursorButton::Left && activeButton)
	{
		ClearActiveButton(ctx, windowId);
	}

	stackLayout.CursorClick(
		ctx,
		windowId,
		widgetRect,
		visibleRect,
		cursorPos,
		event);
}

void MenuBar::TouchEvent(
	Context& ctx, 
	WindowID windowId, 
	Rect widgetRect, 
	Rect visibleRect, 
	Gui::TouchEvent event)
{
	if (event.id == 0 && event.type == TouchEventType::Down)
	{
		bool cursorIsInside = widgetRect.PointIsInside(event.position) && visibleRect.PointIsInside(event.position);

		// If cursor is outside and we have an active button, we destroy any open menus.
		// And make button unactive
		if (!cursorIsInside && activeButton)
		{
			ClearActiveButton(ctx, windowId);
		}
	}


	stackLayout.TouchEvent(
		ctx,
		windowId,
		widgetRect,
		visibleRect,
		event);
}

MenuBar::Button::Button(MenuBar* parentMenuBar, std::string_view title) :
	parentMenuBar{ parentMenuBar },
	title{ title }
{

}

void MenuBar::Button::Clear(
	Context& ctx, 
	WindowID windowId)
{
	impl::ImplData& implData = *(impl::ImplData*)ctx.Internal_ImplData();
	
	if (this->menuPtr)
	{
		MenuBar* ptr = dynamic_cast<MenuBar*>(this->menuPtr);
		if (ptr && ptr->activeButton)
		{
			ptr->ClearActiveButton(ctx, windowId);
		}
	}
	ctx.Test_DestroyMenu(windowId, this->menuPtr);

	this->active = false;
	this->menuPtr = nullptr;
}

SizeHint MenuBar::Button::GetSizeHint(
	Context const& ctx) const
{
	impl::ImplData& implData = *(impl::ImplData*)ctx.Internal_ImplData();

	return impl::TextManager::GetSizeHint(
		implData.textManager,
		title);
}

void MenuBar::Button::Render(
	Context const& ctx,
	Extent framebufferExtent,
	Rect widgetRect,
	Rect visibleRect,
	DrawInfo& drawInfo) const
{
	impl::ImplData& implData = *(impl::ImplData*)ctx.Internal_ImplData();
	
	if (active)
	{
		drawInfo.PushFilledQuad(widgetRect, { 0.25f, 0.f, 0.25f, 1.f });
	}

	impl::TextManager::RenderText(
		implData.textManager,
		title,
		{ 1.f, 1.f, 1.f, 1.f },
		widgetRect,
		drawInfo);
}

void MenuBar::Button::CursorMove(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	CursorMoveEvent event)
{
	return;

	impl::ImplData& implData = *(impl::ImplData*)ctx.Internal_ImplData();

	// First check if any other button is active in parent MenuBar
	// Then check if we are inside this button
	// If we are inside this button, close old menu, open new one.

	bool cursorIsInside = widgetRect.PointIsInside(event.position) && visibleRect.PointIsInside(event.position);

	if (cursorIsInside && parentMenuBar->activeButton && parentMenuBar->activeButton != this)
	{
		parentMenuBar->ClearActiveButton(ctx, windowId);

		active = true;
		parentMenuBar->activeButton = this;
		activateCallback(
			ctx,
			windowId,
			*this,
			widgetRect);
	}
}

void MenuBar::Button::CursorClick(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	Math::Vec2Int cursorPos,
	CursorClickEvent event)
{
	DENGINE_IMPL_GUI_ASSERT(parentMenuBar);

	impl::ImplData& implData = *(impl::ImplData*)ctx.Internal_ImplData();

	bool cursorIsInside = widgetRect.PointIsInside(cursorPos) && visibleRect.PointIsInside(cursorPos);
	if (!active && cursorIsInside && event.clicked && event.button == CursorButton::Left)
	{
		if (parentMenuBar->activeButton)
		{
			parentMenuBar->ClearActiveButton(ctx, windowId);
		}
		
		active = true;
		parentMenuBar->activeButton = this;
		activateCallback(
			ctx,
			windowId,
			*this,
			widgetRect);
	}
	else if (active && cursorIsInside && event.clicked && event.button == CursorButton::Left)
	{
		parentMenuBar->ClearActiveButton(ctx, windowId);
	}
	else if (active && !cursorIsInside && event.clicked && event.button == CursorButton::Left)
	{

	}
}

void MenuBar::Button::TouchEvent(
	Context& ctx, 
	WindowID windowId, 
	Rect widgetRect,
	Rect visibleRect, 
	Gui::TouchEvent event)
{
	if (event.type == TouchEventType::Down && event.id == 0)
	{
		bool cursorIsInside = widgetRect.PointIsInside(event.position) && visibleRect.PointIsInside(event.position);
		if (!active && cursorIsInside)
		{
			if (parentMenuBar->activeButton)
			{
				parentMenuBar->ClearActiveButton(ctx, windowId);
			}

			active = true;
			parentMenuBar->activeButton = this;
			activateCallback(
				ctx,
				windowId,
				*this,
				widgetRect);
		}
	}
}

MenuBar::ActivatableButton::ActivatableButton(MenuBar* parentMenuBar, std::string_view title) :
	title{ title },
	parentMenuBar{ parentMenuBar }
{

}

SizeHint MenuBar::ActivatableButton::GetSizeHint(
	Context const& ctx) const
{
	impl::ImplData& implData = *(impl::ImplData*)ctx.Internal_ImplData();

	return impl::TextManager::GetSizeHint(
		implData.textManager,
		title);
}

void MenuBar::ActivatableButton::Render(
	Context const& ctx, 
	Extent framebufferExtent,
	Rect widgetRect, 
	Rect visibleRect,
	DrawInfo& drawInfo) const
{
	impl::ImplData& implData = *(impl::ImplData*)ctx.Internal_ImplData();

	impl::TextManager::RenderText(
		implData.textManager,
		title,
		{ 1.f, 1.f, 1.f, 1.f },
		widgetRect,
		drawInfo);
}

void MenuBar::ActivatableButton::CursorClick(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect, 
	Rect visibleRect, 
	Math::Vec2Int cursorPos, 
	CursorClickEvent event)
{
	impl::ImplData& implData = *(impl::ImplData*)ctx.Internal_ImplData();

	bool cursorIsInside = widgetRect.PointIsInside(cursorPos) && visibleRect.PointIsInside(cursorPos);

	// First check if we clicked this button, then invoke callback and remove the menu?
	if (cursorIsInside && event.clicked && event.button == CursorButton::Left)
	{
		activateCallback();

		//ctx.Test_DestroyMenu(windowId, parentMenuBar);
		MenuBar* temp = parentMenuBar;
		while (temp)
		{
			if (temp->activeButton)
				temp->ClearActiveButton(ctx, windowId);
			temp = temp->parentMenuBar;
		}
	}
}

void MenuBar::ActivatableButton::TouchEvent(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	Gui::TouchEvent event)
{
	if (event.type == TouchEventType::Down && event.id == 0)
	{
		bool cursorIsInside = widgetRect.PointIsInside(event.position) && visibleRect.PointIsInside(event.position);
		if (cursorIsInside)
		{
			activateCallback();

			//ctx.Test_DestroyMenu(windowId, parentMenuBar);
			MenuBar* temp = parentMenuBar;
			while (temp)
			{
				if (temp->activeButton)
					temp->ClearActiveButton(ctx, windowId);
				temp = temp->parentMenuBar;
			}
		}
	}
}
