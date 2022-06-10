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

	/*
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
	*/

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

		auto const pointerInside = widgetRect.PointIsInside(pointer.pos) && visibleRect.PointIsInside(pointer.pos);

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

		// REMEMBER TO CENTER THE TEXT
		int glyphRectOffset = 0;
		int btnRectOffsetX = 0;
		for (int i = 0; i < btnCount; i += 1)
		{
			Extent textExtent = { customData.totalTextWidths[i], customData.textHeight };
			Extent btnExtent = { btnWidths[i], widgetRect.extent.height };
			Math::Vec2Int centeringOffset = {
				(i32)Math::Round( (f32)btnExtent.width * 0.5f - (f32)textExtent.width * 0.5f ),
				(i32)Math::Round( (f32)btnExtent.height * 0.5f - (f32)textExtent.height * 0.5f ), };

			Math::Vec2Int textOffset = widgetRect.position;
			textOffset.x += btnRectOffsetX;
			textOffset += centeringOffset;

			auto const& btn = buttons[i];
			uSize const textLength = btn.title.size();
			Std::Span lineGlyphRects = { &customData.glyphRects[glyphRectOffset], textLength };
			for (auto& glyph : lineGlyphRects)
				glyph.position += textOffset;

			btnRectOffsetX += btnExtent.width;
			glyphRectOffset += textLength;
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
	for (uSize i = 0; i < btnCount; i += 1)
	{
		auto const& width = btnWidths[i];

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

		horiOffset += btnRect.extent.width;
	}

	// Our text was positioned in the previous step.
	// So we can draw all the text in one call, but we need to
	// combine all the button titles into one long one.
	auto combinedText = Std::MakeVec<char>(transientAlloc);
	combinedText.Resize(customData.glyphRects.Size());
	int combinedTextOffset = 0;
	for (int i = 0; i < btnCount; i += 1)
	{
		auto const& btn = buttons[i];
		int textLength = (int)btn.title.size();
		for (int j = 0; j < textLength; j += 1)
			combinedText[j + combinedTextOffset] = btn.title[j];
		combinedTextOffset += textLength;
	}

	drawInfo.PushText(
		combinedText.ToSpan(),
		customData.glyphRects.Data(),
		{ 1.f, 1.f, 1.f, 1.f });
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
