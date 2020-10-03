#pragma once

#include <DEngine/Gui/CursorType.hpp>
#include <DEngine/Gui/WindowID.hpp>
#include <DEngine/Gui/WindowHandler.hpp>
#include <DEngine/Gui/Utility.hpp>

#include <string_view>

namespace DEngine::Gui
{
	class Context;

	// This class is an interface to the top-level context and
	// to the Context and the WindowHandler, and also contains window-local information.
	// The lifetime of this class is tied to the lifetime of each event.
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
		WindowHandler& GetWindowHandler() const;
		Rect GetWindowRect() const;
		Context& GetContext() const;

		void SetCursorType(CursorType);
		void OpenSoftInput(std::string_view currentText, SoftInputFilter inputFilter);

	private:
		Context& ctx;
		WindowHandler& windowHandler;
		WindowID windowId{};
		Rect windowRect{};
	};
}