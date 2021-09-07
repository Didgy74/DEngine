#include <DEngine/Gui/MenuButton.hpp>

#include <DEngine/Gui/Context.hpp>
#include <DEngine/Gui/Layer.hpp>
#include "ImplData.hpp"

using namespace DEngine;
using namespace DEngine::Gui;

namespace DEngine::Gui::impl
{
	class MenuLayer : public Layer
	{
	public:
		Math::Vec2Int pos = {};
		MenuButton* menuButton = nullptr;

		virtual void Render(
			Context const& ctx,
			Rect const& windowRect,
			Rect const& usableRect,
			DrawInfo& drawInfo) const override;

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
			CursorClickEvent const& event) override;

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
	};

	struct PointerMove_Pointer
	{
		u8 id;
		Math::Vec2 pos;
		bool occluded;
	};

	struct MenuButtonImpl
	{
		// Common event handler for both touch and cursor
		[[nodiscard]] static bool MenuButton_PointerPress(
			MenuButton& widget,
			Context& ctx,
			WindowID windowId,
			Rect const& widgetRect,
			Rect const& visibleRect,
			PointerPress_Pointer const& pointer);

		static void MenuButton_SpawnSubmenuLayer(
			MenuButton& widget,
			Context& ctx,
			WindowID windowId,
			Rect widgetRect);

		// Common event handler for both touch and cursor
		[[nodiscard]] static Layer::Press_Return MenuLayer_PointerPress(
			MenuLayer& layer,
			Context& ctx,
			Rect const& usableRect,
			PointerPress_Pointer const& pointer);

		struct MenuLayer_PointerPress_Inner_Params
		{
			Context* ctx = nullptr;
			u32 spacing = 0;
			u32 margin = 0;
			u32 lineheight = 0;
			Rect usableRect = {};
			PointerPress_Pointer const* pointer = nullptr;
		};
		[[nodiscard]] static Layer::Press_Return MenuLayer_PointerPress_Inner(
			MenuButton::Submenu& submenu,
			Math::Vec2Int desiredPos,
			MenuLayer_PointerPress_Inner_Params const& params);

		[[nodiscard]] static bool MenuLayer_PointerMove(
			MenuLayer& layer,
			Context& ctx,
			Rect const& usableRect,
			PointerMove_Pointer const& pointer);

		struct MenuLayer_PointerMove_Inner_Params
		{
			Context* ctx = nullptr;
			u32 spacing = 0;
			u32 margin = 0;
			u32 lineheight = 0;
			Rect usableRect = {};
			PointerMove_Pointer const* pointer = nullptr;
		};
		[[nodiscard]] static bool MenuLayer_PointerMove_Inner(
			MenuButton::Submenu& submenu,
			Math::Vec2Int desiredPos,
			MenuLayer_PointerMove_Inner_Params const& params);
	};

	[[nodiscard]] static Rect Submenu_BuildRectOuter(
		Context const& ctx,
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
		Context const* ctx = nullptr;
		u32 spacing = 0;
		u32 margin = 0;
		Math::Vec4 bgColor = {};
		Rect usableRect = {};
		DrawInfo* drawInfo = nullptr;
	};
	void MenuLayer_RenderSubmenu(
		MenuButton::Submenu const& submenu,
		Math::Vec2Int pos,
		MenuLayer_RenderSubmenu_Params const& params)
	{
		auto& implData = *static_cast<impl::ImplData*>(params.ctx->Internal_ImplData());

		auto const lineheight = implData.textManager.lineheight + (params.margin * 2);

		auto const widgetOuterRect = Submenu_BuildRectOuter(
			*params.ctx,
			submenu,
			params.spacing,
			params.margin,
			pos,
			params.usableRect);

		auto const widgetInnerRect = Submenu_BuildRectInner(
			widgetOuterRect,
			params.margin);

		if (submenu.activeSubmenu.HasValue())
		{
			auto index = submenu.activeSubmenu.Value();
			auto const& line = submenu.lines[index];

			DENGINE_IMPL_GUI_ASSERT(line.data.IsA<MenuButton::Submenu>());

			auto& subsubmenu = line.data.Get<MenuButton::Submenu>();

			auto linePos = widgetInnerRect.position;
			linePos.x += widgetOuterRect.extent.width;
			linePos.y += lineheight * index;

			MenuLayer_RenderSubmenu(
				subsubmenu,
				linePos,
				params);
		}

		params.drawInfo->PushFilledQuad(widgetOuterRect, params.bgColor);
		// First draw highlights for toggled lines
		for (uSize i = 0; i < submenu.lines.size(); i += 1)
		{
			auto const& line = submenu.lines[i];

			bool drawHighlight = false;

			if (submenu.hoveredLineCursor.HasValue() && submenu.hoveredLineCursor.Value() == i)
			{
				drawHighlight = true;
			}
			else if (auto btn = line.data.ToPtr<MenuButton::LineButton>())
			{
				if (btn->togglable && btn->toggled)
				{
					drawHighlight = true;
				}
				else if (submenu.pressedLine.HasValue())
				{
					if (i == submenu.pressedLine.Value().lineIndex)
						drawHighlight = true;
				}
			}

			if (drawHighlight)
			{
				auto lineRect = widgetInnerRect;
				lineRect.position.y += lineheight * i;
				lineRect.position.y += params.spacing * i;
				lineRect.extent.height = lineheight;
				params.drawInfo->PushFilledQuad(lineRect, { 1.f, 1.f, 1.f, 0.25f });
			}
		}
		for (uSize i = 0; i < submenu.lines.size(); i += 1)
		{
			auto const& line = submenu.lines[i];

			auto lineRect = widgetInnerRect;
			lineRect.position.y += lineheight * i;
			lineRect.position.y += params.spacing * i;
			lineRect.extent.height = lineheight;

			Math::Vec4 textColor = { 1.f, 1.f, 1.f, 1.f };

			auto textRect = lineRect;
			textRect.position.x += params.margin;
			textRect.position.y += params.margin;
			textRect.extent.width -= params.margin * 2;
			textRect.extent.height -= params.margin * 2;

			impl::TextManager::RenderText(
				implData.textManager,
				{ line.title.data(), line.title.size() },
				textColor,
				textRect,
				*params.drawInfo);
		}
	}
}

Layer::Press_Return impl::MenuButtonImpl::MenuLayer_PointerPress(
	MenuLayer& layer,
	Context& ctx,
	Rect const& usableRect,
	PointerPress_Pointer const& pointer)
{
	auto const& implData = *static_cast<impl::ImplData*>(ctx.Internal_ImplData());

	auto const lineheight = implData.textManager.lineheight;

	MenuLayer_PointerPress_Inner_Params params = {};
	params.ctx = &ctx;
	params.lineheight = lineheight;
	params.margin = layer.menuButton->margin;
	params.pointer = &pointer;
	params.spacing = layer.menuButton->spacing;
	params.usableRect = usableRect;

	auto returnVal = MenuLayer_PointerPress_Inner(
		layer.menuButton->submenu,
		layer.pos,
		params);

	if (returnVal.destroy)
	{
		layer.menuButton->active = false;
		layer.menuButton->pointerId = Std::nullOpt;
		layer.menuButton->submenu.activeSubmenu = Std::nullOpt;
		layer.menuButton->submenu.pressedLine = Std::nullOpt;
		layer.menuButton->submenu.hoveredLineCursor = Std::nullOpt;
	}

	return returnVal;
}

Layer::Press_Return impl::MenuButtonImpl::MenuLayer_PointerPress_Inner(
	MenuButton::Submenu& submenu,
	Math::Vec2Int desiredPos,
	MenuLayer_PointerPress_Inner_Params const& params)
{
	auto const outerLineheight = params.lineheight + (params.margin * 2);

	auto const widgetOuterRect = Submenu_BuildRectOuter(
		*params.ctx,
		submenu,
		params.spacing,
		params.margin,
		desiredPos,
		params.usableRect);
	auto const widgetInnerRect = Submenu_BuildRectInner(
		widgetOuterRect,
		params.margin);

	Layer::Press_Return childReturn = {};
	if (submenu.activeSubmenu.HasValue())
	{
		auto const index = submenu.activeSubmenu.Value();
		auto& line = submenu.lines[index];
		DENGINE_IMPL_GUI_ASSERT(line.data.IsA<MenuButton::Submenu>());
		auto& subsubmenu = line.data.Get<MenuButton::Submenu>();

		auto childPos = widgetInnerRect.position;
		childPos.x += widgetOuterRect.extent.width;
		childPos.y += outerLineheight * index;
		childReturn = MenuLayer_PointerPress_Inner(
			subsubmenu,
			childPos,
			params);
	}
	if (childReturn.eventConsumed)
	{
		Layer::Press_Return returnVal = {};
		returnVal.eventConsumed = true;
		returnVal.destroy = childReturn.destroy;
		return returnVal;
	}

	auto const pointerInsideOuter = widgetOuterRect.PointIsInside(params.pointer->pos);

	if (params.pointer->type != PointerType::Primary)
	{
		Layer::Press_Return returnVal = {};
		returnVal.eventConsumed = pointerInsideOuter;
		returnVal.destroy = !pointerInsideOuter;
		return returnVal;
	}

	if (!childReturn.eventConsumed && !pointerInsideOuter)
	{
		Layer::Press_Return returnVal = {};
		returnVal.eventConsumed = false;
		returnVal.destroy = true;
		return returnVal;
	}

	auto const pointerInsideInner = widgetInnerRect.PointIsInside(params.pointer->pos);

	if (pointerInsideInner)
	{
		// Check if we hit a line and not some spacing.
		u32 lineRectYOffset = 0;
		Std::Opt<uSize> indexHit;
		for (int i = 0; i < submenu.lines.size(); i++)
		{
			Rect lineRect = widgetInnerRect;
			lineRect.position.y += lineRectYOffset;
			lineRect.extent.height = outerLineheight;
			if (lineRect.PointIsInside(params.pointer->pos))
			{
				indexHit = i;
				break;
			}
			lineRectYOffset += lineRect.extent.height + params.spacing;
		}

		if (submenu.pressedLine.HasValue())
		{
			auto const pressedLine = submenu.pressedLine.Value();

			if (indexHit.HasValue() &&
				pressedLine.lineIndex == indexHit.Value() &&
				pressedLine.pointerId == params.pointer->id &&
				!params.pointer->pressed)
			{
				auto& line = submenu.lines[indexHit.Value()];

				if (auto subsubmenu = line.data.ToPtr<MenuButton::Submenu>())
				{
					submenu.activeSubmenu = indexHit.Value();
					submenu.pressedLine = Std::nullOpt;

					Layer::Press_Return returnVal = {};
					returnVal.eventConsumed = true;
					returnVal.destroy = false;
					return returnVal;
				}
				else if (auto btn = line.data.ToPtr<MenuButton::LineButton>())
				{
					if (btn->togglable)
						btn->toggled = !btn->toggled;
					if (btn->callback)
						btn->callback(*btn);

					Layer::Press_Return returnVal = {};
					returnVal.eventConsumed = true;
					returnVal.destroy = true;
					return returnVal;
				}
			}
		}
		else
		{
			if (indexHit.HasValue() && params.pointer->pressed)
			{
				MenuButton::Submenu::PressedLine newPressedLine = {};
				newPressedLine.lineIndex = indexHit.Value();
				newPressedLine.pointerId = params.pointer->id;
				submenu.pressedLine = newPressedLine;

				Layer::Press_Return returnVal = {};
				returnVal.eventConsumed = true;
				returnVal.destroy = false;
				return returnVal;
			}
		}
	}

	Layer::Press_Return returnVal = {};
	returnVal.eventConsumed = pointerInsideOuter;
	returnVal.destroy = false;
	return returnVal;
}

bool impl::MenuButtonImpl::MenuLayer_PointerMove(
	MenuLayer& layer,
	Context& ctx,
	Rect const& usableRect,
	PointerMove_Pointer const& pointer)
{
	auto& implData = *static_cast<impl::ImplData*>(ctx.Internal_ImplData());

	auto const lineheight = implData.textManager.lineheight;

	MenuLayer_PointerMove_Inner_Params params = {};
	params.ctx = &ctx;
	params.lineheight = lineheight;
	params.margin = layer.menuButton->margin;
	params.pointer = &pointer;
	params.spacing = layer.menuButton->spacing;
	params.usableRect = usableRect;

	return MenuLayer_PointerMove_Inner(
		layer.menuButton->submenu,
		layer.pos,
		params);
}

bool impl::MenuButtonImpl::MenuLayer_PointerMove_Inner(
	MenuButton::Submenu& submenu,
	Math::Vec2Int desiredPos,
	MenuLayer_PointerMove_Inner_Params const& params)
{
	auto const outerLineheight = params.lineheight + (params.margin * 2);

	auto const widgetRectOuter = Submenu_BuildRectOuter(
		*params.ctx,
		submenu,
		params.spacing,
		params.margin,
		desiredPos,
		params.usableRect);

	bool occluded = params.pointer->occluded;

	if (submenu.activeSubmenu.HasValue())
	{
		auto const index = submenu.activeSubmenu.Value();
		auto& line = submenu.lines[index];

		DENGINE_IMPL_GUI_ASSERT(line.data.IsA<MenuButton::Submenu>());

		auto& subsubmenu = line.data.Get<MenuButton::Submenu>();

		auto childPos = widgetRectOuter.position;
		childPos.x += widgetRectOuter.extent.width;
		childPos.y += outerLineheight * index;
		bool childOccluded = MenuLayer_PointerMove_Inner(subsubmenu, childPos, params);
		occluded = occluded || childOccluded;
	}

	auto const pointerInsideOuter =
		widgetRectOuter.PointIsInside(params.pointer->pos) &&
		params.usableRect.PointIsInside(params.pointer->pos);

	auto const widgetRectInner = Submenu_BuildRectInner(widgetRectOuter, params.margin);

	auto const pointerInsideInner =  widgetRectInner.PointIsInside(params.pointer->pos);

	// Find out if we're hovering a line. But only if it's the cursor
	if (params.pointer->id == impl::cursorPointerId)
	{
		Std::Opt<uSize> indexHit;
		if (pointerInsideInner)
		{
			// Check if we hit a line and not some spacing.
			u32 lineRectYOffset = 0;
			for (int i = 0; i < submenu.lines.size(); i++)
			{
				Rect lineRect = widgetRectInner;
				lineRect.position.y += lineRectYOffset;
				lineRect.extent.height = outerLineheight;
				if (lineRect.PointIsInside(params.pointer->pos))
				{
					indexHit = i;
					break;
				}
				lineRectYOffset += lineRect.extent.height + params.spacing;
			}
		}
		submenu.hoveredLineCursor = indexHit;
	}

	occluded = occluded || pointerInsideOuter;

	return occluded;
}

bool impl::MenuButtonImpl::MenuButton_PointerPress(
	MenuButton& widget,
	Context& ctx,
	WindowID windowId,
	Rect const& widgetRect,
	Rect const& visibleRect,
	PointerPress_Pointer const& pointer)
{
	bool pointerInside = widgetRect.PointIsInside(pointer.pos) && visibleRect.PointIsInside(pointer.pos);
	if (pointer.pressed && !pointerInside)
		return false;

	if (pointer.type == PointerType::Primary)
	{
		if (pointer.pressed && pointerInside)
		{
			widget.pointerId = pointer.id;
			return pointerInside;
		}
		else if (!pointer.pressed && widget.pointerId.HasValue() && widget.pointerId.Value() == pointer.id)
		{
			if (pointerInside)
			{
				// We are inside the widget, we were in pressed state,
				// and we unpressed the pointerId that was pressing down.

				MenuButton_SpawnSubmenuLayer(widget, ctx, windowId, widgetRect);

				widget.active = true;
				widget.pointerId = Std::nullOpt;
				return pointerInside;
			}
			else
			{
				widget.pointerId = Std::nullOpt;
				return pointerInside;
			}
		}
	}

	// We are inside the button, so we always want to consume the event.
	return pointerInside;
}

Rect impl::Submenu_BuildRectOuter(
	Context const& ctx,
	MenuButton::Submenu const& submenu,
	u32 spacing,
	u32 margin,
	Math::Vec2Int desiredPos,
	Rect const& usableRect)
{
	auto& implData = *static_cast<impl::ImplData*>(ctx.Internal_ImplData());

	// Adds text margin for inside each line
	auto const lineheight = implData.textManager.lineheight + (margin * 2);

	Rect widgetRect = {};
	widgetRect.position = desiredPos;
	widgetRect.extent.height = lineheight * submenu.lines.size();
	widgetRect.extent.height += spacing * (submenu.lines.size() - 1);

	// Calculate width of widget.
	widgetRect.extent.width = 0;
	for (auto const& subLine : submenu.lines)
	{
		auto const sizeHint = impl::TextManager::GetSizeHint(
			implData.textManager,
			{ subLine.title.data(), subLine.title.size() });

		widgetRect.extent.width = Math::Max(widgetRect.extent.width, sizeHint.preferred.width);
	}
	// Add margin for inside line.
	widgetRect.extent.width += margin * 2;

	// Add margin to outside each line
	widgetRect.extent.width += margin * 2;
	widgetRect.extent.height += margin * 2;

	// Adjust the position of the widget.
	widgetRect.position.y = Math::Max(
		widgetRect.Top(),
		usableRect.Top());
	widgetRect.position.y = Math::Min(
		widgetRect.Top(),
		usableRect.Bottom() - (i32)widgetRect.extent.height);

	return widgetRect;
}

Rect impl::Submenu_BuildRectInner(
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

void impl::MenuLayer::Render(
	Context const& ctx,
	Rect const& windowRect,
	Rect const& usableRect,
	DrawInfo& drawInfo) const
{
	auto const intersection = Rect::Intersection(windowRect, usableRect);

	impl::MenuLayer_RenderSubmenu_Params params = {};
	params.bgColor = menuButton->colors.active;
	params.ctx = &ctx;
	params.drawInfo = &drawInfo;
	params.margin = menuButton->margin;
	params.spacing = menuButton->spacing;
	params.usableRect = intersection;

	return impl::MenuLayer_RenderSubmenu(
		menuButton->submenu,
		pos,
		params);
}

bool impl::MenuLayer::CursorMove(
	Context& ctx,
	Rect const& windowRect,
	Rect const& usableRect,
	CursorMoveEvent const& event,
	bool occluded)
{
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
}

Layer::Press_Return impl::MenuLayer::CursorPress(
	Context& ctx,
	Rect const& windowRect,
	Rect const& usableRect,
	Math::Vec2Int cursorPos,
	CursorClickEvent const& event)
{
	auto const intersection = Rect::Intersection(windowRect, usableRect);

	impl::PointerPress_Pointer pointer = {};
	pointer.id = impl::cursorPointerId;
	pointer.pos = { (f32)cursorPos.x, (f32)cursorPos.y };
	pointer.pressed = event.clicked;
	pointer.type = impl::ToPointerType(event.button);

	return impl::MenuButtonImpl::MenuLayer_PointerPress(
		*this,
		ctx,
		intersection,
		pointer);
}

Layer::Press_Return impl::MenuLayer::TouchPress(
	Context& ctx,
	Rect const& windowRect,
	Rect const& usableRect,
	TouchPressEvent const& event)
{
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
}

SizeHint MenuButton::GetSizeHint(
	Context const& ctx) const
{
	auto& implData = *static_cast<impl::ImplData*>(ctx.Internal_ImplData());

	auto returnVal = impl::TextManager::GetSizeHint(
		implData.textManager,
		{ title.data(), title.size() });
	returnVal.preferred.width += margin * 2;
	returnVal.preferred.height += margin * 2;

	return returnVal;
}

void MenuButton::Render(
	Context const& ctx,
	Extent framebufferExtent,
	Rect widgetRect,
	Rect visibleRect,
	DrawInfo& drawInfo) const
{
	if (active)
	{
		if (colors.active.w > 0.f)
			drawInfo.PushFilledQuad(widgetRect, colors.active);
	}
	else
	{
		if (colors.normal.w > 0.f)
			drawInfo.PushFilledQuad(widgetRect, colors.normal);
	}

	auto& implData = *static_cast<impl::ImplData*>(ctx.Internal_ImplData());

	auto textRect = widgetRect;
	textRect.position.x += margin;
	textRect.position.y += margin;
	textRect.extent.width -= margin * 2;
	textRect.extent.height -= margin * 2;

	impl::TextManager::RenderText(
		implData.textManager,
		{ title.data(), title.size() },
		{ 1.f, 1.f, 1.f, 1.f },
		textRect,
		drawInfo);
}

bool MenuButton::CursorPress(
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

	return impl::MenuButtonImpl::MenuButton_PointerPress(
		*this,
		ctx,
		windowId,
		widgetRect,
		visibleRect,
		pointer);
}

bool MenuButton::TouchPressEvent(
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
	return impl::MenuButtonImpl::MenuButton_PointerPress(
		*this,
		ctx,
		windowId,
		widgetRect,
		visibleRect,
		pointer);
}

void impl::MenuButtonImpl::MenuButton_SpawnSubmenuLayer(
	MenuButton& widget,
	Context& ctx,
	WindowID windowId,
	Rect widgetRect)
{
	auto layerPtr = new impl::MenuLayer;

	layerPtr->menuButton = &widget;

	layerPtr->pos = widgetRect.position;
	layerPtr->pos.y += widgetRect.extent.height;

	ctx.SetFrontmostLayer(
		windowId,
		Std::Box{ layerPtr });
}