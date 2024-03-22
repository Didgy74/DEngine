#include "HandleInputEvent.hpp"

#include "BackendData.hpp"

#include <DEngine/impl/AppAssert.hpp>

namespace DEngine::Application::impl
{
	[[nodiscard]] static GamepadKey ToGamepadButton(int32_t androidKeyCode) noexcept
	{
		switch (androidKeyCode)
		{
			case AKEYCODE_BUTTON_A:
				return GamepadKey::A;
		}
		return GamepadKey::Invalid;
	}

	// This function is called from the AInputQueue function when we poll the ALooper
	static bool HandleInputEvents_Key(
		Context::Impl& implData,
		BackendData& backendData,
		AInputEvent* event,
		int32_t sourceFlags)
	{
		bool handled = false;

		auto const keyCode = AKeyEvent_getKeyCode(event);

		auto const gamepadButton = ToGamepadButton(keyCode);

		if (gamepadButton != GamepadKey::Invalid) {
			handled = true;

			auto const action = AKeyEvent_getAction(event);

			if (action != AKEY_EVENT_ACTION_MULTIPLE)
			{
				auto const pressed = action == AKEY_EVENT_ACTION_DOWN;


				//detail::UpdateGamepadButton(
				//appData,
				//gamepadButton,
				//pressed);
			}
		}

		return handled;
	}

	// This function is called from the AInputQueue function when we poll the ALooper
	// Should return true if the event has been handled by the app.
	static bool HandleInputEvents_Motion_Cursor(
		Context::Impl& implData,
		BackendData& backendData,
		AInputEvent const* event,
		i32 action,
		i32 index)
	{
		auto sourceFlags = AInputEvent_getSource(event);
		if ((sourceFlags & AINPUT_SOURCE_MOUSE) != AINPUT_SOURCE_MOUSE)
			return false;

		auto handled = false;

		if (action == AMOTION_EVENT_ACTION_HOVER_ENTER)
		{
			// The cursor started existing
			f32 const x = AMotionEvent_getX(event, index);
			f32 const y = AMotionEvent_getY(event, index);

			implData.cursorOpt = CursorData{};
			auto& cursorData = implData.cursorOpt.Value();
			cursorData.position = { (i32)x, (i32)y };

			handled = true;
		}
		else if (action == AMOTION_EVENT_ACTION_HOVER_MOVE || action == AMOTION_EVENT_ACTION_MOVE)
		{
			f32 const x = AMotionEvent_getX(event, index);
			f32 const y = AMotionEvent_getY(event, index);
			BackendInterface::UpdateCursorPosition(
				implData,
				backendData.currWindowId.Value(),
				{ (i32)x, (i32)y });
			handled = true;
		}
		else if (action == AMOTION_EVENT_ACTION_DOWN)
		{
			//detail::UpdateButton(appData, Button::LeftMouse, true);
			handled = true;
		}
		else if (action == AMOTION_EVENT_ACTION_UP)
		{
			//detail::UpdateButton(appData, Button::LeftMouse, false);
			handled = true;
		}

		return handled;
	}

	// This function is called from the AInputQueue function when we poll the ALooper
	// Should return true if the event has been handled by the app.
	static bool HandleInputEvents_Motion_Joystick(
		Context::Impl& implData,
		BackendData& backendData,
		AInputEvent const* event,
		i32 action)
	{
		auto sourceFlags = AInputEvent_getSource(event);
		if ((sourceFlags & AINPUT_SOURCE_JOYSTICK) == 0)
			return false;

		auto handled = false;

		if (action == AMOTION_EVENT_ACTION_MOVE)
		{
			auto const leftStickX = AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_X, 0);

			/*
			detail::UpdateGamepadAxis(
				appData,
				GamepadAxis::LeftX,
				leftStickX);
			 */

			handled = true;
		}

		return handled;
	}


}

using namespace DEngine;
using namespace DEngine::Application;
using namespace DEngine::Application::impl;

// This function is called from the AInputQueue function when we poll the ALooper
// Should return true if the event has been handled by the app.
bool Application::impl::HandleInputEvent_Motion_Touch(
	Context::Impl& implData,
	BackendData& backendData,
	AInputEvent const* event)
{
	auto sourceFlags = AInputEvent_getSource(event);
	if ((sourceFlags & AINPUT_SOURCE_TOUCHSCREEN) == 0)
		return false;

	auto const actionIndexCombo = AMotionEvent_getAction(event);
	auto const action = actionIndexCombo & AMOTION_EVENT_ACTION_MASK;
	auto const index =
		(actionIndexCombo & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >>
		AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;

	auto handled = false;

	switch (action)
	{
		case AMOTION_EVENT_ACTION_DOWN:
		case AMOTION_EVENT_ACTION_POINTER_DOWN:
		{
			auto const x = AMotionEvent_getRawX(event, index);
			auto const y = AMotionEvent_getRawY(event, index);
			auto const id = AMotionEvent_getPointerId(event, index);
			BackendInterface::UpdateTouch(
				implData,
				backendData.currWindowId.Get(),
				TouchEventType::Down,
				(u8)id,
				x,
				y);
			handled = true;
			break;
		}

		case AMOTION_EVENT_ACTION_MOVE:
		{
			auto const count = AMotionEvent_getPointerCount(event);
			for (auto i = 0; i < count; i++)
			{
				auto const x = AMotionEvent_getX(event, i);
				auto const y = AMotionEvent_getY(event, i);
				auto const  id = AMotionEvent_getPointerId(event, i);
				BackendInterface::UpdateTouch(
					implData,
					backendData.currWindowId.Get(),
					TouchEventType::Moved,
					(u8)id,
					x,
					y);
			}
			handled = true;
			break;
		}

		case AMOTION_EVENT_ACTION_UP:
		case AMOTION_EVENT_ACTION_POINTER_UP:
		{
			auto const  x = AMotionEvent_getX(event, index);
			auto const  y = AMotionEvent_getY(event, index);
			auto const  id = AMotionEvent_getPointerId(event, index);
			BackendInterface::UpdateTouch(
				implData,
				backendData.currWindowId.Get(),
				TouchEventType::Up,
				(u8)id,
				x,
				y);
			handled = true;
			break;
		}

		default:
			break;
	}

	return handled;
}

// This function is called from the AInputQueue function when we poll the ALooper
// Should return true if the event has been handled by the app.
bool Application::impl::HandleInputEvent_Motion(
	Context::Impl& appData,
	BackendData& backendData,
	AInputEvent const* event)
{
	bool handled;

	auto const actionIndexCombo = AMotionEvent_getAction(event);
	auto const action = actionIndexCombo & AMOTION_EVENT_ACTION_MASK;
	auto const index =
		(actionIndexCombo & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >>
		AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;

	handled = HandleInputEvents_Motion_Cursor(
		appData,
		backendData,
		event,
		action,
		index);
	if (handled)
		return true;

	handled = HandleInputEvent_Motion_Touch(
		appData,
		backendData,
		event);
	if (handled)
		return true;

	handled = HandleInputEvents_Motion_Joystick(
		appData,
		backendData,
		event,
		action);
	if (handled)
		return true;

	return handled;
}

int Application::impl::looperCallback_InputEvent(int fd, int events, void* data)
{
	DENGINE_IMPL_APPLICATION_ASSERT(events == ALOOPER_EVENT_INPUT);

	auto& androidPollSource = *static_cast<PollSource*>(data);
	auto& implData = *androidPollSource.implData;
	auto& backendData = *androidPollSource.backendData;

	while (true)
	{
		AInputEvent* event = nullptr;
		auto const eventIndex = AInputQueue_getEvent(backendData.inputQueue, &event);
		if (eventIndex < 0)
			break;

		if (AInputQueue_preDispatchEvent(backendData.inputQueue, event) != 0)
			continue;

		bool handled = false;

		auto const eventType = AInputEvent_getType(event);
		if (!handled && eventType == AINPUT_EVENT_TYPE_MOTION) {
			handled = HandleInputEvent_Motion(
				implData,
				backendData,
				event);
		}
		/*
		if (!handled && eventType == AINPUT_EVENT_TYPE_KEY)
		{
			handled = HandleInputEvents_Key(
				implData,
				backendData,
				event,
				eventSourceFlags);
		}
		*/

		AInputQueue_finishEvent(backendData.inputQueue, event, handled);
	}

	// 1 Means we want this callback to keep being invoked.
	return 1;
}