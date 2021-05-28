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

MenuBar::MenuBar(Direction dir) : 
	parentMenuBar{ nullptr },
	stackLayout{ impl::toStackLayoutDir(dir) }
{
}

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

void MenuBar::AddSubmenuButton(
	Std::Str title,
	std::function<SubmenuActivateCallback> callback)
{
	auto newButton = new MenuBar::Button(this, title, callback);
	stackLayout.AddWidget(Std::Box{ newButton });
}

void MenuBar::AddMenuButton(
	Std::Str title,
	std::function<ButtonActivateCallback> callback)
{
	auto newButton = new MenuBar::ActivatableButton(this, title, callback);
	stackLayout.AddWidget(Std::Box{ newButton });
}

void MenuBar::AddToggleMenuButton(
	Std::Str title,
	bool toggled, 
	std::function<ToggleButtonActivateCallback> callback)
{
	auto newButton = new MenuBar::ToggleButton(
		this,
		title,
		toggled,
		callback);

	stackLayout.AddWidget(Std::Box<Widget>{ newButton });
}

MenuBar::Direction MenuBar::GetDirection() const
{
	if (stackLayout.direction == StackLayout::Direction::Horizontal)
		return Direction::Horizontal;
	else
		return Direction::Vertical;
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

bool MenuBar::CursorMove(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	CursorMoveEvent event,
	bool occluded)
{
	return stackLayout.CursorMove(
		ctx,
		windowId,
		widgetRect,
		visibleRect,
		event,
		occluded);
}

bool MenuBar::CursorPress(
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
	if (!cursorIsInside && event.clicked && event.button == CursorButton::Primary && activeButton)
	{
		ClearActiveButton(ctx, windowId);
	}

	return stackLayout.CursorPress(
		ctx,
		windowId,
		widgetRect,
		visibleRect,
		cursorPos,
		event);
}

/*
void MenuBar::TouchEvent(
	Context& ctx, 
	WindowID windowId, 
	Rect widgetRect, 
	Rect visibleRect, 
	Gui::TouchEvent event,
	bool occluded)
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
		event,
		occluded);
}
*/

bool MenuBar::TouchMoveEvent(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	Gui::TouchMoveEvent event,
	bool occluded)
{
	return false;
}

bool MenuBar::TouchPressEvent(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	Gui::TouchPressEvent event)
{
	return false;
}

MenuBar::Button::Button(
	MenuBar* parentMenuBar, 
	Std::Str title,
	std::function<SubmenuActivateCallback> callback) :
	parentMenuBar{ parentMenuBar },
	title{ std::string(title.Data(), title.Size()) },
	activateCallback{ callback }
{

}

void MenuBar::Button::Clear(
	Context& ctx, 
	WindowID windowId)
{
	impl::ImplData& implData = *(impl::ImplData*)ctx.Internal_ImplData();
	
	if (this->menuPtr)
	{
		auto ptr = dynamic_cast<MenuBar*>(this->menuPtr);
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
		{ title.data(), title.size() });
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
		{ title.data(), title.size() },
		{ 1.f, 1.f, 1.f, 1.f },
		widgetRect,
		drawInfo);
}

bool MenuBar::Button::CursorMove(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	CursorMoveEvent event,
	bool occluded)
{
	return false;
}

bool MenuBar::Button::CursorPress(
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
	if (!active && cursorIsInside && event.clicked && event.button == CursorButton::Primary)
	{
		if (parentMenuBar->activeButton)
		{
			parentMenuBar->ClearActiveButton(ctx, windowId);
		}
		
		active = true;
		parentMenuBar->activeButton = this;
		Gui::MenuBar* newMenuBar = new MenuBar(parentMenuBar, Direction::Vertical);
		this->menuPtr = newMenuBar;
		Math::Vec2Int menuPos{};
		if (parentMenuBar->GetDirection() == Direction::Horizontal)
		{
			menuPos.x = widgetRect.position.x;
			menuPos.y = widgetRect.position.y + (i32)widgetRect.extent.height;
		}
		else
		{
			menuPos.x = widgetRect.position.x + (i32)widgetRect.extent.width;
			menuPos.y = widgetRect.position.y;
		}
		ctx.Test_AddMenu(
			windowId,
			Std::Box<Widget>{ newMenuBar },
			{ menuPos, {} });

		activateCallback(
			*newMenuBar);

	}
	else if (active && cursorIsInside && event.clicked && event.button == CursorButton::Primary)
	{
		parentMenuBar->ClearActiveButton(ctx, windowId);
	}
	else if (active && !cursorIsInside && event.clicked && event.button == CursorButton::Primary)
	{

	}

	return false;
}

bool MenuBar::Button::TouchMoveEvent(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	Gui::TouchMoveEvent event,
	bool occluded)
{
	return false;
}

bool MenuBar::Button::TouchPressEvent(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	Gui::TouchPressEvent event)
{
	return false;
}

MenuBar::ActivatableButton::ActivatableButton(
	MenuBar* parentMenuBar, 
	Std::Str title,
	std::function<ButtonActivateCallback> callback) :
	title{ std::string(title.Data(), title.Size()) },
	parentMenuBar{ parentMenuBar },
	activateCallback{ callback }
{

}

SizeHint MenuBar::ActivatableButton::GetSizeHint(
	Context const& ctx) const
{
	impl::ImplData& implData = *(impl::ImplData*)ctx.Internal_ImplData();

	return impl::TextManager::GetSizeHint(
		implData.textManager,
		{ title.data(), title.size() });
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
		{ title.data(), title.size() },
		{ 1.f, 1.f, 1.f, 1.f },
		widgetRect,
		drawInfo);
}

bool MenuBar::ActivatableButton::CursorPress(
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
	if (cursorIsInside && event.clicked && event.button == CursorButton::Primary)
	{
		activateCallback();

		MenuBar* temp = parentMenuBar;
		while (temp)
		{
			if (temp->activeButton)
				temp->ClearActiveButton(ctx, windowId);
			temp = temp->parentMenuBar;
		}
	}

	return false;
}

/*
void MenuBar::ActivatableButton::TouchEvent(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	Gui::TouchEvent event,
	bool occluded)
{
	if (event.type == TouchEventType::Down && event.id == 0)
	{
		bool cursorIsInside = widgetRect.PointIsInside(event.position) && visibleRect.PointIsInside(event.position);
		if (cursorIsInside)
		{
			activateCallback();

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
*/

bool MenuBar::ActivatableButton::TouchMoveEvent(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	Gui::TouchMoveEvent event,
	bool occluded)
{
	return false;
}

bool MenuBar::ActivatableButton::TouchPressEvent(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	Gui::TouchPressEvent event)
{
	return false;
}

MenuBar::ToggleButton::ToggleButton(
	MenuBar* parentMenuBar,
	Std::Str title,
	bool toggled,
	std::function<ToggleButtonActivateCallback> callback) :
	parentMenuBar{ parentMenuBar },
	title{ std::string(title.Data(), title.Size()) },
	toggled{ toggled },
	activateCallback{ callback }
{

}

SizeHint MenuBar::ToggleButton::GetSizeHint(
	Context const& ctx) const
{
	impl::ImplData& implData = *(impl::ImplData*)ctx.Internal_ImplData();

	return impl::TextManager::GetSizeHint(
		implData.textManager,
		{ title.data(), title.size() });
}

void MenuBar::ToggleButton::Render(
	Context const& ctx, 
	Extent framebufferExtent,
	Rect widgetRect, 
	Rect visibleRect, 
	DrawInfo& drawInfo) const
{
	impl::ImplData& implData = *(impl::ImplData*)ctx.Internal_ImplData();

	if (toggled)
	{
		drawInfo.PushFilledQuad(widgetRect, { 1.f, 1.f, 1.f, 0.25f });
	}

	impl::TextManager::RenderText(
		implData.textManager,
		{ title.data(), title.size() },
		{ 1.f, 1.f, 1.f, 1.f },
		widgetRect,
		drawInfo);
}

bool MenuBar::ToggleButton::CursorPress(
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
	if (cursorIsInside && event.clicked && event.button == CursorButton::Primary)
	{
		toggled = !toggled;
		
		activateCallback(toggled);

		//ctx.Test_DestroyMenu(windowId, parentMenuBar);
		MenuBar* temp = parentMenuBar;
		while (temp)
		{
			if (temp->activeButton)
				temp->ClearActiveButton(ctx, windowId);
			temp = temp->parentMenuBar;
		}
	}

	return false;
}

/*
void MenuBar::ToggleButton::TouchEvent(
	Context& ctx, 
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	Gui::TouchEvent event,
	bool occluded)
{
	if (event.type == TouchEventType::Down && event.id == 0)
	{
		bool cursorIsInside = widgetRect.PointIsInside(event.position) && visibleRect.PointIsInside(event.position);
		if (cursorIsInside)
		{
			toggled = !toggled;
			activateCallback(toggled);

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
*/

bool MenuBar::ToggleButton::TouchMoveEvent(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	Gui::TouchMoveEvent event,
	bool occluded)
{
	return false;
}

bool MenuBar::ToggleButton::TouchPressEvent(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	Gui::TouchPressEvent event)
{
	return false;
}


