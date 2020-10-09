#pragma once

#include "DEngine/Gui/Layout.hpp"
#include "DEngine/Gui/Widget.hpp"

#include <DEngine/Containers/Box.hpp>
#include <DEngine/Math/Vector.hpp>

namespace DEngine::Gui::impl
{
	template<bool tick, typename T, typename Callable>
	void StackLayout_IterateOverChildren(
		Context const& ctx,
		T& layout,
		Rect widgetRect,
		Callable const& callable);
}

namespace DEngine::Gui
{
	class StackLayout : public Layout
	{
	public:
		using ParentType = Layout;

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

		bool test_expand = false;

		u32 ChildCount() const;
		struct Child
		{
			enum class Type
			{
				Widget,
				Layout
			};
			Type type;
			Widget* widget = nullptr;
			Layout* layout = nullptr;
		};
		Child At(u32 index);
		struct ConstChild
		{
			enum class Type
			{
				Widget,
				Layout
			};
			Type type;
			Widget const* widget = nullptr;
			Layout const* layout = nullptr;
		};
		ConstChild At(u32 index) const;
		void AddWidget2(Std::Box<Widget>&& in);
		void AddLayout2(Std::Box<Layout>&& in);
		void InsertLayout(u32 index, Std::Box<Layout>&& in);
		void RemoveItem(u32 index);
		void ClearChildren();

		[[nodiscard]] virtual Gui::SizeHint SizeHint(
			Context const& ctx) const override;

		[[nodiscard]] virtual Gui::SizeHint SizeHint_Tick(
			Context const& ctx) override;

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

		virtual void CursorMove(
			Test& test,
			Rect widgetRect,
			Rect visibleRect,
			CursorMoveEvent event) override;

		virtual void CursorClick(
			Context& ctx,
			Rect widgetRect,
			Rect visibleRect,
			Math::Vec2Int cursorPos,
			CursorClickEvent event) override;

		virtual void TouchEvent(
			Context& ctx,
			Rect widgetRect,
			Rect visibleRect,
			Gui::TouchEvent event) override;

		virtual void Tick(
			Context& ctx,
			Rect widgetRect,
			Rect visibleRect) override;

	protected:
		struct LayoutItem
		{
			enum class Type
			{
				Widget,
				Layout
			};
			Type type;
			Std::Box<Widget> widget;
			Std::Box<Layout> layout;
		};
		std::vector<LayoutItem> children;

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
				static constexpr u32 pushBackIndex = static_cast<u32>(-1);
				u32 index{};
			};
			Insert insert;
			struct Remove
			{
				u32 index;
			};
			Remove remove;
		};
		mutable std::vector<InsertRemoveJob> insertionJobs;
		mutable bool currentlyIterating = false;

		template<bool tick, typename T, typename Callable>
		friend void impl::StackLayout_IterateOverChildren(
			Context const& ctx,
			T& layout,
			Rect widgetRect,
			Callable const& callable);
	};
}