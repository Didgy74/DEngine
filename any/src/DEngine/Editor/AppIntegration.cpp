#include "EditorImpl.hpp"

#include "Editor.hpp"

using namespace DEngine;
using namespace DEngine::Editor;

void Editor::Context::ButtonEvent(
	App::WindowID windowId,
	App::Button button,
	bool state)
{
	auto& editorImpl = this->GetImplData();

	if (button == App::Button::LeftMouse || button == App::Button::RightMouse) {
		editorImpl.PushQueuedGuiEvent(
			[button, state](Editor::Context& editorContext, Gui::Context& guiCtxIn) {
				Gui::CursorPressEvent event = {};
				event.pressed = state;
				if (button == App::Button::LeftMouse)
					event.button = Gui::CursorButton::Primary;
				else
					event.button = Gui::CursorButton::Secondary;
				guiCtxIn.PushEvent(event, Std::AnyRef{ editorContext });
			});
	}
}

void Context::CursorMove(
	App::Context& appCtxIn,
	App::WindowID windowId,
	Math::Vec2Int position,
	Math::Vec2Int positionDelta)
{
	auto& editorImpl = this->GetImplData();
	editorImpl.PushQueuedGuiEvent(
		[=](auto& editor, Gui::Context& guiCtxIn) {
			Gui::CursorMoveEvent event = {};
			event.position = position;
			event.positionDelta = positionDelta;
			guiCtxIn.PushEvent(event, Std::AnyRef{ editor });
		});
}

void Context::TextInputEvent(
	App::Context& ctx,
	App::WindowID windowId,
	uSize oldIndex,
	uSize oldCount,
	Std::Span<u32 const> newString)
{
	auto& editorImpl = this->GetImplData();

	auto oldSize = editorImpl.guiQueuedTextInputData.size();
	auto inputStringSize = newString.Size();

	editorImpl.guiQueuedTextInputData.resize(oldSize + inputStringSize);
	for (int i = 0; i < inputStringSize; i += 1)
		editorImpl.guiQueuedTextInputData[i + oldSize] = newString[i];

	editorImpl.PushQueuedGuiEvent(
		[=](Context& ctx, Gui::Context& guiCtxIn) {
			Gui::TextInputEvent event = {};
			event.windowId = (Gui::WindowID)windowId;
			event.oldIndex = oldIndex;
			event.oldCount = oldCount;
			event.newTextData = ctx.GetImplData().guiQueuedTextInputData.data() + oldSize;
			event.newTextSize = inputStringSize;
			guiCtxIn.PushEvent(event);
		});
}

void Context::EndTextInputSessionEvent(
	App::Context& ctx,
	App::WindowID windowId)
{
	auto& editorImpl = this->GetImplData();
	editorImpl.PushQueuedGuiEvent(
		[=](auto&, Gui::Context& inGuiCtx) {
			Gui::EndTextInputSessionEvent event = {};
			event.windowId = (Gui::WindowID)windowId;
			inGuiCtx.PushEvent(event);
		});
}

void Context::TouchEvent(
	App::WindowID windowId,
	u8 id,
	App::TouchEventType type,
	Math::Vec2 position)
{
	auto& editorImpl = this->GetImplData();
	if (type == App::TouchEventType::Down || type == App::TouchEventType::Up) {
		editorImpl.PushQueuedGuiEvent(
			[=](auto& editor, Gui::Context& inGuiCtx) {
				Gui::TouchPressEvent event = {};
				event.id = id;
				event.position = position;
				event.pressed = type == App::TouchEventType::Down;
				inGuiCtx.PushEvent(event, Std::AnyRef{ editor });
			});
	}
	else if (type == App::TouchEventType::Moved) {
		editorImpl.PushQueuedGuiEvent(
			[=](auto& editor, Gui::Context& inGuiCtx) {
				Gui::TouchMoveEvent event = {};
				event.id = id;
				event.position = position;
				inGuiCtx.PushEvent(event, Std::AnyRef{ editor });
			});
	}
}

bool Context::WindowCloseSignal(
	App::Context& appCtxIn,
	App::WindowID window)
{
	auto& editorImpl = this->GetImplData();
	editorImpl.PushQueuedGuiEvent(
		[=](auto&, Gui::Context& guiCtxIn) {
			Gui::WindowCloseEvent event = {};
			event.windowId = (Gui::WindowID)window;
			//guiCtx.PushEvent(event);
		});
	return false;
}

void Context::WindowCursorEnter(
	App::WindowID window,
	bool entered)
{
	auto& editorImpl = this->GetImplData();
	if (!entered) {
		editorImpl.PushQueuedGuiEvent(
			[=](auto&, Gui::Context& guiCtxIn) {
				Gui::WindowCursorExitEvent event = {};
				event.windowId = (Gui::WindowID)window;
				guiCtxIn.PushEvent(event);
			});
	}
}

void Context::WindowMinimize(
	App::WindowID window,
	bool wasMinimized)
{
	auto& editorImpl = this->GetImplData();
	editorImpl.PushQueuedGuiEvent(
		[=](auto&, Gui::Context& guiCtxIn) {
			Gui::WindowMinimizeEvent event = {};
			event.windowId = (Gui::WindowID)window;
			event.wasMinimized = wasMinimized;
			guiCtxIn.PushEvent(event);
		});
}

void Context::WindowMove(
	App::WindowID window,
	Math::Vec2Int position)
{
	auto& editorImpl = this->GetImplData();
	editorImpl.PushQueuedGuiEvent(
		[=](auto&, Gui::Context& guiCtxIn) {
			Gui::WindowMoveEvent event = {};
			event.windowId = (Gui::WindowID)window;
			event.position   = position;
			guiCtxIn.PushEvent(event);
		});
}

void Context::WindowResize(
	App::Context& appCtxIn,
	App::WindowID window,
	App::Extent extent,
	Math::Vec2UInt visibleOffset,
	App::Extent visibleExtent)
{
	auto& editorImpl = this->GetImplData();
	editorImpl.PushQueuedGuiEvent(
		[=](auto&, Gui::Context& guiCtxIn) {
			Gui::WindowResizeEvent event = {};
			event.windowId = (Gui::WindowID)window;
			event.extent = { extent.width, extent.height };
			event.safeAreaOffset = visibleOffset;
			event.safeAreaExtent = { visibleExtent.width, visibleExtent.height };
			guiCtxIn.PushEvent(event);
		});
}

void Editor::EditorImpl::CloseWindow(Gui::WindowID id) {
	appCtx->DestroyWindow((App::WindowID)id);
}

void Editor::EditorImpl::SetCursorType(Gui::WindowID id, Gui::CursorType cursorType)
{
	App::CursorType appCursorType = {};
	switch (cursorType) {
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
bool Editor::Context::FlushQueuedEventsToGui()
{
    auto& implData = GetImplData();
	if (!implData.queuedGuiEvents.IsEmpty()) {
		ZoneScopedN("Any GUI event");
        implData.queuedGuiEvents.Consume(*this, *implData.guiCtx);
        implData.guiQueuedTextInputData.clear();
        implData.InvalidateRendering();
		return true;
	}

	return false;
}