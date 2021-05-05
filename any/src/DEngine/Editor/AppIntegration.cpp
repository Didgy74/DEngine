#include "EditorImpl.hpp"

using namespace DEngine;
using namespace DEngine::Editor;

void Editor::EditorImpl::ButtonEvent(
	App::Button button,
	bool state)
{
	if (button == App::Button::LeftMouse || button == App::Button::RightMouse)
	{
		impl::GuiEvent event{};
		event.type = impl::GuiEvent::Type::CursorClickEvent;
		if (button == App::Button::LeftMouse)
			event.cursorClick.button = Gui::CursorButton::Primary;
		else if (button == App::Button::RightMouse)
			event.cursorClick.button = Gui::CursorButton::Secondary;
		event.cursorClick.clicked = state;
		queuedGuiEvents.push_back(event);
	}
	else if (button == App::Button::Enter)
	{
		impl::GuiEvent event{};
		event.type = impl::GuiEvent::Type::CharEnterEvent;
		event.charEnter = {};
		queuedGuiEvents.push_back(event);
	}
}

void Editor::EditorImpl::CharEnterEvent()
{
	impl::GuiEvent event{};
	event.type = impl::GuiEvent::Type::CharEnterEvent;
	event.charEnter = {};
	queuedGuiEvents.push_back(event);
}

void Editor::EditorImpl::CharEvent(
	u32 value)
{
	impl::GuiEvent event{};
	event.type = impl::GuiEvent::Type::CharEvent;
	event.charEvent.utfValue = value;
	queuedGuiEvents.push_back(event);
}

void Editor::EditorImpl::CharRemoveEvent()
{
	impl::GuiEvent event{};
	event.type = impl::GuiEvent::Type::CharRemoveEvent;
	event.charRemove = {};
	queuedGuiEvents.push_back(event);
}

void Editor::EditorImpl::CursorMove(
	Math::Vec2Int position,
	Math::Vec2Int positionDelta)
{
	impl::GuiEvent event{};
	event.type = impl::GuiEvent::Type::CursorMoveEvent;
	event.cursorMove.position = position;
	event.cursorMove.positionDelta = positionDelta;
	queuedGuiEvents.push_back(event);
}

void Editor::EditorImpl::TouchEvent(
	u8 id,
	App::TouchEventType type,
	Math::Vec2 position)
{
	impl::GuiEvent event{};
	event.type = impl::GuiEvent::Type::TouchEvent;
	event.touch.id = id;
	event.touch.position = position;
	if (type == App::TouchEventType::Down)
		event.touch.type = Gui::TouchEventType::Down;
	else if (type == App::TouchEventType::Moved)
		event.touch.type = Gui::TouchEventType::Moved;
	else if (type == App::TouchEventType::Up)
		event.touch.type = Gui::TouchEventType::Up;
	queuedGuiEvents.push_back(event);
}

void Editor::EditorImpl::WindowClose(App::WindowID windowId)
{
	impl::GuiEvent event{};
	event.type = impl::GuiEvent::Type::WindowCloseEvent;
	event.windowClose.windowId = (Gui::WindowID)windowId;
	queuedGuiEvents.push_back(event);
}

void Editor::EditorImpl::WindowCursorEnter(
	App::WindowID window,
	bool entered)
{
	impl::GuiEvent event{};
	event.type = impl::GuiEvent::Type::WindowCursorEnterEvent;
	event.windowCursorEnter.windowId = (Gui::WindowID)window;
	event.windowCursorEnter.entered = entered;
	queuedGuiEvents.push_back(event);
}

void Editor::EditorImpl::WindowMinimize(
	App::WindowID window,
	bool wasMinimized)
{
	impl::GuiEvent event{};
	event.type = impl::GuiEvent::Type::WindowMinimizeEvent;
	event.windowMinimize.windowId = (Gui::WindowID)window;
	event.windowMinimize.wasMinimized = wasMinimized;
	queuedGuiEvents.push_back(event);
}

void Editor::EditorImpl::WindowMove(
	App::WindowID window,
	Math::Vec2Int position)
{
	impl::GuiEvent event{};
	event.type = impl::GuiEvent::Type::WindowMoveEvent;
	event.windowMove.windowId = (Gui::WindowID)window;
	event.windowMove.position = position;
	queuedGuiEvents.push_back(event);
}

void Editor::EditorImpl::WindowResize(
	App::WindowID window,
	App::Extent newExtent,
	Math::Vec2Int visiblePos,
	App::Extent visibleSize)
{
	impl::GuiEvent event{};
	event.type = impl::GuiEvent::Type::WindowResizeEvent;
	event.windowResize.windowId = (Gui::WindowID)window;
	event.windowResize.extent = Gui::Extent{ newExtent.width, newExtent.height };
	event.windowResize.visibleRect = Gui::Rect{ visiblePos, Gui::Extent{ visibleSize.width, visibleSize.height } };
	queuedGuiEvents.push_back(event);
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
		DENGINE_DETAIL_UNREACHABLE();
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
		DENGINE_DETAIL_UNREACHABLE();
	}
	App::OpenSoftInput(currentText, filter);
}