#include <DEngine/Gui/MenuButton.hpp>
#include <DEngine/Gui/TextManager.hpp>
#include <DEngine/Gui/Layer.hpp>
#include <DEngine/Gui/Context.hpp>


#include <DEngine/Std/Containers/Defer.hpp>
#include <DEngine/Std/Containers/Vec.hpp>
#include <DEngine/Std/Containers/FnRef.hpp>

using namespace DEngine;
using namespace DEngine::Gui;
using Line = MenuButton::Line;
using LineAny = MenuButton::LineAny;
using Submenu = MenuButton::Submenu;


namespace DEngine::Gui::impl {
	class MenuLayer : public Layer {
	public:
		struct CustomData {
			CustomData(RectCollection::AllocRefT alloc) :
				sizeHintExtents{ alloc } {}

			FontFaceSizeId fontSizeId = FontFaceSizeId::Invalid;
			u32 marginAmount = 0;
			Std::Vec<Extent, RectCollection::AllocRefT> sizeHintExtents;
		};

		MenuButton* _menuButton = nullptr;
		[[nodiscard]] MenuButton& GetMenuBtn() { return *_menuButton; }
		[[nodiscard]] MenuButton const& GetMenuBtn() const { return *_menuButton; }

		virtual void BuildSizeHints(BuildSizeHints_Params const& params) const override;
		virtual void BuildRects(BuildRects_Params const& params) const override;
		virtual void Render(Render_Params const& params) const override;

		virtual bool CursorMove(
			CursorMoveParams const& params,
			bool occluded) override;

		virtual Press_Return CursorPress(
			CursorPressParams const& params,
			bool eventConsumed) override;

		virtual bool TouchMove2(
			TouchMoveParams const& params,
			bool occluded) override;

		virtual Press_Return TouchPress2(
			TouchPressParams const& params,
			bool consumed) override;
	};

	enum class PointerType : u8 { Primary, Secondary };
	[[nodiscard]] static PointerType ToPointerType(CursorButton in) noexcept
	{
		switch (in)
		{
			case CursorButton::Primary: return PointerType::Primary;
			case CursorButton::Secondary: return PointerType::Secondary;
			default: break;
		}
		DENGINE_IMPL_UNREACHABLE();
		return {};
	}

	static constexpr u8 cursorPointerId = (u8)-1;

	struct PointerPress_Pointer {
		u8 id;
		PointerType type;
		Math::Vec2 pos;
		bool pressed;
		bool consumed;
	};

	struct PointerMove_Pointer {
		u8 id;
		Math::Vec2 pos;
		bool occluded;
	};

	struct MenuBtnImpl {
		// This data is only available when rendering.
		struct CustomData {
			explicit CustomData(RectCollection::AllocRefT const& alloc) :
				glyphRects{ alloc } {}

			Extent textOuterExtent = {};
			FontFaceSizeId fontSizeId = FontFaceSizeId::Invalid;
			Std::Vec<Rect, RectCollection::AllocRefT> glyphRects;
		};

		struct MenuBtn_PointerMove_Params {
			MenuButton& widget;
			Rect widgetRect;
			Rect visibleRect;
			PointerMove_Pointer const& pointer;
		};
		[[nodiscard]] static bool MenuBtn_PointerMove(
			MenuBtn_PointerMove_Params const& params);

		struct MenuBtn_PointerPress_Params {
			MenuButton& widget;
			Context& ctx;
			WindowID const& windowId;
			Rect const& widgetRect;
			Rect const& visibleRect;
			PointerPress_Pointer const& pointer;
		};
		[[nodiscard]] static bool MenuBtn_PointerPress(
			MenuBtn_PointerPress_Params const& params);

		static void MenuBtn_SpawnSubmenuLayer(
			MenuButton& widget,
			Context& ctx,
			WindowID windowId,
			Rect widgetRect);


		struct MenuLayer_PointerPress_Params {
			MenuLayer& layer;
			Context& ctx;
			RectCollection const& rectCollection;
			MenuLayer::CustomData const& layerCustomData;
			TextManager& textManager;
			PointerPress_Pointer const& pointer;
			Rect usableRect = {};
			Std::FnRef<bool(Widget&, Rect const&, Rect const&, bool)> childDispatch;
		};
		// Common event handler for both touch and cursor
		[[nodiscard]] static Layer::Press_Return MenuLayer_PointerPress(
			MenuButton::Submenu& submenu,
			MenuLayer_PointerPress_Params const& params);


		struct MenuLayer_PointerMove_Params {
			MenuLayer& layer;
			TextManager& textManager;
			RectCollection const& rectCollection;
			u32 margin = 0;
			Rect windowRect = {};
			Rect usableRect = {};
			PointerMove_Pointer const& pointer;
			Std::FnRef<void(Widget&, Rect const&, Rect const&, bool)> childDispatch;
		};
		[[nodiscard]] static bool MenuLayer_PointerMove(
			MenuButton::Submenu& submenu,
			MenuLayer_PointerMove_Params const& params);
	};

	[[nodiscard]] static Math::Vec2Int MenuLayer_GetTopSubmenuPos(
		MenuLayer const& layer,
		RectCollection const& rectCollection)
	{
		auto const* rectPairPtr = rectCollection.GetRect(layer.GetMenuBtn());
		DENGINE_IMPL_GUI_ASSERT(rectPairPtr);
		Math::Vec2Int pos = rectPairPtr->widgetRect.position;
		pos.y = rectPairPtr->widgetRect.Bottom();
		return pos;
	}

	[[nodiscard]] static Math::Vec2Int MenuLayer_GetTopSubmenuPos(
		MenuLayer const& layer,
		RectCollection::RectPusher const& rectCollection)
	{
		auto const* rectPairPtr = rectCollection.RectPair(layer.GetMenuBtn());
		DENGINE_IMPL_GUI_ASSERT(rectPairPtr);
		Math::Vec2Int pos = rectPairPtr->widgetRect.position;
		pos.y = rectPairPtr->widgetRect.Bottom();
		return pos;
	}

	[[nodiscard]] static Rect Submenu_BuildRectOuter(
		TextManager& textManager,
		MenuButton::Submenu const& submenu,
		u32 margin,
		Math::Vec2Int desiredPos,
		Rect const& usableRect);

	[[nodiscard]] static Rect Submenu_BuildRectOuter(
		Std::Span<Extent const> const& extents,
		u32 margin,
		Math::Vec2Int desiredPos,
		Rect const& usableRect)
	{
		Rect outRect = {};
		outRect.extent = Extent::StackVertically(extents);
		outRect.position = usableRect.ClampPoint(desiredPos);
		return outRect;
	}

	[[nodiscard]] static Rect Submenu_BuildRectInner(
		Rect outerRect,
		u32 margin) noexcept;

	struct MenuLayer_RenderSubmenu_Params
	{
		MenuButton::Submenu const& submenu;
		TextManager& textManager;
		u32 margin {};
		FontFaceSizeId const& fontSizeId;
		Math::Vec4 bgColor {};
		Rect submenuRect {};
		Rect visibleRect {};
		Std::Span<Extent const> lineExtents;
		Widget::Render_Params const& widgetParams;
		DrawInfo& drawInfo;
	};
	static void MenuLayer_RenderSubmenu(
		MenuLayer_RenderSubmenu_Params const& params);

	[[nodiscard]] static Math::Vec2Int BuildTextOffset(Extent const& widgetExtent, Extent const& textExtent) noexcept {
		Math::Vec2Int out = {};
		for (int i = 0; i < 2; i++) {
			auto temp = (i32)Math::Round((f32)widgetExtent[i] * 0.5f - (f32)textExtent[i] * 0.5f);
			temp = Math::Max(0, temp);
			out[i] = temp;
		}

		out.x = 0.f;
		return out;
	}

	struct Submenu_LineIt_End {};
	struct Submenu_LineIt {
		Std::Span<Extent const> lineExtents;
		Math::Vec2Int position;
		uSize currIndex = 0;

		bool operator==(Submenu_LineIt_End const&) const {
			return currIndex == lineExtents.Size();
		}
		void operator++() {
			currIndex += 1;
		}
		struct ReturnT {
			Rect lineRect = {};
			uSize index = 0;
		};
		auto operator*() const {
			ReturnT temp = {};
			temp.index = currIndex;

			for (auto const& item : lineExtents)
				temp.lineRect.extent.width = Max(temp.lineRect.extent.width, item.width);
			temp.lineRect.extent.height = lineExtents[currIndex].height;

			temp.lineRect.position = position;
			// Offset the position
			u32 sumHeight = 0;
			for (int i = 0; i < currIndex; i++)
				sumHeight += lineExtents[i].height;
			temp.lineRect.position.y += (i32)sumHeight;

			return temp;
		}
	};
	struct Submenu_LineItPair {
		Std::Span<Extent const> lineExtents = {};
		Math::Vec2Int position = {};
		[[nodiscard]] auto begin() const {
			Submenu_LineIt it = {};
			it.lineExtents = lineExtents;
			it.position = position;
			return it;
		}
		[[nodiscard]] static auto end() { return Submenu_LineIt_End{}; }
	};

	[[nodiscard]] auto IterateLines(
		Std::Span<Extent const> const& lineExtents,
		Math::Vec2Int position)
	{
		return Submenu_LineItPair {
			.lineExtents = lineExtents,
			.position = position, };
	}

	[[nodiscard]] Std::Opt<uSize> Submenu_CheckHitLine(
		Std::Span<Extent const> const& lineExtents,
		Math::Vec2Int position,
		Math::Vec2 point)
	{
		for (auto const& line : IterateLines(lineExtents, position)) {
			if (line.lineRect.PointIsInside(point))
				return line.index;
		}
		return Std::nullOpt;
	}
}

Layer::Press_Return Gui::impl::MenuBtnImpl::MenuLayer_PointerPress(
	MenuButton::Submenu& submenu,
	MenuLayer_PointerPress_Params const& params)
{
	auto& ctx = params.ctx;
	auto& layer = params.layer;
	auto& textManager = params.textManager;
	auto const& childDispatch = params.childDispatch;
	auto const& rectCollection = params.rectCollection;
	auto const& pointer = params.pointer;
	auto const& safeArea = params.usableRect;
	auto const& layerCustomData = params.layerCustomData;
	auto marginAmount = layerCustomData.marginAmount;


	Layer::Press_Return returnValue = {};
	returnValue.eventConsumed = params.pointer.consumed;

	Std::Defer deferredCleanup = [&] {
		if (returnValue.destroyLayer) {
			submenu.pressedLineOpt = Std::nullOpt;
			submenu.activeSubmenu = Std::nullOpt;
			submenu.cursorHoveredLineOpt = Std::nullOpt;
		}
	};

	auto entry = rectCollection.GetEntry(layer).Get();
	auto const& customData =
		*rectCollection.GetCustomData2<MenuLayer::CustomData>(entry);
	auto const& lineExtents = customData.sizeHintExtents.ToSpan();

	auto const desiredPos = MenuLayer_GetTopSubmenuPos(layer, rectCollection);

	auto submenuRect = Submenu_BuildRectOuter(
		lineExtents,
		marginAmount,
		desiredPos,
		safeArea);
	auto pointerInsideSubmenu = submenuRect.PointIsInside(pointer.pos);
	auto lineHitOpt = Submenu_CheckHitLine(lineExtents, submenuRect.position, pointer.pos);


	auto checkForStartPressing =
		!returnValue.eventConsumed &&
		pointer.pressed &&
		!submenu.pressedLineOpt.Has() &&
		lineHitOpt.Has();
	if (checkForStartPressing)
	{
		auto lineHit = lineHitOpt.Get();
		// If this is an Any line, it gets handled later in the code.
		if (submenu.lines[lineHit].IsA<Line>()) {
			Submenu::PressedLineData pressedLineData = {};
			pressedLineData.lineIndex = lineHitOpt.Get();
			pressedLineData.pointerId = pointer.id;
			submenu.pressedLineOpt = pressedLineData;
			returnValue.eventConsumed = true;
		}
	}

	if (submenu.pressedLineOpt.Has() &&
		!pointer.pressed)
	{
		auto pressedLine = submenu.pressedLineOpt.Get();
		if (!returnValue.eventConsumed &&
			lineHitOpt.Has())
		{
			auto lineHit = lineHitOpt.Get();
			if (pressedLine.pointerId == pointer.id &&
				pressedLine.lineIndex == lineHit)
			{
				auto* ptr = submenu.lines[pressedLine.lineIndex].ToPtr<Line>();
				DENGINE_IMPL_ASSERT(ptr);
				auto& line = *ptr;
				if (line.togglable)
					line.toggled = !line.toggled;
				if (line.callback) {
					line.callback(line, ctx);
				}
				returnValue.destroyLayer = true;
			}
			returnValue.eventConsumed = true;
		}

		if (pressedLine.pointerId == pointer.id)
			submenu.pressedLineOpt.Clear();
	}

	// Dispatch the event to children.
	for (auto const& it : IterateLines(lineExtents, submenuRect.position)) {
		if (auto ptr = submenu.lines[it.index].ToPtr<LineAny>()) {
			auto temp = childDispatch.Invoke(
				*ptr->widget,
				it.lineRect,
				safeArea,
				returnValue.eventConsumed);
			returnValue.eventConsumed = returnValue.eventConsumed || temp;
		}
	}

	bool destroyLayer =
		!returnValue.eventConsumed &&
		pointer.pressed &&
		!pointerInsideSubmenu;
	if (destroyLayer) {
		returnValue.destroyLayer = true;
	}

	returnValue.eventConsumed = returnValue.eventConsumed || pointerInsideSubmenu;

	return returnValue;
}

bool Gui::impl::MenuBtnImpl::MenuLayer_PointerMove(
	MenuButton::Submenu& submenu,
	MenuLayer_PointerMove_Params const& params)
{
	auto& layer = params.layer;
	auto const& rectColl = params.rectCollection;
	auto const& pointer = params.pointer;
	auto const& margin = params.margin;
	auto const& safeArea = params.usableRect;
	auto const& childDispatch = params.childDispatch;

	auto entry = rectColl.GetEntry(layer).Get();

	auto* customDataPtr =
		rectColl.GetCustomData2<MenuLayer::CustomData>(entry);
	DENGINE_IMPL_GUI_ASSERT(customDataPtr);
	auto const& customData = *customDataPtr;
	auto const& lineExtents = customData.sizeHintExtents.ToSpan();

	auto const desiredPos = MenuLayer_GetTopSubmenuPos(layer, rectColl);

	if (pointer.occluded)
		submenu.cursorHoveredLineOpt = Std::nullOpt;

	auto const outerRect = Submenu_BuildRectOuter(
		lineExtents,
		margin,
		desiredPos,
		safeArea);

	if (pointer.id == impl::cursorPointerId) {
		submenu.cursorHoveredLineOpt =
			Submenu_CheckHitLine(lineExtents, outerRect.position, pointer.pos);
	}


	for (auto const& it : IterateLines(lineExtents, outerRect.position)) {
		auto& tempLine = submenu.lines[it.index];

		if (auto ptr = tempLine.ToPtr<LineAny>()) {
			childDispatch.Invoke(
				*ptr->widget,
				it.lineRect,
				safeArea,
				pointer.occluded);
		}
	}

	return false;
}

Rect Gui::impl::Submenu_BuildRectInner(
	Rect outerRect,
	u32 margin) noexcept
{
	Rect returnVal = outerRect.Reduce(margin);
	return returnVal;
}

namespace DEngine::Gui::impl {
	void Submenu_DrawHighlightsForToggledLines(
		Submenu const& submenu,
		Std::Span<Extent const> lineExtents,
		Math::Vec2Int position,
		DrawInfo& drawInfo)
	{
		for (auto const& it : impl::IterateLines(lineExtents, position)) {
			auto i = it.index;

			bool drawHighlight = false;
			if (auto linePtr = submenu.lines[it.index].ToPtr<Line>()) {
				auto const& line = *linePtr;
				if (auto temp = submenu.cursorHoveredLineOpt.ToPtr(); temp && *temp == i) {
					drawHighlight = true;
				} else {
					if (line.togglable && line.toggled) {
						drawHighlight = true;
					} else if (submenu.pressedLineOpt.HasValue()) {
						if (i == submenu.pressedLineOpt.Value().lineIndex)
							drawHighlight = true;
					}
				}
			}
			if (drawHighlight) {
				drawInfo.PushFilledQuad(it.lineRect, { 1.f, 1.f, 1.f, 0.25f });
			}
		}
	}

	void Submenu_DrawLines(
		Submenu const& submenu,
		Std::Span<Extent const> lineExtents,
		Math::Vec2Int position,
		Rect const& visibleRect,
		FontFaceSizeId fontSizeId,
		u32 margin,
		Widget::Render_Params const& renderParams,
		TextManager& textManager,
		DrawInfo& drawInfo)
	{
		for (auto const& it : IterateLines(lineExtents, position)) {
			auto const& tempLine = submenu.lines[it.index];

			if (auto ptr = tempLine.ToPtr<Line>()) {
				Math::Vec4 textColor = { 1.f, 1.f, 1.f, 1.f };
				textManager.RenderText(
					{ ptr->title.data(), ptr->title.size() },
					textColor,
					it.lineRect.Reduce(margin),
					fontSizeId,
					drawInfo);
			}
			if (auto ptr = tempLine.ToPtr<LineAny>()) {
				ptr->widget->Render2(
					renderParams,
					it.lineRect,
					visibleRect);
			}
		}
	}
}

void Gui::impl::MenuLayer_RenderSubmenu(
	MenuLayer_RenderSubmenu_Params const& params)
{
	auto& submenu = params.submenu;
	auto const& submenuRect = params.submenuRect;
	auto const& visibleRect = params.visibleRect;
	auto const& margin = params.margin;
	auto const& lineExtents = params.lineExtents;
	auto& textManager = params.textManager;
	auto& drawInfo = params.drawInfo;
	auto fontSizeId = params.fontSizeId;

	auto const intersection = Rect::Intersection(submenuRect, visibleRect);
	if (intersection.IsNothing())
		return;

	// Draw the background
	drawInfo.PushFilledQuad(submenuRect, params.bgColor);

	Submenu_DrawHighlightsForToggledLines(
		submenu,
		lineExtents,
		submenuRect.position,
		drawInfo);

	Submenu_DrawLines(
		submenu,
		lineExtents,
		submenuRect.position,
		visibleRect,
		fontSizeId,
		margin,
		params.widgetParams,
		textManager,
		drawInfo);
}

namespace DEngine::Gui::impl {
	// Kinda shit function, it does too much.
	auto GetChildSizeHints_GatherExtents(
		Std::Span<Std::Variant<Line, LineAny> const> const& lines,
		TextManager& textManager,
		FontFaceSizeId fontSizeId,
		u32 marginAmount,
		Widget::GetSizeHint2_Params const& widgetParams,
		AllocRef transientAlloc)
	{
		auto lineExtents = Std::NewVec<Extent>(transientAlloc);
		lineExtents.Reserve(lines.Size());

		for (auto const& line : lines) {
			Extent lineExtent = {};
			if (auto linePtr = line.ToPtr<Line>()) {
				lineExtent = textManager.GetOuterExtent(
					{ linePtr->title.data(), linePtr->title.size() },
					fontSizeId);
				lineExtent.AddPadding(marginAmount);
			}
			// else
			if (auto ptr = line.ToPtr<LineAny>())
				lineExtent = ptr->widget->GetSizeHint2(widgetParams).minimum;
			lineExtents.PushBack(lineExtent);
		}
		return lineExtents;
	}
}

void Gui::impl::MenuLayer::BuildSizeHints(
	Layer::BuildSizeHints_Params const& params) const
{
	auto const& ctx = params.ctx;
	auto& textMgr = params.textManager;
	auto const& window = params.window;
	auto const& transientAlloc = params.transientAlloc;
	auto& pusher = params.pusher;
	auto& menuBtn = GetMenuBtn();
	auto const& lineSpacing = menuBtn.spacing;
	auto const& submenu = menuBtn.submenu;

	auto entry = pusher.AddEntry(*this);
	auto& customData = pusher.AttachCustomData(entry, MenuLayer::CustomData{ pusher.Alloc() });

	Widget::GetSizeHint2_Params widgetParams = {
		.ctx = ctx,
		.window = window,
		.textManager = textMgr,
		.transientAlloc = transientAlloc,
		.pusher = pusher,
	};

	auto normalTextScale = ctx.fontScale * window.contentScale;
	auto fontSizeId = FontFaceSizeId::Invalid;
	auto marginAmount = 0;
	{
		auto normalHeight = textMgr.GetLineheight(normalTextScale, window.dpiX, window.dpiY);
		auto normalHeightMargin = (u32)Math::Round((f32)normalHeight * ctx.defaultMarginFactor);
		normalHeight += 2 * normalHeightMargin;

		auto minHeight = CmToPixels(ctx.minimumHeightCm, window.dpiY);

		if (normalHeight > minHeight) {
			fontSizeId = textMgr.GetFontFaceSizeId(normalTextScale, window.dpiX, window.dpiY);
			marginAmount = (i32)normalHeightMargin;
		} else {
			// We can't just do minHeight * defaultMarginFactor for this one, because defaultMarginFactor applies to
			// content, not the outer size. So we set up an equation `height = 2 * marginFactor * content + content`
			// and we solve for content.
			auto contentSizeCm = ctx.minimumHeightCm / ((2 * ctx.defaultMarginFactor) + 1.f);
			auto contentSize = CmToPixels(contentSizeCm, window.dpiY);
			fontSizeId = textMgr.FontFaceSizeIdForLinePixelHeight(
				contentSize,
				TextHeightType::Alphas);
			marginAmount = CmToPixels((f32)contentSizeCm * ctx.defaultMarginFactor, window.dpiY);
		}
	}
	customData.fontSizeId = fontSizeId;
	customData.marginAmount = marginAmount;

	auto lineExtents = GetChildSizeHints_GatherExtents(
		{ submenu.lines.data(), submenu.lines.size() },
		textMgr,
		fontSizeId,
		marginAmount,
		widgetParams,
		transientAlloc);

	// Add margin to outside all lines
	auto extent = Extent::StackVertically(lineExtents.ToSpan());

	SizeHint returnVal = {};
	returnVal.minimum = extent;


	for (auto const& item : lineExtents)
		customData.sizeHintExtents.PushBack(item);

	pusher.SetSizeHint(entry, returnVal);
}

void Gui::impl::MenuLayer::BuildRects(
	Layer::BuildRects_Params const& params) const
{
	auto& layer = *this;
	auto& pusher = params.pusher;
	auto const& windowRect = params.windowRect;
	auto const& safeAreaRect = params.visibleRect;
	auto& menuBtn = GetMenuBtn();

	auto entry = pusher.GetEntry(*this);
	auto const* customDataPtr = pusher.GetCustomData2<MenuLayer::CustomData>(entry);
	DENGINE_IMPL_GUI_ASSERT(customDataPtr != nullptr);
	auto const& customData = *customDataPtr;
	auto marginAmount = customData.marginAmount;

	auto const& lineExtents = customData.sizeHintExtents.ToSpan();

	auto desiredPos = MenuLayer_GetTopSubmenuPos(layer, pusher);

	auto const safeIntersection = Rect::Intersection(windowRect, safeAreaRect);

	Widget::BuildChildRects_Params widgetParams = {
		.ctx = params.ctx,
		.window = params.window,
		.textManager = params.textManager,
		.transientAlloc = params.transientAlloc,
		.pusher = params.pusher,
	};

	auto const& submenu = menuBtn.submenu;

	auto outerRect = Submenu_BuildRectOuter(
		customData.sizeHintExtents.ToSpan(),
		marginAmount,
		desiredPos,
		safeIntersection);
	pusher.SetRectPair(entry, { outerRect, safeIntersection });

	for (auto const& it : impl::IterateLines(lineExtents, outerRect.position)) {
		if (auto ptr = submenu.lines[it.index].ToPtr<LineAny>()) {
			ptr->widget->BuildChildRects(
				widgetParams,
				it.lineRect,
				safeIntersection);
		}
	}
}

void Gui::impl::MenuLayer::Render(
	Render_Params const& params) const
{
	auto& layer = *this;
	auto const& ctx = params.ctx;
	auto& menuBtn = GetMenuBtn();
	auto const& submenu = menuBtn.submenu;
	auto const& rectColl = params.rectCollection;
	auto const& window = params.window;
	auto& textManager = params.textManager;
	auto& drawInfo = params.drawInfo;
	auto const& windowRect = params.windowRect;
	auto const& safeAreaRect = params.safeAreaRect;

	auto const intersection = Rect::Intersection(windowRect, safeAreaRect);
	if (intersection.IsNothing())
		return;

	auto const entry = rectColl.GetEntry(*this).Get();
	auto const* customDataPtr = rectColl.GetCustomData2<MenuLayer::CustomData>(entry);
	DENGINE_IMPL_GUI_ASSERT(customDataPtr != nullptr);
	auto const& customData = *customDataPtr;
	auto fontSizeId = customData.fontSizeId;
	auto marginAmount = customData.marginAmount;
	auto lineExtents = customData.sizeHintExtents.ToSpan();

	auto desiredPos = MenuLayer_GetTopSubmenuPos(layer, rectColl);

	auto const submenuRect = impl::Submenu_BuildRectOuter(
		lineExtents,
		marginAmount,
		desiredPos,
		safeAreaRect);

	Widget::Render_Params widgetParams = {
		.ctx = ctx,
		.window = window,
		.textManager = textManager,
		.rectCollection = rectColl,
		.framebufferExtent = drawInfo.GetFramebufferExtent(),
		.transientAlloc = params.transientAlloc,
		.drawInfo = drawInfo,
	};

	impl::MenuLayer_RenderSubmenu_Params tempParams = {
		.submenu = submenu,
		.textManager = textManager,
		.margin = marginAmount,
		.fontSizeId = fontSizeId,
		.bgColor = menuBtn.colors.active,
		.submenuRect = submenuRect,
		.visibleRect = safeAreaRect,
		.lineExtents = lineExtents,
		.widgetParams = widgetParams,
		.drawInfo = params.drawInfo, };
	impl::MenuLayer_RenderSubmenu(tempParams);
}

bool Gui::impl::MenuLayer::CursorMove(
	CursorMoveParams const& params,
	bool occluded)
{
	auto& menuBtn = GetMenuBtn();
	auto& ctx = params.ctx;
	auto& window = params.window;
	auto& event = params.event;
	auto& rectColl = params.rectCollection;
	auto& textManager = params.textManager;
	auto& transientAlloc = params.transientAlloc;

	impl::PointerMove_Pointer pointer = {};
	pointer.id = impl::cursorPointerId;
	pointer.pos = { (f32)event.position.x, (f32)event.position.y };
	pointer.occluded = occluded;

	Widget::CursorMoveParams widgetParams = {
		.ctx = ctx,
		.window = window,
		.rectCollection = rectColl,
		.textManager = textManager,
		.transientAlloc = transientAlloc,
		.event = event,
	};
	auto childDispatch = [&widgetParams](
		Widget& widget,
		Rect const& widgetRect,
		Rect const& visibleRect,
		bool occluded)
	{
		widget.CursorMove(
			widgetParams,
			widgetRect,
			visibleRect,
			occluded);
	};

	impl::MenuBtnImpl::MenuLayer_PointerMove_Params tempParams = {
		.layer = *this,
		.textManager = params.textManager,
		.rectCollection = params.rectCollection,
		.windowRect = params.windowRect,
		.usableRect = params.safeAreaRect,
		.pointer = pointer,
		.childDispatch = childDispatch };

	return impl::MenuBtnImpl::MenuLayer_PointerMove(
		menuBtn.submenu,
		tempParams);
}

Layer::Press_Return Gui::impl::MenuLayer::CursorPress(
	Layer::CursorPressParams const& params,
	bool eventConsumed)
{
	auto& menuBtn = GetMenuBtn();
	auto& ctx = params.ctx;
	auto& window = params.window;
	auto& event = params.event;
	auto& textManager = params.textManager;
	auto& rectColl = params.rectCollection;
	auto& cursorPos = params.cursorPos;
	auto& transientAlloc = params.transientAlloc;

	auto const* customDataPtr = rectColl.GetCustomData2<MenuLayer::CustomData>(*this);
	DENGINE_IMPL_GUI_ASSERT(customDataPtr != nullptr);
	auto const& customData = *customDataPtr;

	impl::PointerPress_Pointer pointer = {};
	pointer.id = impl::cursorPointerId;
	pointer.pos = { (f32)cursorPos.x, (f32)cursorPos.y };
	pointer.type = impl::ToPointerType(event.button);
	pointer.pressed = event.pressed;
	pointer.consumed = eventConsumed;

	Widget::CursorPressParams widgetParams = {
		.ctx = ctx,
		.window = window,
		.rectCollection = rectColl,
		.textManager = textManager,
		.transientAlloc = transientAlloc,
		.cursorPos = cursorPos,
		.event = event, };
	auto childDispatch = [&widgetParams](
		Widget& widget,
		Rect const& widgetRect,
		Rect const& visibleRect,
		bool consumed)
			-> bool
	{
		return widget.CursorPress2(
			widgetParams,
			widgetRect,
			visibleRect,
			consumed);
	};

	impl::MenuBtnImpl::MenuLayer_PointerPress_Params tempParams = {
		.layer = *this,
		.ctx = ctx,
		.rectCollection = rectColl,
		.layerCustomData = customData,
		.textManager = params.textManager,
		.pointer = pointer,
		.usableRect = params.safeAreaRect,
		.childDispatch = childDispatch, };

	return impl::MenuBtnImpl::MenuLayer_PointerPress(
		menuBtn.submenu,
		tempParams);
}

bool Gui::impl::MenuLayer::TouchMove2(
	TouchMoveParams const& params,
	bool occluded)
{
	auto& menuBtn = GetMenuBtn();
	auto& ctx = params.ctx;
	auto& window = params.window;
	auto& event = params.event;
	auto& rectColl = params.rectCollection;
	auto& textManager = params.textManager;
	auto& transientAlloc = params.transientAlloc;

	impl::PointerMove_Pointer pointer = {};
	pointer.id = params.event.id;
	pointer.pos = params.event.position;
	pointer.occluded = occluded;

	TextSizeInfo textSize = {
		.scale = ctx.fontScale * window.contentScale,
		.dpiX = window.dpiX,
		.dpiY = window.dpiY };

	Widget::TouchMoveParams widgetParams = {
		.ctx = ctx,
		.window = window,
		.rectCollection = rectColl,
		.textManager = textManager,
		.transientAlloc = transientAlloc,
		.event = event,
	};
	auto childDispatch = [&widgetParams](
		Widget& widget,
		Rect const& widgetRect,
		Rect const& visibleRect,
		bool occluded)
	{
		widget.TouchMove2(
			widgetParams,
			widgetRect,
			visibleRect,
			occluded);
	};

	impl::MenuBtnImpl::MenuLayer_PointerMove_Params tempParams = {
		.layer = *this,
		.textManager = params.textManager,
		.rectCollection = params.rectCollection,
		.windowRect = params.windowRect,
		.usableRect = params.safeAreaRect,
		.pointer = pointer,
		.childDispatch = childDispatch };

	return impl::MenuBtnImpl::MenuLayer_PointerMove(
		menuBtn.submenu,
		tempParams);
}

Layer::Press_Return Gui::impl::MenuLayer::TouchPress2(
	TouchPressParams const& params,
	bool consumed)
{
	auto& menuBtn = GetMenuBtn();
	auto& ctx = params.ctx;
	auto& window = params.window;
	auto& event = params.event;
	auto& rectColl = params.rectCollection;
	auto& textManager = params.textManager;
	auto& transientAlloc = params.transientAlloc;

	auto const* customDataPtr = rectColl.GetCustomData2<MenuLayer::CustomData>(*this);
	DENGINE_IMPL_GUI_ASSERT(customDataPtr != nullptr);
	auto const& customData = *customDataPtr;

	impl::PointerPress_Pointer pointer = {};
	pointer.id = event.id;
	pointer.pos = event.position;
	pointer.type = PointerType::Primary;
	pointer.pressed = event.pressed;
	pointer.consumed = consumed;

	Widget::TouchPressParams widgetParams = {
		.ctx = ctx,
		.window = window,
		.rectCollection = rectColl,
		.textManager = textManager,
		.transientAlloc = transientAlloc,
		.event = event, };
	auto childDispatch = [&widgetParams](
		Widget& widget,
		Rect const& widgetRect,
		Rect const& visibleRect,
		bool consumed)
		-> bool
	{
		return widget.TouchPress2(
			widgetParams,
			widgetRect,
			visibleRect,
			consumed);
	};

	impl::MenuBtnImpl::MenuLayer_PointerPress_Params tempParams = {
		.layer = *this,
		.ctx = ctx,
		.rectCollection = rectColl,
		.layerCustomData = customData,
		.textManager = textManager,
		.pointer = pointer,
		.usableRect = params.safeAreaRect,
		.childDispatch = childDispatch, };

	return impl::MenuBtnImpl::MenuLayer_PointerPress(
		menuBtn.submenu,
		tempParams);
}

bool Gui::impl::MenuBtnImpl::MenuBtn_PointerMove(
	MenuBtn_PointerMove_Params const& params)
{
	auto& widget = params.widget;
	auto const& widgetRect = params.widgetRect;
	auto const& visibleRect = params.visibleRect;
	auto const& pointer = params.pointer;

	auto const pointerInside =
		widgetRect.PointIsInside(pointer.pos) &&
		visibleRect.PointIsInside(pointer.pos);

	if (pointer.id == cursorPointerId) {
		widget.hoveredByCursor = pointerInside && !pointer.occluded;
	}

	bool newPointerOccluded = params.pointer.occluded;
	newPointerOccluded = pointerInside || !pointer.occluded;

	return newPointerOccluded;
}

bool Gui::impl::MenuBtnImpl::MenuBtn_PointerPress(
	MenuBtn_PointerPress_Params const& params)
{
	auto& widget = params.widget;
	auto const& widgetRect = params.widgetRect;
	auto const& visibleRect = params.visibleRect;
	auto const& pointer = params.pointer;

	bool newEventConsumed = params.pointer.consumed;

	auto const pointerInside = PointIsInAll(pointer.pos, { widgetRect, visibleRect });
	newEventConsumed = newEventConsumed || pointerInside;

	if (pointer.type != PointerType::Primary)
		return newEventConsumed;

	if (pointerInside && pointer.pressed && !pointer.consumed)
	{
		newEventConsumed = true;

		MenuBtn_SpawnSubmenuLayer(
			widget,
			params.ctx,
			params.windowId,
			widgetRect);
	}

	// We are inside the button, so we always want to consume the event.
	return newEventConsumed;
}

void Gui::impl::MenuBtnImpl::MenuBtn_SpawnSubmenuLayer(
	MenuButton& widget,
	Context& ctx,
	WindowID windowId,
	Rect widgetRect)
{
	auto job = [&widget, windowId](Context& ctx, Std::AnyRef customData) {
		auto* layerPtr = new impl::MenuLayer;

		layerPtr->_menuButton = &widget;

		ctx.SetFrontmostLayer(
			windowId,
			Std::Box{ layerPtr });
	};

	ctx.PushPostEventJob(job);
}

SizeHint MenuButton::GetSizeHint2(Widget::GetSizeHint2_Params const& params) const
{
	auto const& ctx = params.ctx;
	auto& textMgr = params.textManager;
	auto& pusher = params.pusher;
	auto const& window = params.window;

	auto pusherIt = pusher.AddEntry(*this);
	auto& customData = pusher.AttachCustomData(pusherIt, impl::MenuBtnImpl::CustomData{ pusher.Alloc() });
	if (pusher.IncludeRendering()) {
		customData.glyphRects.Resize(this->title.size());
	}

	auto normalTextScale = ctx.fontScale * window.contentScale;
	auto fontSizeId = FontFaceSizeId::Invalid;
	auto marginAmount = 0;
	{
		auto normalHeight = textMgr.GetLineheight(normalTextScale, window.dpiX, window.dpiY);
		auto normalHeightMargin = (u32)Math::Round((f32)normalHeight * ctx.defaultMarginFactor);
		normalHeight += 2 * normalHeightMargin;

		auto minHeight = CmToPixels(ctx.minimumHeightCm, window.dpiY);

		if (normalHeight > minHeight) {
			fontSizeId = textMgr.GetFontFaceSizeId(normalTextScale, window.dpiX, window.dpiY);
			marginAmount = (i32)normalHeightMargin;
		} else {
			// We can't just do minHeight * defaultMarginFactor for this one, because defaultMarginFactor applies to
			// content, not the outer size. So we set up an equation `height = 2 * marginFactor * content + content`
			// and we solve for content.
			auto contentSizeCm = ctx.minimumHeightCm / ((2 * ctx.defaultMarginFactor) + 1.f);
			auto contentSize = CmToPixels(contentSizeCm, window.dpiY);
			fontSizeId = textMgr.FontFaceSizeIdForLinePixelHeight(
				contentSize,
				TextHeightType::Alphas);
			marginAmount = CmToPixels((f32)contentSizeCm * ctx.defaultMarginFactor, window.dpiY);
		}
	}
	customData.fontSizeId = fontSizeId;

	auto textOuterExtent = textMgr.GetOuterExtent(
		{ this->title.data(), this->title.size() },
		fontSizeId,
		TextHeightType::Alphas,
		customData.glyphRects.ToSpan());
	customData.textOuterExtent = textOuterExtent;

	SizeHint returnVal = {};
	returnVal.minimum = textOuterExtent;
	returnVal.minimum.AddPadding(marginAmount);
	returnVal.minimum.width = Math::Max(
		returnVal.minimum.width,
		(u32)CmToPixels(ctx.minimumHeightCm, window.dpiX));
	returnVal.minimum.height = Math::Max(
		returnVal.minimum.height,
		(u32)CmToPixels(ctx.minimumHeightCm, window.dpiY));

	pusher.SetSizeHint(pusherIt, returnVal);

	return returnVal;
}

void MenuButton::BuildChildRects(
	BuildChildRects_Params const& params,
	Rect const& widgetRect,
	Rect const& visibleRect) const
{
}

void MenuButton::Render2(
	Widget::Render_Params const& params,
	Rect const& widgetRect,
	Rect const& visibleRect) const
{
	auto& drawInfo = params.drawInfo;
	auto& rectCollection = params.rectCollection;

	auto const intersection = Rect::Intersection(widgetRect, visibleRect);
	if (intersection.IsNothing())
		return;

	DENGINE_IMPL_GUI_ASSERT(rectCollection.BuiltForRendering());

	auto* customDataPtr = rectCollection.GetCustomData2<impl::MenuBtnImpl::CustomData>(*this);
	DENGINE_IMPL_GUI_ASSERT(customDataPtr);
	auto& customData = *customDataPtr;
	DENGINE_IMPL_GUI_ASSERT(this->title.size() == customData.glyphRects.Size());
	auto fontSizeId = customData.fontSizeId;

	auto centeringOffset = Extent::CenteringOffset(widgetRect.extent, customData.textOuterExtent);
	centeringOffset.x = Math::Max(centeringOffset.x, 0);
	centeringOffset.y = Math::Max(centeringOffset.y, 0);
	auto textRect = Rect{ widgetRect.position + centeringOffset, customData.textOuterExtent };

	auto scissor = DrawInfo::ScopedScissor(drawInfo, textRect, widgetRect);
	drawInfo.PushText(
		(u64)fontSizeId,
		{ title.data(), title.size() },
		customData.glyphRects.Data(),
		textRect.position,
		{ 1.f, 1.f, 1.f, 1.f });
}

void MenuButton::AccessibilityTest(
	AccessibilityTest_Params const& params,
	Rect const& widgetRect,
	Rect const& visibleRect) const
{
	auto& pusher = params.pusher;

	AccessibilityInfoElement element = {};
	element.textStart = pusher.PushText({ this->title.data(), this->title.size() });
	element.textCount = (int)this->title.size();
	element.rect = Intersection(widgetRect, visibleRect);
	element.isClickable = true;
	pusher.PushElement(element);
}

void MenuButton::CursorExit(Context& ctx)
{
	hoveredByCursor = false;
}

bool MenuButton::CursorMove(
	Widget::CursorMoveParams const& params,
	Rect const& widgetRect,
	Rect const& visibleRect,
	bool occluded)
{
	impl::PointerMove_Pointer pointer = {};
	pointer.id = impl::cursorPointerId;
	pointer.pos = { (f32)params.event.position.x, (f32)params.event.position.y };
	pointer.occluded = occluded;

	impl::MenuBtnImpl::MenuBtn_PointerMove_Params tempParams {
		.widget = *this,
		.widgetRect = widgetRect,
		.visibleRect = visibleRect,
		.pointer = pointer };

	return impl::MenuBtnImpl::MenuBtn_PointerMove(tempParams);
}

bool MenuButton::CursorPress2(
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

	impl::MenuBtnImpl::MenuBtn_PointerPress_Params tempParams {
		.widget = *this,
		.ctx = params.ctx,
		.windowId = params.windowId,
		.widgetRect = widgetRect,
		.visibleRect = visibleRect,
		.pointer = pointer };

	return impl::MenuBtnImpl::MenuBtn_PointerPress(tempParams);
}

bool MenuButton::TouchMove2(
	TouchMoveParams const& params,
	Rect const& widgetRect,
	Rect const& visibleRect,
	bool occluded)
{
	impl::PointerMove_Pointer pointer = {};
	pointer.id = params.event.id;
	pointer.pos = params.event.position;
	pointer.occluded = occluded;

	impl::MenuBtnImpl::MenuBtn_PointerMove_Params tempParams {
		.widget = *this,
		.widgetRect = widgetRect,
		.visibleRect = visibleRect,
		.pointer = pointer };

	return impl::MenuBtnImpl::MenuBtn_PointerMove(tempParams);
}

bool MenuButton::TouchPress2(
	TouchPressParams const& params,
	Rect const& widgetRect,
	Rect const& visibleRect,
	bool consumed)
{
	impl::PointerPress_Pointer pointer = {};
	pointer.id = params.event.id;
	pointer.pos = params.event.position;
	pointer.type = impl::PointerType::Primary;
	pointer.pressed = params.event.pressed;
	pointer.consumed = consumed;

	impl::MenuBtnImpl::MenuBtn_PointerPress_Params tempParams {
		.widget = *this,
		.ctx = params.ctx,
		.windowId = params.windowId,
		.widgetRect = widgetRect,
		.visibleRect = visibleRect,
		.pointer = pointer };

	return impl::MenuBtnImpl::MenuBtn_PointerPress(tempParams);
}