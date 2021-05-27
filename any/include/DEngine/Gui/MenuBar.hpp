#pragma once

#include <DEngine/Gui/Widget.hpp>
#include <DEngine/Gui/StackLayout.hpp>

#include <DEngine/Std/Containers/Opt.hpp>
#include <DEngine/Std/Containers/Str.hpp>

#include <functional>
#include <string>

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
				Std::Str title,
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

			virtual bool CursorMove(
				Context& ctx,
				WindowID windowId,
				Rect widgetRect,
				Rect visibleRect,
				CursorMoveEvent event,
				bool occluded) override;

			virtual bool CursorPress(
				Context& ctx,
				WindowID windowId,
				Rect widgetRect,
				Rect visibleRect,
				Math::Vec2Int cursorPos,
				CursorClickEvent event) override;

			virtual bool TouchMoveEvent(
				Context& ctx,
				WindowID windowId,
				Rect widgetRect,
				Rect visibleRect,
				Gui::TouchMoveEvent event,
				bool occluded) override;

			virtual bool TouchPressEvent(
				Context& ctx,
				WindowID windowId,
				Rect widgetRect,
				Rect visibleRect,
				Gui::TouchPressEvent event) override;
		};

		class ActivatableButton : public Widget
		{
		public:
			MenuBar* parentMenuBar = nullptr;
			std::string title;
			std::function<ButtonActivateCallback> activateCallback;

			ActivatableButton(
				MenuBar* parentMenuBar, 
				Std::Str title,
				std::function<ButtonActivateCallback> callback);

			[[nodiscard]] virtual SizeHint GetSizeHint(
				Context const& ctx) const override;

			virtual void Render(
				Context const& ctx,
				Extent framebufferExtent,
				Rect widgetRect,
				Rect visibleRect,
				DrawInfo& drawInfo) const override;

			virtual bool CursorPress(
				Context& ctx,
				WindowID windowId,
				Rect widgetRect,
				Rect visibleRect,
				Math::Vec2Int cursorPos,
				CursorClickEvent even) override;

			virtual bool TouchMoveEvent(
				Context& ctx,
				WindowID windowId,
				Rect widgetRect,
				Rect visibleRect,
				Gui::TouchMoveEvent event,
				bool occluded) override;

			virtual bool TouchPressEvent(
				Context& ctx,
				WindowID windowId,
				Rect widgetRect,
				Rect visibleRect,
				Gui::TouchPressEvent event) override;
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
				Std::Str title,
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

			virtual bool CursorPress(
				Context& ctx,
				WindowID windowId,
				Rect widgetRect,
				Rect visibleRect,
				Math::Vec2Int cursorPos,
				CursorClickEvent event) override;

			virtual bool TouchMoveEvent(
				Context& ctx,
				WindowID windowId,
				Rect widgetRect,
				Rect visibleRect,
				Gui::TouchMoveEvent event,
				bool occluded) override;

			virtual bool TouchPressEvent(
				Context& ctx,
				WindowID windowId,
				Rect widgetRect,
				Rect visibleRect,
				Gui::TouchPressEvent event) override;
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
			Std::Str title,
			std::function<SubmenuActivateCallback> callback);

		void AddMenuButton(
			Std::Str title,
			std::function<ButtonActivateCallback> callback);

		void AddToggleMenuButton(
			Std::Str title,
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

		virtual bool CursorMove(
			Context& ctx,
			WindowID windowId,
			Rect widgetRect,
			Rect visibleRect,
			CursorMoveEvent event,
			bool occluded) override;

		virtual bool CursorPress(
			Context& ctx,
			WindowID windowId,
			Rect widgetRect,
			Rect visibleRect,
			Math::Vec2Int cursorPos,
			CursorClickEvent event) override;

		virtual bool TouchMoveEvent(
			Context& ctx,
			WindowID windowId,
			Rect widgetRect,
			Rect visibleRect,
			Gui::TouchMoveEvent event,
			bool occluded) override;

		virtual bool TouchPressEvent(
			Context& ctx,
			WindowID windowId,
			Rect widgetRect,
			Rect visibleRect,
			Gui::TouchPressEvent event) override;
	};
}