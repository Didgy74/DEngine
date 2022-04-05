#include <DEngine/Gui/MenuButton.hpp>
#include <DEngine/Gui/TextManager.hpp>
#include <DEngine/Gui/Layer.hpp>
#include <DEngine/Gui/Context.hpp>

#include <DEngine/Std/Containers/Defer.hpp>

using namespace DEngine;
using namespace DEngine::Gui;

namespace DEngine::Gui::impl
{
	class MenuLayer : public Layer
	{
	public:
		Math::Vec2Int pos = {};
		MenuButton* menuButton = nullptr;

		virtual void BuildSizeHints(BuildSizeHints_Params const& params) const override;
		virtual void BuildRects(BuildRects_Params const& params) const override;
		virtual void Render(Render_Params const& params) const override;

		[[nodiscard]] virtual bool CursorMove(
			CursorMoveParams const& params,
			bool occluded) override;

		[[nodiscard]] virtual Press_Return CursorPress(
			CursorPressParams const& params,
			bool eventConsumed) override;

		virtual bool CursorMove(
			Context& ctx,
			Rect const& windowRect,
			Rect const& usableRect,
			CursorMoveEvent const& event,
			bool occluded) override;

		virtual Press_Return CursorPress(
			Context& ctx,
			Rect const& windowRect,
			Rect const& usableRect,
			Math::Vec2Int cursorPos,
			CursorPressEvent const& event) override;

		virtual Press_Return TouchPress(
			Context& ctx,
			Rect const& windowRect,
			Rect const& usableRect,
			TouchPressEvent const& event) override;
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

	struct MenuBtnImpl
	{
		struct MenuBtn_PointerMove_Params
		{
			MenuButton& widget;
			Rect widgetRect;
			Rect visibleRect;
			PointerMove_Pointer const& pointer;
		};
		[[nodiscard]] static bool MenuBtn_PointerMove(
			MenuBtn_PointerMove_Params const& params);

		struct MenuBtn_PointerPress_Params
		{
			MenuButton& widget;
			Context& ctx;
			WindowID windowId;
			Rect widgetRect;
			Rect visibleRect;
			PointerPress_Pointer const& pointer;
		};
		[[nodiscard]] static bool MenuBtn_PointerPress(
			MenuBtn_PointerPress_Params const& params);

		static void MenuBtn_SpawnSubmenuLayer(
			MenuButton& widget,
			Context& ctx,
			WindowID windowId,
			Rect widgetRect);


		struct MenuLayer_PointerPress_Params
		{
			Context& ctx;
			MenuLayer& layer;
			RectCollection const& rectCollection;
			TextManager& textManager;
			PointerPress_Pointer const& pointer;
			u32 spacing = 0;
			u32 margin = 0;
			Rect usableRect = {};
		};
		// Common event handler for both touch and cursor
		[[nodiscard]] static Layer::Press_Return MenuLayer_PointerPress(
			MenuButton::Submenu& submenu,
			MenuLayer_PointerPress_Params const& params);


		struct MenuLayer_PointerMove_Params
		{
			u32 spacing = 0;
			u32 margin = 0;
			Rect windowRect = {};
			Rect usableRect = {};
			MenuLayer& layer;
			TextManager& textManager;
			RectCollection const& rectCollection;
			PointerMove_Pointer const& pointer;
		};
		[[nodiscard]] static bool MenuLayer_PointerMove(
			MenuButton::Submenu& submenu,
			MenuLayer_PointerMove_Params const& params);
	};

	[[nodiscard]] static Rect Submenu_BuildRectOuter(
		TextManager& textManager,
		MenuButton::Submenu const& submenu,
		u32 spacing,
		u32 margin,
		Math::Vec2Int desiredPos,
		Rect const& usableRect);

	[[nodiscard]] static Rect Submenu_BuildRectInner(
		Rect outerRect,
		u32 margin) noexcept;

	struct MenuLayer_RenderSubmenu_Params
	{
		MenuButton::Submenu const& submenu;
		TextManager& textManager;
		u32 spacing {};
		u32 margin {};
		Math::Vec4 bgColor {};
		Rect submenuRect {};
		Rect visibleRect {};
		DrawInfo& drawInfo;
	};
	static void MenuLayer_RenderSubmenu(
		MenuLayer_RenderSubmenu_Params const& params);
}

Layer::Press_Return Gui::impl::MenuBtnImpl::MenuLayer_PointerPress(
	MenuButton::Submenu& submenu,
	MenuLayer_PointerPress_Params const& params)
{
	auto& ctx = params.ctx;
	auto& layer = params.layer;
	auto& textManager = params.textManager;
	auto const& rectCollection = params.rectCollection;
	auto const& pointer = params.pointer;

	Layer::Press_Return returnValue = {};

	Std::Defer deferredCleanup { [&]() {
		if (returnValue.destroyLayer)
		{
			submenu.pressedLineOpt = Std::nullOpt;
			submenu.activeSubmenu = Std::nullOpt;
			submenu.cursorHoveredLineOpt = Std::nullOpt;
		}
	} };

	if (params.pointer.type != PointerType::Primary)
	{
		returnValue.eventConsumed = false;
		returnValue.destroyLayer = true;
		return returnValue;
	}

	auto const* rectPairPtr = rectCollection.GetRect(layer);
	DENGINE_IMPL_GUI_ASSERT(rectPairPtr);
	auto const& rectPair = *rectPairPtr;
	auto const& submenuRectOuter = rectPair.widgetRect;
	auto const widgetInnerRect = Submenu_BuildRectInner(
		submenuRectOuter,
		params.margin);

	auto const pointerInsideOuter = submenuRectOuter.PointIsInside(pointer.pos);
	if (!pointerInsideOuter && pointer.pressed)
	{
		returnValue.eventConsumed = false;
		returnValue.destroyLayer = true;
		return returnValue;
	}
	else if (!pointerInsideOuter && !pointer.pressed)
	{
		returnValue.eventConsumed = false;
		returnValue.destroyLayer = false;
		return returnValue;
	}

	returnValue.eventConsumed = pointer.consumed || pointerInsideOuter;

	auto const pointerInsideInner = widgetInnerRect.PointIsInside(params.pointer.pos);
	if (!pointerInsideInner && pointer.pressed)
	{
		returnValue.eventConsumed = returnValue.eventConsumed || !pointerInsideInner;
		return returnValue;
	}


	auto const outerLineheight = textManager.GetLineheight() + (params.margin * 2);
	// Check if we hit a line and not some spacing.
	u32 lineRectYOffset = 0;
	Std::Opt<uSize> indexHit;
	int const submenuLineCount = (i32)submenu.lines.size();
	for (int i = 0; i < submenuLineCount; i++)
	{
		Rect lineRect = widgetInnerRect;
		lineRect.position.y += (i32)lineRectYOffset;
		lineRect.extent.height = outerLineheight;
		if (lineRect.PointIsInside(pointer.pos))
		{
			indexHit = i;
			break;
		}
		lineRectYOffset += lineRect.extent.height + params.spacing;
	}

	if (submenu.pressedLineOpt.HasValue())
	{
		auto const pressedLine = submenu.pressedLineOpt.Value();

		if (indexHit.HasValue() &&
			pressedLine.lineIndex == indexHit.Value() &&
			pressedLine.pointerId == pointer.id &&
			!pointer.pressed)
		{
			auto& line = submenu.lines[indexHit.Value()];
			if (line.togglable)
				line.toggled = !line.toggled;
			if (line.callback)
				line.callback(line, &ctx);

			returnValue.eventConsumed = true;
			returnValue.destroyLayer = true;
			return returnValue;
		}
	}
	else
	{
		if (indexHit.HasValue() && pointer.pressed)
		{
			using Submenu = MenuButton::Submenu;
			Submenu::PressedLineData newPressedLineData = {};
			newPressedLineData.pointerId = pointer.id;
			newPressedLineData.lineIndex = indexHit.Value();
			submenu.pressedLineOpt = newPressedLineData;

			returnValue.eventConsumed = true;
			returnValue.destroyLayer = false;
			return returnValue;
		}
	}

	return returnValue;
}

bool Gui::impl::MenuBtnImpl::MenuLayer_PointerMove(
	MenuButton::Submenu& submenu,
	MenuLayer_PointerMove_Params const& params)
{
	auto& layer = params.layer;
	auto const& rectColl = params.rectCollection;

	auto const& pointer = params.pointer;
	auto& textManager = params.textManager;

	auto const lineheight = textManager.GetLineheight();

	if (params.pointer.occluded)
	{
		submenu.cursorHoveredLineOpt = Std::nullOpt;
		return params.pointer.occluded;
	}

	auto const outerLineheight = lineheight + (params.margin * 2);

	auto const* rectPairPtr = rectColl.GetRect(layer);
	DENGINE_IMPL_GUI_ASSERT(rectPairPtr);
	auto const& rectPair = *rectPairPtr;
	auto const& submenuRectOuter = rectPair.widgetRect;

	auto const pointerInsideOuter =
		submenuRectOuter.PointIsInside(pointer.pos) &&
		rectPair.visibleRect.PointIsInside(pointer.pos);

	bool newOccluded = params.pointer.occluded || pointerInsideOuter;

	// Find out if we're hovering a line. But only if it's the cursor
	if (params.pointer.id == impl::cursorPointerId && pointerInsideOuter)
	{
		Std::Opt<uSize> indexHit;

		auto const widgetRectInner = Submenu_BuildRectInner(submenuRectOuter, params.margin);
		auto const pointerInsideInner =
			widgetRectInner.PointIsInside(pointer.pos) &&
			rectPair.visibleRect.PointIsInside(pointer.pos);
		if (pointerInsideInner)
		{
			// Check if we hit a line and not some spacing.
			u32 lineRectYOffset = 0;
			for (int i = 0; i < submenu.lines.size(); i++)
			{
				Rect lineRect = widgetRectInner;
				lineRect.position.y += lineRectYOffset;
				lineRect.extent.height = outerLineheight;
				if (lineRect.PointIsInside(params.pointer.pos))
				{
					indexHit = i;
					break;
				}
				lineRectYOffset += lineRect.extent.height + params.spacing;
			}
		}
		submenu.cursorHoveredLineOpt = indexHit;
	}

	return newOccluded;
}

Rect Gui::impl::Submenu_BuildRectInner(
	Rect outerRect,
	u32 margin) noexcept
{
	Rect returnVal = outerRect;
	returnVal.position.x += margin;
	returnVal.position.y += margin;
	returnVal.extent.width -= margin * 2;
	returnVal.extent.height -= margin * 2;
	return returnVal;
}

void Gui::impl::MenuLayer_RenderSubmenu(
	MenuLayer_RenderSubmenu_Params const& params)
{
	auto& submenu = params.submenu;
	auto const& submenuRect = params.submenuRect;
	auto const& visibleRect = params.visibleRect;
	auto& textManager = params.textManager;

	auto const intersection = Rect::Intersection(submenuRect, visibleRect);
	if (intersection.IsNothing())
		return;

	auto const lineheight = textManager.GetLineheight() + (params.margin * 2);

	auto const submenuInnerRect = impl::Submenu_BuildRectInner(submenuRect, params.margin);

	params.drawInfo.PushFilledQuad(submenuRect, params.bgColor);
	// First draw highlights for toggled lines
	int const submenuLineCount = (int)submenu.lines.size();
	for (int i = 0; i < submenuLineCount; i += 1)
	{
		auto const& line = submenu.lines[i];

		bool drawHighlight = false;

		if (submenu.cursorHoveredLineOpt.HasValue() && submenu.cursorHoveredLineOpt.Value() == i)
		{
			drawHighlight = true;
		}
		else
		{
			if (line.togglable && line.toggled)
			{
				drawHighlight = true;
			}
			else if (submenu.pressedLineOpt.HasValue())
			{
				if (i == submenu.pressedLineOpt.Value().lineIndex)
					drawHighlight = true;
			}
		}

		if (drawHighlight)
		{
			auto lineRect = submenuInnerRect;
			lineRect.position.y += lineheight * i;
			lineRect.position.y += params.spacing * i;
			lineRect.extent.height = lineheight;
			params.drawInfo.PushFilledQuad(lineRect, { 1.f, 1.f, 1.f, 0.25f });
		}
	}
	for (uSize i = 0; i < submenu.lines.size(); i += 1)
	{
		auto const& line = submenu.lines[i];

		auto lineRect = submenuInnerRect;
		lineRect.position.y += lineheight * i;
		lineRect.position.y += params.spacing * i;
		lineRect.extent.height = lineheight;

		Math::Vec4 textColor = { 1.f, 1.f, 1.f, 1.f };

		auto textRect = lineRect;
		textRect.position.x += params.margin;
		textRect.position.y += params.margin;
		textRect.extent.width -= params.margin * 2;
		textRect.extent.height -= params.margin * 2;

		textManager.RenderText(
			{ line.title.data(), line.title.size() },
			textColor,
			textRect,
			params.drawInfo);
	}
}

void Gui::impl::MenuLayer::BuildSizeHints(
	Layer::BuildSizeHints_Params const& params) const
{
	auto& textManager = params.textManager;
	auto& pusher = params.pusher;
	auto const& textMargin = menuButton->margin;
	auto const& lineSpacing = menuButton->spacing;
	auto const& submenu = menuButton->submenu;

	// Adds text margin for inside each line
	auto const lineheight = textManager.GetLineheight() + (textMargin * 2);

	Extent extent = {};
	extent.height = lineheight * submenu.lines.size();
	extent.height += lineSpacing * (submenu.lines.size() - 1);

	// Calculate width of widget.
	extent.width = 0;
	for (auto const& subLine : submenu.lines)
	{
		auto const textOuterExtent = textManager.GetOuterExtent({ subLine.title.data(), subLine.title.size() });
		extent.width = Math::Max(extent.width, textOuterExtent.width);
	}
	// Add margin for inside line.
	extent.width += textMargin * 2;

	// Add margin to outside each line
	extent.width += textMargin * 2;
	extent.height += textMargin * 2;

	SizeHint returnVal = {};
	returnVal.minimum = extent;

	auto entry = pusher.AddEntry(*this);
	pusher.SetSizeHint(entry, returnVal);
}

void Gui::impl::MenuLayer::BuildRects(
	Layer::BuildRects_Params const& params) const
{
	auto& pusher = params.pusher;
	auto const& windowRect = params.windowRect;
	auto const& safeAreaRect = params.visibleRect;

	auto entry = pusher.GetEntry(*this);

	auto const& sizeHint = pusher.GetSizeHint(entry);

	auto const intersection = Rect::Intersection(windowRect, safeAreaRect);

	Rect returnValue = {};
	returnValue.extent.width = Math::Min(intersection.extent.width, sizeHint.minimum.width);
	returnValue.extent.height = Math::Min(intersection.extent.height, sizeHint.minimum.height);

	returnValue.position = windowRect.position + pos;

	// Adjust the position of the widget.
	returnValue.position.y = Math::Max(
		intersection.Top(),
		returnValue.Top());
	returnValue.position.y = Math::Min(
		returnValue.Top(),
		intersection.Bottom() - (i32)returnValue.extent.height);

	pusher.SetRectPair(entry, { returnValue, safeAreaRect });
}

void Gui::impl::MenuLayer::Render(
	Render_Params const& params) const
{
	auto const& submenu = menuButton->submenu;
	auto const& rectColl = params.rectCollection;
	auto& textManager = params.textManager;
	auto& drawInfo = params.drawInfo;
	auto const& windowRect = params.windowRect;
	auto const& safeAreaRect = params.safeAreaRect;

	auto const intersection = Rect::Intersection(windowRect, safeAreaRect);
	if (intersection.IsNothing())
		return;

	auto const* rectPairPtr = rectColl.GetRect(*this);
	DENGINE_IMPL_GUI_ASSERT(rectPairPtr);
	auto const& rectPair = *rectPairPtr;

	impl::MenuLayer_RenderSubmenu_Params tempParams = {
		.submenu = submenu,
		.textManager = textManager,
		.drawInfo = params.drawInfo, };
	tempParams.submenuRect = rectPair.widgetRect;
	tempParams.visibleRect = rectPair.visibleRect;
	tempParams.margin = menuButton->margin;
	tempParams.spacing = menuButton->spacing;
	tempParams.bgColor = menuButton->colors.active;

	impl::MenuLayer_RenderSubmenu(tempParams);
}

bool Gui::impl::MenuLayer::CursorMove(
	CursorMoveParams const& params,
	bool occluded)
{
	impl::PointerMove_Pointer pointer = {};
	pointer.id = impl::cursorPointerId;
	pointer.pos = { (f32)params.event.position.x, (f32)params.event.position.y };
	pointer.occluded = occluded;

	impl::MenuBtnImpl::MenuLayer_PointerMove_Params tempParams = {
		.layer = *this,
		.textManager = params.textManager,
		.rectCollection = params.rectCollection,
		.pointer = pointer };
	tempParams.margin = menuButton->margin;
	tempParams.spacing = menuButton->spacing;
	tempParams.windowRect = params.windowRect;
	tempParams.usableRect = params.safeAreaRect;

	return impl::MenuBtnImpl::MenuLayer_PointerMove(
		this->menuButton->submenu,
		tempParams);
}

bool Gui::impl::MenuLayer::CursorMove(
	Context& ctx,
	Rect const& windowRect,
	Rect const& usableRect,
	CursorMoveEvent const& event,
	bool occluded)
{
/*
	auto const intersection = Rect::Intersection(windowRect, usableRect);

	impl::PointerMove_Pointer pointer = {};
	pointer.id = impl::cursorPointerId;
	pointer.pos = { (f32)event.position.x, (f32)event.position.y };
	pointer.occluded = occluded;

	return impl::MenuButtonImpl::MenuLayer_PointerMove(
		*this,
		ctx,
		intersection,
		pointer);
*/

	return false;
}

Layer::Press_Return Gui::impl::MenuLayer::CursorPress(
	Context& ctx,
	Rect const& windowRect,
	Rect const& usableRect,
	Math::Vec2Int cursorPos,
	CursorPressEvent const& event)
{
/*
	auto const intersection = Rect::Intersection(windowRect, usableRect);

	impl::PointerPress_Pointer pointer = {};
	pointer.id = impl::cursorPointerId;
	pointer.pos = { (f32)cursorPos.x, (f32)cursorPos.y };
	pointer.pressed = event.pressed;
	pointer.type = impl::ToPointerType(event.button);

	return impl::MenuButtonImpl::MenuLayer_PointerPress(
		*this,
		ctx,
		intersection,
		pointer);
*/

	return {};
}

Layer::Press_Return Gui::impl::MenuLayer::TouchPress(
	Context& ctx,
	Rect const& windowRect,
	Rect const& usableRect,
	TouchPressEvent const& event)
{
/*
	auto const intersection = Rect::Intersection(windowRect, usableRect);

	impl::PointerPress_Pointer pointer = {};
	pointer.id = event.id;
	pointer.pos = event.position;
	pointer.pressed = event.pressed;
	pointer.type = impl::PointerType::Primary;

	return impl::MenuButtonImpl::MenuLayer_PointerPress(
		*this,
		ctx,
		intersection,
		pointer);
*/

	return {};
}

Layer::Press_Return Gui::impl::MenuLayer::CursorPress(
	Layer::CursorPressParams const& params,
	bool eventConsumed)
{
	impl::PointerPress_Pointer pointer = {};
	pointer.id = impl::cursorPointerId;
	pointer.pos = { (f32)params.cursorPos.x, (f32)params.cursorPos.y };
	pointer.type = impl::ToPointerType(params.event.button);
	pointer.pressed = params.event.pressed;
	pointer.consumed = eventConsumed;

	impl::MenuBtnImpl::MenuLayer_PointerPress_Params tempParams = {
		.ctx = params.ctx,
		.layer = *this,
		.rectCollection = params.rectCollection,
		.textManager = params.textManager,
		.pointer = pointer };
	tempParams.spacing = menuButton->spacing;
	tempParams.margin = menuButton->margin;
	tempParams.usableRect = params.safeAreaRect;

	return impl::MenuBtnImpl::MenuLayer_PointerPress(
		menuButton->submenu,
		tempParams);
}

bool MenuButton::CursorPress(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	Math::Vec2Int cursorPos,
	CursorPressEvent event)
{

	return {};
}

bool MenuButton::TouchPressEvent(
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
	return impl::MenuButtonImpl::MenuButton_PointerPress(
		*this,
		ctx,
		windowId,
		widgetRect,
		visibleRect,
		pointer);
*/

	return false;
}

bool Gui::impl::MenuBtnImpl::MenuBtn_PointerMove(
	MenuBtn_PointerMove_Params const& params)
{
	auto& widget = params.widget;
	auto const& widgetRect = params.widgetRect;
	auto const& visibleRect = params.visibleRect;
	auto const& pointer = params.pointer;

	auto const pointerInside = widgetRect.PointIsInside(pointer.pos) && visibleRect.PointIsInside(pointer.pos);

	if (pointer.id == cursorPointerId)
	{
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

	auto const pointerInside = widgetRect.PointIsInside(pointer.pos) && visibleRect.PointIsInside(pointer.pos);
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
	auto job = [&widget, widgetRect, windowId](Context& ctx) {
		auto* layerPtr = new impl::MenuLayer;

		layerPtr->menuButton = &widget;

		layerPtr->pos = widgetRect.position;
		layerPtr->pos.y += widgetRect.extent.height;

		ctx.SetFrontmostLayer(
			windowId,
			Std::Box{ layerPtr });
	};

	ctx.PushPostEventJob(job);
}

SizeHint MenuButton::GetSizeHint2(Widget::GetSizeHint2_Params const& params) const
{
	auto& pusher = params.pusher;
	auto& textManager = params.textManager;

	SizeHint returnValue = {};

	returnValue.minimum = textManager.GetOuterExtent({ title.data(), title.size() });
	returnValue.minimum.width += margin * 2;
	returnValue.minimum.height += margin * 2;

	auto entry = pusher.AddEntry(*this);
	pusher.SetSizeHint(entry, returnValue);

	return returnValue;
}

void MenuButton::Render2(
	Widget::Render_Params const& params,
	Rect const& widgetRect,
	Rect const& visibleRect) const
{
	auto& textManager = params.textManager;
	auto& drawInfo = params.drawInfo;

	Math::Vec4 renderColor = colors.normal;
	if (hoveredByCursor)
		renderColor = colors.hovered;
	if (active)
		renderColor = colors.active;

	if (renderColor.w >= 0.f)
		drawInfo.PushFilledQuad(widgetRect, renderColor);

	auto textRect = widgetRect;
	textRect.position.x += margin;
	textRect.position.y += margin;
	textRect.extent.width -= margin * 2;
	textRect.extent.height -= margin * 2;

	textManager.RenderText(
		{ title.data(), title.size() },
		{ 1.f, 1.f, 1.f, 1.f },
		textRect,
		drawInfo);
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
		.pointer = pointer };
	tempParams.windowId = params.windowId;
	tempParams.widgetRect = widgetRect;
	tempParams.visibleRect = visibleRect;

	return impl::MenuBtnImpl::MenuBtn_PointerPress(tempParams);
}
