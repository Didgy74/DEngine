#include <DEngine/Gui/ButtonGroup.hpp>

#include <DEngine/Gui/Context.hpp>

#include <DEngine/Math/Common.hpp>

#include "ImplData.hpp"

using namespace DEngine;
using namespace DEngine::Gui;

void ButtonGroup::AddButton(std::string const& title)
{
	InternalButton newInternal = {};
	newInternal.title = title;
	buttons.push_back(newInternal);
}

u32 ButtonGroup::GetButtonCount() const
{
	return (u32)buttons.size();
}

u32 ButtonGroup::GetActiveButtonIndex() const
{
	return activeIndex;
}

SizeHint ButtonGroup::GetSizeHint(Context const& ctx) const
{
	impl::ImplData& implData = *(impl::ImplData*)ctx.Internal_ImplData();
	
	SizeHint returnVal = {};

	impl::TextManager& textManager = implData.textManager;

	for (auto const& btn : buttons)
	{
		DENGINE_IMPL_GUI_ASSERT(!btn.title.empty());
		SizeHint btnSizeHint = impl::TextManager::GetSizeHint(textManager, { btn.title.data(), btn.title.size() });
		returnVal.preferred.width += btnSizeHint.preferred.width;
		returnVal.preferred.height = Math::Max(returnVal.preferred.height, btnSizeHint.preferred.height);
	}

	return returnVal;
}

void ButtonGroup::Render(
	Context const& ctx, 
	Extent framebufferExtent, 
	Rect widgetRect, 
	Rect visibleRect, 
	DrawInfo& drawInfo) const
{
	DENGINE_IMPL_GUI_ASSERT(activeIndex < buttons.size());

	impl::ImplData& implData = *(impl::ImplData*)ctx.Internal_ImplData();
	impl::TextManager& textManager = implData.textManager;

	std::vector<SizeHint> sizeHints;
	sizeHints.reserve(buttons.size());
	u32 maxHeight = 0;
	for (auto const& btn : buttons)
	{
		DENGINE_IMPL_GUI_ASSERT(!btn.title.empty());
		SizeHint btnSizeHint = impl::TextManager::GetSizeHint(textManager, { btn.title.data(), btn.title.size() });
		sizeHints.push_back(btnSizeHint);
		maxHeight = Math::Max(maxHeight, btnSizeHint.preferred.height);
	}
	
	u32 horiOffset = 0;
	for (u32 i = 0; i < sizeHints.size(); i += 1)
	{
		SizeHint const& sizeHint = sizeHints[i];
		Rect btnRect = {};
		btnRect.position = widgetRect.position;
		btnRect.position.x += horiOffset;
		btnRect.extent.width = sizeHint.preferred.width;
		btnRect.extent.height = maxHeight;

		Math::Vec4 color = { 0.25f, 0.25f, 0.25f, 1.f };
		if (i == activeIndex)
			color = { 0.5f, 0.5f, 0.5f, 1.f };
		drawInfo.PushFilledQuad(btnRect, color);

		auto const& title = buttons[i].title;
		impl::TextManager::RenderText(
			textManager,
			{ title.data(), title.size() },
			{ 1.f, 1.f, 1.f, 1.f },
			btnRect,
			drawInfo);

		horiOffset += btnRect.extent.width;
	}
}

namespace DEngine::Gui::impl
{
	constexpr u8 ButtonGroup_cursorPointerId = (u8)-1;
	enum class ButtonGroup_PointerType : u8
	{
		Primary,
		Secondary,
	};
	[[nodiscard]] static constexpr ButtonGroup_PointerType ButtonGroup_ToPointerType(CursorButton in) noexcept
	{
		switch (in)
		{
			case CursorButton::Primary:
				return ButtonGroup_PointerType::Primary;
			case CursorButton::Secondary:
				return ButtonGroup_PointerType::Secondary;
			default:
				break;
		}
		DENGINE_IMPL_UNREACHABLE();
		return {};
	}
	void ButtonGroup_PointerPress(
		ButtonGroup& widget,
		Context& ctx,
		WindowID windowId,
		Rect widgetRect,
		Rect visibleRect,
		u8 pointerId,
		ButtonGroup_PointerType pointerType,
		Math::Vec2 pointerPos,
		bool pressed)
	{
		DENGINE_IMPL_GUI_ASSERT(widget.activeIndex < widget.buttons.size());

		impl::ImplData& implData = *(impl::ImplData*)ctx.Internal_ImplData();
		impl::TextManager& textManager = implData.textManager;

		std::vector<SizeHint> sizeHints;
		sizeHints.reserve(widget.buttons.size());
		u32 maxHeight = 0;
		for (auto const& btn : widget.buttons)
		{
			DENGINE_IMPL_GUI_ASSERT(!btn.title.empty());
			SizeHint btnSizeHint = impl::TextManager::GetSizeHint(textManager, { btn.title.data(), btn.title.size() });
			sizeHints.push_back(btnSizeHint);
			maxHeight = Math::Max(maxHeight, btnSizeHint.preferred.height);
		}

		u32 horiOffset = 0;
		for (u32 i = 0; i < sizeHints.size(); i += 1)
		{
			SizeHint const& sizeHint = sizeHints[i];
			Rect btnRect = {};
			btnRect.position = widgetRect.position;
			btnRect.position.x += horiOffset;
			btnRect.extent.width = sizeHint.preferred.width;
			btnRect.extent.height = maxHeight;

			bool a =
				pressed &&
				i != widget.activeIndex &&
				btnRect.PointIsInside(pointerPos) &&
				!widget.heldPointerId.HasValue();
			if (a)
			{
				ButtonGroup::HeldPointerData temp = {};
				temp.buttonIndex = i;
				temp.pointerId = pointerId;
				widget.heldPointerId = temp;
			}

			bool b =
				!pressed &&
				i != widget.activeIndex &&
				btnRect.PointIsInside(pointerPos) &&
				widget.heldPointerId.HasValue();
			if (b)
			{
				auto heldPointer = widget.heldPointerId.Value();
				if (heldPointer.buttonIndex == i && heldPointer.pointerId == pointerId)
				{
					widget.heldPointerId = Std::nullOpt;

					// Change the active index
					widget.activeIndex = i;
					if (widget.activeChangedCallback)
						widget.activeChangedCallback(widget, widget.activeIndex);
				}
			}

			horiOffset += btnRect.extent.width;
		}
	}
}

bool ButtonGroup::CursorMove(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	CursorMoveEvent event,
	bool occluded)
{
	return false;
}

bool ButtonGroup::CursorPress(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	Math::Vec2Int cursorPos,
	CursorClickEvent event)
{
	impl::ButtonGroup_PointerPress(
		*this,
		ctx,
		windowId,
		widgetRect,
		visibleRect,
		impl::ButtonGroup_cursorPointerId,
		impl::ButtonGroup_ToPointerType(event.button),
		{ (f32)cursorPos.x, (f32)cursorPos.y },
		event.clicked);

	return false;
}

void ButtonGroup::TouchEvent(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	Gui::TouchEvent event,
	bool occluded)
{
	if (event.type == Gui::TouchEventType::Down || event.type == Gui::TouchEventType::Up)
	{
		bool isDown = event.type == Gui::TouchEventType::Down;
		impl::ButtonGroup_PointerPress(
			*this,
			ctx,
			windowId,
			widgetRect,
			visibleRect,
			event.id,
			impl::ButtonGroup_PointerType::Primary,
			event.position,
			isDown);
	}
}