#include "EditorImpl.hpp"

using namespace DEngine;
using namespace DEngine::Editor;

void Editor::EditorImpl::ButtonEvent(
	App::WindowID windowId,
	App::Button button,
	bool state)
{
	if (button == App::Button::LeftMouse || button == App::Button::RightMouse) {
		PushQueuedGuiEvent(
			[=](EditorImpl& implData, Gui::Context& guiCtx) {
				Gui::CursorPressEvent event = {};
				event.pressed = state;
				if (button == App::Button::LeftMouse)
					event.button = Gui::CursorButton::Primary;
				else
					event.button = Gui::CursorButton::Secondary;
				guiCtx.PushEvent(event);
			});
	}
}

void Editor::EditorImpl::CursorMove(
	App::Context& appCtx,
	App::WindowID windowId,
	Math::Vec2Int position,
	Math::Vec2Int positionDelta)
{
	PushQueuedGuiEvent(
		[=](EditorImpl& implData, Gui::Context& guiCtx) {
			Gui::CursorMoveEvent event = {};
			event.position = position;
			event.positionDelta = positionDelta;
			guiCtx.PushEvent(event);
		});
}

void EditorImpl::TextInputEvent(
	App::Context& ctx,
	App::WindowID windowId,
	uSize oldIndex,
	uSize oldCount,
	Std::Span<u32 const> newString)
{
	auto oldSize = guiQueuedTextInputData.size();
	auto inputStringSize = newString.Size();

	guiQueuedTextInputData.resize(oldSize + inputStringSize);
	for (int i = 0; i < inputStringSize; i += 1)
		guiQueuedTextInputData[i + oldSize] = newString[i];

	PushQueuedGuiEvent(
		[=](EditorImpl& implData, Gui::Context& guiCtx) {
			Gui::TextInputEvent event = {};
			event.windowId = (Gui::WindowID)windowId;
			event.oldIndex = oldIndex;
			event.oldCount = oldCount;
			event.newTextData = implData.guiQueuedTextInputData.data() + oldSize;
			event.newTextSize = inputStringSize;
			guiCtx.PushEvent(event);
		});
}

void EditorImpl::EndTextInputSessionEvent(
	App::Context& ctx,
	App::WindowID windowId)
{
	PushQueuedGuiEvent(
		[=](EditorImpl& implData, Gui::Context& guiCtx) {
			Gui::EndTextInputSessionEvent event = {};
			event.windowId = (Gui::WindowID)windowId;
			guiCtx.PushEvent(event);
		});
}

void Editor::EditorImpl::TouchEvent(
	App::WindowID windowId,
	u8 id,
	App::TouchEventType type,
	Math::Vec2 position)
{
	if (type == App::TouchEventType::Down || type == App::TouchEventType::Up)
	{
		PushQueuedGuiEvent(
			[=](EditorImpl& implData, Gui::Context& guiCtx) {
				Gui::TouchPressEvent event = {};
				event.id = id;
				event.position = position;
				event.pressed = type == App::TouchEventType::Down;
				guiCtx.PushEvent(event);
			});
	}
	else if (type == App::TouchEventType::Moved)
	{
		PushQueuedGuiEvent(
			[=](EditorImpl& implData, Gui::Context& guiCtx) {
				Gui::TouchMoveEvent event = {};
				event.id = id;
				event.position = position;
				guiCtx.PushEvent(event);
			});
	}
}

bool Editor::EditorImpl::WindowCloseSignal(
	App::Context& appCtx,
	App::WindowID window)
{
	PushQueuedGuiEvent(
		[=](EditorImpl& implData, Gui::Context& guiCtx) {
			Gui::WindowCloseEvent event = {};
			event.windowId = (Gui::WindowID)window;
			//guiCtx.PushEvent(event);
		});
	return false;
}

void Editor::EditorImpl::WindowCursorEnter(
	App::WindowID window,
	bool entered)
{
	if (!entered)
	{
		PushQueuedGuiEvent(
			[=](EditorImpl& implData, Gui::Context& guiCtx) {
				Gui::WindowCursorExitEvent event = {};
				event.windowId = (Gui::WindowID)window;
				guiCtx.PushEvent(event);
			});
	}
}

void Editor::EditorImpl::WindowMinimize(
	App::WindowID window,
	bool wasMinimized)
{
	PushQueuedGuiEvent(
		[=](EditorImpl& implData, Gui::Context& guiCtx) {
			Gui::WindowMinimizeEvent event = {};
			event.windowId = (Gui::WindowID)window;
			event.wasMinimized = wasMinimized;
			guiCtx.PushEvent(event);
		});
}

void Editor::EditorImpl::WindowMove(
	App::WindowID window,
	Math::Vec2Int position)
{
	PushQueuedGuiEvent(
		[=](EditorImpl& implData, Gui::Context& guiCtx) {
			Gui::WindowMoveEvent event = {};
			event.windowId = (Gui::WindowID)window;
			event.position = position;
			guiCtx.PushEvent(event);
		});
}

void Editor::EditorImpl::WindowResize(
	App::Context& appCtx,
	App::WindowID window,
	App::Extent extent,
	Math::Vec2UInt visibleOffset,
	App::Extent visibleExtent)
{
	PushQueuedGuiEvent(
		[=](EditorImpl& implData, Gui::Context& guiCtx) {
			Gui::WindowResizeEvent event = {};
			event.windowId = (Gui::WindowID)window;
			event.extent = { extent.width, extent.height };
			event.safeAreaOffset = visibleOffset;
			event.safeAreaExtent = { visibleExtent.width, visibleExtent.height };
			guiCtx.PushEvent(event);
		});
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
	auto toFilter = [](Gui::SoftInputFilter inputFilter) {
		switch (inputFilter) {
			case Gui::SoftInputFilter::NoFilter:
				return App::SoftInputFilter::Text;
			case Gui::SoftInputFilter::SignedFloat:
				return App::SoftInputFilter::Float;
			case Gui::SoftInputFilter::UnsignedFloat:
				return App::SoftInputFilter::UnsignedFloat;
			case Gui::SoftInputFilter::SignedInteger:
				return App::SoftInputFilter::Integer;
			case Gui::SoftInputFilter::UnsignedInteger:
				return App::SoftInputFilter::UnsignedInteger;
			default:
				DENGINE_IMPL_UNREACHABLE();
				return App::SoftInputFilter{};
		}
	};
	appCtx->StartTextInputSession(toFilter(inputFilter), text);
}

#include <tracy/Tracy.hpp>
void Editor::EditorImpl::FlushQueuedEventsToGui()
{
	if (!queuedGuiEvents.IsEmpty()) {
		ZoneScopedN("Any GUI event");
		queuedGuiEvents.Consume(*this, *guiCtx);
		guiQueuedTextInputData.clear();
		InvalidateRendering();
	}
}