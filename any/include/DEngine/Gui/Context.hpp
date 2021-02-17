#pragma once

#include <DEngine/Gui/Events.hpp>
#include <DEngine/Gui/WindowHandler.hpp>
#include <DEngine/Gui/DrawInfo.hpp>

#include <DEngine/Std/Containers/Box.hpp>

#include <string_view>

namespace DEngine::Gui
{
	class StackLayout; // TEMPORARY

	class Widget;

	class Context
	{
	public:
		static Context Create(
			WindowHandler& windowHandler,
			Gfx::Context* gfxCtx);
		Context(Context&&) noexcept;
		Context(Context const&) = delete;

		Context& operator=(Context const&) = delete;
		Context& operator=(Context&&) noexcept;

		void Render() const;

		void PushEvent(CharEnterEvent);
		void PushEvent(CharEvent);
		void PushEvent(CharRemoveEvent);
		void PushEvent(CursorClickEvent);
		void PushEvent(CursorMoveEvent);
		void PushEvent(TouchEvent);
		void PushEvent(WindowCloseEvent);
		void PushEvent(WindowCursorEnterEvent);
		void PushEvent(WindowMinimizeEvent);
		void PushEvent(WindowMoveEvent);
		void PushEvent(WindowResizeEvent);

		void TakeInputConnection(
			Widget& widget,
			SoftInputFilter softInputFilter,
			std::string_view currentText);

		void ClearInputConnection(
			Widget& widget);

		WindowHandler& GetWindowHandler() const;

		void Test_AddMenu(WindowID windowId, Std::Box<Widget>&& widget, Rect rect);
		void Test_DestroyMenu(WindowID windowId, Widget* widget);
		
		// TEMPORARY FIELDS
		StackLayout* outerLayout = nullptr;
		mutable std::vector<Gfx::GuiVertex> vertices;
		mutable std::vector<u32> indices;
		mutable std::vector<Gfx::GuiDrawCmd> drawCmds;
		mutable std::vector<Gfx::NativeWindowUpdate> windowUpdates;

		void* Internal_ImplData() const { return pImplData; }

	private:
		Context() = default;

		void* pImplData = nullptr;
	};
}