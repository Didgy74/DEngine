#pragma once

#include <DEngine/Gui/Layout.hpp>
#include <DEngine/Gui/StackLayout.hpp>

#include <DEngine/Std/Containers/Opt.hpp>

#include <functional>
#include <string>
#include <string_view>

namespace DEngine::Gui
{
	class MenuBar : public Layout
	{
	public:
		using SubmenuActivateCallback = void(
			MenuBar& newMenuBar);
		using ButtonActivateCallback = void();

		class Button : public Widget
		{
		public:
			std::string title;
			bool active = false;
			MenuBar* parentMenuBar = nullptr;
			Layout* menuPtr = nullptr;
			std::function<SubmenuActivateCallback> activateCallback;

			Button(
				MenuBar* parentMenuBar,
				std::string_view title,
				std::function<SubmenuActivateCallback> callback);

			void Clear(
				Context& ctx,
				WindowID windowId);

			[[nodiscard]] virtual Gui::SizeHint GetSizeHint(
				Context const& ctx) const override;

			virtual void Render(
				Context const& ctx,
				Extent framebufferExtent,
				Rect widgetRect,
				Rect visibleRect,
				DrawInfo& drawInfo) const override;

			virtual void CursorMove(
				Context& ctx,
				WindowID windowId,
				Rect widgetRect,
				Rect visibleRect,
				CursorMoveEvent event) override;

			virtual void CursorClick(
				Context& ctx,
				WindowID windowId,
				Rect widgetRect,
				Rect visibleRect,
				Math::Vec2Int cursorPos,
				CursorClickEvent event) override;

			virtual void TouchEvent(
				Context& ctx,
				WindowID windowId,
				Rect widgetRect,
				Rect visibleRect,
				Gui::TouchEvent event) override;
		};

		class ActivatableButton : public Widget
		{
		public:
			std::string title;
			MenuBar* parentMenuBar = nullptr;

			std::function<ButtonActivateCallback> activateCallback;

			ActivatableButton(
				MenuBar* parentMenuBar, 
				std::string_view title,
				std::function<ButtonActivateCallback> callback);

			[[nodiscard]] virtual SizeHint GetSizeHint(
				Context const& ctx) const override;

			virtual void Render(
				Context const& ctx,
				Extent framebufferExtent,
				Rect widgetRect,
				Rect visibleRect,
				DrawInfo& drawInfo) const override;

			virtual void CursorClick(
				Context& ctx,
				WindowID windowId,
				Rect widgetRect,
				Rect visibleRect,
				Math::Vec2Int cursorPos,
				CursorClickEvent event) override;

			virtual void TouchEvent(
				Context& ctx,
				WindowID windowId,
				Rect widgetRect,
				Rect visibleRect,
				Gui::TouchEvent event) override;
		};

		enum class Direction : u8
		{
			Horizontal,
			Vertical,
		};

		MenuBar* parentMenuBar = nullptr;
		StackLayout stackLayout;
		Button* activeButton = nullptr;

		MenuBar(MenuBar* parentMenuBar, Direction dir);

		void ClearActiveButton(
			Context& ctx,
			WindowID windowId);
		
		void AddSubmenuButton(
			std::string_view title,
			std::function<SubmenuActivateCallback> callback);

		void AddMenuButton(
			std::string_view title,
			std::function<ButtonActivateCallback> callback);

		Direction GetDirection() const;

		[[nodiscard]] virtual Gui::SizeHint GetSizeHint(
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

		virtual void CursorMove(
			Context& ctx,
			WindowID windowId,
			Rect widgetRect,
			Rect visibleRect,
			CursorMoveEvent event) override;

		virtual void CursorClick(
			Context& ctx,
			WindowID windowId,
			Rect widgetRect,
			Rect visibleRect,
			Math::Vec2Int cursorPos,
			CursorClickEvent event) override;

		virtual void TouchEvent(
			Context& ctx,
			WindowID windowId,
			Rect widgetRect,
			Rect visibleRect,
			Gui::TouchEvent event) override;
	};
}