#include <DEngine/Gui/ButtonGroup.hpp>

#include <DEngine/Gui/Context.hpp>

#include "ImplData.hpp"

#include <vector>

using namespace DEngine;
using namespace DEngine::Gui;

namespace DEngine::Gui::impl
{
	struct GetButtonSizes_ReturnT
	{
		std::vector<u32> widths = {};
		Extent outerExtent = {};
	};
	[[nodiscard]] static GetButtonSizes_ReturnT GetButtonSizes(
		Std::Span<decltype(ButtonGroup::buttons)::value_type const> buttons,
		impl::TextManager& textManager,
		u32 margin)
	{
		GetButtonSizes_ReturnT returnVal = {};

		for (auto const& btn : buttons)
		{
			DENGINE_IMPL_GUI_ASSERT(!btn.title.empty());
			auto const btnSizeHint = impl::TextManager::GetSizeHint(textManager, { btn.title.data(), btn.title.size() });

			auto const width = btnSizeHint.preferred.width + (margin * 2);
			returnVal.widths.push_back(width);

			returnVal.outerExtent.width += width;
			returnVal.outerExtent.height = Math::Max(returnVal.outerExtent.height, btnSizeHint.preferred.height);
		}
		returnVal.outerExtent.height += margin * 2;

		return returnVal;
	}

	// Returns the index of the button, if any hit.
	[[nodiscard]] static Std::Opt<uSize> CheckHitButtons(
		Math::Vec2Int widgetPos,
		Std::Span<u32 const> widths,
		u32 height,
		Math::Vec2 pointPos) noexcept
	{
		Std::Opt<uSize> returnVal;
		u32 horiOffset = 0;
		for (u32 i = 0; i < widths.Size(); i += 1)
		{
			auto const& width = widths[i];

			Rect btnRect = {};
			btnRect.position = widgetPos;
			btnRect.position.x += horiOffset;
			btnRect.extent.width = width;
			btnRect.extent.height = height;
			auto const pointerInsideBtn = btnRect.PointIsInside(pointPos);
			if (pointerInsideBtn)
			{
				returnVal = i;
				break;
			}
			horiOffset += btnRect.extent.width;
		}
		return returnVal;
	}

	constexpr u8 cursorPointerId = (u8)-1;

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

	struct PointerMove_Pointer
	{
		u8 id;
		Math::Vec2 pos;
		bool occluded;
	};
}

struct Gui::impl::BtnGroupImpl
{
public:
	[[nodiscard]] static bool PointerMove(
		ButtonGroup& widget,
	  	Context& ctx,
	  	Rect const& widgetRect,
	  	Rect const& visibleRect,
		PointerMove_Pointer const& pointer)
	{
		auto& implData = *static_cast<impl::ImplData*>(ctx.Internal_ImplData());
		impl::TextManager& textManager = implData.textManager;

		auto pointerInside = widgetRect.PointIsInside(pointer.pos) && visibleRect.PointIsInside(pointer.pos);

		if (pointer.id == cursorPointerId)
		{
			if (!pointerInside)
			{
				widget.cursorHoverIndex = Std::nullOpt;
			}
			else
			{
				auto const btnSizes = GetButtonSizes(
					{ widget.buttons.data(), widget.buttons.size() },
					textManager,
					widget.margin);

				Std::Opt<uSize> hoveredIndexOpt = CheckHitButtons(
					widgetRect.position,
					{ btnSizes.widths.data(), btnSizes.widths.size() },
					btnSizes.outerExtent.height,
					pointer.pos);

				if (hoveredIndexOpt.HasValue())
				{
					widget.cursorHoverIndex = hoveredIndexOpt.Value();
				}
			}
		}

		return pointerInside;
	}

	[[nodiscard]] static bool PointerPress(
		ButtonGroup& widget,
		Context& ctx,
		Rect const& widgetRect,
		Rect const& visibleRect,
		PointerPress_Pointer const& pointer)
	{
		auto const pointerInside = widgetRect.PointIsInside(pointer.pos) && visibleRect.PointIsInside(pointer.pos);

		DENGINE_IMPL_GUI_ASSERT(widget.activeIndex < widget.buttons.size());

		impl::ImplData& implData = *(impl::ImplData*)ctx.Internal_ImplData();
		impl::TextManager& textManager = implData.textManager;

		auto const btnSizes = GetButtonSizes(
			{ widget.buttons.data(), widget.buttons.size() },
			textManager,
			widget.margin);

		auto const hoveredIndexOpt = CheckHitButtons(
			widgetRect.position,
			{ btnSizes.widths.data(), btnSizes.widths.size() },
			btnSizes.outerExtent.height,
			pointer.pos);

		if (widget.heldPointerData.HasValue())
		{
			auto const heldPointer = widget.heldPointerData.Value();
			if (pointer.id == heldPointer.pointerId && !pointer.pressed)
			{
				widget.heldPointerData = Std::nullOpt;

				if (hoveredIndexOpt.HasValue() && hoveredIndexOpt.Value() == heldPointer.buttonIndex)
				{
					// Change the active index
					widget.activeIndex = hoveredIndexOpt.Value();
					if (widget.activeChangedCallback)
						widget.activeChangedCallback(widget, widget.activeIndex);
				}
			}
		}
		else
		{
			if (hoveredIndexOpt.HasValue() && pointer.pressed)
			{
				ButtonGroup::HeldPointerData temp = {};
				temp.buttonIndex = hoveredIndexOpt.Value();
				temp.pointerId = pointer.id;
				widget.heldPointerData = temp;
				return pointerInside;
			}
		}

		return pointerInside;
	}
};

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

	auto& textManager = implData.textManager;

	for (auto const& btn : buttons)
	{
		DENGINE_IMPL_GUI_ASSERT(!btn.title.empty());
		SizeHint btnSizeHint = impl::TextManager::GetSizeHint(textManager, { btn.title.data(), btn.title.size() });
		returnVal.preferred.width += btnSizeHint.preferred.width + (margin * 2);
		returnVal.preferred.height = Math::Max(returnVal.preferred.height, btnSizeHint.preferred.height);
	}
	returnVal.preferred.height = margin * 2;

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

	auto const buttonSizes = impl::GetButtonSizes(
		{ buttons.data(), buttons.size() },
		textManager,
		margin);
	
	u32 horiOffset = 0;
	for (uSize i = 0; i < buttons.size(); i += 1)
	{
		auto const& width = buttonSizes.widths[i];

		Rect btnRect = {};
		btnRect.position = widgetRect.position;
		btnRect.position.x += horiOffset;
		btnRect.extent.width = width;
		btnRect.extent.height = buttonSizes.outerExtent.height;

		Math::Vec4 color = inactiveColor;
		if (i == activeIndex)
			color = activeColor;
		else if (cursorHoverIndex.HasValue() && cursorHoverIndex.Value() == i)
			color = hoveredColor;
		else if (heldPointerData.HasValue() && heldPointerData.Value().buttonIndex == i)
			color = hoveredColor;

		drawInfo.PushFilledQuad(btnRect, color);

		auto textRect = btnRect;
		textRect.position.x += margin;
		textRect.position.y += margin;
		textRect.extent.width -= margin * 2;
		textRect.extent.height -= margin * 2;

		auto const& title = buttons[i].title;
		impl::TextManager::RenderText(
			textManager,
			{ title.data(), title.size() },
			{ 1.f, 1.f, 1.f, 1.f },
			textRect,
			drawInfo);

		horiOffset += btnRect.extent.width;
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
	impl::PointerMove_Pointer pointer = {};
	pointer.id = impl::cursorPointerId;
	pointer.pos = { (f32)event.position.x, (f32)event.position.y };
	pointer.occluded = occluded;

	return impl::BtnGroupImpl::PointerMove(
		*this,
		ctx,
		widgetRect,
		visibleRect,
		pointer);
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
	return impl::BtnGroupImpl::PointerPress(
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
	impl::PointerMove_Pointer pointer = {};
	pointer.id = event.id;
	pointer.pos = event.position;
	pointer.occluded = occluded;

	return impl::BtnGroupImpl::PointerMove(
		*this,
		ctx,
		widgetRect,
		visibleRect,
		pointer);
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
	return impl::BtnGroupImpl::PointerPress(
		*this,
		ctx,
		widgetRect,
		visibleRect,
		pointer);
}