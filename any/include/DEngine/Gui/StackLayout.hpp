#pragma once

#include <DEngine/Gui/Widget.hpp>

#include <DEngine/Std/Containers/Box.hpp>
#include <DEngine/Math/Vector.hpp>

#include <vector>

namespace DEngine::Gui::impl
{
	class StackLayoutImpl;
}

namespace DEngine::Gui
{
	class StackLayout : public Widget
	{
	public:
		using ParentType = Widget;

		enum class Direction
		{
			Horizontal,
			Vertical,
		};
		using Dir = Direction;

		StackLayout(Direction direction = Direction::Horizontal) : 
			direction(direction)
		{}

		virtual ~StackLayout() override;

		Direction direction{};
		Math::Vec4 color{ 1.f, 1.f, 1.f, 0.f };

		i32 padding = 0;
		i32 spacing = 0;

		bool expandNonDirection = false;

		uSize ChildCount() const;
		Widget& At(uSize index);
		Widget const& At(uSize index) const;
		void AddWidget(Std::Box<Widget>&& in);
		Std::Box<Widget> ExtractChild(uSize index);
		void InsertWidget(uSize index, Std::Box<Widget>&& in);
		void RemoveItem(uSize index);
		void ClearChildren();

		[[nodiscard]] virtual SizeHint GetSizeHint(
			Context const& ctx) const override;

		virtual SizeHint GetSizeHint2(
			GetSizeHint2_Params const& params) const override;

		virtual void BuildChildRects(
			BuildChildRects_Params const& params,
			Rect const& widgetRect,
			Rect const& visibleRect) const override;

		virtual void Render(
			Context const& ctx,
			Extent framebufferExtent,
			Rect widgetRect,
			Rect visibleRect,
			DrawInfo& drawInfo) const override;

		virtual void Render2(
			Render_Params const& params,
			Rect const& widgetRect,
			Rect const& visibleRect) const override;

		virtual void CharEnterEvent(
			Context& ctx) override;

		virtual void CharEvent(
			Context& ctx,
			u32 utfValue) override;

		virtual void CharRemoveEvent(
			Context& ctx) override;

		virtual void InputConnectionLost() override;

		virtual void CursorExit(
			Context& ctx) override;

		virtual bool CursorPress(
			Context& ctx,
			WindowID windowId,
			Rect widgetRect,
			Rect visibleRect,
			Math::Vec2Int cursorPos,
			CursorPressEvent event) override;

		virtual bool CursorPress2(CursorPressParams const& params) override;

		virtual bool CursorMove(
			Context& ctx,
			WindowID windowId,
			Rect widgetRect,
			Rect visibleRect,
			CursorMoveEvent event,
			bool occluded) override;

		virtual bool CursorMove(
			CursorMoveParams const& params,
			bool occluded) override;

		virtual bool TouchPressEvent(
			Context& ctx,
			WindowID windowId,
			Rect widgetRect,
			Rect visibleRect,
			Gui::TouchPressEvent event) override;

		virtual bool TouchMoveEvent(
			Context& ctx,
			WindowID windowId,
			Rect widgetRect,
			Rect visibleRect,
			Gui::TouchMoveEvent event,
			bool occluded) override;

	protected:
		std::vector<Std::Box<Widget>> children;

		struct InsertRemoveJob
		{
			enum class Type : u8
			{
				Insert,
				Remove
			};
			Type type{};
			struct Insert
			{
				static constexpr uSize pushBackIndex = static_cast<uSize>(-1);
				uSize index{};
			};
			Insert insert;
			struct Remove
			{
				uSize index;
			};
			Remove remove;
		};
		std::vector<InsertRemoveJob> insertionJobs;
		bool currentlyIterating = false;
		
		friend impl::StackLayoutImpl;
	};
}