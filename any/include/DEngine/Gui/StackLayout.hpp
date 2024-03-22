#pragma once

#include <DEngine/Gui/Widget.hpp>

#include <DEngine/Std/Containers/Box.hpp>
#include <DEngine/Math/Vector.hpp>

#include <vector>

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

		[[nodiscard]] uSize ChildCount() const;
		[[nodiscard]] Widget& At(uSize index);
		[[nodiscard]] Widget const& At(uSize index) const;
		void AddWidget(Std::Box<Widget>&& in);
		Std::Box<Widget> ExtractChild(uSize index);
		void InsertWidget(uSize index, Std::Box<Widget>&& in);
		void RemoveItem(uSize index);
		void ClearChildren();



		SizeHint GetSizeHint2(
			GetSizeHint2_Params const& params) const override;
		void BuildChildRects(
			BuildChildRects_Params const& params,
			Rect const& widgetRect,
			Rect const& visibleRect) const override;
		void Render2(
			Render_Params const& params,
			Rect const& widgetRect,
			Rect const& visibleRect) const override;
		void CursorExit(
			Context& ctx) override;
		bool CursorMove(
			CursorMoveParams const& params,
			Rect const& widgetRect,
			Rect const& visibleRect,
			bool occluded) override;
		bool CursorPress2(
			CursorPressParams const& params,
			Rect const& widgetRect,
			Rect const& visibleRect,
			bool consumed) override;
		void TextInput(
			Context& ctx,
			AllocRef const& transientAlloc,
			TextInputEvent const& event) override;
		void TextDelete(
			Context& ctx,
			AllocRef const& transientAlloc,
			WindowID windowId) override;
		virtual void TextSelection(
			Context& ctx,
			AllocRef const& transientAlloc,
			TextSelectionEvent const& event) override;
		void EndTextInputSession(
			Context& ctx,
			AllocRef const& transientAlloc,
			EndTextInputSessionEvent const& event) override;

		bool TouchMove2(
			TouchMoveParams const& params,
			Rect const& widgetRect,
			Rect const& visibleRect,
			bool occluded) override;
		bool TouchPress2(
			TouchPressParams const& params,
			Rect const& widgetRect,
			Rect const& visibleRect,
			bool consumed) override;

		void AccessibilityTest(
			AccessibilityTest_Params const& params,
			Rect const& widgetRect,
			Rect const& visibleRect) const override;

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

		struct Impl;
		friend Impl;
	};
}