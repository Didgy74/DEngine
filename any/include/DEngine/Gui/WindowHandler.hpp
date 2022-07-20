#pragma once

#include <DEngine/FixedWidthTypes.hpp>

#include <DEngine/Gui/CursorType.hpp>
#include <DEngine/Gui/WindowID.hpp>

#include <DEngine/Std/Containers/Span.hpp>

namespace DEngine::Gui
{
	enum class SoftInputFilter : u8
	{
		NoFilter,
		SignedFloat,
		UnsignedFloat,
		SignedInteger,
		UnsignedInteger
	};

	class WindowHandler
	{
	public:
		virtual ~WindowHandler() noexcept = 0;

		virtual void CloseWindow(WindowID) = 0;

		virtual void SetCursorType(WindowID, CursorType) = 0;

		virtual void HideSoftInput() = 0;
		virtual void OpenSoftInput(Std::Span<char const> inputText, SoftInputFilter inputFilter) = 0;
	};

	inline WindowHandler::~WindowHandler() noexcept {}


	struct WindowHandlerCapabilities
	{
		bool lockCursor;
	};
}