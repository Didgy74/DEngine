#include "EditorImpl.hpp"

using namespace DEngine;
using namespace DEngine::Editor;

void Editor::EditorImpl::ButtonEvent(
	App::Button button,
	bool state)
{
	if (button == App::Button::LeftMouse || button == App::Button::RightMouse)
	{
		Gui::CursorClickEvent event = {};
		event.clicked = state;

		if (button == App::Button::LeftMouse)
			event.button = Gui::CursorButton::Primary;
		else if (button == App::Button::RightMouse)
			event.button = Gui::CursorButton::Secondary;

		queuedGuiEvents.emplace_back(event);
	}
}

void Editor::EditorImpl::CharEnterEvent()
{
	Gui::CharEnterEvent event = {};
	queuedGuiEvents.emplace_back(event);
}

void Editor::EditorImpl::CharEvent(
	u32 value)
{
	Gui::CharEvent event = {};
	event.utfValue = value;
	queuedGuiEvents.emplace_back(event);
}

void Editor::EditorImpl::CharRemoveEvent()
{
	Gui::CharRemoveEvent event = {};
	queuedGuiEvents.push_back(event);
}

void Editor::EditorImpl::CursorMove(
	Math::Vec2Int position,
	Math::Vec2Int positionDelta)
{
	Gui::CursorMoveEvent event = {};
	event.position = position;
	event.positionDelta = positionDelta;
	queuedGuiEvents.emplace_back(event);
}

void Editor::EditorImpl::TouchEvent(
	u8 id,
	App::TouchEventType type,
	Math::Vec2 position)
{
	if (type == App::TouchEventType::Down || type == App::TouchEventType::Up)
	{
		Gui::TouchPressEvent event = {};
		event.id = id;
		event.position = position;
		event.pressed = type == App::TouchEventType::Down;
		queuedGuiEvents.emplace_back(event);
	}
	else if (type == App::TouchEventType::Moved)
	{
		Gui::TouchMoveEvent event = {};
		event.id = id;
		event.position = position;
		queuedGuiEvents.emplace_back(event);
	}
}

void Editor::EditorImpl::WindowClose(App::WindowID windowId)
{
	Gui::WindowCloseEvent event = {};
	event.windowId = (Gui::WindowID)windowId;
	queuedGuiEvents.emplace_back(event);
}

void Editor::EditorImpl::WindowCursorEnter(
	App::WindowID window,
	bool entered)
{
	Gui::WindowCursorEnterEvent event = {};
	event.windowId = (Gui::WindowID)window;
	event.entered = entered;
	queuedGuiEvents.emplace_back(event);
}

void Editor::EditorImpl::WindowMinimize(
	App::WindowID window,
	bool wasMinimized)
{
	Gui::WindowMinimizeEvent event = {};
	event.windowId = (Gui::WindowID)window;
	event.wasMinimized = wasMinimized;
	queuedGuiEvents.emplace_back(event);
}

void Editor::EditorImpl::WindowMove(
	App::WindowID window,
	Math::Vec2Int position)
{
	Gui::WindowMoveEvent event = {};
	event.windowId = (Gui::WindowID)window;
	event.position = position;
	queuedGuiEvents.emplace_back(event);
}

void Editor::EditorImpl::WindowResize(
	App::WindowID window,
	App::Extent newExtent,
	Math::Vec2Int visiblePos,
	App::Extent visibleSize)
{
	Gui::WindowResizeEvent event = {};
	event.windowId = (Gui::WindowID)window;
	event.extent = Gui::Extent{ newExtent.width, newExtent.height };
	event.visibleRect = Gui::Rect{ visiblePos, Gui::Extent{ visibleSize.width, visibleSize.height } };
	queuedGuiEvents.emplace_back(event);
}

void Editor::EditorImpl::CloseWindow(Gui::WindowID id)
{
	App::DestroyWindow((App::WindowID)id);
}

void Editor::EditorImpl::SetCursorType(Gui::WindowID id, Gui::CursorType cursorType)
{
	App::CursorType appCursorType{};
	switch (cursorType)
	{
	case Gui::CursorType::Arrow:
		appCursorType = App::CursorType::Arrow;
		break;
	case Gui::CursorType::HorizontalResize:
		appCursorType = App::CursorType::HorizontalResize;
		break;
	case Gui::CursorType::VerticalResize:
		appCursorType = App::CursorType::VerticalResize;
		break;
	default:
		DENGINE_IMPL_UNREACHABLE();
		break;
	}
	App::SetCursor((App::WindowID)id, appCursorType);
}

void Editor::EditorImpl::HideSoftInput()
{
	App::HideSoftInput();
}

void Editor::EditorImpl::OpenSoftInput(
	Std::Str currentText,
	Gui::SoftInputFilter inputFilter)
{
	App::SoftInputFilter filter{};
	switch (inputFilter)
	{
	case Gui::SoftInputFilter::SignedFloat:
		filter = App::SoftInputFilter::Float;
		break;
	case Gui::SoftInputFilter::UnsignedFloat:
		filter = App::SoftInputFilter::UnsignedFloat;
		break;
	case Gui::SoftInputFilter::SignedInteger:
		filter = App::SoftInputFilter::Integer;
		break;
	case Gui::SoftInputFilter::UnsignedInteger:
		filter = App::SoftInputFilter::UnsignedInteger;
		break;
	default:
		DENGINE_IMPL_UNREACHABLE();
	}
	App::OpenSoftInput(currentText, filter);
}