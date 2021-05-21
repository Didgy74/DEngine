#include <DEngine/Gui/Dropdown.hpp>

#include <DEngine/Gui/Button.hpp>
#include <DEngine/Gui/Context.hpp>
#include <DEngine/Gui/StackLayout.hpp>

#include "ImplData.hpp"

#include <DEngine/Math/Common.hpp>

using namespace DEngine;
using namespace DEngine::Gui;

Dropdown::Dropdown()
{
}

Dropdown::~Dropdown()
{
	if (menuWidget)
	{
		ClearDropdownMenu();
	}
}

SizeHint Dropdown::GetSizeHint(Context const& ctx) const
{
	impl::ImplData& implData = *static_cast<impl::ImplData*>(ctx.Internal_ImplData());

	u32 lineHeight = implData.textManager.lineheight;

	SizeHint returnVal{};
	returnVal.preferred.height = lineHeight;
	returnVal.preferred.width = 25;

	for (auto& item : items)
	{
		SizeHint childSizeHint = impl::TextManager::GetSizeHint(
			implData.textManager,
			{ item.data(), item.size() });
		returnVal.preferred.width = Math::Max(returnVal.preferred.width, childSizeHint.preferred.width);
	}
	
	return returnVal;
}

void Dropdown::Render(
	Context const& ctx, 
	Extent framebufferExtent, 
	Rect widgetRect,
	Rect visibleRect, 
	DrawInfo& drawInfo) const
{
	impl::ImplData& implData = *static_cast<impl::ImplData*>(ctx.Internal_ImplData());

	drawInfo.PushFilledQuad(
		widgetRect,
		{ 0.f, 0.f, 0.f, 0.25f });
	auto const& string = items[selectedItem];
	
	impl::TextManager::RenderText(
		implData.textManager,
		{ string.data(), string.size() },
		{ 1.f, 1.f, 1.f, 1.f },
		widgetRect,
		drawInfo);
}

bool Dropdown::CursorPress(
	Context& ctx, 
	WindowID windowId, 
	Rect widgetRect, 
	Rect visibleRect, 
	Math::Vec2Int cursorPos,
	CursorClickEvent event)
{
	bool isInside = widgetRect.PointIsInside(cursorPos) && visibleRect.PointIsInside(cursorPos);

	if (menuWidget != nullptr && !event.clicked)
	{
		ClearDropdownMenu();
	}
	else if (isInside && !event.clicked && event.button == CursorButton::Primary && menuWidget == nullptr)
	{
		Math::Vec2Int position = widgetRect.position;
		position.y += widgetRect.extent.height;

		CreateDropdownMenu(
			ctx,
			windowId,
			position);
	}
	return false;
}

void Dropdown::TouchEvent(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect, 
	Rect visibleRect,
	Gui::TouchEvent event,
	bool occluded)
{
	if (event.id == 0)
	{
		bool isInside = widgetRect.PointIsInside(event.position) && visibleRect.PointIsInside(event.position);
		if (menuWidget != nullptr && event.type == TouchEventType::Up)
		{
			ClearDropdownMenu();
		}
		else if (isInside && event.type == TouchEventType::Up && menuWidget == nullptr)
		{
			Math::Vec2Int position = widgetRect.position;
			position.y += widgetRect.extent.height;

			CreateDropdownMenu(
				ctx,
				windowId,
				position);
		}
	}
}

void Dropdown::CreateDropdownMenu(
	Context& ctx, 
	WindowID windowId, 
	Math::Vec2Int menuPosition)
{
	StackLayout* layout = new StackLayout(StackLayout::Direction::Vertical);
	layout->color = { 0.25f, 0.25f, 0.25f, 1.f };

	menuWidgetCtx = &ctx;
	menuWidgetWindowId = windowId;
	menuWidget = layout;

	for (u32 i = 0; i < items.size(); i += 1)
	{
		auto const& itemText = items[i];
		Button* button = new Button;
		layout->AddWidget(Std::Box<Widget>{ button });
		button->text = itemText;

		if (i == selectedItem)
		{
			// Highlight this one
			button->normalColor = button->hoverColor;
		}

		button->activateFn = [this, i](
			Button& btn)
		{
			u32 previousSelectedItem = selectedItem;
			selectedItem = i;
			ClearDropdownMenu();
			if (i != previousSelectedItem)
			{
				// Call the callback
				if (selectedItemChangedCallback)
					selectedItemChangedCallback(*this);
			}
		};
	}

	ctx.Test_AddMenu(
		windowId,
		Std::Box<Widget>{ layout },
		{ menuPosition, {} });
}

void Dropdown::ClearDropdownMenu()
{
	DENGINE_DETAIL_ASSERT(menuWidget);
	menuWidgetCtx->Test_DestroyMenu(
		menuWidgetWindowId,
		menuWidget);
	menuWidgetCtx = nullptr;
	menuWidgetWindowId = {};
	menuWidget = nullptr;
}
