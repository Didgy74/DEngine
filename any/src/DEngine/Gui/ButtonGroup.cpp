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
	constexpr u8 cursorPointerId = (u8)-1;

	[[nodiscard]] static bool PointerMove(
		Rect const& widgetRect,
		Rect const& visibleRect,
		Math::Vec2 pointerPos) noexcept
	{
		bool pointerInside = widgetRect.PointIsInside(pointerPos) && visibleRect.PointIsInside(pointerPos);
		return pointerInside;
	}

	enum class PointerType : u8
	{
		Primary,
		Secondary,
	};
	[[nodiscard]] static constexpr PointerType ToPointerType(CursorButton in) noexcept
	{
		switch (in)
		{
			case CursorButton::Primary:
				return PointerType::Primary;
			case CursorButton::Secondary:
				return PointerType::Secondary;
			default:
				break;
		}
		DENGINE_IMPL_UNREACHABLE();
		return {};
	}
	struct PointerPress_Pointer
	{
		u8 id;
		PointerType type;
		Math::Vec2 pos;
		bool pressed;
	};
	[[nodiscard]] static bool PointerPress(
		ButtonGroup& widget,
		Context& ctx,
		Rect const& widgetRect,
		Rect const& visibleRect,
		PointerPress_Pointer const& pointer) noexcept
	{
		bool pointerInside = widgetRect.PointIsInside(pointer.pos) && visibleRect.PointIsInside(pointer.pos);
		if (!pointerInside && pointer.pressed)
			return false;

		if (widget.heldPointerData.HasValue() && pointer.pressed)
			return pointerInside;

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

		Std::Opt<uSize> hoveredIndexOpt;
		u32 horiOffset = 0;
		for (u32 i = 0; i < sizeHints.size(); i += 1)
		{
			SizeHint const& sizeHint = sizeHints[i];
			Rect btnRect = {};
			btnRect.position = widgetRect.position;
			btnRect.position.x += horiOffset;
			btnRect.extent.width = sizeHint.preferred.width;
			btnRect.extent.height = maxHeight;
			bool pointerInsideBtn = btnRect.PointIsInside(pointer.pos) && visibleRect.PointIsInside(pointer.pos);
			if (pointerInsideBtn)
			{
				hoveredIndexOpt = i;
				break;
			}
			horiOffset += btnRect.extent.width;
		}

		if (hoveredIndexOpt.HasValue())
		{
			auto hoveredIndex = hoveredIndexOpt.Value();

			if (widget.heldPointerData.HasValue())
			{
				auto const held = widget.heldPointerData.Value();
				if (!pointer.pressed && held.pointerId == pointer.id && held.buttonIndex == hoveredIndex)
				{
					widget.heldPointerData = Std::nullOpt;
					// Change the active index
					widget.activeIndex = hoveredIndex;
					if (widget.activeChangedCallback)
						widget.activeChangedCallback(widget, widget.activeIndex);
					return pointerInside;
				}
			}
			else
			{
				if (pointer.pressed && hoveredIndex != widget.activeIndex)
				{
					ButtonGroup::HeldPointerData temp = {};
					temp.buttonIndex = hoveredIndex;
					temp.pointerId = pointer.id;
					widget.heldPointerData = temp;
					return pointerInside;
				}
			}
		}
		return pointerInside;
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
	return impl::PointerMove(
		widgetRect,
		visibleRect,
		{ (f32)event.position.x, (f32)event.position.y });
}

bool ButtonGroup::CursorPress(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	Math::Vec2Int cursorPos,
	CursorClickEvent event)
{
	impl::PointerPress_Pointer pointer = {};
	pointer.id = impl::cursorPointerId;
	pointer.pos = { (f32)cursorPos.x, (f32)cursorPos.y };
	pointer.pressed = event.clicked;
	pointer.type = impl::ToPointerType(event.button);
	return impl::PointerPress(
		*this,
		ctx,
		widgetRect,
		visibleRect,
		pointer);
}

bool ButtonGroup::TouchMoveEvent(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	Gui::TouchMoveEvent event,
	bool occluded)
{
	return impl::PointerMove(
		widgetRect,
		visibleRect,
		event.position);
}

bool ButtonGroup::TouchPressEvent(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	Gui::TouchPressEvent event)
{
	impl::PointerPress_Pointer pointer = {};
	pointer.id = event.id;
	pointer.pos = event.position;
	pointer.pressed = event.pressed;
	pointer.type = impl::PointerType::Primary;
	return impl::PointerPress(
		*this,
		ctx,
		widgetRect,
		visibleRect,
		pointer);
}