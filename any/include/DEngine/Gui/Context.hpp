#pragma once

#include <DEngine/Gui/Events.hpp>
#include <DEngine/Gui/WindowHandler.hpp>
#include <DEngine/Gui/DrawInfo.hpp>

#include <DEngine/Std/Containers/Box.hpp>
#include <DEngine/Std/Containers/Span.hpp>

#include <functional>
#include <string>
#include <vector>

namespace DEngine::Gui
{
	class Widget;
	class Layer;

	class Context
	{
	public:
		static Context Create(
			WindowHandler& windowHandler,
			Gfx::Context* gfxCtx);
		Context(Context&&) noexcept;
		Context(Context const&) noexcept = delete;

		Context& operator=(Context const&) noexcept = delete;
		Context& operator=(Context&&) noexcept;

		void Render(
			std::vector<Gfx::GuiVertex>& vertices,
			std::vector<u32>& indices,
			std::vector<Gfx::GuiDrawCmd>& drawCmds,
			std::vector<Gfx::NativeWindowUpdate>& windowUpdates) const;

		void PushEvent(CharEnterEvent const&);
		void PushEvent(CharEvent const&);
		void PushEvent(CharRemoveEvent const&);
		void PushEvent(CursorClickEvent const&);
		void PushEvent(CursorMoveEvent const&);
		void PushEvent(TouchMoveEvent const&);
		void PushEvent(TouchPressEvent const&);
		void PushEvent(WindowCloseEvent const&);
		void PushEvent(WindowCursorEnterEvent const&);
		void PushEvent(WindowMinimizeEvent const&);
		void PushEvent(WindowMoveEvent const&);
		void PushEvent(WindowResizeEvent const&);

		void AdoptWindow(
			WindowID id,
			Math::Vec4 const& clearColor,
			Rect const& windowRect,
			Rect const& visibleRect,
			Std::Box<Widget>&& widget);

		void TakeInputConnection(
			Widget& widget,
			SoftInputFilter softInputFilter,
			Std::Span<char const> currentText);

		void ClearInputConnection(
			Widget& widget);

		WindowHandler& GetWindowHandler() const;

		void SetFrontmostLayer(
			WindowID windowId,
			Std::Box<Layer>&& layer);

		void* Internal_ImplData() const { return pImplData; }

	private:
		Context() = default;

		void* pImplData = nullptr;
	};
}