#include <DEngine/Gui/MenuButton.hpp>

#include <DEngine/Gui/Context.hpp>
#include <DEngine/Gui/Layer.hpp>
#include "ImplData.hpp"

#include <DEngine/Std/Utility.hpp>

using namespace DEngine;
using namespace DEngine::Gui;

namespace DEngine::Gui::impl
{
	class MenuLayer : public Layer
	{
	public:
		Math::Vec2Int pos;
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

	[[nodiscard]] static Rect BuildRect(
		MenuButton::Submenu const& submenu,
		Math::Vec2Int desiredPos,
		Context const& ctx,
		Rect const& usableRect);

	void RenderSubmenu(
		MenuButton::Submenu const& submenu,
		Math::Vec2Int pos,
		Context const& ctx,
		Rect const& usableRect,
		DrawInfo& drawInfo)
	{
		auto& implData = *static_cast<impl::ImplData*>(ctx.Internal_ImplData());

		auto const lineheight = implData.textManager.lineheight;

		auto const widgetRect = BuildRect(
			submenu,
			pos,
			ctx,
			usableRect);

		if (submenu.activeSubmenu.HasValue())
		{
			auto index = submenu.activeSubmenu.Value();
			auto const& line = submenu.lines[index];

			DENGINE_IMPL_GUI_ASSERT(line.data.IsA<MenuButton::Submenu>());

			auto& subsubmenu = line.data.Get<MenuButton::Submenu>();

			auto linePos = widgetRect.position;
			linePos.x += widgetRect.extent.width;
			linePos.y += lineheight * index;

			RenderSubmenu(
				subsubmenu,
				linePos,
				ctx,
				usableRect,
				drawInfo);
		}

		drawInfo.PushFilledQuad(widgetRect, { 0.25f, 0.25f, 0.25f, 1.f });
		// First draw highlights for toggled lines
		for (uSize i = 0; i < submenu.lines.size(); i += 1)
		{
			auto const& line = submenu.lines[i];

			bool drawHighlight = false;

			if (auto btn = line.data.ToPtr<MenuButton::LineButton>(); btn && btn->togglable && btn->toggled)
				drawHighlight = true;

			if (drawHighlight)
			{
				auto lineRect = widgetRect;
				lineRect.position.y += lineheight * i;
				lineRect.extent.height = lineheight;
				drawInfo.PushFilledQuad(lineRect, { 1.f, 1.f, 1.f, 0.25f });
			}
		}
		for (uSize i = 0; i < submenu.lines.size(); i += 1)
		{
			auto const& line = submenu.lines[i];

			auto lineRect = widgetRect;
			lineRect.position.y += lineheight * i;
			lineRect.extent.height = lineheight;

			Math::Vec4 textColor = { 1.f, 1.f, 1.f, 1.f };

			impl::TextManager::RenderText(
				implData.textManager,
				{ line.title.data(), line.title.size() },
				textColor,
				lineRect,
				drawInfo);
		}
	}

	enum class PointerType : u8 { Primary, Secondary };
	[[nodiscard]] static PointerType ToPointerType(Gui::CursorButton in) noexcept
	{
		switch (in)
		{
			case Gui::CursorButton::Primary: return PointerType::Primary;
			case Gui::CursorButton::Secondary: return PointerType::Secondary;
			default: break;
		}
		DENGINE_IMPL_UNREACHABLE();
		return {};
	}

	static constexpr u8 cursorPointerId = (u8)-1;

	struct PointerMove_Pointer
	{
		u8 id;
		Math::Vec2 pos;
		bool occluded;
	};
	struct PointerPress_Pointer
	{
		u8 id;
		PointerType type;
		Math::Vec2 pos;
		bool pressed;
	};

	[[nodiscard]] static bool MenuButton_PointerPress(
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
					
					widget.Test(ctx, windowId, widgetRect);

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

	[[nodiscard]] static bool MenuLayer_PointerMove(
		MenuButton::Submenu& submenu,
		Math::Vec2Int desiredPos,
		Context& ctx,
		Rect const& usableRect,
		PointerMove_Pointer const& pointer)
	{
		auto& implData = *static_cast<impl::ImplData*>(ctx.Internal_ImplData());
		auto const lineheight = implData.textManager.lineheight;

		auto const widgetRect = BuildRect(
			submenu,
			desiredPos,
			ctx,
			usableRect);

		bool occluded = pointer.occluded;

		if (submenu.activeSubmenu.HasValue())
		{
			auto const index = submenu.activeSubmenu.Value();
			auto& line = submenu.lines[index];

			DENGINE_IMPL_GUI_ASSERT(line.data.IsA<MenuButton::Submenu>());

			auto& subsubmenu = line.data.Get<MenuButton::Submenu>();

			auto childPos = widgetRect.position;
			childPos.x += widgetRect.extent.width;
			childPos.y += lineheight * index;
			bool childOccluded = MenuLayer_PointerMove(
				subsubmenu,
				childPos,
				ctx,
				usableRect,
				pointer);
			occluded = occluded || childOccluded;	
		}

		bool pointerInside = widgetRect.PointIsInside(pointer.pos) && usableRect.PointIsInside(pointer.pos);

		occluded = occluded || pointerInside;

		return occluded;
	}

	[[nodiscard]] static Layer::Press_Return MenuLayer_PointerPress(
		MenuButton::Submenu& submenu,
		Math::Vec2Int desiredPos,
		Context& ctx,
		Rect const& usableRect,
		PointerPress_Pointer const& pointer)
	{
		auto& implData = *static_cast<impl::ImplData*>(ctx.Internal_ImplData());

		auto const lineheight = implData.textManager.lineheight;

		auto const widgetRect = BuildRect(
			submenu,
			desiredPos,
			ctx,
			usableRect);

		Layer::Press_Return childReturn = {};
		if (submenu.activeSubmenu.HasValue())
		{
			auto const index = submenu.activeSubmenu.Value();
			auto& line = submenu.lines[index];

			DENGINE_IMPL_GUI_ASSERT(line.data.IsA<MenuButton::Submenu>());

			auto& subsubmenu = line.data.Get<MenuButton::Submenu>();

			auto childPos = widgetRect.position;
			childPos.x += widgetRect.extent.width;
			childPos.y += lineheight * index;

			childReturn = MenuLayer_PointerPress(
				subsubmenu,
				childPos,
				ctx,
				usableRect,
				pointer);
		}

		if (childReturn.eventConsumed)
		{
			submenu.pressedLine = Std::nullOpt;

			Layer::Press_Return returnVal = {};
			returnVal.eventConsumed = true;
			returnVal.destroy = childReturn.destroy;
			return returnVal;
		}

		bool const pointerInside = widgetRect.PointIsInside(pointer.pos);

		if (pointer.type != PointerType::Primary)
		{
			Layer::Press_Return returnVal = {};
			returnVal.eventConsumed = pointerInside;
			returnVal.destroy = !pointerInside;
			return returnVal;
		}

		if (!childReturn.eventConsumed && !pointerInside)
		{
			submenu.pressedLine = Std::nullOpt;
			submenu.activeSubmenu = Std::nullOpt;

			Layer::Press_Return returnVal = {};
			returnVal.eventConsumed = false;
			returnVal.destroy = true;
			return returnVal;
		}

		if (pointerInside)
		{
			// Find out which index we hit
			uSize indexHit = (pointer.pos.y - widgetRect.position.y) / lineheight;

			if (submenu.pressedLine.HasValue())
			{
				auto const pressedLine = submenu.pressedLine.Value();
				if (pressedLine.lineIndex == indexHit &&
					pressedLine.pointerId == pointer.id &&
					!pointer.pressed)
				{
					auto& line = submenu.lines[indexHit];

					if (auto subsubmenu = line.data.ToPtr<MenuButton::Submenu>())
					{
						submenu.activeSubmenu = indexHit;
						submenu.pressedLine = Std::nullOpt;

						Layer::Press_Return returnVal = {};
						returnVal.eventConsumed = true;
						returnVal.destroy = false;
						return returnVal;
					}
					else if (auto btn = line.data.ToPtr<MenuButton::LineButton>())
					{
						submenu.activeSubmenu = Std::nullOpt;
						submenu.pressedLine = Std::nullOpt;

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
				if (pointer.pressed)
				{
					MenuButton::Submenu::PressedLine newPressedLine = {};
					newPressedLine.lineIndex = indexHit;
					newPressedLine.pointerId = pointer.id;

					submenu.pressedLine = newPressedLine;

					Layer::Press_Return returnVal = {};
					returnVal.eventConsumed = true;
					returnVal.destroy = false;
					return returnVal;
				}
			}
		}

		Layer::Press_Return returnVal = {};
		returnVal.eventConsumed = pointerInside;
		returnVal.destroy = false;
		return returnVal;
	}
}

Rect impl::BuildRect(
	MenuButton::Submenu const& submenu,
	Math::Vec2Int desiredPos,
	Context const& ctx,
	Rect const& usableRect)
{
	auto& implData = *static_cast<impl::ImplData*>(ctx.Internal_ImplData());

	auto const lineheight = implData.textManager.lineheight;

	Rect widgetRect = {};
	widgetRect.position = desiredPos;
	widgetRect.extent.height = lineheight * submenu.lines.size();

	// Calculate width of widget.
	widgetRect.extent.width = 0;
	for (auto const& subLine : submenu.lines)
	{
		auto const sizeHint = impl::TextManager::GetSizeHint(
			implData.textManager,
			{ subLine.title.data(), subLine.title.size() });

		widgetRect.extent.width = Math::Max(
			widgetRect.extent.width,
			sizeHint.preferred.width);
	}

	// Adjust the position of the widget.
	widgetRect.position.y = Math::Max(
		widgetRect.Top(),
		usableRect.Top());
	widgetRect.position.y = Math::Min(
		widgetRect.Top(),
		usableRect.Bottom() - (i32)widgetRect.extent.height);

	return widgetRect;
}

void impl::MenuLayer::Render(
	Context const& ctx,
	Rect const& windowRect,
	Rect const& usableRect,
	DrawInfo& drawInfo) const
{
	auto const intersection = Rect::Intersection(windowRect, usableRect);

	return impl::RenderSubmenu(
		menuButton->submenu,
		pos,
		ctx,
		intersection,
		drawInfo);
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

	return impl::MenuLayer_PointerMove(
		menuButton->submenu,
		pos,
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

	return impl::MenuLayer_PointerPress(
		menuButton->submenu,
		pos,
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

	return impl::MenuLayer_PointerPress(
		menuButton->submenu,
		pos,
		ctx,
		intersection,
		pointer);
}

SizeHint MenuButton::GetSizeHint(
	Context const& ctx) const
{
	auto& implData = *static_cast<impl::ImplData*>(ctx.Internal_ImplData());

	return impl::TextManager::GetSizeHint(
		implData.textManager,
		{ title.data(), title.size() });
}

void MenuButton::Render(
	Context const& ctx,
	Extent framebufferExtent,
	Rect widgetRect,
	Rect visibleRect,
	DrawInfo& drawInfo) const
{
	auto& implData = *static_cast<impl::ImplData*>(ctx.Internal_ImplData());

	impl::TextManager::RenderText(
		implData.textManager,
		{ title.data(), title.size() },
		{ 1.f, 1.f, 1.f, 1.f },
		widgetRect,
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

	return impl::MenuButton_PointerPress(
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
	return impl::MenuButton_PointerPress(
		*this,
		ctx,
		windowId,
		widgetRect,
		visibleRect,
		pointer);
}

void MenuButton::Test(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect)
{
	auto layerPtr = new impl::MenuLayer;

	layerPtr->menuButton = this;
	
	layerPtr->pos = widgetRect.position;
	layerPtr->pos.y += widgetRect.extent.height;

	ctx.SetFrontmostLayer(
		windowId,
		Std::Box{ layerPtr });
}