#include "EditorImpl.hpp"

#include "Editor.hpp"
#include "PlatformToEditorEventForwarder.hpp"

#include <DEngine/PlatformGuiGlue.hpp>

using namespace DEngine;
using namespace DEngine::Editor;

void PlatformToEditorEventForwarder::ButtonEvent(
	Platform::Context& appCtx,
	Platform::WindowID windowId,
	Platform::Button button,
	bool state)
{
	// Currently the editor never uses button events.
	// Just forward it to the secondary forwarder
	if (this->m_extraEventForwarder) {
		this->m_extraEventForwarder->ButtonEvent(
			appCtx,
			windowId,
			button,
			state);
	}
}

void PlatformToEditorEventForwarder::GamepadAxisEvent(
	Platform::Context& appCtx,
	Platform::WindowID windowId,
	Platform::GamepadDeviceId deviceId,
	Platform::GamepadAxis axis,
	f32 value)
{
	// Currently the editor never uses axis events.
	// Just forward it to the secondary forwarder
	if (this->m_extraEventForwarder != nullptr) {
		this->m_extraEventForwarder->GamepadAxisEvent(
			appCtx,
			windowId,
			deviceId,
			axis,
			value);
	}
}

void PlatformToEditorEventForwarder::GamepadKeyEvent(
	Platform::Context& ctx,
	Platform::WindowID windowId,
	Platform::GamepadDeviceId deviceId,
	Platform::GamepadKey key,
	bool pressed)
{
	if (this->m_extraEventForwarder != nullptr) {
		this->m_extraEventForwarder->GamepadKeyEvent(
			ctx,
			windowId,
			deviceId,
			key,
			pressed);
	}
}

void PlatformToEditorEventForwarder::CursorMove(
	Platform::Context& appCtx,
	Platform::WindowID windowId,
	Math::Vec2Int position,
	Math::Vec2Int positionDelta,
	Platform::CursorData cursorData)
{
	auto& editorImpl = this->GetEditorImplData();
	editorImpl.PushQueuedGuiEvent(
		[=](Editor::Context& editor, Gui::Context& guiCtxIn) {
			Editor::EditorGuiAppData appData { editor };

			Gui::CursorMoveEvent event = {};
			event.windowId = (Gui::WindowID)windowId;
			event.position = position;
			event.positionDelta = positionDelta;
			guiCtxIn.PushEvent(
				event,
				editor.GetTextEngineImpl(),
				Std::AnyRef{ appData });
		});
}

void PlatformToEditorEventForwarder::CursorPress(
	Platform::Context& appCtx,
	Platform::WindowID windowId,
	Platform::CursorButton button,
	bool pressed,
	Platform::CursorData cursorData)
{
	if (button != Platform::CursorButton::Primary)
		return;

	auto& editorImpl = this->GetEditorImplData();
	editorImpl.PushQueuedGuiEvent(
		[=](Editor::Context& editor, Gui::Context& guiCtxIn) {
			Editor::EditorGuiAppData appData { editor };

			Gui::CursorPressEvent event = {};
			event.windowId = (Gui::WindowID)windowId;
			event.button = Gui::CursorButton::Primary;
			event.pressed = pressed;
			guiCtxIn.PushEvent(
				event,
				editor.GetTextEngineImpl(),
				Std::AnyRef{ appData });
		});
}

void PlatformToEditorEventForwarder::TextInputEvent(
	Platform::Context& ctx,
	Platform::WindowID windowId,
	u64 oldIndex,
	u64 oldCount,
	Std::Span<u32 const> newString)
{
	auto& editorImpl = this->GetEditorImplData();

	auto oldSize = editorImpl.guiQueuedTextInputData.size();
	auto inputStringSize = newString.Size();

	editorImpl.guiQueuedTextInputData.resize(oldSize + inputStringSize);
	for (int i = 0; i < inputStringSize; i += 1) {
		editorImpl.guiQueuedTextInputData[i + oldSize] = newString[i];
	}

	editorImpl.PushQueuedGuiEvent(
		[=](Context& editorCtx, Gui::Context& guiCtxIn) {
			Gui::Context::TextInputEvent event = {};
			event.windowId = (Gui::WindowID)windowId;
			event.start = oldIndex;
			event.count = oldCount;
			event.newText = {
				editorCtx.GetImplData().guiQueuedTextInputData.data() + oldSize,
				inputStringSize };
			guiCtxIn.PushEvent(event);
		});

	// Relay into the scene's in-scene GUI. newString stays valid for this synchronous call, so the
	// secondary forwarder can read it directly without the editor's deferred-copy dance above.
	if (this->m_extraEventForwarder != nullptr) {
		this->m_extraEventForwarder->TextInputEvent(
			ctx,
			windowId,
			oldIndex,
			oldCount,
			newString);
	}
}

void PlatformToEditorEventForwarder::TextSelectionEvent(
	Platform::Context& ctx,
	Platform::WindowID windowId,
	u64 start,
	u64 count)
{
	// The editor's own Gui::Context does not currently consume selection events, so we only relay
	// to the in-scene GUI.
	if (this->m_extraEventForwarder != nullptr) {
		this->m_extraEventForwarder->TextSelectionEvent(
			ctx,
			windowId,
			start,
			count);
	}
}

void PlatformToEditorEventForwarder::EndTextInputSessionEvent(
	Platform::Context& ctx,
	Platform::WindowID windowId)
{
	auto& editorImpl = this->GetEditorImplData();
	editorImpl.PushQueuedGuiEvent(
		[=](auto&, Gui::Context& inGuiCtx) {
			Gui::Context::EndTextInputSessionEvent event = {};
			event.windowId = (Gui::WindowID)windowId;
			inGuiCtx.PushEvent(event);
		});

	if (this->m_extraEventForwarder != nullptr) {
		this->m_extraEventForwarder->EndTextInputSessionEvent(
			ctx,
			windowId);
	}
}

void PlatformToEditorEventForwarder::TouchEvent(
	Platform::WindowID windowId,
	u8 id,
	Platform::TouchEventType type,
	Math::Vec2 position)
{
	auto& editorImpl = this->GetEditorImplData();
	if (type == Platform::TouchEventType::Down || type == Platform::TouchEventType::Up) {
		editorImpl.PushQueuedGuiEvent(
			[=](Editor::Context& editor, Gui::Context& inGuiCtx) {
				Editor::EditorGuiAppData appData { editor };

				Gui::TouchPressEvent event = {};
				event.windowId = (Gui::WindowID)windowId;
				event.id = id;
				event.position = position;
				event.pressed = type == Platform::TouchEventType::Down;
				inGuiCtx.PushEvent(
					event,
					editor.GetTextEngineImpl(),
					Std::AnyRef{ appData });
			});
	}
	else if (type == Platform::TouchEventType::Moved) {
		editorImpl.PushQueuedGuiEvent(
			[=](auto& editor, Gui::Context& inGuiCtx) {
				Editor::EditorGuiAppData appData { editor };

				Gui::TouchMoveEvent event = {};
				event.windowId = (Gui::WindowID)windowId;
				event.id = id;
				event.position = position;
				inGuiCtx.PushEvent(
					event,
					editor.GetTextEngineImpl(),
					Std::AnyRef{ appData });
			});
	}
}

bool PlatformToEditorEventForwarder::WindowCloseSignal(
	Platform::Context& appCtxIn,
	Platform::WindowID window)
{
	auto& editorImpl = this->GetEditorImplData();
	editorImpl.PushQueuedGuiEvent(
		[=](auto&, Gui::Context& guiCtxIn) {
			Gui::WindowCloseEvent event = {};
			event.windowId = (Gui::WindowID)window;
			//guiCtx.PushEvent(event);
		});
	return false;
}

void PlatformToEditorEventForwarder::WindowCursorEnter(
	Platform::WindowID window,
	bool entered)
{
	auto& editorImpl = this->GetEditorImplData();
	if (!entered) {
		editorImpl.PushQueuedGuiEvent(
			[=](auto&, Gui::Context& guiCtxIn) {
				Gui::WindowCursorExitEvent event = {};
				event.windowId = (Gui::WindowID)window;
				guiCtxIn.PushEvent(event);
			});
	}
}

void PlatformToEditorEventForwarder::WindowMinimize(
	Platform::WindowID window,
	bool wasMinimized)
{
	auto& editorImpl = this->GetEditorImplData();
	editorImpl.PushQueuedGuiEvent(
		[=](auto&, Gui::Context& guiCtxIn) {
			Gui::WindowMinimizeEvent event = {};
			event.windowId = (Gui::WindowID)window;
			event.wasMinimized = wasMinimized;
			guiCtxIn.PushEvent(event);
		});
}

[[nodiscard]] static Std::Opt<Gui::WindowInsetSource> ToGuiWindowInsetSource(
	Platform::WindowInsetSource source)
{
	switch (source) {
		case Platform::WindowInsetSource::SystemBars:
			return Gui::WindowInsetSource::SystemBar;
		case Platform::WindowInsetSource::TextInputSystem:
			return Gui::WindowInsetSource::TextInputSystem;
		default: return Std::nullOpt;
	}
}

[[nodiscard]] static auto ToGuiWindowInsets(
	Std::Array<Platform::WindowInsets,
	(int)Platform::WindowInsetSource::COUNT> const& windowInsets)
{
	Gui::WindowInsets out = {};

	for (
		int platformInsetSource = 0;
		platformInsetSource < (int)Platform::WindowInsetSource::COUNT;
		platformInsetSource++)
	{
		auto guiInsetSourceOpt = ToGuiWindowInsetSource((Platform::WindowInsetSource)platformInsetSource);
		if (guiInsetSourceOpt.Has()) {
			auto const& guiInsetSource = guiInsetSourceOpt.Value();
			auto& guiInsets = out.m_insets[(int)guiInsetSource];
			auto const& platformInsets = windowInsets[platformInsetSource];
			guiInsets.bottom = platformInsets.bottom;
			guiInsets.left = platformInsets.left;
			guiInsets.right = platformInsets.right;
			guiInsets.top = platformInsets.top;
		}
	}

	return out;
}

void PlatformToEditorEventForwarder::WindowResize(
	Platform::Context& appCtx,
	Platform::WindowID window,
	Platform::Extent extent,
	Std::Array<Platform::WindowInsets, (int)Platform::WindowInsetSource::COUNT> windowInsets)
{
	auto& editorImpl = this->GetEditorImplData();
	editorImpl.PushQueuedGuiEvent(
		[=](auto&, Gui::Context& guiCtxIn) {
			Gui::WindowResizeEvent event = {};
			event.windowId = (Gui::WindowID)window;
			event.extent = { extent.width, extent.height };
			event.insets = ToGuiWindowInsets(windowInsets);
			guiCtxIn.PushEvent(event);
		});
}

void Editor::EditorImpl::CloseWindow(Gui::WindowID id) {
	appCtx->DestroyWindow((Platform::WindowID)id);
}

void Editor::EditorImpl::SetCursorType(Gui::WindowID id, Gui::CursorType cursorType) {
	Platform::CursorType appCursorType = {};
	switch (cursorType) {
	case Gui::CursorType::Arrow:
		appCursorType = Platform::CursorType::Arrow;
		break;
	case Gui::CursorType::HorizontalResize:
		appCursorType = Platform::CursorType::HorizontalResize;
		break;
	case Gui::CursorType::VerticalResize:
		appCursorType = Platform::CursorType::VerticalResize;
		break;
	default:
		DENGINE_IMPL_UNREACHABLE();
		break;
	}

	appCtx->SetCursor((Platform::WindowID)id, appCursorType);
}

void Editor::EditorImpl::HideSoftInput()
{
	appCtx->StopTextInputSession();
}

void Editor::EditorImpl::OpenSoftInput(
	Gui::WindowID windowId,
	Std::Span<char const> inputText,
	u64 selectionStart,
	u64 selectionCount,
	Gui::TextInputType inputFilter)
{
	appCtx->StartTextInputSession(
		(Platform::WindowID)windowId,
		PlatformGuiGlue::ToPlatformSoftInputFilter(inputFilter),
		inputText,
		selectionStart,
		selectionCount);
}

void Editor::EditorImpl::UpdateTextInputConnection(
	Gui::WindowID windowId,
	u64 selIndex,
	u64 selCount,
	Std::Span<u32 const> inputText)
{
	appCtx->UpdateTextInputConnection(
		(Platform::WindowID)windowId,
		selIndex,
		selCount,
		inputText);
}

void Editor::EditorImpl::UpdateTextInputConnectionSelection(
	Gui::WindowID windowId,
	u64 selIndex,
	u64 selCount)
{
	appCtx->UpdateTextInputConnectionSelection(selIndex, selCount);
}

//#include <tracy/Tracy.hpp>
bool Editor::Context::FlushQueuedEventsToGui()
{
    auto& implData = GetImplData();
	if (!implData.queuedGuiEvents.IsEmpty()) {
		//ZoneScopedN("Any GUI event");
        implData.queuedGuiEvents.Consume(*this, *implData.guiCtx);
        implData.guiQueuedTextInputData.clear();
        implData.InvalidateRendering();
		return true;
	}

	return false;
}