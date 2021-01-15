#pragma once

#include <DEngine/Gui/Widget.hpp>
#include <DEngine/Gui/StackLayout.hpp>

#include <DEngine/Std/Containers/Opt.hpp>

#include <functional>
#include <string>
#include <string_view>

namespace DEngine::Gui
{
	class MenuBar : public Widget
	{
	public:
		using SubmenuActivateCallback = void(
			MenuBar& newMenuBar);
		using ButtonActivateCallback = void();
		using ToggleButtonActivateCallback = void(bool);

	private:
		class Button : public Widget
		{
		public:
			MenuBar* parentMenuBar = nullptr;
			std::string title;
			bool active = false;
			std::function<SubmenuActivateCallback> activateCallback;
			Widget* menuPtr = nullptr;

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
			MenuBar* parentMenuBar = nullptr;
			std::string title;
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

		class ToggleButton : public Widget
		{
		public:
			MenuBar* parentMenuBar = nullptr;
			std::string title;
			bool toggled = false;
			std::function<ToggleButtonActivateCallback> activateCallback;

			ToggleButton(
				MenuBar* parentMenuBar,
				std::string_view title,
				bool toggled,
				std::function<ToggleButtonActivateCallback> callback);
			
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

	public:

		enum class Direction : u8
		{
			Horizontal,
			Vertical,
		};

		MenuBar* parentMenuBar = nullptr;
		StackLayout stackLayout;
		Button* activeButton = nullptr;

	public:
		MenuBar(Direction dir);
	private:
		MenuBar(MenuBar* parentMenuBar, Direction dir);
	public:

		void ClearActiveButton(
			Context& ctx,
			WindowID windowId);
		
		void AddSubmenuButton(
			std::string_view title,
			std::function<SubmenuActivateCallback> callback);

		void AddMenuButton(
			std::string_view title,
			std::function<ButtonActivateCallback> callback);

		void AddToggleMenuButton(
			std::string_view title,
			bool toggled,
			std::function<ToggleButtonActivateCallback> callback);

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