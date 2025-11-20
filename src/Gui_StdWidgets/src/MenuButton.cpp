#include <DEngine/Gui/StdWidgets/MenuButton.hpp>

import DEngine.Gui.DrawEngine;
import DEngine.Gui.FontFaceSizeId;
import DEngine.Gui.TextEngine;

using namespace DEngine;
using namespace DEngine::Gui;
using Line = MenuButton::Line;
using Submenu = MenuButton::Submenu;

namespace DEngine::Gui::impl {
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

	static constexpr u8 cursorPointerId = (u8)-1;

	struct PointerPress_Pointer {
		u8 id;
		Math::Vec2 pos;
		bool pressed;
		PointerType type;
	};

	struct PointerMove_Pointer {
		u8 id;
		bool occluded;
		Math::Vec2 pos;
	};

	// Hit-tests a stacked list of equal-height lines. Returns the index of the line under the point.
	[[nodiscard]] static Std::Opt<u64> MenuLine_CheckHit(
		Math::Vec2Int rectPos,
		u32 rectWidth,
		u64 lineCount,
		u32 lineHeight,
		Math::Vec2 pointerPos)
	{
		for (u64 i = 0; i < lineCount; i += 1) {
			Rect lineRect = {};
			lineRect.position = rectPos;
			lineRect.position.y += (i32)lineHeight * (i32)i;
			lineRect.extent.width = rectWidth;
			lineRect.extent.height = lineHeight;
			if (lineRect.PointIsInside(pointerPos))
				return i;
		}
		return Std::nullOpt;
	}
}

struct MenuButton::Impl {
	// This data is only available when rendering.
	struct CustomData {
		explicit CustomData(RectCollection::AllocRefT const& alloc) :
			glyphRects{ alloc }
		{}

		Extent textOuterExtent = {};
		FontFaceSizeId fontSizeId = FontFaceSizeId::Invalid;
		Std::Vec<Rect, RectCollection::AllocRefT> glyphRects;
	};

	struct Menu_PointerPress_Params {
		Std::ConstAnyRef appData;
		bool eventConsumed;
		impl::PointerPress_Pointer const& pointer;
		RectCollection const& rectColl;
		Std::Opt<RectCollection::Iter> const& rectCollIter;
		TextEngine& textEngine;
		MenuButton& widget;
		EventWindowInfo const& window;
		WidgetEvent_WindowHandler& windowHandler;
	};
	[[nodiscard]] static TouchEventConsumption Menu_PointerPress(
		Menu_PointerPress_Params const& params);

	static void OpenMenu(
		MenuButton& widget,
		Widget::WidgetEvent_WindowHandler& windowHandler,
		Rect widgetRect,
		EventWindowInfo const& window,
		TextEngine& textEngine,
		Std::ConstAnyRef appData);

	// The submenu floats in its own layer. It is the root Widget of that layer's tree and reads
	// the lines it should display straight from the MenuButton that opened it.
	class MenuButtonOverlayWidget : public Widget {
	public:
		MenuButton* sourceMenuButton = nullptr;

		// The pointer currently pressing a line, if any.
		Std::Opt<u8> heldPointerId;
		u64 heldLineIndex = 0;
		// Which line the cursor is currently hovering, if any.
		Std::Opt<u64> hoveredLineIndex;

		struct CustomData {
			FontFaceSizeId fontSizeId = FontFaceSizeId::Invalid;
			u32 marginPx = 0;
			u32 totalLineHeight = 0;
			u32 cornerRadiusPx = 0;
		};

		struct Overlay_PointerPress_Params {
			bool eventConsumed;
			WidgetEvent_DeferredJobQueue& jobQueue;
			impl::PointerPress_Pointer const& pointer;
			RectCollection const& rectColl;
			Std::Opt<RectCollection::Iter> const& rectCollIter;
			MenuButtonOverlayWidget& widget;
			WidgetEvent_WindowHandler& windowHandler;
		};
		[[nodiscard]] static TouchEventConsumption Overlay_PointerPress(
			Overlay_PointerPress_Params const& params);

		virtual GetSizeHint2_ReturnT GetSizeHint2(GetSizeHint2_Params const&) const override;
		virtual void BuildChildRects(
			BuildChildRects_Params const&,
			Rect const&,
			Rect const&,
			Std::Opt<RectCollection::Iter> const&) const override;
		virtual void Render2(
			Render_Params const&,
			Std::Opt<RectCollection::Iter> const&) const override;

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

		virtual bool WidgetEvent_Back(
			WidgetEvent_BackParams const&,
			Std::Opt<RectCollection::Iter> const&) override;
	};
};

void MenuButton::Impl::OpenMenu(
	MenuButton& widget,
	Widget::WidgetEvent_WindowHandler& windowHandler,
	Rect widgetRect,
	EventWindowInfo const& window,
	TextEngine& textEngine,
	Std::ConstAnyRef appData)
{
	auto const& submenu = widget.submenu;

	// Nothing to show, so we don't bother spawning a layer. SetFrontmostLayer also forbids
	// a zero-extent layer, which an empty submenu would produce.
	if (submenu.lines.empty())
		return;

	MenuButton::Theme theme;
	if (widget.getThemeFn)
		theme = widget.getThemeFn({ .widget = widget, .appData = appData });
	auto marginPx = theme.textMargin.ResolvePx(window.dpi, window.contentScale);

	auto normalTextScale = window.fontScale * window.contentScale;
	auto fontSizeId = textEngine.GetFontFaceSizeId(normalTextScale, window.dpi, window.dpi);
	auto normalHeight = textEngine.GetFontFaceSizeMetrics(fontSizeId).lineHeight;
	auto totalLineHeight = normalHeight + 2 * marginPx;
	totalLineHeight = Std::Max(totalLineHeight, (u32)CmToPixels(window.minimumHeightCm, window.dpi));

	u32 maxTextWidth = 0;
	for (auto const& line : submenu.lines) {
		auto textExtent = textEngine.GetOuterExtent(
			{ line.title.data(), line.title.size() },
			fontSizeId);
		maxTextWidth = Std::Max(maxTextWidth, textExtent.width);
	}

	Extent layerExtent = {};
	layerExtent.width = maxTextWidth + 2 * marginPx;
	layerExtent.height = totalLineHeight * (u32)submenu.lines.size();

	// Position the menu directly beneath the button.
	auto layerPos = widgetRect.position;
	layerPos.y += (i32)widgetRect.extent.height;

	auto overlay = Std::BoxAdopt(new MenuButtonOverlayWidget);
	overlay->sourceMenuButton = &widget;

	[[maybe_unused]] bool result = windowHandler.SetFrontmostLayer(
		Std::Move(overlay),
		layerPos,
		layerExtent,
		Std::nullOpt);
}

TouchEventConsumption MenuButton::Impl::Menu_PointerPress(
	Menu_PointerPress_Params const& params)
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
	// just consume the event if it's inside the widget and move on.
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
			// Releasing inside the button opens the submenu.
			Impl::OpenMenu(
				widget,
				params.windowHandler,
				widgetRect,
				params.window,
				params.textEngine,
				params.appData);
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

Widget::GetSizeHint2_ReturnT MenuButton::Impl::MenuButtonOverlayWidget::GetSizeHint2(
	Widget::GetSizeHint2_Params const& params) const
{
	DENGINE_IMPL_GUI_ASSERT(sourceMenuButton != nullptr);
	auto& mainWidget = *sourceMenuButton;
	auto const& window = params.window;
	auto& textEngine = params.textEngine;
	auto& pusher = params.pusher;
	auto const& fontScale = params.FontScale();
	auto const& minimumSizeCm = params.MinimumHeightCm();
	auto const& submenu = mainWidget.submenu;

	MenuButton::Theme theme;
	if (mainWidget.getThemeFn)
		theme = mainWidget.getThemeFn({ .widget = mainWidget, .appData = params.appData });
	auto marginPx = theme.textMargin.ResolvePx(window.dpi, window.contentScale);

	auto normalTextScale = fontScale * window.contentScale;
	auto fontSizeId = textEngine.GetFontFaceSizeId(normalTextScale, window.dpi, window.dpi);
	auto normalHeight = textEngine.GetFontFaceSizeMetrics(fontSizeId).lineHeight;
	auto totalLineHeight = normalHeight + 2 * marginPx;
	totalLineHeight = Std::Max(totalLineHeight, (u32)CmToPixels(minimumSizeCm, window.dpi));

	u32 maxTextWidth = 0;
	for (auto const& line : submenu.lines) {
		auto textExtent = textEngine.GetOuterExtent(
			{ line.title.data(), line.title.size() },
			fontSizeId);
		maxTextWidth = Std::Max(maxTextWidth, textExtent.width);
	}

	CustomData customData = {};
	customData.fontSizeId = fontSizeId;
	customData.marginPx = marginPx;
	customData.totalLineHeight = totalLineHeight;
	customData.cornerRadiusPx = theme.cornerRadius.ResolvePx(window.dpi, window.contentScale);

	int lineCount = (int)submenu.lines.size();
	SizeHint returnValue = {};
	returnValue.minimum.width = maxTextWidth + 2 * marginPx;
	returnValue.minimum.height = totalLineHeight * lineCount;
	returnValue.expandX = false;
	returnValue.expandY = false;

	auto pusherIt = pusher.PushWithCustom(
		*this,
		returnValue,
		false,
		Std::Move(customData));

	return { .iter = pusherIt, .sizeHint = returnValue };
}

void MenuButton::Impl::MenuButtonOverlayWidget::BuildChildRects(
	BuildChildRects_Params const& params,
	Rect const& widgetRect,
	Rect const& visibleRect,
	Std::Opt<RectCollection::Iter> const& rectCollIter) const
{
	// The lines are plain text drawn during Render2, so there are no child Widgets to lay out.
}

void MenuButton::Impl::MenuButtonOverlayWidget::Render2(
	Widget::Render_Params const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter) const
{
	DENGINE_IMPL_GUI_ASSERT(sourceMenuButton != nullptr);
	auto& mainWidget = *sourceMenuButton;
	auto const& submenu = mainWidget.submenu;
	auto const& rectColl = params.rectCollection;
	auto& drawInfo = params.drawEngine;
	auto& textEngine = params.textEngine;
	auto& transientAlloc = params.transientAlloc;

	auto rectCollEntryOpt = rectColl.GetEntry(*this, rectCollIter);
	DENGINE_IMPL_GUI_ASSERT(rectCollEntryOpt.HasValue());
	auto const& rectCollEntry = rectCollEntryOpt.Value();

	auto const* customDataPtr = rectColl.GetCustomData<CustomData>(rectCollEntry);
	DENGINE_IMPL_GUI_ASSERT(customDataPtr != nullptr);
	auto const& customData = *customDataPtr;
	auto fontSizeId = customData.fontSizeId;
	auto marginPx = customData.marginPx;
	auto totalLineHeight = customData.totalLineHeight;
	auto cornerRadiusPx = customData.cornerRadiusPx;

	auto const& rectPair = rectColl.GetRect(rectCollEntry);
	auto const& widgetRect = rectPair.widgetRect;
	auto const& visibleRect = rectPair.visibleRect;

	auto const intersection = Rect::Intersection(widgetRect, visibleRect);
	if (intersection.IsNothing()) {
		return;
	}

	auto scissorGuard = DrawEngine::ScopedScissor{ drawInfo, widgetRect };
	auto scopedMask = drawInfo.PushRectMaskScoped(widgetRect, cornerRadiusPx, DrawEngine::MaskOp::Outside);

	// Background of the whole submenu.
	drawInfo.PushFilledQuad(widgetRect, mainWidget.colors.active);

	for (int i = 0; i < (int)submenu.lines.size(); i++) {
		auto const& line = submenu.lines[i];

		auto lineRect = widgetRect;
		lineRect.extent.height = totalLineHeight;
		lineRect.position.y += (i32)totalLineHeight * i;

		bool highlight = false;
		if (hoveredLineIndex.HasValue() && hoveredLineIndex.Value() == (uSize)i) {
			highlight = true;
		} else if (line.togglable && line.toggled) {
			highlight = true;
		} else if (heldPointerId.HasValue() && heldLineIndex == (uSize)i) {
			highlight = true;
		}
		if (highlight) {
			drawInfo.PushFilledQuad(lineRect, mainWidget.colors.hovered);
		}

		if (line.title.empty())
			continue;

		auto glyphRects = Std::NewVec<Rect>(transientAlloc);
		glyphRects.Resize(line.title.size());
		auto textExtent = textEngine.GetOuterExtent(
			{ line.title.data(), line.title.size() },
			fontSizeId,
			glyphRects.ToSpan());
		// Left-align with a margin, but center vertically within the line.
		auto textPos = lineRect.position;
		textPos.x += (i32)marginPx;
		textPos.y += Std::Max(0, Extent::CenteringOffset(lineRect.extent, textExtent).y);
		drawInfo.PushText(
			fontSizeId,
			{ line.title.data(), line.title.size() },
			glyphRects.Data(),
			textPos,
			{ 1.f, 1.f, 1.f, 1.f });
	}
}

TouchEventConsumption MenuButton::Impl::MenuButtonOverlayWidget::Overlay_PointerPress(
	Overlay_PointerPress_Params const& params)
{
	auto& widget = params.widget;
	auto const& rectColl = params.rectColl;
	auto const& rectCollIter = params.rectCollIter;
	auto const& pointer = params.pointer;
	auto const oldEventConsumed = params.eventConsumed;

	DENGINE_IMPL_GUI_ASSERT(widget.sourceMenuButton != nullptr);
	auto& submenu = widget.sourceMenuButton->submenu;

	auto rectCollEntryOpt = rectColl.GetEntry(widget, rectCollIter);
	DENGINE_IMPL_GUI_ASSERT(rectCollEntryOpt.HasValue());
	auto const& rectCollEntry = rectCollEntryOpt.Value();

	auto const* customDataPtr = rectColl.GetCustomData<CustomData>(rectCollEntry);
	DENGINE_IMPL_GUI_ASSERT(customDataPtr != nullptr);
	auto totalLineHeight = customDataPtr->totalLineHeight;

	auto const& rectPair = rectColl.GetRect(rectCollEntry);
	auto const& widgetRect = rectPair.widgetRect;
	auto const& visibleRect = rectPair.visibleRect;

	auto outerRect = Rect::Intersection(widgetRect, visibleRect);
	auto const pointerInside = outerRect.PointIsInside(pointer.pos);

	auto const lineHitOpt = impl::MenuLine_CheckHit(
		widgetRect.position,
		widgetRect.extent.width,
		submenu.lines.size(),
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
				DENGINE_IMPL_GUI_ASSERT(heldLineIndex < submenu.lines.size());
				auto& line = submenu.lines[heldLineIndex];
				if (line.togglable)
					line.toggled = !line.toggled;
				if (line.callback)
					line.callback(line, params.jobQueue);
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
				// A press outside the overlay rect dismisses the menu without committing
				// a line. The event is left unconsumed so the widget underneath can react.
				params.windowHandler.QueueRemoveCurrentLayer();
			}
		}
	}

	returnValue.consumed = returnValue.consumed || pointerInside;
	return returnValue;
}

bool MenuButton::Impl::MenuButtonOverlayWidget::CursorMove(
	CursorMoveParams const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter,
	bool occluded)
{
	DENGINE_IMPL_GUI_ASSERT(sourceMenuButton != nullptr);
	auto& submenu = sourceMenuButton->submenu;
	auto const& rectColl = params.rectCollection;

	auto rectCollEntryOpt = rectColl.GetEntry(*this, rectCollIter);
	DENGINE_IMPL_GUI_ASSERT(rectCollEntryOpt.HasValue());
	auto const& rectCollEntry = rectCollEntryOpt.Value();

	auto const* customDataPtr = rectColl.GetCustomData<CustomData>(rectCollEntry);
	DENGINE_IMPL_GUI_ASSERT(customDataPtr != nullptr);
	auto totalLineHeight = customDataPtr->totalLineHeight;

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
		hoveredLineIndex = impl::MenuLine_CheckHit(
			widgetRect.position,
			widgetRect.extent.width,
			submenu.lines.size(),
			totalLineHeight,
			pointerPos);
	}

	return pointerInside;
}

void MenuButton::Impl::MenuButtonOverlayWidget::CursorExit() {
	hoveredLineIndex = Std::nullOpt;
}

bool MenuButton::Impl::MenuButtonOverlayWidget::CursorPress2(
	CursorPressParams const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter,
	bool consumed)
{
	impl::PointerPress_Pointer pointer = {
		.id = impl::cursorPointerId,
		.pos = { (f32)params.cursorPos.x, (f32)params.cursorPos.y },
		.pressed = params.CursorPressed(),
		.type = impl::ToPointerType(params.CursorButton()), };

	Overlay_PointerPress_Params tempParams = {
		.eventConsumed = consumed,
		.jobQueue = params.jobQueue,
		.pointer = pointer,
		.rectColl = params.rectCollection,
		.rectCollIter = rectCollIter,
		.widget = *this,
		.windowHandler = params.windowHandler, };

	return Overlay_PointerPress(tempParams).consumed;
}

TouchEventConsumption MenuButton::Impl::MenuButtonOverlayWidget::WidgetEvent_TouchPress(
	WidgetEvent_TouchPressParams const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter,
	bool consumed)
{
	impl::PointerPress_Pointer pointer = {
		.id = params.event.id,
		.pos = params.event.position,
		.pressed = params.event.pressed,
		.type = impl::PointerType::Primary, };

	Overlay_PointerPress_Params tempParams = {
		.eventConsumed = consumed,
		.jobQueue = params.jobQueue,
		.pointer = pointer,
		.rectColl = params.rectCollection,
		.rectCollIter = rectCollIter,
		.widget = *this,
		.windowHandler = params.windowHandler, };

	return Overlay_PointerPress(tempParams);
}

bool MenuButton::Impl::MenuButtonOverlayWidget::WidgetEvent_Back(
	WidgetEvent_BackParams const& params,
	Std::Opt<RectCollection::Iter> const&)
{
	// A back request closes the menu without committing any line.
	params.QueueRemoveFrontmostLayer();
	return true;
}

Widget::GetSizeHint2_ReturnT MenuButton::GetSizeHint2(Widget::GetSizeHint2_Params const& params) const
{
	auto& textMgr = params.textEngine;
	auto& pusher = params.pusher;
	auto const& window = params.window;
	auto const& fontScale = params.FontScale();
	auto const& minimumSizeCm = params.MinimumHeightCm();

	auto pusherIt = pusher.AddEntry(*this);
	auto& customData = pusher.AttachCustomData(pusherIt, Impl::CustomData{ pusher.Alloc() });
	if (pusher.IncludeRendering()) {
		customData.glyphRects.Resize(this->title.size());
	}

	MenuButton::Theme theme;
	if (this->getThemeFn)
		theme = this->getThemeFn({ .widget = *this, .appData = params.appData });
	auto textMarginPx = theme.textMargin.ResolvePx(window.dpi, window.contentScale);

	auto normalTextScale = fontScale * window.contentScale;
	auto normalFontFaceId = textMgr.GetFontFaceSizeId(normalTextScale, window.dpi, window.dpi);
	auto normalHeight = textMgr.GetFontFaceSizeMetrics(normalFontFaceId).lineHeight;
	auto normalButtonHeight = normalHeight + 2*textMarginPx;

	auto textOuterExtent = textMgr.GetOuterExtent(
		{ this->title.data(), this->title.size() },
		normalFontFaceId,
		customData.glyphRects.ToSpan());

	customData.fontSizeId = normalFontFaceId;
	customData.textOuterExtent = textOuterExtent;

	SizeHint returnVal = {};
	returnVal.minimum.width = textOuterExtent.width + 2*textMarginPx;
	returnVal.minimum.width = Std::Max(
		returnVal.minimum.width,
		(u32)CmToPixels(minimumSizeCm, window.dpi));
	returnVal.minimum.height = normalButtonHeight;
	returnVal.minimum.height = Std::Max(
		returnVal.minimum.height,
		(u32)CmToPixels(minimumSizeCm, window.dpi));

	pusher.SetSizeHint(pusherIt, returnVal, false);

	return {
		.iter = pusherIt,
		.sizeHint = returnVal };
}

void MenuButton::BuildChildRects(
	BuildChildRects_Params const& params,
	Rect const& widgetRect,
	Rect const& visibleRect,
	Std::Opt<RectCollection::Iter> const& rectCollIter) const
{
}

void MenuButton::Render2(
	Render_Params const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter) const
{
	auto& drawInfo = params.drawEngine;
	auto& rectColl = params.rectCollection;

	auto rectPairOpt = rectColl.GetRect(*this, rectCollIter);
	DENGINE_IMPL_GUI_ASSERT(rectPairOpt.Has());
	auto const& absWidgetRect = rectPairOpt.Value().widgetRect;
	auto const& absVisibleRect = rectPairOpt.Value().visibleRect;

	auto const intersection = Rect::Intersection({ absWidgetRect, absVisibleRect });
	if (intersection.IsNothing())
		return;

	DENGINE_IMPL_GUI_ASSERT(rectColl.BuiltForRendering());

	auto* customDataPtr = rectColl.GetCustomData<Impl::CustomData>(*this);
	DENGINE_IMPL_GUI_ASSERT(customDataPtr);
	auto& customData = *customDataPtr;
	DENGINE_IMPL_GUI_ASSERT(this->title.size() == customData.glyphRects.Size());

	if (this->title.size() == 0)
		return;

	auto fontSizeId = customData.fontSizeId;

	auto centeringOffset = Extent::CenteringOffset(absWidgetRect.extent, customData.textOuterExtent);
	centeringOffset.x = Std::Max(centeringOffset.x, 0);
	centeringOffset.y = Std::Max(centeringOffset.y, 0);
	auto textRect = Rect{ absWidgetRect.position + centeringOffset, customData.textOuterExtent };

	if (textRect.GetIntersect(absVisibleRect).IsNothing())
		return;

	auto scissor = DrawEngine::ScopedScissor(drawInfo, textRect, absWidgetRect);
	drawInfo.PushText(
		fontSizeId,
		{ title.data(), title.size() },
		customData.glyphRects.Data(),
		textRect.position,
		{ 1.f, 1.f, 1.f, 1.f });
}

void MenuButton::AccessibilityTest(
	AccessibilityTest_Params const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter) const
{
	auto& pusher = params.pusher;
	auto& rectColl = params.rectColl;

	auto rectPairOpt = rectColl.GetRect(*this, rectCollIter);
	DENGINE_IMPL_GUI_ASSERT(rectPairOpt.Has());
	auto const& absWidgetRect = rectPairOpt.Value().widgetRect;
	auto const& absVisibleRect = rectPairOpt.Value().visibleRect;

	AccessibilityInfoElement element = {};
	element.textStart = pusher.PushText({ this->title.data(), this->title.size() });
	element.textCount = (int)this->title.size();
	element.rect = Rect::Intersection({ absWidgetRect, absVisibleRect });
	element.isClickable = true;
	pusher.PushElement(reinterpret_cast<uSize>(this), element);
}

void MenuButton::CursorExit() {
	hoveredByCursor = false;
}

bool MenuButton::CursorMove(
	Widget::CursorMoveParams const& params,
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

bool MenuButton::CursorPress2(
	Widget::CursorPressParams const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter,
	bool consumed)
{
	impl::PointerPress_Pointer pointer = {
		.id = impl::cursorPointerId,
		.pos = { (f32)params.cursorPos.x, (f32)params.cursorPos.y },
		.pressed = params.CursorPressed(),
		.type = impl::ToPointerType(params.CursorButton()), };

	Impl::Menu_PointerPress_Params tempParams = {
		.appData = params.customData.ToConst(),
		.eventConsumed = consumed,
		.pointer = pointer,
		.rectColl = params.rectCollection,
		.rectCollIter = rectCollIter,
		.textEngine = params.textEngine,
		.widget = *this,
		.window = params.window,
		.windowHandler = params.windowHandler, };

	return Impl::Menu_PointerPress(tempParams).consumed;
}

TouchEventConsumption MenuButton::WidgetEvent_TouchMove(
	WidgetEvent_TouchMoveParams const& params,
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

	auto const pointerInside =
		widgetRect.PointIsInside(params.event.position) &&
		visibleRect.PointIsInside(params.event.position);

	return { .consumed = pointerInside && !occluded };
}

TouchEventConsumption MenuButton::WidgetEvent_TouchPress(
	WidgetEvent_TouchPressParams const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter,
	bool consumed)
{
	impl::PointerPress_Pointer pointer = {
		.id = params.event.id,
		.pos = params.event.position,
		.pressed = params.event.pressed,
		.type = impl::PointerType::Primary, };

	Impl::Menu_PointerPress_Params tempParams = {
		.appData = params.customData.ToConst(),
		.eventConsumed = consumed,
		.pointer = pointer,
		.rectColl = params.rectCollection,
		.rectCollIter = rectCollIter,
		.textEngine = params.textEngine,
		.widget = *this,
		.window = params.window,
		.windowHandler = params.windowHandler, };

	return Impl::Menu_PointerPress(tempParams);
}
