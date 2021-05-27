#pragma once

#include <DEngine/Gui/Events.hpp>
#include <DEngine/Gui/WindowHandler.hpp>
#include <DEngine/Gui/DrawInfo.hpp>

#include <DEngine/Std/Containers/Box.hpp>
#include <DEngine/Std/Containers/Str.hpp>

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

		void Render(
			std::vector<Gfx::GuiVertex>& vertices,
			std::vector<u32>& indices,
			std::vector<Gfx::GuiDrawCmd>& drawCmds,
			std::vector<Gfx::NativeWindowUpdate>& windowUpdates) const;

		void PushEvent(CharEnterEvent);
		void PushEvent(CharEvent);
		void PushEvent(CharRemoveEvent);
		void PushEvent(CursorClickEvent);
		void PushEvent(CursorMoveEvent);
		void PushEvent(TouchMoveEvent);
		void PushEvent(TouchPressEvent);
		void PushEvent(WindowCloseEvent);
		void PushEvent(WindowCursorEnterEvent);
		void PushEvent(WindowMinimizeEvent);
		void PushEvent(WindowMoveEvent);
		void PushEvent(WindowResizeEvent);

		void TakeInputConnection(
			Widget& widget,
			SoftInputFilter softInputFilter,
			Std::Str currentText);

		void ClearInputConnection(
			Widget& widget);

		WindowHandler& GetWindowHandler() const;

		void Test_AddMenu(WindowID windowId, Std::Box<Widget>&& widget, Rect rect);
		void Test_DestroyMenu(WindowID windowId, Widget* widget);
		
		// TEMPORARY FIELDS
		StackLayout* outerLayout = nullptr;

		void* Internal_ImplData() const { return pImplData; }

	private:
		Context() = default;

		void* pImplData = nullptr;
	};
}