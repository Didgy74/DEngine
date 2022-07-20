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
		AllocRef const& transientAlloc)
	{
		auto const count = desiredButtonWidths.Size();

		u32 sumDesiredWidth = 0;
		for (auto const& desiredWidth : desiredButtonWidths)
			sumDesiredWidth += desiredWidth;

		f32 const scaleFactor = (f32)widgetWidth / (f32)sumDesiredWidth;

		auto returnWidths = Std::NewVec<u32>(transientAlloc);
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

	[[nodiscard]] static auto GetButtonRects(
		Std::Span<u32 const> btnWidths,
		Math::Vec2Int widgetPos,
		u32 widgetHeight,
		AllocRef const& transientAlloc)
	{
		int btnCount = (int)btnWidths.Size();

		auto returnRects = Std::NewVec<u32>(transientAlloc);
		returnRects.Resize(btnCount);

		i32 rectPosOffsetX = 0;
		for (int i = 0; i < btnCount; i++) {
			auto const& btnWidth = btnWidths[i];

			rectPosOffsetX += btnWidth;
		}

		return returnRects;
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


	[[nodiscard]] static Math::Vec2Int BuildTextOffset(Extent const& widgetExtent, Extent const& textExtent) noexcept {
		Math::Vec2Int out = {};
		for (int i = 0; i < 2; i++) {
			auto temp = (i32)Math::Round((f32)widgetExtent[i] * 0.5f - (f32)textExtent[i] * 0.5f);
			temp = Math::Max(0, temp);
			out[i] = temp;
		}
		return out;
	}
}

struct Gui::ButtonGroup::Impl
{
	// This data is only available when rendering.
	struct [[maybe_unused]] CustomData
	{
		explicit CustomData(RectCollection::AllocRefT const& alloc) :
			desiredBtnWidths{ alloc },
			totalTextWidths{ alloc },
			glyphRects{ alloc } {}

		Std::Vec<u32, RectCollection::AllocRefT> desiredBtnWidths;

		// Only available
		u32 textHeight;
		Std::Vec<u32, RectCollection::AllocRefT> totalTextWidths;
		Std::Vec<Rect, RectCollection::AllocRefT> glyphRects;
	};

	static constexpr u8 cursorPointerId = (u8)-1;

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

	struct PointerMove_Params
	{
		ButtonGroup& widget;
		Rect const& widgetRect;
		Rect const& visibleRect;
		RectCollection const& rectCollection;
		AllocRef const& transientAlloc;
		PointerMove_Pointer const& pointer;
	};
	[[nodiscard]] static bool PointerMove(
		PointerMove_Params const& params)
	{
		auto& widget = params.widget;
		auto& widgetRect = params.widgetRect;
		auto& visibleRect = params.visibleRect;
		auto& pointer = params.pointer;
		auto& rectColl = params.rectCollection;
		auto& transientAlloc = params.transientAlloc;

		auto const* customDataPtr = rectColl.GetCustomData2<Impl::CustomData>(widget);
		DENGINE_IMPL_GUI_ASSERT(customDataPtr);
		auto const& customData = *customDataPtr;
		auto const& desiredBtnWidths = customData.desiredBtnWidths;

		auto const pointerInside = widgetRect.PointIsInside(pointer.pos) && visibleRect.PointIsInside(pointer.pos);

		if (pointer.id == cursorPointerId)
		{
			if (!pointerInside || pointer.occluded)
			{
				widget.cursorHoverIndex = Std::nullOpt;
			}
			else
			{
				auto const buttonWidths = impl::GetButtonWidths(
					desiredBtnWidths.ToSpan(),
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
		 AllocRef const& transientAlloc;
		 PointerPress_Pointer const& pointer;
	 };
	[[nodiscard]] static bool PointerPress(
		PointerPress_Params const& params)
	{
		auto& widget = params.widget;
		auto& widgetRect = params.widgetRect;
		auto& visibleRect = params.visibleRect;
		auto& rectColl = params.rectCollection;
		auto& transientAlloc = params.transientAlloc;
		auto const& pointer = params.pointer;

		DENGINE_IMPL_GUI_ASSERT(widget.activeIndex < widget.buttons.size());

		auto const* customDataPtr = rectColl.GetCustomData2<Impl::CustomData>(widget);
		DENGINE_IMPL_GUI_ASSERT(customDataPtr);
		auto const& customData = *customDataPtr;
		auto const& desiredBtnWidths = customData.desiredBtnWidths;

		auto const pointerInside = PointIsInAll(pointer.pos, { widgetRect, visibleRect });

		auto const buttonWidths = impl::GetButtonWidths(
			desiredBtnWidths.ToSpan(),
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

SizeHint ButtonGroup::GetSizeHint2(Widget::GetSizeHint2_Params const& params) const
{
	SizeHint returnVal = {};

	auto& pusher = params.pusher;
	auto& textManager = params.textManager;

	auto entry = pusher.AddEntry(*this);

	auto const btnCount = buttons.size();

	// Count the total amount of text-glyphs
	uSize totalGlyphCount = 0;
	for (auto const& item : buttons)
		totalGlyphCount += item.title.size();

	auto& customData = pusher.AttachCustomData(entry, Impl::CustomData{ pusher.Alloc() });
	customData.desiredBtnWidths.Resize(btnCount);
	if (pusher.IncludeRendering())
	{
		customData.textHeight = textManager.GetLineheight();
		customData.glyphRects.Resize(totalGlyphCount);
		customData.totalTextWidths.Resize(btnCount);
	}


	// Tracks the offset where we will push each
	// line of glyph rects into
	int glyphRectsOffset = 0;

	for (int i = 0; i < btnCount; i += 1)
	{
		auto const& btn = buttons[i];
		DENGINE_IMPL_GUI_ASSERT(!btn.title.empty());

		auto const textLength = btn.title.size();

		// Load the outer extent of this button text
		// And store the rects of each glyph
		Extent textOuterExtent = {};
		if (pusher.IncludeRendering())
		{
			textOuterExtent = textManager.GetOuterExtent(
				{ btn.title.data(), textLength },
				{},
				customData.glyphRects.Data() + glyphRectsOffset);
			glyphRectsOffset += (int)textLength;
			customData.totalTextWidths[i] = textOuterExtent.width;
		}
		else
			textOuterExtent = textManager.GetOuterExtent({ btn.title.data(), textLength });

		// Store the outer extent in our custom-data for later use.
		auto const btnWidth = textOuterExtent.width + (margin * 2);
		customData.desiredBtnWidths[i] = btnWidth;

		returnVal.minimum.width += btnWidth;
		returnVal.minimum.height = Math::Max(returnVal.minimum.height, textOuterExtent.height);
	}
	returnVal.minimum.height = margin * 2;

	pusher.SetSizeHint(entry, returnVal);
	return returnVal;
}

void ButtonGroup::BuildChildRects(
	Widget::BuildChildRects_Params const& params,
	Rect const& widgetRect,
	Rect const& visibleRect) const
{
	auto& pusher = params.pusher;
	auto& transientAlloc = params.transientAlloc;

	auto* customDataPtr = pusher.GetCustomData2<Impl::CustomData>(*this);
	DENGINE_IMPL_GUI_ASSERT(customDataPtr);
	auto& customData = *customDataPtr;

	// Offset the glyphs we stored into correct positions.
	if (pusher.IncludeRendering())
	{
		auto const& desiredBtnWidths = customData.desiredBtnWidths;
		auto const btnWidths = impl::GetButtonWidths(
			desiredBtnWidths.ToSpan(),
			widgetRect.extent.width,
			transientAlloc);

		auto const btnCount = buttons.size();

		int glyphRectIndexOffset = 0;
		int btnRectPosOffsetX = 0;
		for (int i = 0; i < btnCount; i += 1)
		{
			Extent textExtent = { customData.totalTextWidths[i], customData.textHeight };
			Extent btnExtent = { btnWidths[i], widgetRect.extent.height };
			auto textPos = widgetRect.position + impl::BuildTextOffset(btnExtent, textExtent);
			textPos.x += btnRectPosOffsetX;

			auto const& btn = buttons[i];
			auto textLength = btn.title.size();
			Std::Span lineGlyphRects = { &customData.glyphRects[glyphRectIndexOffset], textLength };
			for (auto& glyph : lineGlyphRects)
				glyph.position += textPos;

			btnRectPosOffsetX += btnExtent.width;
			glyphRectIndexOffset += textLength;
		}

	}
}

void ButtonGroup::Render2(
	Widget::Render_Params const& params,
	Rect const& widgetRect,
	Rect const& visibleRect) const
{
	DENGINE_IMPL_GUI_ASSERT(activeIndex < buttons.size());

	auto& drawInfo = params.drawInfo;
	auto const& rectColl = params.rectCollection;
	auto& transientAlloc = params.transientAlloc;

	auto const btnCount = buttons.size();

	auto entryOpt = rectColl.GetEntry(*this);
	DENGINE_IMPL_GUI_ASSERT(entryOpt.HasValue());
	auto entry = entryOpt.Value();
	auto const* customDataPtr = rectColl.GetCustomData2<Impl::CustomData>(entry);
	DENGINE_IMPL_GUI_ASSERT(customDataPtr);
	auto const& customData = *customDataPtr;

	auto const& desiredBtnWidths = customData.desiredBtnWidths;

	auto const btnWidths = impl::GetButtonWidths(
		desiredBtnWidths.ToSpan(),
		widgetRect.extent.width,
		transientAlloc);

	u32 horiOffset = 0;
	int glyphRectIndexOffset = 0;
	for (uSize i = 0; i < btnCount; i += 1)
	{
		auto const& width = btnWidths[i];

		Rect btnRect = {};
		btnRect.position = widgetRect.position;
		btnRect.position.x += horiOffset;
		btnRect.extent = { width, widgetRect.extent.height };

		Math::Vec4 color = colors.inactiveColor;

		if (cursorHoverIndex.HasValue() && cursorHoverIndex.Value() == i)
			color = colors.hoveredColor;
		if (heldPointerData.HasValue() && heldPointerData.Value().buttonIndex == i)
			color = colors.hoveredColor;
		if (i == activeIndex)
			color = colors.activeColor;

		drawInfo.PushFilledQuad(btnRect, color);

		auto const& btnText = buttons[i].title;
		auto textLen = buttons[i].title.size();
		Std::Span btnGlyphRects = { &customData.glyphRects[glyphRectIndexOffset], textLen };
		drawInfo.PushText(
			{ btnText.data(), btnText.size() },
			btnGlyphRects.Data(),
			{ 1.f, 1.f, 1.f, 1.f });

		horiOffset += btnRect.extent.width;
		glyphRectIndexOffset += textLen;
	}
}

bool ButtonGroup::CursorMove(
	Widget::CursorMoveParams const& params,
	Rect const& widgetRect,
	Rect const& visibleRect,
	bool occluded)
{
	Impl::PointerMove_Pointer pointer = {};
	pointer.id = Impl::cursorPointerId;
	pointer.pos = { (f32)params.event.position.x, (f32)params.event.position.y };
	pointer.occluded = occluded;

	Impl::PointerMove_Params tempParams {
		.widget = *this,
		.widgetRect = widgetRect,
		.visibleRect = visibleRect,
		.rectCollection = params.rectCollection,
		.transientAlloc = params.transientAlloc,
		.pointer = pointer, };
	return Impl::PointerMove(tempParams);
}

bool ButtonGroup::CursorPress2(
	Widget::CursorPressParams const& params,
	Rect const& widgetRect,
	Rect const& visibleRect,
	bool consumed)
{
	Impl::PointerPress_Pointer pointer = {};
	pointer.id = Impl::cursorPointerId;
	pointer.pos = { (f32)params.cursorPos.x, (f32)params.cursorPos.y };
	pointer.type = Impl::ToPointerType(params.event.button);
	pointer.pressed = params.event.pressed;
	pointer.consumed = consumed;

	Impl::PointerPress_Params tempParams {
		.widget = *this,
		.widgetRect = widgetRect,
		.visibleRect = visibleRect,
		.rectCollection = params.rectCollection,
		.transientAlloc = params.transientAlloc,
		.pointer = pointer, };
	return Impl::PointerPress(tempParams);
}

void ButtonGroup::CursorExit(Context& ctx)
{
	cursorHoverIndex = Std::nullOpt;
}

bool ButtonGroup::TouchMove2(
	TouchMoveParams const& params,
	Rect const& widgetRect,
	Rect const& visibleRect,
	bool occluded)
{
	Impl::PointerMove_Pointer pointer = {};
	pointer.id = params.event.id;
	pointer.pos = params.event.position;
	pointer.occluded = occluded;

	Impl::PointerMove_Params tempParams {
		.widget = *this,
		.widgetRect = widgetRect,
		.visibleRect = visibleRect,
		.rectCollection = params.rectCollection,
		.transientAlloc = params.transientAlloc,
		.pointer = pointer, };
	return Impl::PointerMove(tempParams);
}

bool ButtonGroup::TouchPress2(
	TouchPressParams const& params,
	Rect const& widgetRect,
	Rect const& visibleRect,
	bool consumed)
{
	Impl::PointerPress_Pointer pointer = {};
	pointer.id = params.event.id;
	pointer.pos = params.event.position;
	pointer.type = Impl::PointerType::Primary;
	pointer.pressed = params.event.pressed;
	pointer.consumed = consumed;

	Impl::PointerPress_Params tempParams {
		.widget = *this,
		.widgetRect = widgetRect,
		.visibleRect = visibleRect,
		.rectCollection = params.rectCollection,
		.transientAlloc = params.transientAlloc,
		.pointer = pointer, };
	return Impl::PointerPress(tempParams);
}