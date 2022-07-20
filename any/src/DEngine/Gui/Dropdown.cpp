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
		for (int i = 0; i < lineCount; i++)
		{
			Rect lineRect = {};
			lineRect.position = rectPos;
			lineRect.position.y += (i32)lineHeight * i;
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
		explicit CustomData(RectCollection::AllocRefT const& alloc) : glyphRects{ alloc } {}

		Extent titleTextOuterExtent = {};
		Std::Vec<Rect, RectCollection::AllocRefT> glyphRects;
	};

	struct Dropdown_PointerPress_Params
	{
		Dropdown& widget;
		Context& ctx;
		WindowID windowId;
		Rect const& widgetRect;
		Rect const& visibleRect;
		RectCollection const& rectCollection;
		impl::PointerPress_Pointer const& pointer;
		bool eventConsumed;
	};
	[[nodiscard]] static bool Dropdown_PointerPress(
		Dropdown_PointerPress_Params const& params);

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

		virtual bool CursorMove(
			CursorMoveParams const& params,
			bool occluded) override;
		virtual Press_Return CursorPress(
			CursorPressParams const& params,
			bool eventConsumed) override;

		virtual Press_Return TouchPress2(
			TouchPressParams const& params,
			bool eventConsumed) override;
		virtual bool TouchMove2(
			TouchMoveParams const& params,
			bool occluded) override;
	};

	// This data is only available when rendering.
	struct [[maybe_unused]] DropdownLayer_CustomData
	{
		explicit DropdownLayer_CustomData(RectCollection::AllocRefT const& alloc) :
			lineGlyphRects{ alloc },
			lineGlyphRectOffsets{ alloc} {}

		int textheight = 0;
		// Only available when rendering.
		Std::Vec<Rect, RectCollection::AllocRefT> lineGlyphRects;
		Std::Vec<uSize, RectCollection::AllocRefT> lineGlyphRectOffsets;
	};

	struct DropdownLayer_PointerMove_Params
	{
		DropdownLayer& layer;
		Context& ctx;
		RectCollection const& rectCollection;
		Rect const& windowRect;
		Rect const& usableRect;
		impl::PointerMove_Pointer const& pointer;
	};
	[[nodiscard]] static bool DropdownLayer_PointerMove(
		DropdownLayer_PointerMove_Params const& params);


	struct DropdownLayer_PointerPress_Params
	{
		DropdownLayer& layer;
		Context& ctx;
		RectCollection const& rectCollection;
		Rect const& windowRect;
		Rect const& usableRect;
		impl::PointerPress_Pointer const& pointer;
		bool eventConsumed;
	};
	[[nodiscard]] static Layer::Press_Return DropdownLayer_PointerPress(
		DropdownLayer_PointerPress_Params const& params);
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
	Dropdown_PointerPress_Params const& params)
{
	auto& widget = params.widget;
	auto& rectColl = params.rectCollection;
	auto& ctx = params.ctx;
	auto const& widgetRect = params.widgetRect;
	auto const& visibleRect = params.visibleRect;
	auto const& windowId = params.windowId;
	auto const& pointer = params.pointer;
	auto const& oldEventConsumed = params.eventConsumed;

	auto outerRect = Rect::Intersection(widgetRect, visibleRect);

	auto const pointerInside = outerRect.PointIsInside(pointer.pos);

	auto newEventConsumed = oldEventConsumed || pointerInside;

	// If our pointer-type is not primary, we don't want to do anything,
	// Just consume the event if it's inside the widget and move on.
	if (pointer.type != impl::PointerType::Primary)
		return newEventConsumed;

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
			newEventConsumed = true;
		}
	}
	else
	{
		if (!oldEventConsumed && pointerInside && pointer.pressed)
		{
			widget.heldPointerId = pointer.id;
			newEventConsumed = true;
		}
	}

	// We are inside the button, so we always want to consume the event.
	return newEventConsumed;
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

	DropdownLayer_CustomData customDataTemp { pusher.Alloc() };
	auto& customData = pusher.AttachCustomData(pusherIt, Std::Move(customDataTemp));

	customData.textheight = (int)lineheight;

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

	int textheight = customData.textheight;
	int lineheight = textheight + mainWidget.textMargin * 2;
	int linecount = (int)mainWidget.items.size();

	// Figure out the rect of the list of items.
	Rect outerBoxRect = {};
	outerBoxRect.extent.width = dropdownWidgetWidth;
	outerBoxRect.extent.height = lineheight * linecount;
	// Position it and clamp it to the usable area.
	outerBoxRect.position = pos;
	outerBoxRect.position.x = Math::Min(
		outerBoxRect.position.x,
		usableRect.Right() - (i32)outerBoxRect.extent.width);
	outerBoxRect.position.x = Math::Max(
		outerBoxRect.position.x,
		usableRect.position.x);
	outerBoxRect.position.y = Math::Min(
		outerBoxRect.position.y,
		usableRect.Bottom() - (i32)outerBoxRect.extent.height);
	outerBoxRect.position.y = Math::Max(
		outerBoxRect.position.y,
		usableRect.position.y);

	pusher.SetRectPair(pusherIt, { outerBoxRect, usableRect });

	// Position our line glyphs correctly
	if (pusher.IncludeRendering())
	{
		// First we calculate total amount of glyphs of all the lines, and
		// the index for each line
		customData.lineGlyphRectOffsets.Resize(linecount);
		int currentOffset = 0;
		for (int i = 0; i < linecount; i++)
		{
			customData.lineGlyphRectOffsets[i] = currentOffset;
			currentOffset += mainWidget.items[i].size();
		}

		// Then populate our list with the rects
		auto const totalStrLength = currentOffset;
		customData.lineGlyphRects.Resize(totalStrLength);
		i32 linePosOffsetY = {};
		for (int i = 0; i < linecount; i++)
		{
			// We can't know the outer extent of the text until we have gathered the rects.
			// We need to know the outer extent to center it.
			// We gather the rects first then offset them accordingly.
			auto const& line = mainWidget.items[i];
			auto const linelength = line.size();
			auto const& offset = customData.lineGlyphRectOffsets[i];

			auto textExtent = textManager.GetOuterExtent(
				{ line.data(), linelength },
				{},
				&customData.lineGlyphRects[offset]);

			Rect lineRect = outerBoxRect;
			lineRect.extent.height = lineheight;
			lineRect.position.y += (i32)lineRect.extent.height * i;

			Math::Vec2Int textPos = lineRect.position;
			textPos.x += (i32)(lineRect.extent.width / 2 - textExtent.width / 2);
			textPos.y += (i32)(lineRect.extent.height / 2 - textExtent.height / 2);
			auto lineGlyphs = Std::Span{ &customData.lineGlyphRects[offset], linelength };
			for (auto& glyph : lineGlyphs)
				glyph.position += textPos;

			linePosOffsetY += textheight;
			linePosOffsetY += (i32)mainWidget.textMargin * 2;
		}
	}
}

void Dropdown::Impl::DropdownLayer::Render(Render_Params const& params) const
{
	auto const& rectColl = params.rectCollection;
	auto& transientAlloc = params.transientAlloc;
	auto& drawInfo = params.drawInfo;
	auto const& windowRect = params.windowRect;
	auto const& safeAreaRect = params.safeAreaRect;

	auto const& mainWidget = GetDropdownWidget();

	auto usableRect = Rect::Intersection(windowRect, safeAreaRect);
	if (usableRect.IsNothing())
		return;

	// First find the rect for the background of the list-items
	auto rectCollEntryOpt = rectColl.GetEntry(*this);
	DENGINE_IMPL_GUI_ASSERT(rectCollEntryOpt.HasValue());
	auto const& rectCollEntry = rectCollEntryOpt.Value();
	auto const* customDataPtr = rectColl.GetCustomData2<Impl::DropdownLayer_CustomData>(rectCollEntry);
	DENGINE_IMPL_GUI_ASSERT(customDataPtr);
	auto& customData = *customDataPtr;

	auto lineheight = customData.textheight;

	// First we want to draw the background rect
	auto const& rectPair = rectColl.GetRect(rectCollEntry);
	drawInfo.PushFilledQuad(rectPair.widgetRect, { 0.5f, 0.5f, 0.5f, 1.f });

	auto scissorGuard = DrawInfo::ScopedScissor{ drawInfo, rectPair.widgetRect };

	// Then we display the hovered item if there is one.
	if (hoveredByCursorIndex.HasValue())
	{
		auto const& hoveredIndex = hoveredByCursorIndex.Value();
		auto lineRect = rectPair.widgetRect;
		lineRect.extent.height = lineheight + mainWidget.textMargin * 2;
		lineRect.position.y += (i32)lineRect.extent.height * (i32)hoveredIndex;
		drawInfo.PushFilledQuad(lineRect, { 1.f, 1.f, 1.f, 0.5f });
	}

	// Append all text into a single list.
	int allTextCount = 0;
	for (auto const& item : mainWidget.items)
		allTextCount += (int)item.length();
	auto allText = Std::NewVec<char>(transientAlloc);
	allText.Resize(allTextCount);
	int offset = 0;
	for (auto const& item : mainWidget.items)
	{
		int strLength = (int)item.length();
		for (int i = 0; i < strLength; i += 1)
			allText[i + offset] = item[i];
		offset += strLength;
	}

	// Next draw each line
	drawInfo.PushText(
		{ allText.Data(), allText.Size() },
		customData.lineGlyphRects.Data(),
		{ 1.f, 1.f, 1.f, 1.f });
}

bool Dropdown::Impl::DropdownLayer::CursorMove(
	Layer::CursorMoveParams const& params,
	bool occluded)
{
	impl::PointerMove_Pointer pointer = {};
	pointer.id = impl::cursorPointerId;
	pointer.pos = { (f32)params.event.position.x, (f32)params.event.position.y };
	pointer.occluded = occluded;

	DropdownLayer_PointerMove_Params temp = {
		.layer = *this,
		.ctx = params.ctx,
		.rectCollection = params.rectCollection,
		.windowRect = params.windowRect,
		.usableRect = params.safeAreaRect,
		.pointer = pointer, };

	return DropdownLayer_PointerMove(temp);
}

bool Dropdown::Impl::DropdownLayer::TouchMove2(
	TouchMoveParams const& params,
	bool occluded)
{
	impl::PointerMove_Pointer pointer = {};
	pointer.id = params.event.id;
	pointer.pos = params.event.position;
	pointer.occluded = occluded;

	DropdownLayer_PointerMove_Params temp = {
		.layer = *this,
		.ctx = params.ctx,
		.rectCollection = params.rectCollection,
		.windowRect = params.windowRect,
		.usableRect = params.safeAreaRect,
		.pointer = pointer, };

	return DropdownLayer_PointerMove(temp);
}

Layer::Press_Return
Dropdown::Impl::DropdownLayer::CursorPress(
	Layer::CursorPressParams const& params,
	bool eventConsumed)
{
	impl::PointerPress_Pointer pointer = {};
	pointer.type = impl::ToPointerType(params.event.button);
	pointer.id = impl::cursorPointerId;
	pointer.pos = { (f32)params.cursorPos.x, (f32)params.cursorPos.y };
	pointer.pressed = params.event.pressed;

	DropdownLayer_PointerPress_Params temp = {
		.layer = *this,
		.ctx = params.ctx,
		.rectCollection = params.rectCollection,
		.windowRect = params.windowRect,
		.usableRect = params.safeAreaRect,
		.pointer = pointer,
		.eventConsumed = eventConsumed,
	};
	return DropdownLayer_PointerPress(temp);
}

Layer::Press_Return
Dropdown::Impl::DropdownLayer::TouchPress2(
	TouchPressParams const& params,
	bool eventConsumed)
{
	impl::PointerPress_Pointer pointer = {};
	pointer.type = impl::PointerType::Primary;
	pointer.id = params.event.id;
	pointer.pos = params.event.position;
	pointer.pressed = params.event.pressed;

	DropdownLayer_PointerPress_Params temp = {
		.layer = *this,
		.ctx = params.ctx,
		.rectCollection = params.rectCollection,
		.windowRect = params.windowRect,
		.usableRect = params.safeAreaRect,
		.pointer = pointer,
		.eventConsumed = eventConsumed,
	};
	return DropdownLayer_PointerPress(temp);
}

bool Dropdown::Impl::DropdownLayer_PointerMove(
	DropdownLayer_PointerMove_Params const& params)
{
	auto& layer = params.layer;
	auto& rectColl = params.rectCollection;
	auto& pointer = params.pointer;

	auto& mainWidget = layer.GetDropdownWidget();

	auto rectCollEntryOpt = rectColl.GetEntry(layer);
	DENGINE_IMPL_GUI_ASSERT(rectCollEntryOpt.HasValue());
	auto const& rectCollEntry = rectCollEntryOpt.Value();
	auto const* customDataPtr = rectColl.GetCustomData2<DropdownLayer_CustomData>(rectCollEntry);
	DENGINE_IMPL_GUI_ASSERT(customDataPtr);
	auto const& customData = *customDataPtr;

	auto const outerRect = rectColl.GetRect(rectCollEntry);

	bool newOccluded = pointer.occluded;

	if (pointer.id == impl::cursorPointerId)
	{
		if (pointer.occluded)
			layer.hoveredByCursorIndex = Std::nullOpt;
		else
		{
			auto const lineHeight = customData.textheight;

			// Figure out if we hit a line
			auto lineHit = impl::DropdownLayer_CheckLineHit(
				outerRect.widgetRect.position,
				outerRect.widgetRect.extent.width,
				mainWidget.items.size(),
				lineHeight + mainWidget.textMargin * 2,
				pointer.pos);

			layer.hoveredByCursorIndex = lineHit;
		}
	}

	newOccluded = newOccluded || outerRect.widgetRect.PointIsInside(pointer.pos);

	return newOccluded;
}

Layer::Press_Return Dropdown::Impl::DropdownLayer_PointerPress(
	DropdownLayer_PointerPress_Params const& params)
{
	auto& layer = params.layer;
	auto& mainWidget = layer.GetDropdownWidget();
	auto const& rectColl = params.rectCollection;
	auto const& pointer = params.pointer;
	auto const& eventConsumed = params.eventConsumed;

	auto rectCollEntryOpt = rectColl.GetEntry(layer);
	DENGINE_IMPL_GUI_ASSERT(rectCollEntryOpt.HasValue());
	auto const& rectCollEntry = rectCollEntryOpt.Value();
	auto const* customDataPtr = rectColl.GetCustomData2<Impl::DropdownLayer_CustomData>(rectCollEntry);
	DENGINE_IMPL_GUI_ASSERT(customDataPtr);
	auto const& customData = *customDataPtr;

	auto const textheight = customData.textheight;
	auto const lineheight = textheight + mainWidget.textMargin * 2;

	auto const& outerRectPair = rectColl.GetRect(rectCollEntry);
	auto const outerRect = Rect::Intersection(outerRectPair.widgetRect, outerRectPair.visibleRect);

	auto const pointerInsideOuter = outerRect.PointIsInside(pointer.pos);

	Layer::Press_Return returnVal = {};
	// We want to consume the event if it's inside our rect.
	returnVal.eventConsumed = eventConsumed || pointerInsideOuter;
	// We want to destroy this layer if the even has not already been consumed
	// and we missed the rect.
	returnVal.destroyLayer = !eventConsumed && !pointerInsideOuter;

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

	// Figure out if we hit a line
	auto lineHit = impl::DropdownLayer_CheckLineHit(
		outerRect.position,
		outerRect.extent.width,
		mainWidget.items.size(),
		lineheight,
		pointer.pos);

	if (layer.pressedLine.HasValue())
	{
		auto const pressedLine = layer.pressedLine.Value();
		if (pressedLine.pointerId == pointer.id)
		{
			if (!pointer.pressed)
				layer.pressedLine = Std::nullOpt;

			if (lineHit.HasValue() &&
				lineHit.Value() == pressedLine.index &&
				!pointer.pressed)
			{
				mainWidget.selectedItem = pressedLine.index;

				if (mainWidget.selectionChangedCallback)
					mainWidget.selectionChangedCallback(mainWidget);

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
			layer.pressedLine = newPressedLine;
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

	// We want to find the size of the biggest text in the dropdown entries
	// We could measure each lines outer rect, but I'm boring so I'm gonna
	// just find the line with the longest count kek
	int longestElementIndex = 0;
	int linecount = (int)items.size();
	for (int i = 0; i < linecount; i += 1)
	{
		if (items[i].size() > items[longestElementIndex].size())
			longestElementIndex = i;
	}

	auto& text = items[longestElementIndex];

	// We do not store the glyphs of this text, because it's not the
	// line we want to render later. This is just the longest line.
	auto const textOuterExtent = textManager.GetOuterExtent({ text.data(), text.size() });
	returnValue.minimum = textOuterExtent;

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
	auto& textMan = params.textManager;
	auto& pusher = params.pusher;

	if (!pusher.IncludeRendering())
		return;

	auto customData = Impl::CustomData{ pusher.Alloc() };

	auto& text = items[selectedItem];
	auto const textLength = (int)text.size();
	customData.glyphRects.Resize(text.size());
	auto const textExtent = textMan.GetOuterExtent(
		{ text.data(), (uSize)textLength },
		{},
		customData.glyphRects.Data());

	Math::Vec2Int const centerOffset = {
		(i32)(widgetRect.extent.width / 2 - textExtent.width / 2),
		(i32)(widgetRect.extent.height / 2 - textExtent.height / 2) };
	for (int i = 0; i < textLength; i += 1)
	{
		auto& rect = customData.glyphRects[i];
		rect.position += widgetRect.position;
		rect.position += centerOffset;
	}

	auto rectCollEntry = pusher.GetEntry(*this);
	pusher.AttachCustomData(rectCollEntry, Std::Move(customData));
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
	impl::PointerPress_Pointer pointer = {};
	pointer.id = impl::cursorPointerId;
	pointer.pos = { (f32)params.cursorPos.x, (f32)params.cursorPos.y };
	pointer.pressed = params.event.pressed;
	pointer.type = impl::ToPointerType(params.event.button);

	Impl::Dropdown_PointerPress_Params temp = {
		.widget = *this,
		.ctx = params.ctx,
		.windowId = params.windowId,
		.widgetRect = widgetRect,
		.visibleRect = visibleRect,
		.rectCollection = params.rectCollection,
		.pointer = pointer,
		.eventConsumed = consumed, };

	return Impl::Dropdown_PointerPress(temp);
}

bool Dropdown::TouchPress2(
	TouchPressParams const& params,
	Rect const& widgetRect,
	Rect const& visibleRect,
	bool consumed)
{
	impl::PointerPress_Pointer pointer = {};
	pointer.id = params.event.id;
	pointer.pos = params.event.position;
	pointer.pressed = params.event.pressed;
	pointer.type = impl::PointerType::Primary;

	Impl::Dropdown_PointerPress_Params temp = {
		.widget = *this,
		.ctx = params.ctx,
		.windowId = params.windowId,
		.widgetRect = widgetRect,
		.visibleRect = visibleRect,
		.rectCollection = params.rectCollection,
		.pointer = pointer,
		.eventConsumed = consumed, };

	return Impl::Dropdown_PointerPress(temp);
}