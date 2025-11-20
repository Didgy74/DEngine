#pragma once

#include <DEngine/Platform/Platform.hpp>

#include "Editor.hpp"
#include "EditorImpl.hpp"

namespace DEngine::Editor {
	class PlatformToEditorEventForwarder : public Platform::EventForwarder {
	public:
		explicit PlatformToEditorEventForwarder(Editor::Context& editorCtx)
		{
			m_editorCtx = &editorCtx;
		}

		[[nodiscard]] Editor::Context& GetEditorContext() const {
			DENGINE_IMPL_ASSERT(m_editorCtx != nullptr);
			return *m_editorCtx;
		}
		[[nodiscard]] Editor::EditorImpl& GetEditorImplData() const {
			DENGINE_IMPL_ASSERT(m_editorCtx != nullptr);
			return m_editorCtx->GetImplData();
		}
		Editor::Context* m_editorCtx = nullptr;
		Platform::EventForwarder* m_extraEventForwarder = nullptr;

		virtual void ButtonEvent(
			Platform::Context& appCtx,
			Platform::WindowID windowId,
			Platform::Button button,
			bool state) override;
		virtual void CursorMove(
			Platform::Context& appCtx,
			Platform::WindowID windowId,
			Math::Vec2Int position,
			Math::Vec2Int positionDelta,
			Platform::CursorData cursorData) override;
		virtual void CursorPress(
			Platform::Context& appCtx,
			Platform::WindowID windowId,
			Platform::CursorButton button,
			bool pressed,
			Platform::CursorData cursorData) override;
		virtual void TextInputEvent(
			Platform::Context& ctx,
			Platform::WindowID windowId,
			u64 oldIndex,
			u64 oldCount,
			Std::Span<u32 const> newString) override;
		virtual void TextSelectionEvent(
			Platform::Context& ctx,
			Platform::WindowID windowId,
			u64 start,
			u64 count) override;
		virtual void EndTextInputSessionEvent(
			Platform::Context& ctx,
			Platform::WindowID windowId) override;
		virtual void TouchEvent(
			Platform::WindowID windowId,
			u8 id,
			Platform::TouchEventType type,
			Math::Vec2 position) override;
		virtual bool WindowCloseSignal(
			Platform::Context& appCtx,
			Platform::WindowID window) override;
		virtual void WindowCursorEnter(
			Platform::WindowID window,
			bool entered) override;
		virtual void WindowMinimize(
			Platform::WindowID window,
			bool wasMinimized) override;
		virtual void WindowResize(
			Platform::Context& appCtx,
			Platform::WindowID window,
			Platform::Extent extent,
			Std::Array<Platform::WindowInsets, (int)Platform::WindowInsetSource::COUNT> windowInsets) override;

		virtual void GamepadAxisEvent(
			Platform::Context& ctx,
			Platform::WindowID windowId,
			Platform::GamepadDeviceId deviceId,
			Platform::GamepadAxis axis,
			f32 value) override;
		virtual void GamepadKeyEvent(
			Platform::Context& ctx,
			Platform::WindowID windowId,
			Platform::GamepadDeviceId deviceId,
			Platform::GamepadKey key,
			bool pressed) override;
	};
}