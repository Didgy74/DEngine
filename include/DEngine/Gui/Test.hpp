#pragma once

#include <DEngine/Gui/CursorType.hpp>
#include <DEngine/Gui/WindowID.hpp>
#include <DEngine/Gui/Utility.hpp>

namespace DEngine::Gui
{
	class Context;
	class WindowHandler;

	class Test
	{
	public:
		Test(
			Context& ctx,
			WindowHandler& windowHandler,
			WindowID windowId,
			Rect windowRect) noexcept;
		Test(Test const&) = delete;

		WindowID GetWindowID() const;
		Rect GetWindowRect() const;
		Context& GetContext() const;

		void SetCursorType(CursorType);

	private:
		Context& ctx;
		WindowHandler& windowHandler;
		WindowID windowId{};
		Rect windowRect{};
	};
}