#include <DEngine/PlatformGuiGlue.hpp>

using namespace DEngine;

using PlatformGuiGlue::BasicPlatformToGuiEventForwarder;

void BasicPlatformToGuiEventForwarder::WindowResize(
	Platform::Context& appCtx,
	Platform::WindowID window,
	Platform::Extent extent,
	Std::Array<Platform::WindowInsets, (int)Platform::WindowInsetSource::COUNT> windowInsets)
{
	auto insetsOut = ToGuiWindowInsetsCollection(windowInsets);
	fnList.Push([=](Gui::Context& guiCtx, Gui::TextEngine& textEngine) {
		Gui::WindowResizeEvent event = {};
		event.windowId = (Gui::WindowID)window;
		event.extent = { extent.width, extent.height };
		event.insets = insetsOut;
		guiCtx.PushEvent(event);
	});
}

void BasicPlatformToGuiEventForwarder::ButtonEvent(
	Platform::Context& appCtx,
	Platform::WindowID windowId,
	Platform::Button button,
	bool state)
{
	auto isDirectionalButton =
		button == Platform::Button::Left
		|| button == Platform::Button::Right
		|| button == Platform::Button::Up
		|| button == Platform::Button::Down;

	if (isDirectionalButton && state == true) {
		f32 angle = 0.f;
		switch (button) {
			case Platform::Button::Up: angle = 0.f; break;
			case Platform::Button::Left: angle = 0.25f; break;
			case Platform::Button::Down: angle = 0.5f; break;
			case Platform::Button::Right: angle = 0.75f; break;
			default: DENGINE_IMPL_UNREACHABLE();
		}
	}
}

void BasicPlatformToGuiEventForwarder::CursorMove(
	Platform::Context& appCtx,
	Platform::WindowID windowId,
	Math::Vec2Int position,
	Math::Vec2Int positionDelta,
	Platform::CursorData cursorData)
{
	if (this->emulateCursorAsTouch && cursorData.ButtonPressed(Platform::CursorButton::Primary)) {
		// Only report TouchMove if we are currently pressed
		Gui::TouchMoveEvent event = {};
		event.windowId = (Gui::WindowID)windowId;
		event.id = 0;
		event.position = { (f32)position.x, (f32)position.y };
		fnList.Push([=](Gui::Context& guiCtx, Gui::TextEngine& textEngine) {
			guiCtx.PushEvent(event, textEngine, {});
		});
	} else {
		Gui::CursorMoveEvent event = {};
		event.windowId = (Gui::WindowID)windowId;
		event.position = { position.x, position.y };
		event.positionDelta = { positionDelta.x, positionDelta.y };
		fnList.Push([=](Gui::Context& guiCtx, Gui::TextEngine& textEngine) {
			guiCtx.PushEvent(event, textEngine, {});
		});
	}
}

void BasicPlatformToGuiEventForwarder::CursorPress(
	Platform::Context& appCtx,
	Platform::WindowID windowId,
	Platform::CursorButton button,
	bool pressed,
	Platform::CursorData cursorData)
{
	if (button == Platform::CursorButton::Primary) {
		if (this->emulateCursorAsTouch) {
			Gui::TouchPressEvent event = {};
			event.windowId = (Gui::WindowID)windowId;
			event.id = 0;
			event.position = { (f32)cursorData.position.x, (f32)cursorData.position.y };
			event.pressed = pressed;
			fnList.Push([=](Gui::Context& guiCtx, Gui::TextEngine& textEngine) {
				guiCtx.PushEvent(event, textEngine, {});
			});
		} else {
			Gui::CursorPressEvent event = {};
			event.windowId = (Gui::WindowID)windowId;
			event.button = Gui::CursorButton::Primary;
			event.pressed = pressed;
			fnList.Push([=](Gui::Context& guiCtx, Gui::TextEngine& textEngine) {
				guiCtx.PushEvent(event, textEngine, {});
			});
		}
	}
}

void BasicPlatformToGuiEventForwarder::TouchEvent(
	Platform::WindowID windowId,
	u8 id,
	Platform::TouchEventType type,
	Math::Vec2 position)
{
	if (type == Platform::TouchEventType::Down || type == Platform::TouchEventType::Up) {
		Gui::TouchPressEvent event = {};
		event.windowId = (Gui::WindowID)windowId;
		event.id = id;
		event.position = position;
		event.pressed = type == Platform::TouchEventType::Down;
		fnList.Push([=](Gui::Context& guiCtx, Gui::TextEngine& textEngine) {
			guiCtx.PushEvent(event, textEngine, {});
		});
	}
	else if (type == Platform::TouchEventType::Moved) {
		Gui::TouchMoveEvent event = {};
		event.windowId = (Gui::WindowID)windowId;
		event.id = id;
		event.position = position;
		fnList.Push([=](Gui::Context& guiCtx, Gui::TextEngine& textEngine) {
			guiCtx.PushEvent(event, textEngine, {});
		});
	}
}

void BasicPlatformToGuiEventForwarder::TextInputEvent(
	Platform::Context& ctx,
	Platform::WindowID windowId,
	u64 start,
	u64 count,
	Std::Span<u32 const> newString)
{
	std::vector<u32> inputText;
	inputText.resize(newString.Size());
	for (int i = 0; i < newString.Size(); i++) {
		inputText[i] = newString[i];
	}

	fnList.Push([=, text = Std::Move(inputText)](Gui::Context& guiCtx, Gui::TextEngine& textEngine) {
		Gui::Context::TextInputEvent event = {};
		event.windowId = (Gui::WindowID)windowId;
		event.start = start;
		event.count = count;
		event.newText = { text.data(), text.size() };
		guiCtx.PushEvent(event);
	});
}

void BasicPlatformToGuiEventForwarder::TextSelectionEvent(
	Platform::Context& ctx,
	Platform::WindowID windowId,
	u64 start,
	u64 count)
{
	fnList.Push([=](Gui::Context& guiCtx, Gui::TextEngine&) {
		Gui::Context::TextSelectionEvent event = {};
		event.windowId = (Gui::WindowID)windowId;
		event.start = start;
		event.count = count;
		guiCtx.PushEvent(event);
	});
}
