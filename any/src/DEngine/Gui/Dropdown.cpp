#include <DEngine/Gui/Dropdown.hpp>

#include <DEngine/Gui/Layer.hpp>

#include <DEngine/Gui/Context.hpp>
#include <DEngine/Gui/StackLayout.hpp>

#include <DEngine/Std/Containers/Opt.hpp>
#include <DEngine/Std/Utility.hpp>

#include "ImplData.hpp"

#include <DEngine/Math/Common.hpp>

using namespace DEngine;
using namespace DEngine::Gui;

namespace DEngine::Gui::impl
{
	class DropdownLayer : public Layer
	{
	public:
		Dropdown* dropdownWidget = nullptr;

		Math::Vec2Int pos;
		std::vector<std::string> lines;
		struct PressedLine
		{
			u8 pointerId;
			uSize index;
		};
		Std::Opt<PressedLine> pressedLine;

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

	static void CreateDropdownLayer(
		Dropdown& widget,
		Context& ctx,
		WindowID windowId,
		Rect const& widgetRect)
	{
		auto layer = Std::Box{ new DropdownLayer };
		layer->dropdownWidget = &widget;
		layer->lines = widget.items;
		layer->pos = widgetRect.position;
		layer->pos.y += widgetRect.extent.height;

		ctx.SetFrontmostLayer(
			windowId,
			Std::Move(layer));
	}

	[[nodiscard]] static Rect DropdownLayer_BuildRect(
		DropdownLayer const& widget,
		Context const& ctx,
		Rect const& usableRect)
	{
		auto& implData = *static_cast<impl::ImplData*>(ctx.Internal_ImplData());

		auto const lineheight = implData.textManager.lineheight;

		Rect widgetRect = {};
		widgetRect.position = widget.pos;
		widgetRect.extent.height = lineheight * widget.lines.size();

		// Calculate width of widget.
		widgetRect.extent.width = 0;
		for (auto const& subLine : widget.lines)
		{
			auto const sizeHint = impl::TextManager::GetSizeHint(
				implData.textManager,
				{ subLine.data(), subLine.size() });

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

	constexpr u8 cursorPointerId = (u8)-1;

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

	[[nodiscard]] bool DropdownLayer_PointerMove(
		DropdownLayer& widget,
		Context& ctx,
		Rect const& windowRect,
		Rect const& usableRect,
		PointerMove_Pointer const& pointer)
	{
		auto const intersection = Rect::Intersection(windowRect, usableRect);

		auto const widgetRect = DropdownLayer_BuildRect(
			widget,
			ctx,
			intersection);

		return widgetRect.PointIsInside(pointer.pos);
	}

	[[nodiscard]] static Layer::Press_Return DropdownLayer_PointerPress(
		DropdownLayer& widget,
		Context& ctx,
		Rect const& usableRect,
		PointerPress_Pointer const& pointer)
	{
		auto& implData = *static_cast<impl::ImplData*>(ctx.Internal_ImplData());

		auto const lineheight = implData.textManager.lineheight;

		auto const widgetRect = DropdownLayer_BuildRect(
			widget,
			ctx,
			usableRect);

		bool const pointerInside = widgetRect.PointIsInside(pointer.pos);
		if (!pointerInside && pointer.pressed)
		{
			widget.pressedLine = Std::nullOpt;

			Layer::Press_Return returnVal = {};
			returnVal.eventConsumed = false;
			returnVal.destroy = true;
			return returnVal;
		}

		if (pointer.type != PointerType::Primary)
		{
			Layer::Press_Return returnVal = {};
			returnVal.eventConsumed = pointerInside;
			returnVal.destroy = !pointerInside;
			return returnVal;
		}
		
		if (pointerInside)
		{
			uSize indexHit = (pointer.pos.y - widgetRect.position.y) / lineheight;

			if (widget.pressedLine.HasValue())
			{
				auto const pressedLine = widget.pressedLine.Value();

				if (!pointer.pressed && pressedLine.index == indexHit && pressedLine.pointerId == pointer.id)
				{
					widget.pressedLine = Std::nullOpt;
					widget.dropdownWidget->selectedItem = indexHit;
					if (widget.dropdownWidget->selectionChangedCallback)
						widget.dropdownWidget->selectionChangedCallback(*widget.dropdownWidget);

					Layer::Press_Return returnVal = {};
					returnVal.eventConsumed = true;
					returnVal.destroy = true;
					return returnVal;
				}
			}
			else
			{
				if (pointer.pressed)
				{
					DropdownLayer::PressedLine newPressedLine = {};
					newPressedLine.index = indexHit;
					newPressedLine.pointerId = pointer.id;

					widget.pressedLine = newPressedLine;

					Layer::Press_Return returnVal = {};
					returnVal.eventConsumed = true;
					returnVal.destroy = false;
					return returnVal;
				}
			}
		}

		return {};
	}

	[[nodiscard]] static bool Dropdown_PointerMove(
		Dropdown& widget,
		Context& ctx,
		WindowID windowId,
		Rect const& widgetRect,
		Rect const& visibleRect,
		PointerMove_Pointer const& pointer)
	{
		bool pointerInside = widgetRect.PointIsInside(pointer.pos) && visibleRect.PointIsInside(pointer.pos);
		
		return pointerInside;
	}

	[[nodiscard]] static bool Dropdown_PointerPress(
		Dropdown& widget,
		Context& ctx,
		WindowID windowId,
		Rect const& widgetRect,
		Rect const& visibleRect,
		PointerPress_Pointer const& pointer)
	{
		bool pointerInside = widgetRect.PointIsInside(pointer.pos) && visibleRect.PointIsInside(pointer.pos);
		if (pointer.pressed && !pointerInside)
			return false;

		// If our pointer-type is not primary, we don't want to do anything,
		// Just consume the event if it's inside the widget and move on.
		if (pointer.type != PointerType::Primary)
		{
			return pointerInside;
		}

		// We can now trust pointer.type == Primary
		if (widget.heldPointerId.HasValue() && widget.heldPointerId.Value() == pointer.id && !pointer.pressed)
		{
			widget.heldPointerId = Std::nullOpt;
			if (pointerInside)
			{
				// Now we open the dropdown menu
				CreateDropdownLayer(widget, ctx, windowId, widgetRect);
			}
			return pointerInside;
		}

		if (pointerInside && !widget.heldPointerId.HasValue() && pointer.pressed)
		{
			widget.heldPointerId = pointer.id;
			return true;
		}

		// We are inside the button, so we always want to consume the event.
		return pointerInside;
	}
}

void impl::DropdownLayer::Render(
	Context const& ctx,
	Rect const& windowRect,
	Rect const& usableRect,
	DrawInfo& drawInfo) const
{
	auto& implData = *static_cast<impl::ImplData*>(ctx.Internal_ImplData());

	auto const lineheight = implData.textManager.lineheight;

	auto const intersectionRect = Rect::Intersection(windowRect, usableRect);

	auto const widgetRect = DropdownLayer_BuildRect(
		*this,
		ctx,
		intersectionRect);

	drawInfo.PushFilledQuad(widgetRect, { 0.25f, 0.25f, 0.25f, 1.f });
	for (uSize i = 0; i < lines.size(); i += 1)
	{
		auto const& line = lines[i];

		auto lineRect = widgetRect;
		lineRect.position.y += lineheight * i;
		lineRect.extent.height = lineheight;

		Math::Vec4 textColor = { 1.f, 1.f, 1.f, 1.f };
		impl::TextManager::RenderText(
			implData.textManager,
			{ line.data(), line.size() },
			textColor,
			lineRect,
			drawInfo);
	}
}

bool impl::DropdownLayer::CursorMove(
	Context& ctx,
	Rect const& windowRect,
	Rect const& usableRect,
	CursorMoveEvent const& event,
	bool occluded)
{
	PointerMove_Pointer pointer = {};
	pointer.id = impl::cursorPointerId;
	pointer.occluded = occluded;
	pointer.pos = { (f32)event.position.x, (f32)event.position.y };
	
	return DropdownLayer_PointerMove(
		*this,
		ctx,
		windowRect,
		usableRect,
		pointer);
}

Layer::Press_Return impl::DropdownLayer::CursorPress(
	Context& ctx,
	Rect const& windowRect,
	Rect const& usableRect,
	Math::Vec2Int cursorPos,
	CursorClickEvent const& event)
{
	impl::PointerPress_Pointer pointer = {};
	pointer.id = impl::cursorPointerId;
	pointer.pos = { (f32)cursorPos.x, (f32)cursorPos.y };
	pointer.pressed = event.clicked;
	pointer.type = impl::ToPointerType(event.button);

	auto const intersection = Rect::Intersection(windowRect, usableRect);

	return DropdownLayer_PointerPress(
		*this,
		ctx,
		intersection,
		pointer);
}

Layer::Press_Return impl::DropdownLayer::TouchPress(
	Context& ctx,
	Rect const& windowRect,
	Rect const& usableRect,
	TouchPressEvent const& event)
{
	impl::PointerPress_Pointer pointer = {};
	pointer.id = event.id;
	pointer.pos = event.position;
	pointer.pressed = event.pressed;
	pointer.type = impl::PointerType::Primary;

	auto const intersection = Rect::Intersection(windowRect, usableRect);

	return DropdownLayer_PointerPress(
		*this,
		ctx,
		intersection,
		pointer);
}

Dropdown::Dropdown()
{
}

Dropdown::~Dropdown()
{
}

SizeHint Dropdown::GetSizeHint(Context const& ctx) const
{
	impl::ImplData& implData = *static_cast<impl::ImplData*>(ctx.Internal_ImplData());

	u32 lineHeight = implData.textManager.lineheight;

	SizeHint returnVal = {};
	returnVal.preferred.height = lineHeight;
	returnVal.preferred.width = 25;

	for (auto& item : items)
	{
		SizeHint childSizeHint = impl::TextManager::GetSizeHint(
			implData.textManager,
			{ item.data(), item.size() });
		returnVal.preferred.width = Math::Max(
			returnVal.preferred.width, 
			childSizeHint.preferred.width);
	}
	
	return returnVal;
}

void Dropdown::Render(
	Context const& ctx, 
	Extent framebufferExtent, 
	Rect widgetRect,
	Rect visibleRect, 
	DrawInfo& drawInfo) const
{
	impl::ImplData& implData = *static_cast<impl::ImplData*>(ctx.Internal_ImplData());

	drawInfo.PushFilledQuad(
		widgetRect,
		{ 0.f, 0.f, 0.f, 0.25f });
	auto const& string = items[selectedItem];
	
	impl::TextManager::RenderText(
		implData.textManager,
		{ string.data(), string.size() },
		{ 1.f, 1.f, 1.f, 1.f },
		widgetRect,
		drawInfo);
}

bool Dropdown::CursorPress(
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

	return impl::Dropdown_PointerPress(
		*this,
		ctx,
		windowId,
		widgetRect,
		visibleRect,
		pointer);
}

bool Dropdown::TouchPressEvent(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	Gui::TouchPressEvent event)
{
	impl::PointerPress_Pointer pointer = {};
	pointer.id = impl::cursorPointerId;
	pointer.pos = event.position;
	pointer.pressed = event.pressed;
	pointer.type = impl::PointerType::Primary;

	return impl::Dropdown_PointerPress(
		*this,
		ctx,
		windowId,
		widgetRect,
		visibleRect,
		pointer);
}
