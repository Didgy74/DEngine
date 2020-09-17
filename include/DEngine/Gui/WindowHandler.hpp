#pragma once

#include <DEngine/Gui/CursorType.hpp>
#include <DEngine/Gui/WindowID.hpp>


namespace DEngine::Gui
{
	class WindowHandler
	{
	public:
		virtual ~WindowHandler() noexcept = 0;

		virtual void SetCursorType(WindowID, CursorType) = 0;
	};

	inline WindowHandler::~WindowHandler() noexcept {}
}