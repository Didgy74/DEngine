#pragma once

#include "DEngine/Gui/Layout.hpp"
#include "DEngine/Gui/Widget.hpp"

#include <DEngine/Containers/Box.hpp>
#include <DEngine/Math/Vector.hpp>

namespace DEngine::Gui
{
	class StackLayout : public Layout
	{
	public:
		enum class Direction
		{
			Horizontal,
			Vertical,
		};

		StackLayout(Direction direction = Direction::Horizontal) : 
			direction(direction)
		{}

		virtual ~StackLayout() override {}

		Direction direction{};
		Math::Vec4 color{ 1.f, 1.f, 1.f, 0.f };

		struct LayoutItem
		{
			f32 relativeSize = 1.f;
			bool sizeLocked = false;
			enum class ItemType
			{
				Widget,
				Layout
			};
			ItemType type{};
			Std::Box<Widget> widget;
			Std::Box<Layout> layout;
		};
		std::vector<LayoutItem> children;
		i32 padding = 0;
		i32 spacing = 0;

		virtual void Render(
			Context& ctx,
			Extent framebufferExtent,
			Rect widgetRect,
			DrawInfo& drawInfo) const override;

		void SetChildRelativeSize(u32 index, f32 input, bool locked);

		void AddWidget2(Std::Box<Widget>&& in);
		void AddLayout2(Std::Box<Layout>&& in);
		void InsertLayout(u32 index, Std::Box<Layout>&& in);
		void RemoveItem(u32 index);
		void ClearChildren();

		virtual void CharEvent(
			Context& ctx,
			u32 utfValue) override;

		virtual void CharRemoveEvent(
			Context& ctx) override;

		virtual void CursorMove(
			Rect widgetRect,
			CursorMoveEvent event) override;

		virtual void CursorClick(
			Rect widgetRect,
			Math::Vec2Int cursorPos,
			CursorClickEvent event) override;

		virtual void TouchEvent(
			Rect widgetRect,
			Gui::TouchEvent touch) override;

		virtual void Tick(
			Context& ctx,
			Rect widgetRect) override;
	};
}