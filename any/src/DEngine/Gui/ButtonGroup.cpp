#include <DEngine/Gui/ButtonGroup.hpp>
#include <DEngine/Gui/TextManager.hpp>
#include <DEngine/Gui/DrawInfo.hpp>

#include <DEngine/Math/Common.hpp>

#include <DEngine/Std/Containers/Vec.hpp>

using namespace DEngine;
using namespace DEngine::Gui;

namespace DEngine::Gui::impl
{
	// Returns the widths of each child
	[[nodiscard]] static auto GetButtonWidths(
		Std::Span<u32 const> desiredButtonWidths,
		u32 widgetWidth,
		Std::FrameAlloc& transientAlloc)
	{
		auto const count = desiredButtonWidths.Size();

		u32 sumDesiredWidth = 0;
		for (auto const& desiredWidth : desiredButtonWidths)
			sumDesiredWidth += desiredWidth;

		f32 const scaleFactor = (f32)widgetWidth / (f32)sumDesiredWidth;

		auto returnWidths = Std::MakeVec<u32>(transientAlloc);
		returnWidths.Resize(count);

		u32 remainingWidth = widgetWidth;
		for (int i = 0; i < count; i += 1)
		{
			u32 btnWidth;
			// If we're not the last element, use the scale factor instead.
			if (count >= 0 && i != count - 1)
				btnWidth = (u32)Math::Round((f32)desiredButtonWidths[i] * scaleFactor);
			else
				btnWidth = remainingWidth;

			returnWidths[i] = btnWidth;
			remainingWidth -= btnWidth;
		}

		return returnWidths;
	}

	[[nodiscard]] Std::Span<u32 const> GetDesiredButtonWidths(
		ButtonGroup const& widget,
		RectCollection const& rectCollection)
	{
		auto const btnWidthsByteSpan = rectCollection.GetCustomData(widget);
		DENGINE_IMPL_GUI_ASSERT((uSize)btnWidthsByteSpan.Data() % alignof(u32) == 0);
		DENGINE_IMPL_GUI_ASSERT((btnWidthsByteSpan.Size() % sizeof(u32)) == 0);
		Std::Span<u32 const> const buttonDesiredWidths = {
			reinterpret_cast<u32 const*>(btnWidthsByteSpan.Data()),
			btnWidthsByteSpan.Size() / sizeof(u32) };

		DENGINE_IMPL_GUI_ASSERT(buttonDesiredWidths.Size() == widget.buttons.size());

		return buttonDesiredWidths;
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
		bool consumed;
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
	struct PointerMove_Params
	{
		ButtonGroup& widget;
		Rect const& widgetRect;
		Rect const& visibleRect;
		RectCollection const& rectCollection;
		Std::FrameAlloc& transientAlloc;
		PointerMove_Pointer const& pointer;
	};
	[[nodiscard]] static bool PointerMove(
		PointerMove_Params const& params)
	{
		auto& widget = params.widget;
		auto& widgetRect = params.widgetRect;
		auto& visibleRect = params.visibleRect;
		auto& pointer = params.pointer;
		auto& rectCollection = params.rectCollection;
		auto& transientAlloc = params.transientAlloc;

	  	auto const pointerInside = widgetRect.PointIsInside(pointer.pos) && visibleRect.PointIsInside(pointer.pos);

		if (pointer.id == cursorPointerId)
		{
			if (!pointerInside || pointer.occluded)
			{
				widget.cursorHoverIndex = Std::nullOpt;
			}
			else
			{
				auto const desiredButtonWidths = impl::GetDesiredButtonWidths(widget, rectCollection);
				auto const buttonWidths = impl::GetButtonWidths(
					desiredButtonWidths,
					widgetRect.extent.width,
					transientAlloc);

				auto const hoveredIndexOpt = impl::CheckHitButtons(
					widgetRect.position,
					buttonWidths.ToSpan(),
					widgetRect.extent.height,
					pointer.pos);

				if (hoveredIndexOpt.HasValue())
				{
					widget.cursorHoverIndex = hoveredIndexOpt.Value();
				}
			}
		}

		return pointerInside;
	}

	struct PointerPress_Params
	{
		ButtonGroup& widget;
		Rect const& widgetRect;
		Rect const& visibleRect;
		RectCollection const& rectCollection;
		Std::FrameAlloc& transientAlloc;
		PointerPress_Pointer const& pointer;
	};
	[[nodiscard]] static bool PointerPress(
		PointerPress_Params const& params)
	{
		auto& widget = params.widget;
		auto& widgetRect = params.widgetRect;
		auto& visibleRect = params.visibleRect;
		auto& rectCollection = params.rectCollection;
		auto& transientAlloc = params.transientAlloc;
		auto const& pointer = params.pointer;

		DENGINE_IMPL_GUI_ASSERT(widget.activeIndex < widget.buttons.size());

		auto const pointerInside = widgetRect.PointIsInside(pointer.pos) && visibleRect.PointIsInside(pointer.pos);

		auto const desiredButtonWidths = impl::GetDesiredButtonWidths(widget, rectCollection);
		auto const buttonWidths = impl::GetButtonWidths(
			desiredButtonWidths,
			widgetRect.extent.width,
			transientAlloc);

		auto const hoveredIndexOpt = impl::CheckHitButtons(
			widgetRect.position,
			buttonWidths.ToSpan(),
			widgetRect.extent.height,
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
			if (hoveredIndexOpt.HasValue() && pointer.pressed && !pointer.consumed)
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

bool ButtonGroup::TouchMoveEvent(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	Gui::TouchMoveEvent event,
	bool occluded)
{
	/*
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
	 */

	return {};
}

bool ButtonGroup::TouchPressEvent(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	Gui::TouchPressEvent event)
{
	/*
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
	*/
	return false;
}

SizeHint ButtonGroup::GetSizeHint2(Widget::GetSizeHint2_Params const& params) const
{
	SizeHint returnVal = {};

	auto& pusher = params.pusher;

	auto pusherIt = pusher.AddEntry(*this);

	auto customData = pusher.AttachCustomData(pusherIt, buttons.size() * sizeof(u32));

	Std::Span<u32> btnDesiredWidths = {
		reinterpret_cast<u32*>(customData.Data()),
		buttons.size() };

	for (int i = 0; i < btnDesiredWidths.Size(); i += 1)
	{
		auto const& btn = buttons[i];
		DENGINE_IMPL_GUI_ASSERT(!btn.title.empty());

		auto const textOuterExtent = params.textManager.GetOuterExtent({ btn.title.data(), btn.title.size() });
		auto const btnWidth = textOuterExtent.width + (margin * 2);

		btnDesiredWidths[i] = btnWidth;

		returnVal.minimum.width += btnWidth;
		returnVal.minimum.height = Math::Max(returnVal.minimum.height, textOuterExtent.height);
	}
	returnVal.minimum.height = margin * 2;

	pusher.SetSizeHint(pusherIt, returnVal);

	return returnVal;
}

void ButtonGroup::Render2(
	Widget::Render_Params const& params,
	Rect const& widgetRect,
	Rect const& visibleRect) const
{
	DENGINE_IMPL_GUI_ASSERT(activeIndex < buttons.size());

	auto& ctx = params.ctx;
	auto& drawInfo = params.drawInfo;
	auto& textManager = params.textManager;

	auto const buttonDesiredWidths = impl::GetDesiredButtonWidths(*this, params.rectCollection);
	auto const buttonWidths = impl::GetButtonWidths(
		buttonDesiredWidths,
		widgetRect.extent.width,
		params.transientAlloc);

	u32 horiOffset = 0;
	for (uSize i = 0; i < buttons.size(); i += 1)
	{
		auto const& width = buttonWidths[i];

		Rect btnRect = {};
		btnRect.position = widgetRect.position;
		btnRect.position.x += horiOffset;
		btnRect.extent.width = width;
		btnRect.extent.height = widgetRect.extent.height;

		Math::Vec4 color = colors.inactiveColor;

		if (cursorHoverIndex.HasValue() && cursorHoverIndex.Value() == i)
			color = colors.hoveredColor;
		if (heldPointerData.HasValue() && heldPointerData.Value().buttonIndex == i)
			color = colors.hoveredColor;
		if (i == activeIndex)
			color = colors.activeColor;


		drawInfo.PushFilledQuad(btnRect, color);

		auto textRect = btnRect;
		textRect.position.x += margin;
		textRect.position.y += margin;
		textRect.extent.width -= margin * 2;
		textRect.extent.height -= margin * 2;

		auto const& title = buttons[i].title;

		textManager.RenderText(
			{ title.data(), title.size() },
			{ 1.f, 1.f, 1.f, 1.f },
			textRect,
			drawInfo);

		horiOffset += btnRect.extent.width;
	}
}

bool ButtonGroup::CursorMove(
	Widget::CursorMoveParams const& params,
	Rect const& widgetRect,
	Rect const& visibleRect,
	bool occluded)
{
	impl::PointerMove_Pointer pointer = {};
	pointer.id = impl::cursorPointerId;
	pointer.pos = { (f32)params.event.position.x, (f32)params.event.position.y };
	pointer.occluded = occluded;

	impl::BtnGroupImpl::PointerMove_Params tempParams {
		.widget = *this,
		.widgetRect = widgetRect,
		.visibleRect = visibleRect,
		.rectCollection = params.rectCollection,
		.transientAlloc = params.transientAlloc,
		.pointer = pointer };
	return impl::BtnGroupImpl::PointerMove(tempParams);
}

bool ButtonGroup::CursorPress2(
	Widget::CursorPressParams const& params,
	Rect const& widgetRect,
	Rect const& visibleRect,
	bool consumed)
{
	impl::PointerPress_Pointer pointer = {};
	pointer.id = impl::cursorPointerId;
	pointer.pos = { (f32)params.cursorPos.x, (f32)params.cursorPos.y };
	pointer.type = impl::ToPointerType(params.event.button);
	pointer.pressed = params.event.pressed;
	pointer.consumed = consumed;

	impl::BtnGroupImpl::PointerPress_Params tempParams {
		.widget = *this,
		.widgetRect = widgetRect,
		.visibleRect = visibleRect,
		.rectCollection = params.rectCollection,
		.transientAlloc = params.transientAlloc,
		.pointer = pointer };
	return impl::BtnGroupImpl::PointerPress(tempParams);
}

void ButtonGroup::CursorExit(Context& ctx)
{
	cursorHoverIndex = Std::nullOpt;
}
