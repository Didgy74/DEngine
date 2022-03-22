#include <DEngine/Gui/Dropdown.hpp>
#include <DEngine/Gui/TextManager.hpp>
#include <DEngine/Gui/Layer.hpp>

#include <DEngine/Gui/Context.hpp>
#include <DEngine/Gui/StackLayout.hpp>

#include <DEngine/Std/Containers/Vec.hpp>

#include <DEngine/Std/Utility.hpp>

using namespace DEngine;
using namespace DEngine::Gui;

namespace DEngine::Gui::impl
{
	enum class PointerType : u8 { Primary, Secondary };
	[[nodiscard]] static PointerType ToPointerType(CursorButton in) noexcept
	{
		switch (in)
		{
			case CursorButton::Primary: return PointerType::Primary;
			case CursorButton::Secondary: return PointerType::Secondary;
			default:
				DENGINE_IMPL_UNREACHABLE();
				break;
		}
		return {};
	}

	constexpr u8 cursorPointerId = (u8)-1;

	[[nodiscard]] static Extent DropdownLayer_BuildOuterExtent(
		Std::Span<decltype(Dropdown::items)::value_type const> widgetItems,
		u32 textMargin,
		TextManager& textManager,
		Rect const& safeAreaRect)
	{
		auto const textHeight = textManager.GetLineheight();
		auto const lineHeight = textHeight + (textMargin * 2);

		Extent returnValue = {};
		returnValue.height = lineHeight * widgetItems.Size();

		for (auto const& line : widgetItems)
		{
			auto const textExtent = textManager.GetOuterExtent({ line.data(), line.size() });

			returnValue.width = Math::Max(
				returnValue.width,
				textExtent.width);
		}
		// Add margin to extent.
		returnValue.width += textMargin * 2;

		return returnValue;
	}

	[[nodiscard]] static Std::Opt<uSize> DropdownLayer_CheckLineHit(
		Math::Vec2Int rectPos,
		u32 rectWidth,
		uSize lineCount,
		u32 lineHeight,
		Math::Vec2 pointerPos)
	{
		Std::Opt<uSize> lineHit;
		for (uSize i = 0; i < lineCount; i++)
		{
			Rect lineRect = {};
			lineRect.position = rectPos;
			lineRect.position.y += lineHeight * i;
			lineRect.extent.width = rectWidth;
			lineRect.extent.height = lineHeight;

			if (lineRect.PointIsInside(pointerPos))
			{
				lineHit = i;
				break;
			}
		}
		return lineHit;
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


	[[nodiscard]] static Rect Dropdown_BuildTextRect(
		Rect const& widgetRect,
		u32 textMargin) noexcept
	{
		Rect returnVal = widgetRect;
		returnVal.position.x += textMargin;
		returnVal.position.y += textMargin;
		returnVal.extent.width -= textMargin * 2;
		returnVal.extent.height -= textMargin * 2;
		return returnVal;
	}
}

Dropdown::Dropdown()
{
}

Dropdown::~Dropdown()
{
}

struct Dropdown::Impl
{
	// This data is only available when rendering.
	struct [[maybe_unused]] CustomData
	{
		explicit CustomData(RectCollection::AllocT& alloc) : glyphRects{ alloc } {}

		Extent titleTextOuterExtent = {};
		Std::Vec<Rect, RectCollection::AllocT> glyphRects;
	};

	[[nodiscard]] static bool Dropdown_PointerPress(
		Dropdown& widget,
		Context& ctx,
		WindowID windowId,
		Rect const& widgetRect,
		Rect const& visibleRect,
		impl::PointerPress_Pointer const& pointer);

	class DropdownLayer : public Layer
	{
	public:
		Dropdown* dropdownWidget = nullptr;
		[[nodiscard]] Dropdown& GetDropdownWidget() { return *dropdownWidget; }
		[[nodiscard]] Dropdown const& GetDropdownWidget() const { return *dropdownWidget; }

		Math::Vec2Int pos = {};
		u32 dropdownWidgetWidth = 0;
		struct PressedLine
		{
			u8 pointerId;
			uSize index;
		};
		Std::Opt<PressedLine> pressedLine;
		Std::Opt<uSize> hoveredByCursorIndex;

		virtual void BuildSizeHints(BuildSizeHints_Params const& params) const override;

		virtual void BuildRects(BuildRects_Params const& params) const override;

		virtual void Render(Render_Params const& params) const override;
	};

	// This data is only available when rendering.
	struct [[maybe_unused]] DropdownLayer_CustomData
	{
		explicit DropdownLayer_CustomData(RectCollection::AllocT& alloc) :
			lineGlyphRects{ alloc },
			lineGlyphRectOffsets{ alloc} {}

		// Only available when rendering.
		Std::Vec<Rect, RectCollection::AllocT> lineGlyphRects;
		Std::Vec<uSize, RectCollection::AllocT> lineGlyphRectOffsets;
	};

	[[nodiscard]] static Rect DropdownLayer_BuildOuterRect(
		DropdownLayer const& layer,
		TextManager& textManager,
		Rect const& usableRect)
	{
		auto const& dropdownWidget = *layer.dropdownWidget;

		Rect widgetRect = {};
		widgetRect.position = layer.pos;

		widgetRect.extent = impl::DropdownLayer_BuildOuterExtent(
			{ dropdownWidget.items.data(), dropdownWidget.items.size() },
			dropdownWidget.textMargin,
			textManager,
			usableRect);

		// Adjust the position of the widget.
		widgetRect.position.y = Math::Max(
			widgetRect.Top(),
			usableRect.Top());
		widgetRect.position.y = Math::Min(
			widgetRect.Top(),
			usableRect.Bottom() - (i32)widgetRect.extent.height);

		return widgetRect;
	}

	[[nodiscard]] static bool DropdownLayer_PointerMove(
		DropdownLayer& layer,
		Context& ctx,
		TextManager& textManager,
		Rect const& windowRect,
		Rect const& usableRect,
		Rect const& listRectOuter,
		impl::PointerMove_Pointer const& pointer);

	[[nodiscard]] static Layer::Press_Return DropdownLayer_PointerPress(
		DropdownLayer& widget,
		Context& ctx,
		TextManager& textManager,
		Rect const& usableRect,
		Rect const& listRectOuter,
		impl::PointerPress_Pointer const& pointer,
		bool eventConsumed);
};

namespace DEngine::Gui::impl
{
	static void CreateDropdownLayer(
		Dropdown& widget,
		Context& ctx,
		WindowID windowId,
		Rect const& widgetRect)
	{
		auto layer = Std::Box{ new Dropdown::Impl::DropdownLayer };
		layer->dropdownWidget = &widget;
		layer->pos = widgetRect.position;
		layer->pos.y += (i32)widgetRect.extent.height;
		layer->dropdownWidgetWidth = widgetRect.extent.width;

		ctx.SetFrontmostLayer(
			windowId,
			Std::Move(layer));
	}
}

bool Dropdown::Impl::Dropdown_PointerPress(
	Dropdown& widget,
	Context& ctx,
	WindowID windowId,
	Rect const& widgetRect,
	Rect const& visibleRect,
	impl::PointerPress_Pointer const& pointer)
{
	auto const pointerInside =
		widgetRect.PointIsInside(pointer.pos) &&
		visibleRect.PointIsInside(pointer.pos);
	if (pointer.pressed && !pointerInside)
		return false;

	// If our pointer-type is not primary, we don't want to do anything,
	// Just consume the event if it's inside the widget and move on.
	if (pointer.type != impl::PointerType::Primary)
		return pointerInside;

	// We can now trust pointer.type == Primary
	if (widget.heldPointerId.HasValue())
	{
		auto const heldPointerId = widget.heldPointerId.Value();
		if (!pointer.pressed)
			widget.heldPointerId = Std::nullOpt;

		if (heldPointerId == pointer.id && pointerInside && !pointer.pressed)
		{
			// Now we open the dropdown menu
			impl::CreateDropdownLayer(widget, ctx, windowId, widgetRect);
		}
	}
	else
	{
		if (pointerInside && pointer.pressed)
		{
			widget.heldPointerId = pointer.id;
		}
	}

	// We are inside the button, so we always want to consume the event.
	return pointerInside;
}

void Dropdown::Impl::DropdownLayer::BuildSizeHints(
	BuildSizeHints_Params const& params) const
{
	auto& pusher = params.pusher;
	auto& textManager = params.textManager;

	auto const& widget = GetDropdownWidget();
	int lineCount = (int)widget.items.size();

	auto lineheight = textManager.GetLineheight();

	auto pusherIt = pusher.AddEntry(*this);

	if (pusher.IncludeRendering())
	{
		DropdownLayer_CustomData customDataTemp { params.pusher.Alloc() };
		auto& customData = pusher.AttachCustomData(pusherIt, Std::Move(customDataTemp));

		// First we calculate total amount of glyphs of all the lines, and
		// the offset for each line
		customData.lineGlyphRectOffsets.Resize(lineCount);
		uSize currentOffset = 0;
		for (int i = 0; i < lineCount; i += 1)
		{
			customData.lineGlyphRectOffsets[i] = currentOffset;
			currentOffset += widget.items[i].size();
		}

		customData.lineGlyphRects.Resize(currentOffset);

		// Then populate our array with the rects.
		for (int i = 0; i < lineCount; i += 1)
		{
			auto const& line = widget.items[i];
			auto const& offset = customData.lineGlyphRectOffsets[i];
			textManager.GetOuterExtent(
				{ line.data(), line.size() },
				{},
				&customData.lineGlyphRects[offset]);
		}
	}
	else
	{

	}

	SizeHint returnValue = {};
	returnValue.minimum.height = (lineheight + widget.textMargin * 2) * lineCount;
	returnValue.minimum.width = dropdownWidgetWidth;
	returnValue.expandX = false;
	returnValue.expandY = false;
	pusher.SetSizeHint(pusherIt, returnValue);
}

void Dropdown::Impl::DropdownLayer::BuildRects(BuildRects_Params const& params) const
{
	auto& pusher = params.pusher;
	auto& textManager = params.textManager;

	auto const usableRect = Rect::Intersection(params.windowRect, params.visibleRect);
	if (usableRect.IsNothing())
		return;

	auto& mainWidget = GetDropdownWidget();

	auto pusherIt = pusher.GetEntry(*this);

	auto* customDataPtr = pusher.GetCustomData2<Impl::DropdownLayer_CustomData>(pusherIt);
	DENGINE_IMPL_GUI_ASSERT(customDataPtr);
	auto& customData = *customDataPtr;

	int lineheight = textManager.GetLineheight();
	int linecount = (int)mainWidget.items.size();

	// Figure out the rect of the list of items.
	Rect outerBoxRect = {};
	outerBoxRect.extent.width = dropdownWidgetWidth;
	outerBoxRect.extent.height = (lineheight + mainWidget.textMargin * 2) * linecount;
	// Position it and clamp it to the usable area.
	outerBoxRect.position = pos;
	outerBoxRect.position.x = Math::Min(
		outerBoxRect.position.x,
		usableRect.position.x - (i32)outerBoxRect.extent.width);
	outerBoxRect.position.x = Math::Max(
		outerBoxRect.position.x,
		usableRect.position.x);
	outerBoxRect.position.y = Math::Min(
		outerBoxRect.position.y,
		usableRect.position.y - (i32)outerBoxRect.extent.height);
	outerBoxRect.position.y = Math::Max(
		outerBoxRect.position.y,
		usableRect.position.y);

	// Position our line glyphs correctly
	if (pusher.IncludeRendering())
	{
		Math::Vec2Int linePosOffset = {};
		// Iterate over each line and place its glyphs correctly.
		// To figure out the size of each line, we take the
		// current offset compared to the previous one.
		for (int i = 1; i < linecount; i += 1)
		{
			auto startOffset = customData.lineGlyphRectOffsets[i - 1];
			auto endOffset = customData.lineGlyphRectOffsets[i];
			auto length = endOffset - startOffset;
			auto* glyphStart = &customData.lineGlyphRects[startOffset];
			for (int j = 0; j < length; j += 1)
				glyphStart[j].position += outerBoxRect.position + linePosOffset;

			linePosOffset.y += lineheight;
		}
	}
}

void Dropdown::Impl::DropdownLayer::Render(Render_Params const& params) const
{
	auto const& rectCollection = params.rectCollection;
	auto& drawInfo = params.drawInfo;

	auto const& mainWidget = GetDropdownWidget();

	auto usableRect = Rect::Intersection(params.windowRect, params.safeAreaRect);
	if (usableRect.IsNothing())
		return;

	// First find the rect for the background of the list-items
	auto rectCollEntry = rectCollection.GetEntry(*this);

	auto* customDataPtr =
		rectCollection.GetCustomData2<Impl::DropdownLayer_CustomData>(rectCollEntry);
	DENGINE_IMPL_GUI_ASSERT(customDataPtr);
	auto& customData = *customDataPtr;


}

bool Dropdown::Impl::DropdownLayer_PointerMove(
	DropdownLayer& layer,
	Context& ctx,
	TextManager& textManager,
	Rect const& windowRect,
	Rect const& usableRect,
	Rect const& listRectOuter,
	impl::PointerMove_Pointer const& pointer)
{
	auto& dropdownWidget = *layer.dropdownWidget;

	auto const textHeight = textManager.GetLineheight();

	auto const pointerInsideOuter = listRectOuter.PointIsInside(pointer.pos);

	if (pointer.id == impl::cursorPointerId)
	{
		u32 const lineHeight = textHeight + (dropdownWidget.textMargin * 2);

		// Figure out if we hit a line
		auto lineHit = impl::DropdownLayer_CheckLineHit(
			listRectOuter.position,
			listRectOuter.extent.width,
			dropdownWidget.items.size(),
			lineHeight,
			pointer.pos);

		layer.hoveredByCursorIndex = lineHit;
	}

	return pointerInsideOuter;
}

Layer::Press_Return Dropdown::Impl::DropdownLayer_PointerPress(
	DropdownLayer& widget,
	Context& ctx,
	TextManager& textManager,
	Rect const& usableRect,
	Rect const& listRectOuter,
	impl::PointerPress_Pointer const& pointer,
	bool eventConsumed)
{
	auto& dropdownWidget = *widget.dropdownWidget;
	auto const textHeight = textManager.GetLineheight();

	auto const pointerInsideOuter = listRectOuter.PointIsInside(pointer.pos);

	Layer::Press_Return returnVal = {};
	returnVal.eventConsumed = pointerInsideOuter;
	returnVal.destroyLayer = !pointerInsideOuter;

	if (pointer.type != impl::PointerType::Primary)
	{
		return returnVal;
	}
	if (!pointerInsideOuter)
	{
		return returnVal;
	}
	if (eventConsumed)
	{
		return returnVal;
	}

	auto const lineHeight = textHeight + (dropdownWidget.textMargin * 2);

	// Figure out if we hit a line
	auto lineHit = impl::DropdownLayer_CheckLineHit(
		listRectOuter.position,
		listRectOuter.extent.width,
		dropdownWidget.items.size(),
		lineHeight,
		pointer.pos);

	if (widget.pressedLine.HasValue())
	{
		auto const pressedLine = widget.pressedLine.Value();
		if (pressedLine.pointerId == pointer.id)
		{
			if (!pointer.pressed)
				widget.pressedLine = Std::nullOpt;

			if (lineHit.HasValue() &&
				lineHit.Value() == pressedLine.index &&
				!pointer.pressed)
			{
				dropdownWidget.selectedItem = pressedLine.index;

				if (dropdownWidget.selectionChangedCallback)
					dropdownWidget.selectionChangedCallback(dropdownWidget);

				returnVal.destroyLayer = true;
			}
		}
	}
	else
	{
		if (pointerInsideOuter && pointer.pressed && lineHit.HasValue())
		{
			DropdownLayer::PressedLine newPressedLine = {};
			newPressedLine.pointerId = pointer.id;
			newPressedLine.index = lineHit.Value();
			widget.pressedLine = newPressedLine;
		}
	}

	return returnVal;
}


SizeHint Dropdown::GetSizeHint2(
	Widget::GetSizeHint2_Params const& params) const
{
	auto& textManager = params.textManager;
	auto& pusher = params.pusher;

	SizeHint returnValue = {};

	auto pusherIt = pusher.AddEntry(*this);

	auto& text = items[selectedItem];

	if (pusher.IncludeRendering())
	{
		auto customData = Impl::CustomData{ pusher.Alloc() };

		customData.glyphRects.Resize(text.size());
		auto const textOuterExtent = textManager.GetOuterExtent(
			{ text.data(), text.size() },
			{},
			customData.glyphRects.Data());

		customData.titleTextOuterExtent = textOuterExtent;
		returnValue.minimum = textOuterExtent;

		pusher.AttachCustomData(pusherIt, Std::Move(customData));
	}
	else
	{
		returnValue.minimum = textManager.GetOuterExtent({ text.data(), text.size() });
	}

	returnValue.minimum.width += textMargin * 2;
	returnValue.minimum.height += textMargin * 2;

	pusher.SetSizeHint(pusherIt, returnValue);
	return returnValue;
}

void Dropdown::BuildChildRects(
	Widget::BuildChildRects_Params const& params,
	Rect const& widgetRect,
	Rect const& visibleRect) const
{
	auto& pusher = params.pusher;

	if (!pusher.IncludeRendering())
		return;

	auto* customDataPtr = pusher.GetCustomData2<Impl::CustomData>(*this);
	DENGINE_IMPL_GUI_ASSERT(customDataPtr);
	auto& customData = customDataPtr;

	auto const textExtent = customData->titleTextOuterExtent;
	Math::Vec2Int const centerOffset = {
		(i32)(widgetRect.extent.width / 2 - textExtent.width / 2),
		(i32)(widgetRect.extent.height / 2 - textExtent.height / 2) };

	auto& text = items[selectedItem];
	int const textLength = (int)text.size();
	DENGINE_IMPL_GUI_ASSERT(textLength == customData->glyphRects.Size());
	for (int i = 0; i < textLength; i += 1)
	{
		auto& rect = customData->glyphRects[i];
		rect.position += widgetRect.position;
		rect.position += centerOffset;
	}
}

void Dropdown::Render2(
	Widget::Render_Params const& params,
	Rect const& widgetRect,
	Rect const& visibleRect) const
{
	auto& rectCollection = params.rectCollection;
	auto& drawInfo = params.drawInfo;

	auto const intersection = Rect::Intersection(widgetRect, visibleRect);
	if (intersection.IsNothing())
		return;

	drawInfo.PushFilledQuad(
		widgetRect,
		boxColor);

	auto* customDataPtr = rectCollection.GetCustomData2<Impl::CustomData>(*this);
	DENGINE_IMPL_GUI_ASSERT(customDataPtr);
	auto& customData = *customDataPtr;

	auto const textIsBiggerThanExtent =
		customData.titleTextOuterExtent.width > widgetRect.extent.width ||
		customData.titleTextOuterExtent.height > widgetRect.extent.height;

	Std::Opt<DrawInfo::ScopedScissor> scissor;
	if (textIsBiggerThanExtent)
		scissor = DrawInfo::ScopedScissor(drawInfo, intersection);

	auto& text = items[selectedItem];
	drawInfo.PushText(
		{ text.data(), text.size() },
		customData.glyphRects.Data(),
		{ 1.f, 1.f, 1.f, 1.f });
}

bool Dropdown::CursorPress2(
	Widget::CursorPressParams const& params,
	Rect const& widgetRect,
	Rect const& visibleRect,
	bool consumed)
{
	auto& ctx = params.ctx;
	auto windowId = params.windowId;
	auto const& cursorPos = params.cursorPos;
	auto const& event = params.event;

	impl::PointerPress_Pointer pointer = {};
	pointer.id = impl::cursorPointerId;
	pointer.pos = { (f32)cursorPos.x, (f32)cursorPos.y };
	pointer.pressed = event.pressed;
	pointer.type = impl::ToPointerType(event.button);

	return Impl::Dropdown_PointerPress(
		*this,
		ctx,
		windowId,
		widgetRect,
		visibleRect,
		pointer);
}