#include "EditorImpl.hpp"

using namespace DEngine;
using namespace DEngine::Editor;



void Editor::EditorImpl::ButtonEvent(
	App::WindowID windowId,
	App::Button button,
	bool state)
{
	if (button == App::Button::LeftMouse || button == App::Button::RightMouse)
	{
		Gui::CursorPressEvent event = {};
		event.pressed = state;

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

void Editor::EditorImpl::CharRemoveEvent(App::WindowID windowId)
{
	Gui::CharRemoveEvent event = {};
	event.windowId = (Gui::WindowID)windowId;
	queuedGuiEvents.emplace_back(event);
}

void Editor::EditorImpl::CursorMove(
	App::Context& appCtx,
	App::WindowID windowId,
	Math::Vec2Int position,
	Math::Vec2Int positionDelta)
{
	Gui::CursorMoveEvent event = {};
	event.position = position;
	event.positionDelta = positionDelta;
	queuedGuiEvents.emplace_back(event);
}

void EditorImpl::TextInputEvent(
	App::Context& ctx,
	App::WindowID windowId,
	uSize oldIndex,
	uSize oldCount,
	Std::Span<u32 const> newString)
{
	auto oldSize = guiQueuedTextInputData.size();
	guiQueuedTextInputData.resize(oldSize + newString.Size());
	for (int i = 0; i < newString.Size(); i += 1)
		guiQueuedTextInputData[i + oldSize] = newString[i];

	Gui::TextInputEvent event = {};
	event.windowId = (Gui::WindowID)windowId;
	event.oldIndex = oldIndex;
	event.oldCount = oldCount;
	event.newTextData = (u32 const*)oldSize;
	event.newTextSize = newString.Size();

	queuedGuiEvents.emplace_back(event);
}

void EditorImpl::EndTextInputSessionEvent(
	App::Context& ctx,
	App::WindowID windowId)
{
	Gui::EndTextInputSessionEvent event = {};
	event.windowId = (Gui::WindowID)windowId;

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

bool Editor::EditorImpl::WindowCloseSignal(
	App::Context& appCtx,
	App::WindowID window)
{
	Gui::WindowCloseEvent event = {};
	event.windowId = (Gui::WindowID)window;
	queuedGuiEvents.emplace_back(event);

	return false;
}

void Editor::EditorImpl::WindowCursorEnter(
	App::WindowID window,
	bool entered)
{
	if (!entered)
	{
		Gui::WindowCursorExitEvent event = {};
		event.windowId = (Gui::WindowID)window;
		queuedGuiEvents.emplace_back(event);
	}
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
	App::Context& appCtx,
	App::WindowID window,
	App::Extent extent,
	Math::Vec2UInt visibleOffset,
	App::Extent visibleExtent)
{
	Gui::WindowResizeEvent event = {};
	event.windowId = (Gui::WindowID)window;
	event.extent = { extent.width, extent.height };
	event.safeAreaOffset = visibleOffset;
	event.safeAreaExtent = { visibleExtent.width, visibleExtent.height };
	queuedGuiEvents.emplace_back(event);
}

void Editor::EditorImpl::CloseWindow(Gui::WindowID id)
{
	appCtx->DestroyWindow((App::WindowID)id);
}

void Editor::EditorImpl::SetCursorType(Gui::WindowID id, Gui::CursorType cursorType)
{
	App::CursorType appCursorType = {};
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

	appCtx->SetCursor((App::WindowID)id, appCursorType);
}

void Editor::EditorImpl::HideSoftInput()
{
	appCtx->StopTextInputSession();
}

void Editor::EditorImpl::OpenSoftInput(
	Std::Span<char const> text,
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

	appCtx->StartTextInputSession(filter, text);
}

void Editor::EditorImpl::FlushQueuedEventsToGui()
{
	for (auto const& event : queuedGuiEvents)
	{
		using EventT = Std::Trait::RemoveCVRef<decltype(event)>;

		switch (event.GetIndex())
		{
			case EventT::indexOf<Gui::CharEnterEvent>:
				guiCtx->PushEvent(event.Get<Gui::CharEnterEvent>());
				break;
			case EventT::indexOf<Gui::CharEvent>:
				guiCtx->PushEvent(event.Get<Gui::CharEvent>());
				break;
			case EventT::indexOf<Gui::CharRemoveEvent>:
				guiCtx->PushEvent(event.Get<Gui::CharRemoveEvent>());
				break;
			case EventT::indexOf<Gui::CursorPressEvent>:
				guiCtx->PushEvent(event.Get<Gui::CursorPressEvent>());
				break;
			case EventT::indexOf<Gui::CursorMoveEvent>:
				guiCtx->PushEvent(event.Get<Gui::CursorMoveEvent>());
				break;

			case EventT::indexOf<Gui::TextInputEvent>:
			{
				auto test = event.Get<Gui::TextInputEvent>();
				test.newTextData = guiQueuedTextInputData.data() + (uSize)test.newTextData;
				guiCtx->PushEvent(test);
				break;
			}
			case EventT::indexOf<Gui::EndTextInputSessionEvent>:
			{
				guiCtx->PushEvent(event.Get<Gui::EndTextInputSessionEvent>());
				break;
			}

			case EventT::indexOf<Gui::TouchPressEvent>:
				//guiCtx->PushEvent(event.Get<Gui::TouchPressEvent>());
				break;
			case EventT::indexOf<Gui::TouchMoveEvent>:
				//guiCtx->PushEvent(event.Get<Gui::TouchMoveEvent>());
				break;
			case EventT::indexOf<Gui::WindowCloseEvent>:
				//guiCtx->PushEvent(event.Get<Gui::WindowCloseEvent>());
				break;
			case EventT::indexOf<Gui::WindowCursorExitEvent>:
				guiCtx->PushEvent(event.Get<Gui::WindowCursorExitEvent>());
				break;
			case EventT::indexOf<Gui::WindowMinimizeEvent>:
				//guiCtx->PushEvent(event.Get<Gui::WindowMinimizeEvent>());
				break;
			case EventT::indexOf<Gui::WindowMoveEvent>:
				guiCtx->PushEvent(event.Get<Gui::WindowMoveEvent>());
				break;
			case EventT::indexOf<Gui::WindowResizeEvent>:
				guiCtx->PushEvent(event.Get<Gui::WindowResizeEvent>());
				break;

			default:
				DENGINE_IMPL_UNREACHABLE();
				break;
		}

		InvalidateRendering();
	}
	queuedGuiEvents.clear();
	guiQueuedTextInputData.clear();
}