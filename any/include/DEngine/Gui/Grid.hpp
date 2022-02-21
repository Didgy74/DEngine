#pragma once

#include <DEngine/Gui/Widget.hpp>

#include <DEngine/Std/Containers/Box.hpp>

#include <vector>

namespace DEngine::Gui
{
	class Grid : public Widget
	{
	public:
		u32 spacing = 10;

		void SetDimensions(int width, int height);
		void SetWidth(int newWidth);
		[[nodiscard]] int GetWidth() const noexcept;
		void SetHeight(int newHeight);
		[[nodiscard]] int GetHeight() const noexcept;

		int PushBackColumn();
		int PushBackRow();

		void SetChild(int x, int y, Std::Box<Widget>&& in);
		Widget* GetChild(int x, int y);
		Widget const* GetChild(int x, int y) const;

		virtual SizeHint GetSizeHint2(
			GetSizeHint2_Params const& params) const override;
		virtual void BuildChildRects(
			BuildChildRects_Params const& params,
			Rect const& widgetRect,
			Rect const& visibleRect) const override;
		virtual void Render2(
			Render_Params const& params,
			Rect const& widgetRect,
			Rect const& visibleRect) const override;
		virtual bool CursorMove(
			CursorMoveParams const& params,
			Rect const& widgetRect,
			Rect const& visibleRect,
			bool occluded) override;
		virtual bool CursorPress2(
			CursorPressParams const& params,
			Rect const& widgetRect,
			Rect const& visibleRect,
			bool consumed) override;
		virtual void CharRemoveEvent(
			Context& ctx,
			Std::FrameAlloc& transientAlloc) override;
		virtual void TextInput(
			Context& ctx,
			Std::FrameAlloc& transientAlloc,
			TextInputEvent const& event) override;

	protected:
		// Elements are stored row-major.
		std::vector<Std::Box<Widget>> children;
		int width = 0;
		int height = 0;

		struct Impl;
		friend Impl;
	};
}