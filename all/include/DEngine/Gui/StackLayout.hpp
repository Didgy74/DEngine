#pragma once

#include <DEngine/Gui/Widget.hpp>

#include <DEngine/Std/Containers/Box.hpp>
#include <DEngine/Math/Vector.hpp>

namespace DEngine::Gui::impl
{
	template<typename T, typename Callable>
	void StackLayout_IterateOverChildren(
		Context const& ctx,
		T& layout,
		Rect widgetRect,
		Callable const& callable);
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

		virtual void Render(
			Context const& ctx,
			Extent framebufferExtent,
			Rect widgetRect,
			Rect visibleRect,
			DrawInfo& drawInfo) const override;

		virtual void CharEnterEvent(
			Context& ctx) override;

		virtual void CharEvent(
			Context& ctx,
			u32 utfValue) override;

		virtual void CharRemoveEvent(
			Context& ctx) override;

		virtual void InputConnectionLost() override;

		virtual void CursorClick(
			Context& ctx,
			WindowID windowId,
			Rect widgetRect,
			Rect visibleRect,
			Math::Vec2Int cursorPos,
			CursorClickEvent event) override;

		virtual void CursorMove(
			Context& ctx,
			WindowID windowId,
			Rect widgetRect,
			Rect visibleRect,
			CursorMoveEvent event) override;

		virtual void TouchEvent(
			Context& ctx,
			WindowID windowId,
			Rect widgetRect,
			Rect visibleRect,
			Gui::TouchEvent event) override;

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
		mutable std::vector<InsertRemoveJob> insertionJobs;
		mutable bool currentlyIterating = false;

		template<typename T, typename Callable>
		friend void impl::StackLayout_IterateOverChildren(
			Context const& ctx,
			T& layout,
			Rect widgetRect,
			Callable const& callable);
	};
}