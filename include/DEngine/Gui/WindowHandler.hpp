#pragma once

#include <DEngine/Gui/CursorType.hpp>
#include <DEngine/Gui/WindowID.hpp>

#include <string_view>

namespace DEngine::Gui
{
	enum class SoftInputFilter : u8
	{
		Float,
		Integer,
	};

	class WindowHandler
	{
	public:
		virtual ~WindowHandler() noexcept = 0;

		virtual void CloseWindow(WindowID) = 0;

		virtual void SetCursorType(WindowID, CursorType) = 0;

		virtual void OpenSoftInput(WindowID, std::string_view currentText, SoftInputFilter inputFilter) = 0;
	};

	inline WindowHandler::~WindowHandler() noexcept {}
}