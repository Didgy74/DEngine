#include <DEngine/Gui/StdWidgets/Dropdown.hpp>


#include <DEngine/Gui/StdWidgets/ButtonUtils.hpp>


import DEngine.Gui.DrawEngine;
import DEngine.Gui.TextEngine;

using namespace DEngine;
using namespace DEngine::Gui;

namespace DEngine::Gui::impl
{
	enum class PointerType : u8 { Primary, Secondary };
	[[nodiscard]] static PointerType ToPointerType(CursorButton in) noexcept {
		switch (in) {
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
		Std::Span<std::string const> widgetItems,
		FontFaceSizeId fontSizeId,
		int totalLineHeight,
		int marginAmount,
		TextEngine& textManager,
		Rect const& safeAreaRect)
	{
		Extent returnValue = {};
		returnValue.height = totalLineHeight * widgetItems.Size();

		for (auto const& line : widgetItems) {
			auto const textExtent = textManager.GetOuterExtent(
				{ line.data(), line.size() },
				fontSizeId);

			returnValue.width = Std::Max(
				returnValue.width,
				textExtent.width);
		}
		// Add margin to extent.
		returnValue.width += marginAmount * 2;

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
		for (int i = 0; i < lineCount; i++) {
			Rect lineRect = {};
			lineRect.position = rectPos;
			lineRect.position.y += (i32)lineHeight * i;
			lineRect.extent.width = rectWidth;
			lineRect.extent.height = lineHeight;
			if (lineRect.PointIsInside(pointerPos)) {
				lineHit = i;
				break;
			}
		}
		return lineHit;
	}

	struct PointerPress_Pointer {
		u8 id;
		PointerType type;
		Math::Vec2 pos;
		bool pressed;
	};

	struct PointerMove_Pointer {
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

Dropdown::Dropdown() = default;

Dropdown::~Dropdown() = default;

struct Dropdown::Impl {
	// This data is only available when rendering.
	struct [[maybe_unused]] CustomData {
		explicit CustomData(RectCollection::AllocRefT const& alloc) :
			glyphRects{ alloc }
		{}

		Extent titleTextOuterExtent = {};
		FontFaceSizeId fontSizeId = FontFaceSizeId::Invalid;
		Std::Vec<Rect, RectCollection::AllocRefT> glyphRects;
		u32 cornerRadiusPx = 0;
	};

	struct Dropdown_PointerPress_Params {
		bool eventConsumed;
		impl::PointerPress_Pointer const& pointer;
		RectCollection const& rectColl;
		Std::Opt<RectCollection::Iter> const& rectCollIter;
		Dropdown& widget;
		WidgetEvent_WindowHandler& windowHandler;
	};
	[[nodiscard]] static TouchEventConsumption Dropdown_PointerPress(
		Dropdown_PointerPress_Params const& params);

	static void OpenDropdownMenu(
		Dropdown& widget,
		Widget::WidgetEvent_WindowHandler& windowHandler,
		Rect widgetRect);

	class DropdownOverlayWidget : public Widget {
	public:
		Dropdown* sourceDropdown = nullptr;
		u32 sourceWidth = 0;

		Std::Opt<u8> heldPointerId;
		uSize heldLineIndex = 0;
		// Which line the cursor is currently hovering, if any.
		Std::Opt<uSize> hoveredLineIndex;

		struct CustomData {
			u32 totalLineHeight = 0;
			FontFaceSizeId fontSizeId = FontFaceSizeId::Invalid;
			u32 textMarginPx = 0;
			u32 layerCornerRadius = 0;
		};

		struct DropdownOverlay_PointerPress_Params {
			bool eventConsumed;
			impl::PointerPress_Pointer const& pointer;
			RectCollection const& rectColl;
			Std::Opt<RectCollection::Iter> const& rectCollIter;
			DropdownOverlayWidget& widget;
			WidgetEvent_WindowHandler& windowHandler;
		};
		[[nodiscard]] static TouchEventConsumption DropdownOverlay_PointerPress(
			DropdownOverlay_PointerPress_Params const& params);

		virtual GetSizeHint2_ReturnT GetSizeHint2(GetSizeHint2_Params const&) const override;
		virtual void BuildChildRects(
			BuildChildRects_Params const&,
			Rect const&,
			Rect const&,
			Std::Opt<RectCollection::Iter> const&) const override;
		virtual void Render2(Render_Params const&, Std::Opt<RectCollection::Iter> const&) const override;
		virtual void WidgetEvent_Navigation(
			Widget_NavigationParams const&,
			Std::Opt<RectCollection::Iter> const&,
			Widget_NavigationState&) override;
		virtual bool WidgetEvent_Activate(
			WidgetEvent_ActivateParams const&,
			Std::Opt<RectCollection::Iter> const&) override;
		virtual bool WidgetEvent_Back(
			WidgetEvent_BackParams const&,
			Std::Opt<RectCollection::Iter> const&) override;

		virtual bool CursorMove(
			CursorMoveParams const& params,
			Std::Opt<RectCollection::Iter> const& rectCollIter,
			bool occluded) override;

		virtual void CursorExit() override;

		virtual bool CursorPress2(
			CursorPressParams const& params,
			Std::Opt<RectCollection::Iter> const& rectCollIter,
			bool consumed) override;

		virtual TouchEventConsumption WidgetEvent_TouchPress(
			WidgetEvent_TouchPressParams const& params,
			Std::Opt<RectCollection::Iter> const& rectCollIter,
			bool consumed) override;
	};
};

void Dropdown::Impl::OpenDropdownMenu(
	Dropdown& widget,
	Widget::WidgetEvent_WindowHandler& windowHandler,
	Rect widgetRect)
{
	auto overlayWidget = Std::BoxAdopt(new Impl::DropdownOverlayWidget);
	overlayWidget->sourceDropdown = &widget;
	overlayWidget->sourceWidth = widgetRect.extent.width;

	auto layerPos = widgetRect.position;
	layerPos.y += (i32)widgetRect.extent.height;

	// The source Dropdown widget is itself a single-button cell, so its
	// height is the per-line height for the stacked overlay. Stack one
	// entry per item to derive the layer's total height.
	Extent layerExtent = {};
	layerExtent.width = widgetRect.extent.width;
	layerExtent.height = widgetRect.extent.height * (u32)widget.items.size();

	auto* widgetPtrCopy = overlayWidget.Get();
	[[maybe_unused]] bool result = windowHandler.SetFrontmostLayer(
		Std::Move(overlayWidget),
		layerPos,
		layerExtent,
		Widget_FocusWidgetResult {
			.widget = widgetPtrCopy,
			.secondaryIndex = widget.selectedItem,
		});
}

TouchEventConsumption Dropdown::Impl::Dropdown_PointerPress(
	Dropdown_PointerPress_Params const& params)
{
	auto& widget = params.widget;
	auto const& rectColl = params.rectColl;
	auto const& rectCollIter = params.rectCollIter;
	auto const& pointer = params.pointer;
	auto const& oldEventConsumed = params.eventConsumed;

	auto rectCollEntryOpt = rectColl.GetEntry(widget, rectCollIter);
	DENGINE_IMPL_GUI_ASSERT(rectCollEntryOpt.HasValue());
	auto const& rectCollEntry = rectCollEntryOpt.Value();

	auto const& rectPair = rectColl.GetRect(rectCollEntry);
	auto const& widgetRect = rectPair.widgetRect;
	auto const& visibleRect = rectPair.visibleRect;

	auto outerRect = Rect::Intersection(widgetRect, visibleRect);

	auto const pointerInside = outerRect.PointIsInside(pointer.pos);

	TouchEventConsumption returnValue = {};
	returnValue.consumed = oldEventConsumed || pointerInside;

	// If our pointer-type is not primary, we don't want to do anything,
	// Just consume the event if it's inside the widget and move on.
	if (pointer.type != impl::PointerType::Primary) {
		return returnValue;
	}

	// We can now trust pointer.type == Primary
	if (widget.heldPointerId.HasValue()) {
		auto const heldPointerId = widget.heldPointerId.Value();
		if (!pointer.pressed) {
			widget.heldPointerId = Std::nullOpt;
		}

		if (heldPointerId == pointer.id && pointerInside && !pointer.pressed) {
			// Now we open the dropdown menu
			Impl::OpenDropdownMenu(widget, params.windowHandler, widgetRect);
			returnValue.consumed = true;
		}
	} else {
		if (!oldEventConsumed && pointerInside && pointer.pressed) {
			widget.heldPointerId = pointer.id;
			returnValue.consumed = true;
		}
	}

	// We are inside the button, so we always want to consume the event.
	return returnValue;
}

Widget::GetSizeHint2_ReturnT Dropdown::Impl::DropdownOverlayWidget::GetSizeHint2(
	Widget::GetSizeHint2_Params const& params) const
{
	DENGINE_IMPL_GUI_ASSERT(sourceDropdown != nullptr);
	auto& mainWidget = *sourceDropdown;
	auto& window = params.window;
	auto& textEngine = params.textEngine;
	auto& pusher = params.pusher;
	auto const& fontScale = params.FontScale();
	auto const& minimumSizeCm = params.MinimumHeightCm();

	Dropdown::Theme theme;
	if (mainWidget.getThemeFn)
		theme = mainWidget.getThemeFn({ .widget = mainWidget, .appData = params.appData });
	auto textMarginPx = theme.margin.ResolvePx(window.dpi, window.contentScale);

	auto normalTextScale = fontScale * window.contentScale;
	auto fontSizeId = textEngine.GetFontFaceSizeId(normalTextScale, window.dpi, window.dpi);
	auto normalHeight = textEngine.GetFontFaceSizeMetrics(fontSizeId).lineHeight;
	auto totalLineHeight = normalHeight + 2 * textMarginPx;
	totalLineHeight = Std::Max(totalLineHeight, (u32)CmToPixels(minimumSizeCm, window.dpi));

	auto customData = CustomData{};
	customData.textMarginPx = textMarginPx;
	customData.layerCornerRadius = theme.cornerRadius.ResolvePx(window.dpi, window.contentScale);
	customData.totalLineHeight = totalLineHeight;
	customData.fontSizeId = fontSizeId;

	int lineCount = (int)mainWidget.items.size();
	SizeHint returnValue = {};
	returnValue.minimum.height = totalLineHeight * lineCount;
	returnValue.minimum.width = sourceWidth;
	returnValue.expandX = false;
	returnValue.expandY = false;

	auto pusherIt = pusher.PushWithCustom(
		*this,
		returnValue,
		false,
		Std::Move(customData));

	return { .iter = pusherIt, .sizeHint = returnValue };
}

void Dropdown::Impl::DropdownOverlayWidget::BuildChildRects(
	BuildChildRects_Params const& params,
	Rect const& widgetRect,
	Rect const& visibleRect,
	Std::Opt<RectCollection::Iter> const& rectCollIter) const
{
	DENGINE_IMPL_GUI_ASSERT(sourceDropdown != nullptr);
	auto& mainWidget = *sourceDropdown;

	auto& rectPusher = params.pusher;

	auto const* customDataPtr = rectPusher.GetCustomData<CustomData>(*this);
	DENGINE_IMPL_GUI_ASSERT(customDataPtr != nullptr);
	auto& customData = *customDataPtr;
	auto totalLineHeight = customData.totalLineHeight;

	// Add secondary rects?

	// Distribute sizes
	for (int i = 0; i < (int)mainWidget.items.size(); i++) {



	}
}

void Dropdown::Impl::DropdownOverlayWidget::Render2(
	Widget::Render_Params const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter) const
{
	DENGINE_IMPL_GUI_ASSERT(sourceDropdown != nullptr);
	auto& mainWidget = *sourceDropdown;
	auto const& rectColl = params.rectCollection;
	auto& drawInfo = params.drawEngine;
	auto& textEngine = params.textEngine;
	auto& transientAlloc = params.transientAlloc;
	auto const& focusedItemOpt = params.focusedWidget;

	auto rectCollEntryOpt = rectColl.GetEntry(*this, rectCollIter);
	DENGINE_IMPL_GUI_ASSERT(rectCollEntryOpt.HasValue());
	auto const& rectCollEntry = rectCollEntryOpt.Value();

	auto const* customDataPtr = rectColl.GetCustomData<CustomData>(rectCollEntry);
	DENGINE_IMPL_GUI_ASSERT(customDataPtr != nullptr);
	auto& customData = *customDataPtr;
	auto fontSizeId = customData.fontSizeId;
	auto totalLineHeight = customData.totalLineHeight;
	auto cornerRadiusPx = customData.layerCornerRadius;

	auto const& rectPair = rectColl.GetRect(rectCollEntry);
	auto const& widgetRect = rectPair.widgetRect;
	auto const& visibleRect = rectPair.visibleRect;

	auto const intersection = Rect::Intersection(widgetRect, visibleRect);
	if (intersection.IsNothing()) {
		return;
	}

	auto scissorGuard = DrawEngine::ScopedScissor{ drawInfo, widgetRect };
	auto scopedMask = drawInfo.PushRectMaskScoped(widgetRect, cornerRadiusPx, DrawEngine::MaskOp::Outside);

	for (int i = 0; i < (int)mainWidget.items.size(); i++) {
		auto& text = mainWidget.items[i];

		auto lineRect = widgetRect;
		lineRect.extent.height = totalLineHeight;
		lineRect.position.y += (i32)lineRect.extent.height * i;

		Math::Vec4 color = { 0.5f, 0.5f, 0.5f, 1.f };
		if (i == mainWidget.selectedItem) {
			color += {0.2f, 0.2f, 0.2f, 0.f };
		}
		if (hoveredLineIndex.HasValue() && hoveredLineIndex.Value() == (uSize)i) {
			color += {0.1f, 0.1f, 0.1f, 0.f };
		}
		if (focusedItemOpt.Has()) {
			DENGINE_IMPL_GUI_ASSERT(focusedItemOpt.Get().secondaryIndex.Has());
			if (i == focusedItemOpt.Get().secondaryIndex.Get()) {
				color += {0.2f, 0.2f, 0.2f, 0.f };
			}
		}
		drawInfo.PushFilledQuad(lineRect, color);

		auto rects = Std::NewVec<Rect>(transientAlloc);
		rects.Resize(text.size());
		auto textExtent = textEngine.GetOuterExtent(
			{ text.data(), text.size() },
			fontSizeId,
			rects.ToSpan());
		auto textOffset = lineRect.position + Extent::CenteringOffset(lineRect.extent, textExtent);
		drawInfo.PushText(
			fontSizeId,
			{ text.data(), text.size() },
			rects.Data(),
			textOffset,
			{ 1, 1, 1, 1 });
	}
}

TouchEventConsumption Dropdown::Impl::DropdownOverlayWidget::DropdownOverlay_PointerPress(
	DropdownOverlay_PointerPress_Params const& params)
{
	auto& widget = params.widget;
	auto const& rectColl = params.rectColl;
	auto const& rectCollIter = params.rectCollIter;
	auto const& pointer = params.pointer;
	auto const oldEventConsumed = params.eventConsumed;

	DENGINE_IMPL_GUI_ASSERT(widget.sourceDropdown != nullptr);
	auto& sourceWidget = *widget.sourceDropdown;

	auto rectCollEntryOpt = rectColl.GetEntry(widget, rectCollIter);
	DENGINE_IMPL_GUI_ASSERT(rectCollEntryOpt.HasValue());
	auto const& rectCollEntry = rectCollEntryOpt.Value();

	auto const* customDataPtr = rectColl.GetCustomData<CustomData>(rectCollEntry);
	DENGINE_IMPL_GUI_ASSERT(customDataPtr != nullptr);
	auto const& customData = *customDataPtr;
	auto totalLineHeight = customData.totalLineHeight;

	auto const& rectPair = rectColl.GetRect(rectCollEntry);
	auto const& widgetRect = rectPair.widgetRect;
	auto const& visibleRect = rectPair.visibleRect;

	auto outerRect = Rect::Intersection(widgetRect, visibleRect);
	auto const pointerInside = outerRect.PointIsInside(pointer.pos);

	auto const lineHitOpt = impl::DropdownLayer_CheckLineHit(
		widgetRect.position,
		widgetRect.extent.width,
		sourceWidget.items.size(),
		totalLineHeight,
		pointer.pos);

	TouchEventConsumption returnValue = {};
	returnValue.consumed = oldEventConsumed;

	// Non-primary pointers do nothing other than possibly consuming the event
	// so it doesn't leak into widgets behind the overlay.
	if (pointer.type != impl::PointerType::Primary) {
		returnValue.consumed = returnValue.consumed || pointerInside;
		return returnValue;
	}

	if (widget.heldPointerId.HasValue()) {
		auto const heldId = widget.heldPointerId.Value();
		if (heldId == pointer.id && !pointer.pressed) {
			auto const heldLineIndex = widget.heldLineIndex;
			widget.heldPointerId = Std::nullOpt;

			if (lineHitOpt.HasValue() && lineHitOpt.Value() == heldLineIndex) {
				DENGINE_IMPL_GUI_ASSERT(heldLineIndex < sourceWidget.items.size());
				sourceWidget.selectedItem = heldLineIndex;
				if (sourceWidget.selectionChangedCallback)
					sourceWidget.selectionChangedCallback(sourceWidget);
				params.windowHandler.QueueRemoveCurrentLayer();
				returnValue.consumed = true;
			}
		}
	} else {
		if (!oldEventConsumed && pointer.pressed) {
			if (lineHitOpt.HasValue()) {
				widget.heldPointerId = pointer.id;
				widget.heldLineIndex = lineHitOpt.Value();
				returnValue.consumed = true;
			} else if (!pointerInside) {
				// A press outside the overlay rect dismisses the dropdown
				// without committing a selection. The event is left
				// unconsumed so the underlying widget can still react.
				params.windowHandler.QueueRemoveCurrentLayer();
			}
		}
	}

	returnValue.consumed = returnValue.consumed || pointerInside;
	return returnValue;
}

bool Dropdown::Impl::DropdownOverlayWidget::CursorMove(
	CursorMoveParams const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter,
	bool occluded)
{
	DENGINE_IMPL_GUI_ASSERT(sourceDropdown != nullptr);
	auto& sourceWidget = *sourceDropdown;
	auto const& rectColl = params.rectCollection;

	auto rectCollEntryOpt = rectColl.GetEntry(*this, rectCollIter);
	DENGINE_IMPL_GUI_ASSERT(rectCollEntryOpt.HasValue());
	auto const& rectCollEntry = rectCollEntryOpt.Value();

	auto const* customDataPtr = rectColl.GetCustomData<CustomData>(rectCollEntry);
	DENGINE_IMPL_GUI_ASSERT(customDataPtr != nullptr);
	auto const& customData = *customDataPtr;
	auto totalLineHeight = customData.totalLineHeight;

	auto const& rectPair = rectColl.GetRect(rectCollEntry);
	auto const& widgetRect = rectPair.widgetRect;
	auto const& visibleRect = rectPair.visibleRect;

	Math::Vec2 pointerPos = { (f32)params.CursorPos().x, (f32)params.CursorPos().y };

	auto outerRect = Rect::Intersection(widgetRect, visibleRect);
	auto const pointerInside = outerRect.PointIsInside(pointerPos);

	if (!pointerInside || occluded) {
		hoveredLineIndex = Std::nullOpt;
	} else {
		// Assign the hit-test result directly so the hover also clears when the
		// cursor is inside the overlay rect but not over any line.
		hoveredLineIndex = impl::DropdownLayer_CheckLineHit(
			widgetRect.position,
			widgetRect.extent.width,
			sourceWidget.items.size(),
			totalLineHeight,
			pointerPos);
	}

	return pointerInside;
}

void Dropdown::Impl::DropdownOverlayWidget::CursorExit() {
	hoveredLineIndex = Std::nullOpt;
}

bool Dropdown::Impl::DropdownOverlayWidget::CursorPress2(
	CursorPressParams const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter,
	bool consumed)
{
	impl::PointerPress_Pointer pointer = {
		.id = impl::cursorPointerId,
		.type = impl::ToPointerType(params.CursorButton()),
		.pos = { (f32)params.cursorPos.x, (f32)params.cursorPos.y },
		.pressed = params.CursorPressed(), };

	DropdownOverlay_PointerPress_Params tempParams = {
		.eventConsumed = consumed,
		.pointer = pointer,
		.rectColl = params.rectCollection,
		.rectCollIter = rectCollIter,
		.widget = *this,
		.windowHandler = params.windowHandler, };

	return DropdownOverlay_PointerPress(tempParams).consumed;
}

TouchEventConsumption Dropdown::Impl::DropdownOverlayWidget::WidgetEvent_TouchPress(
	WidgetEvent_TouchPressParams const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter,
	bool consumed)
{
	impl::PointerPress_Pointer pointer = {
		.id = params.event.id,
		.type = impl::PointerType::Primary,
		.pos = params.event.position,
		.pressed = params.event.pressed, };

	DropdownOverlay_PointerPress_Params tempParams = {
		.eventConsumed = consumed,
		.pointer = pointer,
		.rectColl = params.rectCollection,
		.rectCollIter = rectCollIter,
		.widget = *this,
		.windowHandler = params.windowHandler, };

	return DropdownOverlay_PointerPress(tempParams);
}

void Dropdown::Impl::DropdownOverlayWidget::WidgetEvent_Navigation(
	Widget_NavigationParams const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter,
	Widget_NavigationState& event)
{
	auto const& rectColl = params.rectCollection;
	auto& sourceWidget = *this->sourceDropdown;
	auto const& currentFocusOpt = params.currentFocusOpt;

	auto entryOpt = rectColl.GetEntry(*this, rectCollIter);
	DENGINE_IMPL_GUI_ASSERT(entryOpt.Has());
	auto const& entry = entryOpt.Get();

	auto const& widgetRect = rectColl.GetRect(entry).widgetRect;

	auto const* customDataPtr = rectColl.GetCustomData<CustomData>(entry);
	DENGINE_IMPL_GUI_ASSERT(customDataPtr != nullptr);
	auto const& customData = *customDataPtr;
	auto totalLineHeight = customData.totalLineHeight;

	if (event.State() == Widget_NavigationEventState::NotConsumed && !currentFocusOpt.Has()) {
		event.Consume(this, 0);
		return;
	}

	bool thisWidgetHasFocus = currentFocusOpt.Has() && currentFocusOpt.Get().widget == this;
	if (event.State() == Widget_NavigationEventState::NotConsumed && thisWidgetHasFocus) {
		auto const& currentFocus = currentFocusOpt.Get();
		DENGINE_IMPL_GUI_ASSERT(
			currentFocus.secondaryIndex.Has()
			&& currentFocus.secondaryIndex.Get() < sourceWidget.items.size());

		auto focusedIndex = currentFocus.secondaryIndex.Get();
		auto focusedBtnRect = widgetRect;
		focusedBtnRect.extent.height = totalLineHeight;
		focusedBtnRect.position.y += (int)focusedBtnRect.extent.height * focusedIndex;

		if (params.directional) {
			if (Gui::DirAngleIsQuadrantDown(params.angle)) {
				if (focusedIndex + 1 < sourceWidget.items.size()) {
					event.Consume(this, focusedIndex + 1);
					return;
				}
				// TODO: Do we jump to the top?
				event.HitBoundary(focusedBtnRect);
				return;
			}
			if (Gui::DirAngleIsQuadrantUp(params.angle)) {
				if (focusedIndex > 0) {
					event.Consume(this, focusedIndex - 1);
					return;
				}
				// TODO: Do we jump to the top?
				event.HitBoundary(focusedBtnRect);
				return;
			}
		} else {

		}
	}
}

bool Dropdown::Impl::DropdownOverlayWidget::WidgetEvent_Activate(
	WidgetEvent_ActivateParams const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter)
{
	DENGINE_IMPL_GUI_ASSERT(sourceDropdown != nullptr);
	auto& mainWidget = *sourceDropdown;

	DENGINE_IMPL_GUI_ASSERT(params.focusSecondaryIndex.Has());
	auto selectedIndex = params.focusSecondaryIndex.Get();
	DENGINE_IMPL_GUI_ASSERT(selectedIndex < mainWidget.items.size());

	mainWidget.selectedItem = selectedIndex;

	if (mainWidget.selectionChangedCallback)
		mainWidget.selectionChangedCallback(mainWidget);

	params.QueueRemoveFrontmostLayer();

	return true;
}

bool Dropdown::Impl::DropdownOverlayWidget::WidgetEvent_Back(
	WidgetEvent_BackParams const& params,
	Std::Opt<RectCollection::Iter> const&)
{
	// Cancel the popup without committing the currently focused entry.
	params.QueueRemoveFrontmostLayer();
	return true;
}

Widget::GetSizeHint2_ReturnT Dropdown::GetSizeHint2(
	Widget::GetSizeHint2_Params const& params) const
{
	auto& window = params.window;
	auto& textEngine = params.textEngine;
	auto& pusher = params.pusher;
	auto const& fontScale = params.FontScale();
	auto const& minimumSizeCm = params.MinimumHeightCm();

	auto pusherIt = pusher.AddEntry(*this);
	auto& customData = pusher.AttachCustomData(pusherIt, Impl::CustomData{ pusher.Alloc() });

	Dropdown::Theme theme;
	if (this->getThemeFn)
		theme = this->getThemeFn({ .widget = *this, .appData = params.appData });
	auto textMarginPx = theme.margin.ResolvePx(window.dpi, window.contentScale);

	auto normalTextScale = fontScale * window.contentScale;
	auto fontSizeId = textEngine.GetFontFaceSizeId(normalTextScale, window.dpi, window.dpi);

	{
		// Load the outer extent of the text currently being displayed.
		// This loads both the outer extent and all the glyphs, so we can render them later.
		auto& currText = this->items[this->selectedItem];
		auto& glyphRects = customData.glyphRects;
		if (pusher.IncludeRendering()) {
			glyphRects.Resize(currText.size());
		}
		auto outerExtent = textEngine.GetOuterExtent(
			{ currText.data(), currText.size() },
			fontSizeId,
			glyphRects.ToSpan());
		customData.titleTextOuterExtent = outerExtent;
	}

	// We want to find the size of the biggest text in the dropdown entries
	// We could measure each lines outer rect, but I'm boring so I'm gonna
	// just find the line with the longest count kek
	int longestElementIndex = 0;
	int linecount = (int)items.size();
	for (int i = 0; i < linecount; i += 1) {
		if (items[i].size() > items[longestElementIndex].size()) {
			longestElementIndex = i;
		}
	}

	auto const& text = items[longestElementIndex];

	// We do not store the glyphs of this text, because it's not the
	// line we want to render later. This is just the longest line.
	auto sizeHintResult = ButtonUtils::GetSizeHint(
		{
			.textEngine = textEngine,
			.window = window,
			.textSpan = { text.data(), text.size() },
			.fontScale = fontScale,
			.textMarginPx = textMarginPx,
			.minimumHeightCm = minimumSizeCm,
		},
		{});

	auto returnVal = sizeHintResult.sizeHint;

	customData.cornerRadiusPx = theme.cornerRadius.ResolvePx(window.dpi, window.contentScale);
	customData.fontSizeId = fontSizeId;

	pusher.SetSizeHint(pusherIt, returnVal, true);
	return {
		.iter = pusherIt,
		.sizeHint = returnVal };
}

void Dropdown::BuildChildRects(
	Widget::BuildChildRects_Params const& params,
	Rect const& widgetRect,
	Rect const& visibleRect,
	Std::Opt<RectCollection::Iter> const& rectCollIter) const
{
}

void Dropdown::Render2(
	Render_Params const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter) const
{
	auto const& rectColl = params.rectCollection;
	auto& drawInfo = params.drawEngine;

	auto rectCollEntryOpt = rectColl.GetEntry(*this, rectCollIter);
	DENGINE_IMPL_GUI_ASSERT(rectCollEntryOpt.HasValue());
	auto const& rectCollEntry = rectCollEntryOpt.Value();

	auto const& rectPair = rectColl.GetRect(rectCollEntry);
	auto const& widgetRect = rectPair.widgetRect;
	auto const& visibleRect = rectPair.visibleRect;

	auto const* customDataPtr = rectColl.GetCustomData<Impl::CustomData>(rectCollEntry);
	DENGINE_IMPL_GUI_ASSERT(customDataPtr != nullptr);
	auto const& customData = *customDataPtr;
	auto cornerRadiusPx = customData.cornerRadiusPx;

	auto const intersection = Rect::Intersection(widgetRect, visibleRect);
	if (intersection.IsNothing()) {
		return;
	}

	Math::Vec4Int radiuses = {
		(i32)cornerRadiusPx,
		0,
		0,
		(i32)cornerRadiusPx, };
	auto backgroundColor = this->boxColor;
	auto isFocused = params.focusedWidget.Has() && params.focusedWidget.Get().widget == this;
	if (isFocused) {
		backgroundColor += { 0.2f, 0.2f, 0.2f, 0.f };
	}
	if (hoveredByCursor) {
		backgroundColor += { 0.1f, 0.1f, 0.1f, 0.f };
	}
	drawInfo.PushFilledQuad(
		widgetRect,
		backgroundColor,
		radiuses);

	auto fontSizeId = customData.fontSizeId;
	auto textOuterExtent = customData.titleTextOuterExtent;

	auto textRect = widgetRect;
	textRect.extent = textOuterExtent;
	textRect.position += Extent::CenteringOffset(widgetRect.extent, textOuterExtent);

	auto scissor = DrawEngine::ScopedScissor(drawInfo, textRect, widgetRect);

	auto const& text = items[selectedItem];
	drawInfo.PushText(
		fontSizeId,
		{ text.data(), text.size() },
		customData.glyphRects.Data(),
		textRect.position,
		{ 1.f, 1.f, 1.f, 1.f });
}

bool Dropdown::CursorMove(
	CursorMoveParams const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter,
	bool occluded)
{
	auto const& rectColl = params.rectCollection;

	auto rectCollEntryOpt = rectColl.GetEntry(*this, rectCollIter);
	DENGINE_IMPL_GUI_ASSERT(rectCollEntryOpt.HasValue());
	auto const& rectCollEntry = rectCollEntryOpt.Value();

	auto const& rectPair = rectColl.GetRect(rectCollEntry);
	auto const& widgetRect = rectPair.widgetRect;
	auto const& visibleRect = rectPair.visibleRect;

	Math::Vec2 pointerPos = { (f32)params.CursorPos().x, (f32)params.CursorPos().y };

	auto const pointerInside =
		widgetRect.PointIsInside(pointerPos) &&
		visibleRect.PointIsInside(pointerPos);

	hoveredByCursor = pointerInside && !occluded;

	return pointerInside;
}

void Dropdown::CursorExit() {
	hoveredByCursor = false;
}

bool Dropdown::CursorPress2(
	CursorPressParams const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter,
	bool consumed)
{
	impl::PointerPress_Pointer pointer = {
		.id = impl::cursorPointerId,
		.type = impl::ToPointerType(params.CursorButton()),
		.pos = { (f32)params.cursorPos.x, (f32)params.cursorPos.y },
		.pressed = params.CursorPressed(), };

	Impl::Dropdown_PointerPress_Params tempParams = {
		.eventConsumed = consumed,
		.pointer = pointer,
		.rectColl = params.rectCollection,
		.rectCollIter = rectCollIter,
		.widget = *this,
		.windowHandler = params.windowHandler, };

	auto temp = Impl::Dropdown_PointerPress(tempParams);
	return temp.consumed;
}

TouchEventConsumption Dropdown::WidgetEvent_TouchPress(
	WidgetEvent_TouchPressParams const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter,
	bool consumed)
{
	impl::PointerPress_Pointer pointer = {
		.id = params.event.id,
		.type = impl::PointerType::Primary,
		.pos = params.event.position,
		.pressed = params.event.pressed, };

	Impl::Dropdown_PointerPress_Params temp = {
		.eventConsumed = consumed,
		.pointer = pointer,
		.rectColl = params.rectCollection,
		.rectCollIter = rectCollIter,
		.widget = *this,
		.windowHandler = params.windowHandler, };

	return Impl::Dropdown_PointerPress(temp);
}

bool Dropdown::WidgetEvent_Activate(
	WidgetEvent_ActivateParams const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIterOptIn)
{
	auto const& rectColl = params.rectCollection;

	auto rectCollIterOpt = rectColl.GetEntry(*this, rectCollIterOptIn);
	DENGINE_IMPL_GUI_ASSERT(rectCollIterOpt.Has());
	auto rectCollIter = rectCollIterOpt.Get();

	auto rectPair = rectColl.GetRect(rectCollIter);

	Impl::OpenDropdownMenu(*this, params.windowHandler, rectPair.widgetRect);
	return false;
}
